// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>
#include <istream>
#include <ostream>
#include <set>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
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

using jt = format::json::tree;
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

// An absent field gives an empty text, a field of another type an error.
static diagnostics::result<std::string> optional_string_field(
	tref q, const char* name)
{
	if (!jt::get(q).find(name)) return ok_result(std::string());
	const std::string* f = format::json::string_member(q, name);
	if (!f) return diagnostics::error<std::string>(
		code::invalid_argument,
		parser_strings::messages::invalid_field_type);
	return ok_result(std::string(*f));
}

// A missing field and a field of another type are different errors.
static diagnostics::result<std::string> required_string_field(
	tref q, const char* name)
{
	if (!jt::get(q).find(name)) return diagnostics::error<std::string>(
		code::invalid_argument, parser_strings::messages::missing_field);
	return optional_string_field(q, name);
}

// Convert a structured request into the canonical REPL text. Every field
// check runs before any text is built.
static diagnostics::result<std::string> request_to_src(
	const tgf_repl_evaluator& re, const std::string& cmd, tref q)
{
	auto unknown = [] {
		return diagnostics::error<std::string>(
			code::invalid_argument,
			parser_strings::messages::unknown_command);
	};
	auto unknown_opt = [](std::string_view name) {
		diagnostics::result<std::string> r;
		return r.with_error(code::not_found,
			parser_strings::messages::option_not_found,
			{ { parser_strings::label::name, name } });
	};
	if (!known_command(cmd)) return unknown();

	if (cmd == "parse" || cmd == "parse file" || cmd == "load") {
		const char* field = cmd == "parse" ? "input" : "file";
		auto s = required_string_field(q, field);
		if (!s.has_value()) return s;
		return ok_result(cmd + " " + quote(s.value()));
	}

	if (cmd == "start" || cmd == "internal-grammar"
			|| cmd == "unreachable") {
		auto sym = optional_string_field(q, "symbol");
		if (!sym.has_value()) return sym;
		if (sym.value().empty()) return ok_result(cmd);
		if (!is_symbol(sym.value()))
			return diagnostics::error<std::string>(
				code::invalid_argument,
				parser_strings::messages::invalid_symbol);
		return ok_result(cmd + " " + sym.value());
	}

	if (cmd == "help") {
		auto c = optional_string_field(q, "command");
		if (!c.has_value()) return c;
		if (c.value().empty()) return ok_result(cmd);
		if (!known_help_argument(c.value()))
			return diagnostics::error<std::string>(
				code::invalid_argument,
				parser_strings::messages::invalid_help_argument);
		return ok_result(cmd + " " + c.value());
	}

	// A declared name is the only text of the option that reaches the
	// REPL text. set, add and delete run without REPL text.
	if (cmd == "get") {
		auto name = optional_string_field(q, "option");
		if (!name.has_value()) return name;
		if (name.value().empty()) return ok_result(cmd);
		if (!re.session_option(name.value()))
			return unknown_opt(name.value());
		return ok_result(cmd + " " + name.value());
	}

	if (cmd == "toggle" || cmd == "enable" || cmd == "disable") {
		auto name = required_string_field(q, "option");
		if (!name.has_value()) return name;
		if (!re.session_option(name.value()))
			return unknown_opt(name.value());
		return ok_result(cmd + " " + name.value());
	}

	// grammar, reload, version, license, quit, clear: no fields
	return ok_result(cmd);
}

static tref report_value(const diagnostics::report& r) {
	return jt::build_report(r, true);
}

tref build_option(const option_value& v) {
	return std::visit([](const auto& x) -> tref {
		using T = std::decay_t<decltype(x)>;
		if constexpr (std::is_same_v<T, bool>)
			return jt::build_bool(x);
		else if constexpr (std::is_same_v<T, std::size_t>)
			return jt::build_number(static_cast<double>(x));
		else if constexpr (std::is_same_v<T, std::string>)
			return jt::build_string(x);
		else {
			trefs a;
			for (const auto& e : x) a.push_back(jt::build_string(e));
			return jt::build_array(a);
		}
	}, v);
}

