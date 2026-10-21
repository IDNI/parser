// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__NET__LINE_CLIENT_H__
#define __IDNI__PARSER__NET__LINE_CLIENT_H__

#include <memory>
#include <string>
#include <string_view>

#include "net/line_address.h"
#include "net/line_transport.h"
#include "format/json/json.h"

namespace idni {

/// The settings of one client run.
struct connect_options {
	connect_target target{};
	/// Plain fields of the first line, after "cmd". For example an
	/// "attach" and its "session" id.
	htref open_fields = format::json::tree::geth(
		format::json::tree::build_object({}));
	/// The limits after the first answer. The first answer has no line
	/// limit and waits for limits.connect_timeout_seconds.
	line_limits limits{};
	/// Read JSON requests on stdin and write each reply as one line.
	bool json = false;
	bool colors = true;
	/// The plain terminal REPL, not FTXUI.
	bool legacy_repl = false;
};

/// Connect to opt.target and send the first line: "cmd" is @p cmd, then
/// the members of opt.open_fields. The value is the open transport, with
/// opt.limits.
/// The first answer of the server goes into @p hello.
diagnostics::result<std::unique_ptr<line_transport>> line_client_open(
	const connect_options& opt, std::string_view cmd, htref& hello);

} // namespace idni

#endif // __IDNI__PARSER__NET__LINE_CLIENT_H__
