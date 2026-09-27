// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_SERVE_H__
#define __IDNI__PARSER__TGF__TGF_SERVE_H__

#include <cstddef>
#include <cstdint>
#include <string>

#include "tgf_cli.h"
#include "utility/diagnostics.h"

namespace idni {

/// Options of the `tgf serve` subcommand. The server never sees a host
/// option: it binds 127.0.0.1 only.
struct serve_options {
	/// TCP port. 0 lets the system pick a free port.
	uint16_t port = 0;
	/// Refuse a new session at this count.
	size_t max_sessions = 16;
	/// End a session after this many minutes with no traffic. 0 disables
	/// the timeout.
	size_t idle_timeout_minutes = 30;
	/// Address space limit of a session in MB. 0 means no limit.
	size_t session_memory_mb = 0;
	/// Largest request line; a longer line is an error and its tail is
	/// dropped up to the next newline. The session child gets the same
	/// limit.
	size_t max_line = tgf_json_max_line;
	/// Close a connection that does not drain its output within this many
	/// seconds. 0 disables the deadline.
	size_t write_timeout_seconds = 30;
	/// Directory of the session logs and of server.log. An empty value
	/// selects ~/.tau/tgf/logs. The server creates the directory when it
	/// is absent.
	std::string log_dir{};
	/// True turns every log off.
	bool no_log = false;
	/// Grammar and REPL options each new session forks. The caller keeps
	/// it alive for the whole run.
	tgf_repl_evaluator* evaluator = nullptr;
};

/// Run the TCP server until a stop signal. The server writes one
/// `{"listening":<port>}` line on stdout. The returned report carries
/// every server error, including a failed log write; the value is 0 when
/// the server stopped cleanly. A caller prints the report.
diagnostics::result<int> tgf_serve_run(const serve_options& opt);

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_SERVE_H__
