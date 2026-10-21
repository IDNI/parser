// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_RESULT_H__
#define __IDNI__PARSER__UTILITY__REPL_RESULT_H__

#include <string>
#include <vector>

#include "utility/diagnostics.h"
#include "format/json/json.h"

namespace idni {

/// Status of one command, or of one request text as a whole.
/// Wire names: "ok", "error", "incomplete", "quit".
enum class cmd_status { ok, error, incomplete, quit };

/// Result of one statement. The text form comes from data only, so a
/// local REPL and a remote client show the same text.
/// Wire: `{"cmd":..,"status":..,"result":<data>,"report":{..}}`
struct cmd_result {
	/// Long command name, for example "parse file" or "account list".
	std::string cmd;
	cmd_status status = cmd_status::ok;
	/// The data outlives the command, so a handle holds it.
	htref data = format::json::tree::geth(
		format::json::tree::build_object({}));
	diagnostics::report report;
};

/// Result of one request text with one or more statements.
/// Wire: {"id":..,"status":..,"results":[..],"state":{..},"report":{..}}
struct eval_result {
	/// quit if a statement quits, else error if one fails, else ok.
	cmd_status status = cmd_status::ok;
	std::vector<cmd_result> results;
	/// Errors of the text itself, for example a parse error.
	diagnostics::report report;
};

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__REPL_RESULT_H__
