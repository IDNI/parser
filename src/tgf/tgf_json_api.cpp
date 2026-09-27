// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <cctype>
#include <iostream>
#include <istream>
#include <ostream>
#include <set>
#include <streambuf>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "defs.h"
#include "parser.h"
#include "parser_strings.h"
#include "tgf_cli.h"
#include "format/json/json.h"
#include "utility/diagnostics.h"
#include "utility/escapes.h"

namespace idni {

using format::json::value;
using diagnostics::code;

// Success result carrying @p v. result<std::string> has no value
// constructor, so every string value goes through emplace().
template <typename T>
static diagnostics::result<T> ok_result(T v) {
	diagnostics::result<T> r;
	r.emplace(std::move(v));
	return r;
}

static bool is_symbol(std::string_view s) {
	if (s.empty()) return false;
	unsigned char c0 = static_cast<unsigned char>(s[0]);
	if (!(std::isalpha(c0) || s[0] == '_')) return false;
	for (size_t i = 1; i < s.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(s[i]);
		if (!(std::isalnum(c) || s[i] == '_')) return false;
	}
	return true;
}

static std::string quote(std::string_view s) {
	return "\"" + escapes::encode(s, escapes::tgf_string) + "\"";
}

static bool known_command(const std::string& c) {
	static const std::set<std::string> names = {
		"parse", "parse file", "grammar", "internal-grammar",
		"start", "unreachable", "reload", "load", "help",
		"version", "license", "quit", "clear", "get", "set",
		"toggle", "enable", "disable", "add", "delete" };
	return names.count(c) != 0;
}

static bool known_help_argument(const std::string& c) {
	static const std::set<std::string> names = {
		"grammar", "internal-grammar", "unreachable", "start",
		"parse", "parse file", "load", "reload", "clear", "help",
		"quit", "version", "license", "get", "set", "add",
		"delete", "toggle", "enable", "disable" };
	return names.count(c) != 0;
}

static diagnostics::result<std::string> string_field(
	const value& q, const char* name)
{
	auto f = q.find(name);
	if (!f) return diagnostics::error<std::string>(code::invalid_argument,
		parser_strings::messages::missing_field);
	if (!f->is_string()) return diagnostics::error<std::string>(
		code::invalid_argument,
		parser_strings::messages::invalid_field_type);
	return ok_result(std::string(f->as_string()));
}

// Render a request value as REPL text, checked against the option kind.
// An empty list or tree path list renders nothing, which clears the
// option.
static diagnostics::result<std::string> value_to_src(
	const option_desc& d, const value& v)
{
	auto type_error = [] {
		return diagnostics::error<std::string>(
			code::invalid_argument,
			parser_strings::messages::invalid_field_type);
	};
	switch (d.kind) {
	case option_kind::boolean:
		if (!v.is_bool()) return type_error();
		return ok_result(std::string(v.as_bool() ? "true" : "false"));
	case option_kind::string_value: {
		if (!v.is_string()) return type_error();
		const std::string& s = v.as_string();
		if (s != "basic" && s != "detailed" && s != "root-cause")
			return diagnostics::error<std::string>(
				code::invalid_argument,
				parser_strings::messages::invalid_error_verbosity);
		return ok_result(s);
	}
	case option_kind::symbol_value: {
		if (!v.is_string()) return type_error();
		if (!is_symbol(v.as_string()))
			return diagnostics::error<std::string>(
				code::invalid_argument,
				parser_strings::messages::invalid_symbol);
		return ok_result(std::string(v.as_string()));
	}
	case option_kind::list: {
		if (!v.is_array()) return type_error();
		std::string out;
		bool first = true;
		for (const auto& e : v) {
			if (!e.is_string() || !is_symbol(e.as_string()))
				return diagnostics::error<std::string>(
					code::invalid_argument,
					parser_strings::messages::invalid_symbol);
			out += first ? first = false, "" : ", ";
			out += e.as_string();
		}
		return ok_result(std::move(out));
	}
	case option_kind::treepaths: {
		if (!v.is_array()) return type_error();
		std::string out;
		bool first = true;
		for (const auto& tp : v) {
			if (!tp.is_array()) return type_error();
			out += first ? first = false, "" : ", ";
			bool first_s = true;
			for (const auto& s : tp) {
				if (!s.is_string() || !is_symbol(s.as_string()))
					return diagnostics::error<std::string>(
						code::invalid_argument,
						parser_strings::messages::
							invalid_symbol);
				out += first_s ? first_s = false, "" : " > ";
				out += s.as_string();
			}
		}
		return ok_result(std::move(out));
	}
	}
	return type_error();
}

// Convert a structured request into the canonical REPL text. Every field
// check runs before any text is built.
static diagnostics::result<std::string> request_to_src(
	const std::string& cmd, const value& q)
{
	auto unknown = [] {
		return diagnostics::error<std::string>(
			code::invalid_argument,
			parser_strings::messages::unknown_command);
	};
	auto unknown_opt = [] {
		return diagnostics::error<std::string>(
			code::invalid_argument,
			parser_strings::messages::unknown_option);
	};
	auto missing = [] {
		return diagnostics::error<std::string>(
			code::invalid_argument,
			parser_strings::messages::missing_field);
	};
	auto bad_type = [] {
		return diagnostics::error<std::string>(
			code::invalid_argument,
			parser_strings::messages::invalid_field_type);
	};
	if (!known_command(cmd)) return unknown();

	if (cmd == "parse" || cmd == "parse file" || cmd == "load") {
		const char* field = cmd == "parse" ? "input" : "file";
		auto s = string_field(q, field);
		if (!s.has_value()) return s;
		return ok_result(cmd + " " + quote(s.value()));
	}

	if (cmd == "start" || cmd == "internal-grammar"
			|| cmd == "unreachable") {
		auto f = q.find("symbol");
		std::string sym;
		if (f) {
			if (!f->is_string()) return bad_type();
			sym = f->as_string();
		}
		if (sym.empty()) return ok_result(cmd);
		if (!is_symbol(sym))
			return diagnostics::error<std::string>(
				code::invalid_argument,
				parser_strings::messages::invalid_symbol);
		return ok_result(cmd + " " + sym);
	}

	if (cmd == "help") {
		auto f = q.find("command");
		std::string c;
		if (f) {
			if (!f->is_string()) return bad_type();
			c = f->as_string();
		}
		if (c.empty()) return ok_result(std::string("help"));
		if (!known_help_argument(c))
			return diagnostics::error<std::string>(
				code::invalid_argument,
				parser_strings::messages::invalid_help_argument);
		return ok_result("help " + c);
	}

	if (cmd == "get") {
		auto f = q.find("option");
		std::string name;
		if (f) {
			if (!f->is_string()) return bad_type();
			name = f->as_string();
		}
		if (name.empty()) return ok_result(std::string("get"));
		if (!option_desc_by_name(name)) return unknown_opt();
		return ok_result("get " + name);
	}

	if (cmd == "set") {
		auto name = string_field(q, "option");
		if (!name.has_value()) return name;
		const option_desc* d = option_desc_by_name(name.value());
		if (!d) return unknown_opt();
		auto val = q.find("value");
		if (!val) return missing();
		auto rendered = value_to_src(*d, *val);
		if (!rendered.has_value()) return rendered;
		std::string src = "set " + name.value();
		if (!rendered.value().empty()) src += " " + rendered.value();
		return ok_result(std::move(src));
	}

	if (cmd == "toggle" || cmd == "enable" || cmd == "disable") {
		auto name = string_field(q, "option");
		if (!name.has_value()) return name;
		const option_desc* d = option_desc_by_name(name.value());
		if (!d) return unknown_opt();
		if (d->kind != option_kind::boolean) return unknown_opt();
		return ok_result(cmd + " " + name.value());
	}

	if (cmd == "add" || cmd == "delete") {
		auto name = string_field(q, "option");
		if (!name.has_value()) return name;
		const option_desc* d = option_desc_by_name(name.value());
		if (!d) return unknown_opt();
		if (d->kind != option_kind::list
				&& d->kind != option_kind::treepaths)
			return unknown_opt();
		auto val = q.find("value");
		if (!val) return missing();
		auto rendered = value_to_src(*d, *val);
		if (!rendered.has_value()) return rendered;
		if (rendered.value().empty()) return missing();
		return ok_result(cmd + " " + name.value() + " "
			+ rendered.value());
	}

	// grammar, reload, version, license, quit, clear: no fields
	return ok_result(cmd);
}

static value report_value(const diagnostics::report& r) {
	return format::json::to_value(r, true);
}

format::json::value state_value(const tgf_repl_evaluator& re) {
	using value = format::json::value;
	value v = value::object();
	v.set("grammar", re.has_grammar()
		? value::string(re.filename()) : value::null());
	v.set("start", re.start_symbol().empty() ? value::null()
		: value::string(re.start_symbol()));
	// The client colors a nonterminal name from this list, so the text
	// matches the local printer's character class color.
	value cc = value::array();
	if (re.has_grammar()) {
		const auto& nts = re.g().get_nts();
		for (size_t i = 0; i != nts.size(); ++i)
			if (re.g().is_cc_fn(i))
				cc.push_back(value::number(
					static_cast<double>(i)));
	}
	v.set("char_classes", std::move(cc));
	return v;
}

static value error_response(const value& id, const value& state,
	const diagnostics::report& r)
{
	value v = value::object();
	v.set("id", id);
	v.set("status", value::string("error"));
	v.set("state", state);
	v.set("report", report_value(r));
	return v;
}

static value incomplete_response(const value& id, const value& state,
	const diagnostics::report& r)
{
	value v = value::object();
	v.set("id", id);
	v.set("status", value::string("incomplete"));
	v.set("state", state);
	v.set("report", report_value(r));
	return v;
}

format::json::value json_result_response(cmd_status status,
	const std::string& cmd, const value& result, const value& state,
	const diagnostics::report& r)
{
	value v = value::object();
	v.set("cmd", value::string(cmd));
	v.set("status", value::string(cmd_status_name(status)));
	v.set("result", result);
	v.set("state", state);
	v.set("report", report_value(r));
	return v;
}

static format::json::value json_id_response(const value& id,
	const std::string& cmd, cmd_status status, const value& result,
	const value& state, const diagnostics::report& r)
{
	value v = value::object();
	v.set("id", id);
	v.set("cmd", value::string(cmd));
	v.set("status", value::string(cmd_status_name(status)));
	v.set("result", result);
	v.set("state", state);
	v.set("report", report_value(r));
	return v;
}

format::json::value json_eval_response(const value& id,
	const eval_result& er, const value& state)
{
	value v = value::object();
	v.set("id", id);
	v.set("status", value::string(cmd_status_name(er.status)));
	value arr = value::array();
	for (const auto& e : er.results) {
		value n = value::object();
		n.set("cmd", value::string(e.cmd));
		n.set("status", value::string(cmd_status_name(e.status)));
		n.set("result", e.data);
		n.set("report", report_value(e.report));
		arr.push_back(std::move(n));
	}
	v.set("results", std::move(arr));
	v.set("state", state);
	v.set("report", report_value(er.report));
	return v;
}

void json_write_line(std::ostream& os, const value& v) {
#ifdef _WIN32
	// A JSON message ends with one '\n', so stdout must not add a '\r'.
	if (&os == &std::cout) ::_setmode(::_fileno(stdout), _O_BINARY);
#endif
	format::json::print(v, os) << '\n' << std::flush;
}

// The hello object carries the evaluator state and the pending report of
// the grammar load, which take_report() then clears. The session field is
// present only when the evaluator has a session id. @p extra holds the
// errors of an init line, which go out with the first hello.
static value hello_object(tgf_repl_evaluator& re,
	const diagnostics::report& extra = diagnostics::report{})
{
	value h = value::object();
	h.set("protocol", value::number(1));
	h.set("version", value::string(tauparser::full_version));
	if (!re.session_id.empty())
		h.set("session", value::string(re.session_id));
	h.set("grammar", re.has_grammar()
		? value::string(re.filename()) : value::null());
	h.set("fixed_grammar", value::boolean(re.has_fixed_grammar()));
	h.set("start", re.start_symbol().empty() ? value::null()
		: value::string(re.start_symbol()));
	h.set("options", re.option_values());
	h.set("directives", re.directives_value());
	diagnostics::report r = re.take_report();
	r.append(extra);
	h.set("report", report_value(r));
	return h;
}

static value hello(tgf_repl_evaluator& re,
	const diagnostics::report& extra = diagnostics::report{})
{
	value v = value::object();
	v.set("hello", hello_object(re, extra));
	v.set("state", state_value(re));
	return v;
}

static std::pair<value, cmd_status> handle_request(
	tgf_repl_evaluator& re, const std::string& line)
{
	auto req = format::json::parse(line);
	if (!req.has_value())
		return { error_response(value::null(), state_value(re),
			req.report()), cmd_status::error };
	const value& q = req.value();
	value id = value::null();
	if (auto i = q.find("id"); i) id = *i;
	auto cmd = string_field(q, "cmd");
	if (!cmd.has_value())
		return { error_response(id, state_value(re), cmd.report()),
			cmd_status::error };
	if (cmd.value() == "eval") {
		auto src = string_field(q, "src");
		if (!src.has_value())
			return { error_response(id, state_value(re),
				src.report()), cmd_status::error };
		auto er = re.run(src.value());
		cmd_status st = er.status == cmd_status::quit
			? cmd_status::quit : cmd_status::ok;
		return { json_eval_response(id, er, state_value(re)), st };
	}
	if (cmd.value() == "hello") {
		value v = value::object();
		v.set("id", id);
		v.set("cmd", value::string("hello"));
		v.set("status", value::string("ok"));
		v.set("hello", hello_object(re));
		v.set("state", state_value(re));
		return { std::move(v), cmd_status::ok };
	}
	// The load form with source text carries no path, so it runs the
	// evaluator directly instead of going through the REPL text.
	if (cmd.value() == "load" && q.find("source") != nullptr) {
		auto name = string_field(q, "name");
		if (!name.has_value())
			return { error_response(id, state_value(re),
				name.report()), cmd_status::error };
		auto source = string_field(q, "source");
		if (!source.has_value())
			return { error_response(id, state_value(re),
				source.report()), cmd_status::error };
		auto data = re.load_source_data(name.value(), source.value());
		auto rep = re.take_report();
		cmd_status st = rep.has_error()
			? cmd_status::error : cmd_status::ok;
		return { json_id_response(id, "load", st, data,
			state_value(re), rep), st };
	}
	auto src = request_to_src(cmd.value(), q);
	if (!src.has_value())
		return { error_response(id, state_value(re), src.report()),
			cmd_status::error };
	auto er = re.run(src.value());
	if (er.status == cmd_status::incomplete)
		return { incomplete_response(id, state_value(re),
			er.report), cmd_status::ok };
	if (er.results.empty())
		return { error_response(id, state_value(re), er.report),
			cmd_status::error };
	const cmd_result& r = er.results.front();
	cmd_status st = r.status == cmd_status::quit
		? cmd_status::quit : cmd_status::ok;
	return { json_id_response(id, r.cmd, r.status, r.data,
		state_value(re), r.report), st };
}

// Apply one element of the init option values as a `set` request. A bad
// value reports an error; the rest of the init stays applied.
static void apply_init_option(tgf_repl_evaluator& re,
	const std::string& name, const value& val, diagnostics::report& r)
{
	const option_desc* d = option_desc_by_name(name);
	if (!d) {
		r.error(diagnostics::code::invalid_argument,
			parser_strings::messages::unknown_option);
		return;
	}
	// A null value names no setting, for example start with no symbol.
	if (val.is_null()) return;
	value q = value::object();
	q.set("option", value::string(name));
	q.set("value", val);
	auto src = request_to_src("set", q);
	if (!src.has_value()) { r.append(src.report()); return; }
	auto er = re.run(src.value());
	r.append(std::move(er.report));
	for (auto& cr : er.results) r.append(std::move(cr.report));
}

// Apply the init line of a session child: the grammar, the start symbol,
// the option values and the line limit. A bad part reports an error and
// the child goes on with what it could apply.
static diagnostics::report apply_init(tgf_repl_evaluator& re,
	const value& init, size_t& max_line)
{
	diagnostics::report r;
	if (!init.is_object()) {
		r.error(diagnostics::code::invalid_argument,
			parser_strings::messages::invalid_field_type);
		return r;
	}
	// The line limit applies to the lines after the init line, which was
	// already read with the limit the caller passed.
	if (auto m = init.find("max_line"); m && !m->is_null()) {
		if (!m->is_number())
			r.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_field_type);
		else max_line = static_cast<size_t>(m->as_number());
	}
	// The grammar key is null with no grammar and absent for a built-in
	// grammar, which the child already has and keeps.
	if (auto g = init.find("grammar"); g && !g->is_null()) {
		if (!g->is_object()) {
			r.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_field_type);
		} else {
			auto name = string_field(*g, "name");
			auto source = string_field(*g, "source");
			if (!name.has_value()) r.append(name.report());
			else if (!source.has_value())
				r.append(source.report());
			else if (re.has_fixed_grammar()) {
				// A built-in grammar is kept, like the normal
				// load of a fixed-grammar evaluator.
				r.warning(parser_strings::messages::
					loading_grammars_unavailable);
			} else {
				re.load_source(name.value(), source.value());
				r.append(re.take_report());
			}
		}
	}
	if (auto s = init.find("start"); s && !s->is_null()) {
		if (!s->is_string()) {
			r.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_field_type);
		} else apply_init_option(re, "start", *s, r);
	}
	if (auto o = init.find("options"); o && !o->is_null()) {
		if (!o->is_object()) {
			r.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_field_type);
		} else {
			for (const auto& kv : o->members())
				apply_init_option(re, kv.first, kv.second, r);
		}
	}
	return r;
}

