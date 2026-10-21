// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_JSON_H__
#define __IDNI__PARSER__UTILITY__REPL_JSON_H__

#include <cstddef>
#include <istream>
#include <string>
#include <string_view>

#include "utility/diagnostics.h"
#include "utility/repl_result.h"
#include "format/json/json.h"

namespace idni {

/// The fields of the JSON wire of a REPL: a request, an answer and the
/// requests that are not REPL commands.
struct repl_json_keys {
	static constexpr const char* id = "id";
	static constexpr const char* cmd = "cmd";
	static constexpr const char* status = "status";
	static constexpr const char* result = "result";
	static constexpr const char* results = "results";
	static constexpr const char* state = "state";
	static constexpr const char* report = "report";
	static constexpr const char* src = "src";
	static constexpr const char* eval = "eval";
	static constexpr const char* hello = "hello";
	static constexpr const char* protocol = "protocol";
	static constexpr const char* version = "version";
};

/// The longest request line of the JSON wire.
inline constexpr size_t repl_json_max_line = 16u * 1024 * 1024;

/// Wire name of @p s: "ok", "error", "incomplete" or "quit".
const char* cmd_status_name(cmd_status s);

/// {"cmd","status","result","report"} of one statement.
tref json_cmd_value(const cmd_result& r);

/// {"id","status","results":[...],"state","report"} of one eval request.
tref json_eval_response(tref id, const eval_result& er, tref state);

/// {"cmd","status","result","state","report"}, after "id" when @p id is
/// not nullptr.
tref json_result_response(cmd_status status, const std::string& cmd,
	tref result, tref state, const diagnostics::report& report,
	tref id = nullptr);

/// {"id","status","state","report"} of a request that runs no command.
tref json_status_response(tref id, cmd_status status, tref state,
	const diagnostics::report& report);

/// json_status_response with cmd_status::error.
tref json_error_response(tref id, tref state,
	const diagnostics::report& report);

/// {"hello":@p hello,"state":@p state}, the first line of a session.
tref json_hello_line(tref hello, tref state);

/// {"id","cmd":"hello","status":"ok","hello","state"}, the answer to a
/// hello request.
tref json_hello_response(tref id, tref hello, tref state);

/// Read one line of at most @p max_line bytes into @p line. The bytes
/// past the limit are dropped up to the next newline and set
/// @p too_long. A CR stays in the line. False only at the end of @p in
/// with no line.
bool json_read_line(std::istream& in, std::string& line,
	size_t max_line, bool& too_long);

/// Parse one wire line with the start symbol wire_line. A line of only
/// whitespace gives nullptr, a line with a value gives its tree.
diagnostics::result<tref> json_parse_line(std::string_view line);

} // namespace idni

#include "repl_json.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__REPL_JSON_H__
