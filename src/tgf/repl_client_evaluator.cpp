// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "repl_client_evaluator.h"

#include <string>
#include <utility>

#include "parser_strings.h"
#include "utility/filesystem.h"
#include "utility/repl.h"

namespace idni {

using jt = format::json::tree;
using jtt = format::json::tt;
using format::json::string_member;
using rp = tgf_repl_parser;
using tp = tgf_protocol;

// One error for the cause @p got of a failed read. @p closed is the
// message of the caller for a lost connection.
static void read_failed(diagnostics::report& rep, read_status got,
	std::string_view closed)
{
	using messages = parser_strings::messages;
	if (got == read_status::timeout)
		rep.error(diagnostics::code::io_error, messages::net_read_timeout);
	else if (got == read_status::too_long)
		rep.error(diagnostics::code::out_of_range,
			messages::net_line_too_long);
	else rep.error(diagnostics::code::io_error, closed);
}

// The long name of the command nonterminal @p nt as a JSON string.
static tref command_value(size_t nt) {
	return jt::build_string(tgf_repl_evaluator_base::name_of(nt));
}

// load and parse file carry the file text, not a path: the server never
// reads a path a client gives. False with the read error in @p rep.
static bool inline_file(tref& req, diagnostics::report& rep) {
	const std::string* name = string_member(req, tp::cmd);
	const size_t nt = name ? tgf_repl_evaluator_base::command_of(*name) : 0;
	if (nt != rp::load_cmd && nt != rp::parse_file_cmd) return true;
	const std::string* path = string_member(req, tp::file);
	if (!path) return true;
	auto text = fs::read(fs::to_path(*path));
	if (!text.has_value()) {
		rep.append(text.report());
		return false;
	}
	trefs r2;
	if (tref id = jt::get(req).find(tp::id); id)
		r2.push_back(jt::build_pair(tp::id, id));
	if (nt == rp::load_cmd) {
		r2.push_back(jt::build_pair(tp::cmd,
			command_value(rp::load_cmd)));
		r2.push_back(jt::build_pair(tp::name, jt::build_string(*path)));
		r2.push_back(jt::build_pair(tp::source,
			jt::build_string(text.value())));
	} else {
		r2.push_back(jt::build_pair(tp::cmd,
			command_value(rp::parse_cmd)));
		r2.push_back(jt::build_pair(tp::input,
			jt::build_string(text.value())));
	}
	req = jt::build_object(r2);
	return true;
}

repl_client_evaluator::repl_client_evaluator(std::ostream& o, options op)
	: tgf_repl_evaluator_base(op), out(o)
{
}

const std::string& repl_client_evaluator::session_id() const noexcept {
	return session_id_;
}

tref repl_client_evaluator::state() const {
	return state_->get();
}

bool repl_client_evaluator::open_session(const connect_options& client)
{
	bintree<format::json::node>::gc_pause pause;
	const bool attach = jt::get(client.open_fields).find(tp::session);
	htref first;
	auto l = line_client_open(client,
		attach ? tp::attach : tp::new_session, first);
	report.append(l.report());
	if (!l.has_value()) return false;
	link = std::move(l).value();
	// A fresh session answers with one hello line. An attach answers with
	// the lines the session buffered first, so render those and read on to
	// the hello. The value is 1 for the hello, -1 for an error of the
	// server and 0 for a buffered answer.
	bool earlier_header = false;
	auto take = [this, &earlier_header](tref resp) {
		if (opt.json) format::json::write_line(resp, out);
		if (auto hello = jtt(resp) | jtt::member(tp::hello); hello) {
			if (auto s = string_member(hello.value(), tp::session);
					s)
				session_id_ = *s;
			if (auto s = jtt(resp) | jtt::member(tp::state);
					s.is(json_parser::object))
				state_ = s | jtt::handle;
			return 1;
		}
		// No hello before this line: it is a buffered answer or the
		// attach error. The server error carries no state and no cmd.
		const jt& r = jt::get(resp);
		const std::string* st = string_member(resp, tp::status);
		if (st && *st == status_name(cmd_status::error)
				&& !r.find(tp::state) && !r.find(tp::cmd)) {
			if (tref rep = r.find(tp::report); rep && !opt.json)
				print_report(format::json::report_of(rep));
			report.error(diagnostics::code::invalid_argument,
				parser_strings::messages::connect_failed);
			return -1;
		}
		if (opt.json) return 0;
		if (!earlier_header) {
			out << "earlier answers:\n";
			earlier_header = true;
		}
		render_response(resp, "");
		return 0;
	};
	for (int got = take(first->get()); ; ) {
		if (got != 0) return got > 0;
		std::string line;
		if (const read_status got = link->read_line(line);
				got != read_status::ok) {
			read_failed(report, got,
				parser_strings::messages::connect_failed);
			return false;
		}
		auto v = jt::get(std::string_view(line));
		got = v.has_value() ? take(v.value()) : 0;
	}
}

// Render one response object the way a normal answer is rendered. The
// reply of an `eval` request carries one entry per statement in `results`.
cmd_status repl_client_evaluator::render_response(tref resp,
	const std::string& fallback_cmd)
{
	if (auto st = jtt(resp) | jtt::member(tp::state);
			st.is(json_parser::object))
		state_ = st | jtt::handle;
	auto render_one = [this](tref r, const std::string& cmd0) {
		cmd_result cr;
		cr.cmd = cmd0;
		if (auto c = string_member(r, tp::cmd); c) cr.cmd = *c;
		if (auto s = string_member(r, tp::status); s)
			cr.status = status_of(*s);
		if (tref d = jt::get(r).find(tp::result); d)
			cr.data = jt::geth(d);
		render_text(cr, out);
		// The client prints the whole report through the text printer
		// of the local REPL, so its info and timing nodes appear in
		// the same cases.
		if (auto rep = jtt(r) | jtt::member(tp::report);
				rep.is(json_parser::object))
			print_report(format::json::report_of(rep.value()));
	};
	const std::string* top = string_member(resp, tp::status);
	if (auto rs = jtt(resp) | jtt::member(tp::results);
			rs.is(json_parser::arr)) {
		for (tref e : rs | jtt::children | jtt::refs) render_one(e, "");
		if (auto rep = jtt(resp) | jtt::member(tp::report);
				rep.is(json_parser::object))
			print_report(format::json::report_of(rep.value()));
	} else render_one(resp, fallback_cmd);
	out.flush();
	return top ? status_of(*top) : cmd_status::ok;
}

cmd_status repl_client_evaluator::request(tref req, bool& gone) {
	bintree<format::json::node>::gc_pause pause;
	gone = false;
	const std::string* cmd = string_member(req, tp::cmd);
	const std::string name = cmd ? *cmd : std::string();
	tref id = jt::_null();
	if (tref i = jt::get(req).find(tp::id); i) id = i;
	diagnostics::report read_rep;
	if (!inline_file(req, read_rep)) {
		if (opt.json) format::json::write_line(
			error_response(id, read_rep), out);
		else print_report(read_rep);
		return cmd_status::error;
	}
	if (!link || !link->send_line(jt::get(req).to_str())) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::connection_gone);
		gone = true;
		return cmd_status::error;
	}
	std::string line;
	if (const read_status got = link->read_line(line);
			got != read_status::ok) {
		read_failed(report, got,
			parser_strings::messages::connection_gone);
		gone = true;
		return cmd_status::error;
	}
	auto p = jt::get(std::string_view(line));
	if (!p.has_value()) {
		if (opt.json) out << line << "\n" << std::flush;
		else out << "error: the server sent an unreadable response\n";
		return cmd_status::error;
	}
	const tref resp = p.value();
	if (opt.json) {
		format::json::write_line(resp, out);
		const std::string* st = string_member(resp, tp::status);
		return st ? status_of(*st) : cmd_status::error;
	}
	return render_response(resp, name);
}

