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
using jt = format::json::tree;
using jtt = format::json::tt;

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

	bool send_line(tref v) {
		std::ostringstream os;
		format::json::write_line(v, os);
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
	diagnostics::report rep = std::move(report);
	report.clear();
	return rep;
}

void tgf_remote_evaluator::apply_state(tref state) {
	const jt& st = jt::get(state);
	if (tref g = st.find("grammar"); g) {
		state_grammar_set = jt::get(g).is_string();
		if (state_grammar_set) state_grammar = jt::get(g).get_string();
	}
	if (tref s = st.find("start"); s) {
		state_start_set = jt::get(s).is_string();
		if (state_start_set) state_start = jt::get(s).get_string();
	}
	// The client colors a nonterminal name from this list, so the text
	// matches the local printer's character class color.
	if (tref c = st.find("char_classes"); c && jt::get(c).is_array()) {
		state_char_classes.clear();
		for (const jt& n : jt::get(c).children_trees())
			if (n.is_number())
				state_char_classes.insert(
					static_cast<size_t>(n.get_number()));
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
	tref req = session.empty()
		? jt::build_object({
			jt::build_pair("cmd", jt::build_string("new")) })
		: jt::build_object({
			jt::build_pair("cmd", jt::build_string("attach")),
			jt::build_pair("session", jt::build_string(session)) });
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
		auto v = jt::get(std::string_view(line));
		if (!v.has_value()) continue;
		const tref resp = v.value();
		const jt& rt = jt::get(resp);
		if (tref hello = rt.find("hello"); hello) {
			if (auto s = format::json::string_member(hello,
					"session"); s)
				session_id_ = *s;
			if (tref s = rt.find("state"); s && jt::get(s).is_object())
				apply_state(s);
			reprompt();
			return true;
		}
		// No hello before this line: it is a buffered answer or the
		// attach error. The server error carries no state and no cmd.
		const std::string* st = format::json::string_member(resp,
			"status");
		const bool is_error = st && *st == "error";
		if (is_error && !rt.find("state") && !rt.find("cmd")) {
			if (tref rep = rt.find("report"); rep)
				tgf_repl_evaluator_base::print_report(
					format::json::report_of(rep), false,
					idni::TC);
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
cmd_status tgf_remote_evaluator::render_response(tref resp,
	const std::string& fallback_cmd)
{
	using format::json::string_member;
	const jt& rt = jt::get(resp);
	if (tref st = rt.find("state"); st && jt::get(st).is_object())
		apply_state(st);
	auto print_report = [](tref r) {
		if (tref rep = jt::get(r).find("report");
				rep && jt::get(rep).is_object())
			tgf_repl_evaluator_base::print_report(
				format::json::report_of(rep), false, idni::TC);
	};
	auto render_one = [this, &print_report](tref r,
		const std::string& cmd0)
	{
		cmd_result cr;
		cr.cmd = cmd0;
		if (auto c = string_member(r, "cmd"); c) cr.cmd = *c;
		if (auto s = string_member(r, "status"); s)
			cr.status = status_from_name(*s);
		if (tref d = jt::get(r).find("result"); d)
			cr.data = jt::geth(d);
		render_command_text(cr, out, TC, &state_char_classes);
		// The client prints the whole report through the text printer
		// of the local REPL, so its info and timing nodes appear in
		// the same cases.
		print_report(r);
	};
	cmd_status top = cmd_status::ok;
	if (auto s = string_member(resp, "status"); s)
		top = status_from_name(*s);
	if (tref rs = rt.find("results"); rs && jt::get(rs).is_array()) {
		for (tref e : (jtt(rs) | jtt::children | jtt::refs))
			render_one(e, "");
		print_report(resp);
	} else render_one(resp, fallback_cmd);
	out.flush();
	return top;
}

// Send one structured request and render the answer. 0 continues, 1 is
// quit and 2 is incomplete.
int tgf_remote_evaluator::send_statement(tref req) {
	using format::json::string_member;
	const std::string* cmd = string_member(req, "cmd");
	std::string name = cmd ? *cmd : "";
	// load and parse file carry the file text, not a path: the server
	// never reads a path a client gives.
	if (name == "load" || name == "parse file") {
		const std::string* f = string_member(req, "file");
		std::string path = f ? *f : "";
		std::string text;
		if (!read_file(path, text)) {
			out << "error: could not open file: " << path << "\n";
			return 0;
		}
		if (name == "load") req = jt::build_object({
			jt::build_pair("cmd", jt::build_string("load")),
			jt::build_pair("name", jt::build_string(path)),
			jt::build_pair("source", jt::build_string(text)) });
		else req = jt::build_object({
			jt::build_pair("cmd", jt::build_string("parse")),
			jt::build_pair("input", jt::build_string(text)) });
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
	auto p = jt::get(std::string_view(line));
	if (!p.has_value()) {
		out << "error: the server sent an unreadable response\n";
		return 0;
	}
	cmd_status status = render_response(p.value(), name);
	reprompt();
	if (status == cmd_status::quit) return 1;
	if (status == cmd_status::incomplete) return 2;
	return 0;
}

diagnostics::result<int> tgf_remote_evaluator::eval(const std::string& src) {
	diagnostics::result<int> res;
	static tgf_repl_parser rp;
	auto parsed = rp.parse(src.c_str(), src.size());
	if (!parsed.found) {
		if (parsed.parse_error.at_eof()) {
			// The client owns the REPL parse, so an incomplete line
			// needs no request to the server.
			diagnostics::result<int> incomplete(2);
			parsed.report().demote_errors_to_warnings();
			incomplete.report().append(std::move(parsed.report()));
			return incomplete;
		}
		// A syntax error is a local report; no request reaches the
		// server. Print it, as the local REPL does.
		parsed.report().print(out, TC);
		out.flush();
		res.report().append(std::move(parsed.report()));
		return res;
	}
	tref ref = parsed.get_shaped_tree2();
	auto t = tt(ref);
	auto statements = t || tgf_repl_parser::statement;
	for (const auto& statement : statements()) {
		auto req = statement_request(statement | tt::only_child);
		int rc = send_statement(req);
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
	auto take = [&res](diagnostics::result<int> run) {
		res.report().append(std::move(run).report());
		return run.has_value() ? run.value() : 1;
	};
	if (opt.legacy_repl) {
		repl<tgf_remote_evaluator> front(re, "tgf> ", { ".tgf_history" });
		re.reprompt();
		code = take(front.run());
	} else {
#ifdef TAU_PARSER_HAS_FTXUI
		repl_ftxui<tgf_remote_evaluator> front(re, "tgf> ",
			{ ".tgf_history" });
		code = take(front.run());
#else
		repl<tgf_remote_evaluator> front(re, "tgf> ", { ".tgf_history" });
		re.reprompt();
		code = take(front.run());
#endif
	}
	res.report().append(re.take_report());
	res.emplace(code);
	return res;
}

} // namespace idni