std::optional<option_value> option_of(tref v) {
	if (v == nullptr) return std::nullopt;
	const jt& t = jt::get(v);
	if (t.is_bool())
		return option_value(std::in_place_type<bool>, t.get_bool());
	if (t.is_number()) {
		const double d = t.get_number();
		// a double holds every whole number up to 2^53 exactly
		constexpr double exact = 9007199254740992.0;
		constexpr double max = static_cast<double>(
			std::numeric_limits<std::size_t>::max());
		if (!(d >= 0) || d > exact || d > max || std::trunc(d) < d)
			return std::nullopt;
		return option_value(std::in_place_type<std::size_t>,
			static_cast<std::size_t>(d));
	}
	if (t.is_string())
		return option_value(std::in_place_type<std::string>,
			t.get_string());
	if (!t.is_array()) return std::nullopt;
	option_list entries;
	for (const jt& e : t.children_trees()) {
		if (!e.is_string()) return std::nullopt;
		entries.push_back(e.get_string());
	}
	return option_value(std::in_place_type<option_list>,
		std::move(entries));
}

tref state_value(const tgf_repl_evaluator& re) {
	// The client colors a nonterminal name from this list, so the text
	// matches the local printer's character class color.
	trefs cc;
	if (re.has_grammar()) {
		const auto& nts = re.g().get_nts();
		for (size_t i = 0; i != nts.size(); ++i)
			if (re.g().is_cc_fn(i)) cc.push_back(
				jt::build_number(static_cast<double>(i)));
	}
	return jt::build_object({
		jt::build_pair("grammar", re.has_grammar()
			? jt::build_string(re.filename()) : jt::_null()),
		jt::build_pair("start", re.start_symbol().empty()
			? jt::_null() : jt::build_string(re.start_symbol())),
		jt::build_pair("char_classes", jt::build_array(cc)) });
}

static tref status_value(cmd_status s) {
	return jt::build_string(cmd_status_name(s));
}

static tref error_response(tref id, tref state, const diagnostics::report& r)
{
	return jt::build_object({
		jt::build_pair("id", id),
		jt::build_pair("status", status_value(cmd_status::error)),
		jt::build_pair("state", state),
		jt::build_pair("report", report_value(r)) });
}

static tref incomplete_response(tref id, tref state,
	const diagnostics::report& r)
{
	return jt::build_object({
		jt::build_pair("id", id),
		jt::build_pair("status", status_value(cmd_status::incomplete)),
		jt::build_pair("state", state),
		jt::build_pair("report", report_value(r)) });
}

tref json_result_response(cmd_status status, const std::string& cmd,
	tref result, tref state, const diagnostics::report& r)
{
	return jt::build_object({
		jt::build_pair("cmd", jt::build_string(cmd)),
		jt::build_pair("status", status_value(status)),
		jt::build_pair("result", result),
		jt::build_pair("state", state),
		jt::build_pair("report", report_value(r)) });
}

static tref json_id_response(tref id, const std::string& cmd,
	cmd_status status, tref result, tref state,
	const diagnostics::report& r)
{
	return jt::build_object({
		jt::build_pair("id", id),
		jt::build_pair("cmd", jt::build_string(cmd)),
		jt::build_pair("status", status_value(status)),
		jt::build_pair("result", result),
		jt::build_pair("state", state),
		jt::build_pair("report", report_value(r)) });
}

tref json_eval_response(tref id, const eval_result& er, tref state) {
	trefs arr;
	for (const auto& e : er.results)
		arr.push_back(jt::build_object({
			jt::build_pair("cmd", jt::build_string(e.cmd)),
			jt::build_pair("status", status_value(e.status)),
			jt::build_pair("result", e.data->get()),
			jt::build_pair("report", report_value(e.report)) }));
	return jt::build_object({
		jt::build_pair("id", id),
		jt::build_pair("status", status_value(er.status)),
		jt::build_pair("results", jt::build_array(arr)),
		jt::build_pair("state", state),
		jt::build_pair("report", report_value(er.report)) });
}