cmd_status repl_client_evaluator::load_grammar(const std::string& path) {
	bintree<format::json::node>::gc_pause pause;
	bool gone = false;
	return request(jt::build_object({
		jt::build_pair(tp::cmd, command_value(rp::load_cmd)),
		jt::build_pair(tp::file, jt::build_string(path)) }), gone);
}

cmd_result repl_client_evaluator::run_statement(const trv& s) {
	cmd_result cr;
	cr.cmd = name_of(s | trv::nonterminal);
	bool gone = false;
	diagnostics::report rep;
	tref req = request_of(s, rep);
	// a text that does not decode is never sent
	if (rep.has_error()) {
		if (opt.json) format::json::write_line(
			error_response(jt::_null(), rep), out);
		else print_report(rep);
		cr.status = cmd_status::error;
		return cr;
	}
	cr.status = request(req, gone);
	// A lost connection ends the REPL; the report carries the error.
	if (gone) cr.status = cmd_status::quit;
	return cr;
}

tref repl_client_evaluator::request_of(const trv& n,
	diagnostics::report& rep)
{
	const auto nt = n | trv::nonterminal;
	trefs req{ jt::build_pair(tp::cmd, command_value(nt)) };
	auto add = [&req](const char* key, std::string_view text) {
		req.push_back(jt::build_pair(key, jt::build_string(text)));
	};
	switch (nt) {
	case rp::parse_cmd: {
		auto i = n | rp::parse_input;
		std::string input;
		if (auto seq = i | rp::parse_input_char_seq; seq.has_value())
			input = seq | trv::terminals;
		else if (auto qs = i | rp::quoted_string; qs.has_value())
			input = unquote(qs | trv::terminals, rep);
		add(tp::input, input);
		break;
	}
	case rp::parse_file_cmd:
	case rp::load_cmd: {
		auto fn = n | rp::filename;
		add(tp::file, unquote(fn | trv::terminals, rep));
		break;
	}
	case rp::start_cmd:
	case rp::igrammar_cmd:
	case rp::unreachable_cmd: {
		auto sym = n | rp::symbol;
		if (sym.has_value())
			add(tp::symbol, sym | trv::terminals);
		break;
	}
	case rp::help_cmd: {
		auto h = n | rp::help_arg;
		if (h.has_value())
			add(tp::command, name_of(
				h | trv::only_child | trv::nonterminal));
		break;
	}
	case rp::get_cmd:
		if (auto name = option_name(n); !name.empty())
			add(tp::option, name);
		break;
	// The client knows no option kind, so the server reads the text.
	case rp::set_cmd:
	case rp::add_cmd:
	case rp::del_cmd:
		add(tp::option, option_name(n));
		add(tp::text, option_text(n));
		break;
	case rp::toggle_cmd:
	case rp::enable_cmd:
	case rp::disable_cmd:
		add(tp::option, option_name(n));
		break;
	default:
		// grammar, reload, version, license, quit and clear carry no
		// field beyond cmd.
		break;
	}
	return jt::build_object(req);
}

} // namespace idni
