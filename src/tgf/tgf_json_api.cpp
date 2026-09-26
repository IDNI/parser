// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <cctype>
#include <istream>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
	v.set("grammar", value::string(re.filename()));
	v.set("start", value::string(re.start_symbol()));
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
	format::json::print(v, os) << '\n' << std::flush;
}

// The hello line carries the evaluator state and the pending report of the
// grammar load, which take_report() then clears.
static value hello(tgf_repl_evaluator& re) {
	value h = value::object();
	h.set("protocol", value::number(1));
	h.set("version", value::string(tauparser::full_version));
	h.set("grammar", value::string(re.filename()));
	h.set("fixed_grammar", value::boolean(re.has_fixed_grammar()));
	h.set("start", value::string(re.start_symbol()));
	h.set("options", re.option_values());
	h.set("report", report_value(re.take_report()));
	value v = value::object();
	v.set("hello", std::move(h));
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

int tgf_json_loop(tgf_repl_evaluator& re, std::istream& in,
	std::ostream& out)
{
	json_write_line(out, hello(re));
	for (std::string line; std::getline(in, line); ) {
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty()) continue;
		auto [resp, st] = handle_request(re, line);
		json_write_line(out, resp);
		if (st == cmd_status::quit) break;
	}
	return 0;
}

} // namespace idni
