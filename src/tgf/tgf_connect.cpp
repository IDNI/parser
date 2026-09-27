// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "tgf_connect.h"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

// The dep script ships headers only, so Boost.System must stay header-only.
#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_HEADER_ONLY
#define BOOST_SYSTEM_HEADER_ONLY
#endif

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "format/json/json.h"
#include "parser_strings.h"
#include "parser_term_color_macros.h"
#include "utility/diagnostics.h"

namespace idni {

namespace ba = boost::asio;
using ba::ip::tcp;
using boost::system::error_code;
using format::json::value;

using tt = tgf_repl_parser::tree::traverser;

// One synchronous TCP connection with one line at a time.
struct tgf_remote_client {
	ba::io_context io;
	tcp::socket sock{io};
	ba::streambuf buf;

	bool connect(const std::string& host, uint16_t port) {
		error_code ec;
		auto addr = ba::ip::make_address(host, ec);
		if (ec) return false;
		sock.connect(tcp::endpoint(addr, port), ec);
		return !ec;
	}

	bool send_line(const value& v) {
		std::ostringstream os;
		format::json::print(v, os);
		os << '\n';
		std::string s = os.str();
		error_code ec;
		ba::write(sock, ba::buffer(s), ec);
		return !ec;
	}

	bool read_line(std::string& line) {
		error_code ec;
		ba::read_until(sock, buf, '\n', ec);
		if (ec && buf.size() == 0) return false;
		std::istream is(&buf);
		if (!std::getline(is, line)) return false;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		return true;
	}
};

static bool read_file(const std::string& path, std::string& text) {
	std::ifstream f(path, std::ios::binary);
	if (!f) return false;
	std::ostringstream ss;
	ss << f.rdbuf();
	text = ss.str();
	return true;
}

tgf_remote_evaluator::tgf_remote_evaluator(std::ostream& o, bool colors)
	: out(o)
{
	TC.set(colors);
}

tgf_remote_evaluator::~tgf_remote_evaluator() = default;

diagnostics::report tgf_remote_evaluator::take_report() {
	diagnostics::report r = std::move(report);
	report.clear();
	return r;
}

void tgf_remote_evaluator::apply_state(const value& state) {
	if (auto g = state.find("grammar"); g) {
		state_grammar_set = g->is_string();
		if (state_grammar_set) state_grammar = g->as_string();
	}
	if (auto s = state.find("start"); s) {
		state_start_set = s->is_string();
		if (state_start_set) state_start = s->as_string();
	}
	// The client colors a nonterminal name from this list, so the text
	// matches the local printer's character class color.
	if (auto c = state.find("char_classes"); c && c->is_array()) {
		state_char_classes.clear();
		for (const auto& n : *c)
			if (n.is_number())
				state_char_classes.insert(
					static_cast<size_t>(n.as_number()));
	}
}

bool tgf_remote_evaluator::connect(const std::string& host, uint16_t port,
	const std::string& session)
{
	client = std::make_unique<tgf_remote_client>();
	if (!client->connect(host, port)) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::connect_failed);
		return false;
	}
	value req = value::object();
	if (session.empty()) req.set("cmd", value::string("new"));
	else req.set("cmd", value::string("attach"))
		.set("session", value::string(session));
	if (!client->send_line(req)) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::connect_failed);
		return false;
	}
	// A fresh session answers with one hello line. An attach answers with
	// the lines the session buffered first, so render those and read on to
	// the hello.
	bool earlier_header = false;
	for (;;) {
		std::string line;
		if (!client->read_line(line)) {
			report.error(diagnostics::code::io_error,
				parser_strings::messages::connect_failed);
			return false;
		}
		auto v = format::json::parse(line);
		if (!v.has_value()) continue;
		const value& resp = v.value();
		auto hello = resp.find("hello");
		if (hello) {
			if (auto s = hello->find("session"); s && s->is_string())
				session_id_ = s->as_string();
			if (auto s = resp.find("state"); s && s->is_object())
				apply_state(*s);
			reprompt();
			return true;
		}
		// No hello before this line: it is a buffered answer or the
		// attach error. The server error carries no state and no cmd.
		bool is_error = false;
		if (auto st = resp.find("status"); st && st->is_string())
			is_error = st->as_string() == "error";
		if (is_error && !resp.find("state")
				&& !resp.find("cmd")) {
			if (auto rep = resp.find("report"); rep)
				print_diagnostics_report(
					format::json::report_from_value(*rep),
					false);
			report.error(diagnostics::code::invalid_argument,
				parser_strings::messages::connect_failed);
			return false;
		}
		if (!earlier_header) {
			out << "earlier answers:\n";
			earlier_header = true;
		}
		render_response(resp, "");
	}
}

void tgf_remote_evaluator::reprompt() {
	std::ostringstream ss;
	ss << TC_STATUS << "[ ";
	if (state_grammar_set)
		ss << TC_STATUS_FILE << "\"" << state_grammar << "\""
			<< TC.CLEAR() << TC_STATUS << " ";
	if (state_grammar_set || state_start_set)
		ss << TC_STATUS_START << state_start << TC.CLEAR()
			<< TC_STATUS << " ]" << TC.CLEAR() << " ";
	else
		ss << "]" << TC.CLEAR() << " ";
	ss << TC_PROMPT << "tgf>" << TC.CLEAR() << " ";
	if (r) r->set_prompt(ss.str());
#ifdef TAU_PARSER_HAS_FTXUI
	if (r_ftx) r_ftx->set_prompt(ss.str());
#endif
}

