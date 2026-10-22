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
	/// TCP port, at most 65535. 0 lets the system pick a free port.
	size_t port = 4000;
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
	/// Close a connection that sends no first line within this many
	/// seconds. 0 disables the deadline.
	size_t first_line_timeout_seconds = 30;
	/// Refuse a new connection while this many have sent no first line.
	size_t max_pending = 16;
	/// Stop the read of a client while this many of its requests wait for
	/// an answer of the child.
	size_t max_open_requests = 64;
	/// Largest answer line of a child. A longer line ends the session.
	size_t max_answer = tgf_json_max_line;
	/// Largest stderr line of a child. The tail of a longer line is
	/// dropped.
	size_t max_stderr_line = 1u * 1024 * 1024;
	/// Child output kept in bytes while no client is attached. The
	/// oldest lines go first.
	size_t detached_buffer = 16u * 1024 * 1024;
	/// Stop a child that does not exit this many seconds after
	/// end-session or after the stop signal.
	size_t end_timeout_seconds = 5;
	/// Directory of the session logs and of server.log. An empty value
	/// selects ~/.tau/tgf/logs. The server creates the directory when it
	/// is absent.
	std::string log_dir{};
	/// True turns every log off.
	bool no_log = false;
	/// True writes JSON lines on stdout, false readable text lines.
	bool json = false;
	/// True also prints each session log line (in, out and stderr) on
	/// stdout, with the session id. It works with no_log too.
	bool debug_sessions = false;
	/// Grammar and REPL options each new session forks. The caller keeps
	/// it alive for the whole run.
	tgf_repl_evaluator* evaluator = nullptr;
};

/// Run the TCP server until a stop signal. Its first stdout line names the
/// port: `{"listening":{"kind":"tcp","port":<port>}}` with
/// @ref serve_options::json, else `listening on 127.0.0.1:<port>`. Each later server event prints one
/// stdout line in the same mode, and server.log gets the event as JSON.
/// With @ref serve_options::debug_sessions each session log line prints too.
/// A busy port is an error that names the port. The returned report
/// carries every server error, including a failed log write; the value is
/// 0 when the server stopped cleanly. A caller prints the report.
diagnostics::result<int> tgf_serve_run(const serve_options& opt);

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_SERVE_H__
