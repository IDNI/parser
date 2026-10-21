// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <array>
#include <utility>

#include "repl_json.h"

namespace idni {

namespace detail {

inline constexpr std::array<std::pair<cmd_status, const char*>, 4>
	cmd_status_names = { {
		{ cmd_status::ok,         "ok" },
		{ cmd_status::error,      "error" },
		{ cmd_status::incomplete, "incomplete" },
		{ cmd_status::quit,       "quit" },
	} };

inline tref json_status_value(cmd_status s) {
	return format::json::tree::build_string(cmd_status_name(s));
}

inline tref json_report_value(const diagnostics::report& r) {
	return format::json::tree::build_report(r, true);
}

} // namespace detail

inline const char* cmd_status_name(cmd_status s) {
	for (const auto& [st, name] : detail::cmd_status_names)
		if (st == s) return name;
	return detail::cmd_status_names[0].second;
}

inline tref json_cmd_value(const cmd_result& r) {
	using jt = format::json::tree;
	using k = repl_json_keys;
	return jt::build_object({
		jt::build_pair(k::cmd, jt::build_string(r.cmd)),
		jt::build_pair(k::status, detail::json_status_value(r.status)),
		jt::build_pair(k::result, r.data->get()),
		jt::build_pair(k::report, detail::json_report_value(r.report)) });
}

inline tref json_eval_response(tref id, const eval_result& er, tref state) {
	using jt = format::json::tree;
	using k = repl_json_keys;
	trefs arr;
	for (const auto& e : er.results) arr.push_back(json_cmd_value(e));
	return jt::build_object({
		jt::build_pair(k::id, id),
		jt::build_pair(k::status, detail::json_status_value(er.status)),
		jt::build_pair(k::results, jt::build_array(arr)),
		jt::build_pair(k::state, state),
		jt::build_pair(k::report, detail::json_report_value(er.report)) });
}

inline tref json_result_response(cmd_status status, const std::string& cmd,
	tref result, tref state, const diagnostics::report& report, tref id)
{
	using jt = format::json::tree;
	using k = repl_json_keys;
	trefs v;
	if (id) v.push_back(jt::build_pair(k::id, id));
	v.push_back(jt::build_pair(k::cmd, jt::build_string(cmd)));
	v.push_back(jt::build_pair(k::status, detail::json_status_value(status)));
	v.push_back(jt::build_pair(k::result, result));
	v.push_back(jt::build_pair(k::state, state));
	v.push_back(jt::build_pair(k::report, detail::json_report_value(report)));
	return jt::build_object(v);
}

inline tref json_status_response(tref id, cmd_status status, tref state,
	const diagnostics::report& report)
{
	using jt = format::json::tree;
	using k = repl_json_keys;
	return jt::build_object({
		jt::build_pair(k::id, id),
		jt::build_pair(k::status, detail::json_status_value(status)),
		jt::build_pair(k::state, state),
		jt::build_pair(k::report, detail::json_report_value(report)) });
}

inline tref json_error_response(tref id, tref state,
	const diagnostics::report& report)
{
	return json_status_response(id, cmd_status::error, state, report);
}

inline tref json_hello_line(tref hello, tref state) {
	using jt = format::json::tree;
	using k = repl_json_keys;
	return jt::build_object({
		jt::build_pair(k::hello, hello),
		jt::build_pair(k::state, state) });
}

inline tref json_hello_response(tref id, tref hello, tref state) {
	using jt = format::json::tree;
	using k = repl_json_keys;
	return jt::build_object({
		jt::build_pair(k::id, id),
		jt::build_pair(k::cmd, jt::build_string(k::hello)),
		jt::build_pair(k::status,
			detail::json_status_value(cmd_status::ok)),
		jt::build_pair(k::hello, hello),
		jt::build_pair(k::state, state) });
}

inline bool json_read_line(std::istream& in, std::string& line,
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

inline diagnostics::result<tref> json_parse_line(std::string_view line) {
	json_parser::parse_options po{ .start = json_parser::wire_line };
	auto r = json_parser::instance().parse(line.data(), line.size(), po);
	return format::json::tree::get(r);
}

} // namespace idni