static cmd_status status_from_name(const std::string& st) {
	if (st == "ok") return cmd_status::ok;
	if (st == "quit") return cmd_status::quit;
	if (st == "incomplete") return cmd_status::incomplete;
	return cmd_status::error;
}

// Render one response object the way a normal answer is rendered. The
// reply of an `eval` request carries one entry per statement in `results`.
cmd_status tgf_remote_evaluator::render_response(const value& resp,
	const std::string& fallback_cmd)
{
	if (auto st = resp.find("state"); st && st->is_object())
		apply_state(*st);
	auto render_one = [this](const value& r, const std::string& cmd0) {
		cmd_result cr;
		cr.cmd = cmd0;
		if (auto c = r.find("cmd"); c && c->is_string())
			cr.cmd = c->as_string();
		if (auto s = r.find("status"); s && s->is_string())
			cr.status = status_from_name(s->as_string());
		if (auto d = r.find("result"); d) cr.data = *d;
		render_command_text(cr, out, TC, &state_char_classes);
		// The client prints the whole report through the text printer
		// of the local REPL, so its info and timing nodes appear in
		// the same cases.
		if (auto rep = r.find("report"); rep && rep->is_object())
			print_diagnostics_report(
				format::json::report_from_value(*rep), false);
	};
	cmd_status top = cmd_status::ok;
	if (auto s = resp.find("status"); s && s->is_string())
		top = status_from_name(s->as_string());
	if (auto rs = resp.find("results"); rs && rs->is_array()) {
		for (const auto& e : *rs) render_one(e, "");
		if (auto rep = resp.find("report"); rep && rep->is_object())
			print_diagnostics_report(
				format::json::report_from_value(*rep), false);
	} else render_one(resp, fallback_cmd);
	out.flush();
	return top;
}

// Send one structured request and render the answer. 0 continues, 1 is
// quit and 2 is incomplete.
int tgf_remote_evaluator::send_statement(value req) {
	auto cmd = req.find("cmd");
	std::string name = cmd && cmd->is_string() ? cmd->as_string() : "";
	// load and parse file carry the file text, not a path: the server
	// never reads a path a client gives.
	if (name == "load" || name == "parse file") {
		auto f = req.find("file");
		std::string path = f && f->is_string() ? f->as_string() : "";
		std::string text;
		if (!read_file(path, text)) {
			out << "error: could not open file: " << path << "\n";
			return 0;
		}
		value r2 = value::object();
		if (name == "load")
			r2.set("cmd", value::string("load"))
			  .set("name", value::string(path))
			  .set("source", value::string(text));
		else
			r2.set("cmd", value::string("parse"))
			  .set("input", value::string(text));
		req = std::move(r2);
	}
	if (!client || !client->send_line(req)) {
		out << "error: the connection is gone\n";
		return 1;
	}
	std::string line;
	if (!client->read_line(line)) {
		out << "error: the connection is gone\n";
		return 1;
	}
	auto p = format::json::parse(line);
	if (!p.has_value()) {
		out << "error: the server sent an unreadable response\n";
		return 0;
	}
	const value& resp = p.value();
	cmd_status status = render_response(resp, name);
	reprompt();
	if (status == cmd_status::quit) return 1;
	if (status == cmd_status::incomplete) return 2;
	return 0;
}

diagnostics::result<int> tgf_remote_evaluator::eval(const std::string& src) {
	diagnostics::result<int> res;
	static tgf_repl_parser rp;
	auto r = rp.parse(src.c_str(), src.size());
	if (!r.found) {
		if (r.parse_error.at_eof()) {
			// The client owns the REPL parse, so an incomplete line
			// needs no request to the server.
			diagnostics::result<int> res(2);
			r.report().demote_errors_to_warnings();
			res.report().append(std::move(r.report()));
			return res;
		}
		// A syntax error is a local report; no request reaches the
		// server. Print it, as the local REPL does.
		r.report().print(out, TC);
		out.flush();
		res.report().append(std::move(r.report()));
		return res;
	}
	tref ref = r.get_shaped_tree2();
	auto t = tt(ref);
	auto statements = t || tgf_repl_parser::statement;
	for (const auto& statement : statements()) {
		auto req = statement_request(statement | tt::only_child);
		int rc = send_statement(std::move(req));
		if (rc == 1) { res.emplace(1); return res; }
		if (rc == 2) { res.emplace(2); return res; }
	}
	res.emplace(0);
	return res;
}

diagnostics::result<int> tgf_connect_run(const connect_options& opt) {
	diagnostics::result<int> res;
	tgf_remote_evaluator re(std::cout);
	if (!re.connect(opt.host, opt.port, opt.session)) {
		res.report().append(re.take_report());
		return res;
	}
	std::cout << "session " << re.session_id() << "\n";
	std::cout.flush();
	int code = 0;
	if (opt.legacy_repl) {
		repl<tgf_remote_evaluator> front(re, "tgf> ", ".tgf_history");
		re.reprompt();
		code = front.run();
	} else {
#ifdef TAU_PARSER_HAS_FTXUI
		repl_ftxui<tgf_remote_evaluator> front(re, "tgf> ",
			".tgf_history");
		code = front.run();
#else
		repl<tgf_remote_evaluator> front(re, "tgf> ", ".tgf_history");
		re.reprompt();
		code = front.run();
#endif
	}
	res.report().append(re.take_report());
	res.emplace(code);
	return res;
}

} // namespace idni