// Read one line into @p line without ever holding more than @p max_line
// bytes. Bytes past the limit are dropped up to the next newline. The
// return value is false only at end of input; @p too_long marks a line
// that was cut at the limit.
static bool json_read_line(std::istream& in, std::string& line,
	size_t max_line, bool& too_long)
{
	line.clear();
	too_long = false;
	std::streambuf* sb = in.rdbuf();
	if (!sb) return false;
	for (;;) {
		std::streambuf::int_type ci = sb->sbumpc();
		if (ci == std::streambuf::traits_type::eof()) {
			in.setstate(std::ios::eofbit);
			return !line.empty() || too_long;
		}
		char c = static_cast<char>(ci);
		if (c == '\n') return true;
		if (line.size() >= max_line) {
			// The bytes past the limit are dropped, so a long
			// line never sits whole in memory.
			too_long = true;
			continue;
		}
		line.push_back(c);
	}
}

int tgf_json_loop(tgf_repl_evaluator& re, std::istream& in,
	std::ostream& out, size_t max_line, bool init_stdin,
	diagnostics::report extra)
{
	diagnostics::report init_report = std::move(extra);
	size_t limit = max_line;
	if (init_stdin) {
		std::string init_line;
		bool too_long = false;
		if (json_read_line(in, init_line, limit, too_long)) {
			if (!init_line.empty() && init_line.back() == '\r')
				init_line.pop_back();
			if (too_long)
				init_report.error(
					diagnostics::code::out_of_range,
					parser_strings::messages::line_too_long);
			else if (init_line.empty())
				init_report.error(
					diagnostics::code::invalid_argument,
					parser_strings::messages::missing_field);
			else {
				auto parsed = format::json::parse(init_line);
				auto node = parsed.has_value()
					? parsed.value().find("init") : nullptr;
				if (!parsed.has_value())
					init_report.append(parsed.report());
				else if (!node)
					init_report.error(
						diagnostics::code::invalid_argument,
						parser_strings::messages::missing_field);
				else init_report.append(apply_init(re, *node, limit));
			}
		} else init_report.error(
			diagnostics::code::invalid_argument,
			parser_strings::messages::missing_field);
	}
	json_write_line(out, hello(re, init_report));
	for (std::string line; ; ) {
		bool too_long = false;
		if (!json_read_line(in, line, limit, too_long)) break;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (too_long) {
			// Parsing a huge line costs time and memory, so refuse
			// it and go on with the next line.
			diagnostics::report rep;
			rep.error(diagnostics::code::out_of_range,
				parser_strings::messages::line_too_long);
			json_write_line(out, error_response(value::null(),
				state_value(re), rep));
			continue;
		}
		if (line.empty()) continue;
		auto [resp, st] = handle_request(re, line);
		json_write_line(out, resp);
		// The child of a server session stays alive after a quit: the
		// parent owns the connection and closes it after this answer.
		if (st == cmd_status::quit && re.session_id.empty()) break;
	}
	return 0;
}

} // namespace idni