void json_write_line(std::ostream& os, const format::json::value& v) {
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
static tref hello_object(tgf_repl_evaluator& re,
	const diagnostics::report& extra = diagnostics::report{})
{
	const jt& state = jt::get(state_value(re));
	trefs h{ jt::build_pair("protocol", jt::build_number(1)),
		jt::build_pair("version",
			jt::build_string(tauparser::full_version)) };
	if (!re.session_id.empty()) h.push_back(
		jt::build_pair("session", jt::build_string(re.session_id)));
	h.push_back(jt::build_pair("grammar", state.find("grammar")));
	h.push_back(jt::build_pair("fixed_grammar",
		jt::build_bool(re.has_fixed_grammar())));
	h.push_back(jt::build_pair("start", state.find("start")));
	h.push_back(jt::build_pair("options", re.option_values()));
	diagnostics::report r = re.take_report();
	r.append(extra);
	h.push_back(jt::build_pair("report", report_value(r)));
	return jt::build_object(h);
}

static tref hello(tgf_repl_evaluator& re,
	const diagnostics::report& extra = diagnostics::report{})
{
	return jt::build_object({
		jt::build_pair("hello", hello_object(re, extra)),
		jt::build_pair("state", state_value(re)) });
}

// A set, add or delete request writes the option through the repository,
// so no value text is parsed as REPL text. The field text holds the value
// as the REPL writes it, the field value holds it as JSON.
static std::pair<tref, diagnostics::report> option_request(
	tgf_repl_evaluator& re, const std::string& cmd, tref q)
{
	auto fail = [](std::string_view msg) {
		diagnostics::report rep;
		rep.error(code::invalid_argument, msg);
		return std::pair{ jt::build_object({}), std::move(rep) };
	};
	const jt& req = jt::get(q);
	auto name = required_string_field(q, "option");
	if (!name.has_value())
		return { jt::build_object({}), name.report() };
	const bool add = cmd == "add";
	tref data = nullptr;
	if (req.find("text") && req.find("value"))
		return fail(parser_strings::messages::invalid_field_type);
	if (req.find("text")) {
		auto t = required_string_field(q, "text");
		if (!t.has_value())
			return { jt::build_object({}), t.report() };
		data = cmd == "set" ? re.set_cmd(name.value(), t.value())
			: re.list_cmd(name.value(), t.value(), add);
		return { data, re.take_report() };
	}
	tref f = req.find("value");
	if (!f) return fail(parser_strings::messages::missing_field);
	auto v = option_of(f);
	if (!v) return fail(parser_strings::messages::invalid_field_type);
	if (cmd == "set") data = re.set_option(name.value(), std::move(*v));
	else if (const auto* entries = std::get_if<option_list>(&*v))
		data = re.edit_option_list(name.value(), *entries, add);
	else return fail(parser_strings::messages::invalid_field_type);
	return { data, re.take_report() };
}

static std::pair<tref, cmd_status> handle_request(
	tgf_repl_evaluator& re, const std::string& line)
{
	auto req = jt::get(line);
	if (!req.has_value())
		return { error_response(jt::_null(), state_value(re),
			req.report()), cmd_status::error };
	tref q = req.value();
	tref id = jt::_null();
	if (tref i = jt::get(q).find("id"); i) id = i;
	auto cmd = required_string_field(q, "cmd");
	if (!cmd.has_value())
		return { error_response(id, state_value(re), cmd.report()),
			cmd_status::error };
	if (cmd.value() == "eval") {
		auto src = required_string_field(q, "src");
		if (!src.has_value())
			return { error_response(id, state_value(re),
				src.report()), cmd_status::error };
		auto er = re.run(src.value());
		cmd_status st = er.status == cmd_status::quit
			? cmd_status::quit : cmd_status::ok;
		return { json_eval_response(id, er, state_value(re)), st };
	}
	if (cmd.value() == "hello") {
		const tref h = hello_object(re);
		return { jt::build_object({
			jt::build_pair("id", id),
			jt::build_pair("cmd", jt::build_string("hello")),
			jt::build_pair("status", status_value(cmd_status::ok)),
			jt::build_pair("hello", h),
			jt::build_pair("state", state_value(re)) }),
			cmd_status::ok };
	}
	// The load form with source text carries no path, so it runs the
	// evaluator directly instead of going through the REPL text.
	if (cmd.value() == "load" && jt::get(q).find("source")) {
		auto name = required_string_field(q, "name");
		if (!name.has_value())
			return { error_response(id, state_value(re),
				name.report()), cmd_status::error };
		auto source = required_string_field(q, "source");
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
	if (cmd.value() == "set" || cmd.value() == "add"
		|| cmd.value() == "delete")
	{
		auto [data, rep] = option_request(re, cmd.value(), q);
		cmd_status st = rep.has_error()
			? cmd_status::error : cmd_status::ok;
		return { json_id_response(id, cmd.value(), st, data,
			state_value(re), rep), st };
	}
	auto src = request_to_src(re, cmd.value(), q);
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
	return { json_id_response(id, r.cmd, r.status, r.data->get(),
		state_value(re), r.report), st };
}

// Apply one element of the init option values. A bad value reports an
// error; the rest of the init stays applied.
static void apply_init_option(tgf_repl_evaluator& re,
	const std::string& name, tref val, diagnostics::report& r)
{
	auto v = option_of(val);
	if (!v) {
		r.error(diagnostics::code::invalid_argument,
			parser_strings::messages::invalid_field_type);
		return;
	}
	re.set_option(name, std::move(*v));
	r.append(re.take_report());
}

// Apply the init line of a session child: the grammar, the start symbol,
// the option values and the line limit. A bad part reports an error and
// the child goes on with what it could apply.
static diagnostics::report apply_init(tgf_repl_evaluator& re,
	tref init_ref, size_t& max_line)
{
	diagnostics::report r;
	const jt& init = jt::get(init_ref);
	if (!init.is_object()) {
		r.error(diagnostics::code::invalid_argument,
			parser_strings::messages::invalid_field_type);
		return r;
	}
	// The line limit applies to the lines after the init line, which was
	// already read with the limit the caller passed.
	if (tref m = init.find("max_line"); m && !jt::get(m).is_null()) {
		if (!jt::get(m).is_number())
			r.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_field_type);
		else max_line = static_cast<size_t>(jt::get(m).get_number());
	}
	// The grammar key is null with no grammar and absent for a built-in
	// grammar, which the child already has and keeps.
	if (tref g = init.find("grammar"); g && !jt::get(g).is_null()) {
		if (!jt::get(g).is_object()) {
			r.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_field_type);
		} else {
			auto name = required_string_field(g, "name");
			auto source = required_string_field(g, "source");
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
	if (tref s = init.find("start"); s && !jt::get(s).is_null()) {
		if (!jt::get(s).is_string()) {
			r.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_field_type);
		} else apply_init_option(re, "start", s, r);
	}
	if (tref o = init.find("options"); o && !jt::get(o).is_null()) {
		if (!jt::get(o).is_object()) {
			r.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_field_type);
		} else {
			for (const jt& kv : jt::get(o).children_trees())
				apply_init_option(re, kv.get_string(),
					kv.first(), r);
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
		bintree<format::json::node>::gc_pause pause;
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
				auto parsed = jt::get(init_line);
				tref node = parsed.has_value()
					? jt::get(parsed.value()).find("init")
					: nullptr;
				if (!parsed.has_value())
					init_report.append(parsed.report());
				else if (!node)
					init_report.error(
						diagnostics::code::invalid_argument,
						parser_strings::messages::missing_field);
				else init_report.append(apply_init(re, node, limit));
			}
		} else init_report.error(
			diagnostics::code::invalid_argument,
			parser_strings::messages::missing_field);
	}
	{
		bintree<format::json::node>::gc_pause pause;
		format::json::write_line(hello(re, init_report), out);
	}
	for (std::string line; ; ) {
		bool too_long = false;
		if (!json_read_line(in, line, limit, too_long)) break;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (!too_long && line.empty()) continue;
		cmd_status st = cmd_status::ok;
		{
			bintree<format::json::node>::gc_pause pause;
			if (too_long) {
				// Parsing a huge line costs time and memory, so
				// refuse it and go on with the next line.
				diagnostics::report rep;
				rep.error(diagnostics::code::out_of_range,
					parser_strings::messages::line_too_long);
				format::json::write_line(error_response(
					jt::_null(), state_value(re), rep), out);
			} else {
				auto [resp, s] = handle_request(re, line);
				format::json::write_line(resp, out);
				st = s;
			}
		}
		// The answer is written and no tref of it is left.
		jt::gc_if_grown();
		// The child of a server session stays alive after a quit: the
		// parent owns the connection and closes it after this answer.
		if (st == cmd_status::quit && re.session_id.empty()) break;
	}
	return 0;
}

} // namespace idni
