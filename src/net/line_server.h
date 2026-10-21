// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__NET__LINE_SERVER_H__
#define __IDNI__PARSER__NET__LINE_SERVER_H__

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "net/line_address.h"
#include "utility/diagnostics.h"
#include "format/json/json.h"

namespace idni {

/// Where a session handler writes its replies. One call is one line.
struct reply_sink {
	virtual ~reply_sink() = default;
	/// The sink prints @p reply before it returns, so a tref is enough.
	virtual void write(tref reply) = 0;
	/// Ends the connection after the queued lines are written.
	virtual void close() = 0;
	/// Write one event of the handler into the server log and the session
	/// log, in the shape of the server events.
	virtual void log(tref event) = 0;
};

/// The program part of one session. The server runs the JSON gc between
/// two reads, so a handler keeps a JSON value from one call to the next
/// only through an htref.
struct session_handler {
	virtual ~session_handler() = default;
	/// One parsed request. The default does nothing.
	virtual void on_line([[maybe_unused]] tref request,
		[[maybe_unused]] reply_sink& out) {}
	/// The first line of the connection, after the peer check and
	/// authenticate accepted it. The default does nothing. The sink stays
	/// valid until on_close returns.
	virtual void on_open([[maybe_unused]] tref first_line,
		[[maybe_unused]] reply_sink& out) {}
	/// The connection ended. The handler clears its state. A close that
	/// the handler asks for in on_line or on_open runs after that call
	/// returns.
	virtual void on_close() {}
};

/// What the server knows about the other end of a connection.
struct peer_info {
	line_transport_kind kind = line_transport_kind::local_socket;
	/// local_socket only: the peer process user, from SO_PEERCRED on
	/// Linux or getpeereid on macOS.
	std::optional<uint64_t> user_id{};
	/// websocket only: the Origin header of the upgrade request.
	std::string origin{};
};

/// One connection on the server.
struct line_session {
	/// 128 random bits as 32 lowercase hex characters.
	std::string id;
	peer_info peer{};
	/// True after the peer check and authenticate accepted it.
	bool authenticated = false;
	/// Time of the last line in or out, for the idle timeout.
	std::chrono::steady_clock::time_point last_traffic{};
	std::unique_ptr<session_handler> handler;
};

/// The settings of the line server. One server listens on all
/// addresses at once.
struct line_server_options {
	/// Default: one local_socket address.
	std::vector<listen_address> listen{};
	/// local_socket: refuse a peer of another user than the server. On
	/// Windows the pipe access list allows only the user.
	bool require_same_user = true;
	/// websocket: the Origin values that the server accepts. Empty
	/// refuses every browser origin, because any web page can open a
	/// WebSocket to 127.0.0.1.
	std::vector<std::string> allowed_origins{};
	/// At this count the server refuses a new connection with busy_message.
	size_t max_sessions = 16;
	/// websocket: close a client that sends no complete upgrade request in
	/// this time. The same time bounds the websocket close handshake.
	size_t upgrade_timeout_seconds = 30;
	/// The wait after a failed accept, so a lack of file descriptors does
	/// not keep the loop busy.
	size_t accept_retry_ms = 100;
	std::string busy_message = "the server takes no more sessions";
	/// End a session with no traffic. 0 turns the timeout off.
	size_t idle_timeout_seconds = 30 * 60;
	/// Largest request line or message. The tail of a longer line is
	/// dropped.
	size_t max_line = 16u * 1024 * 1024;
	/// Close a connection that makes no write progress. 0 turns it off.
	size_t write_timeout_seconds = 30;
	/// Folder of the logs, with no "~" left. Empty writes no log.
	std::string log_dir{};
	/// Also log each request and reply line.
	bool log_lines = true;
	/// The lines of on_output as JSON: first one {"listening":..} line
	/// for each address.
	bool json = false;
	/// Also give each logged line to on_output.
	bool debug_sessions = false;
	/// Checks the first line of a connection after the peer check. False
	/// counts a failed login and closes it. Empty accepts.
	std::function<bool(tref first_line, const peer_info& peer)>
		authenticate{};
	/// Makes the handler of a new session. Must be set.
	std::function<std::unique_ptr<session_handler>(
		const line_session& session)> make_handler{};
	/// Gets each output line of the server, with no newline: the
	/// listening lines at the start, the events and the debug lines. The
	/// program prints them. Empty prints nothing. The call runs on the io
	/// loop.
	std::function<void(std::string_view line)> on_output{};
};

/// Run the server until a stop signal. The value is 0 after a clean
/// stop. The report holds every server error.
diagnostics::result<int> line_server_run(const line_server_options& opt);

/// Stop the server that runs in this process, from any thread. The
/// run returns after the connections are closed.
void line_server_stop();

/// Write one event into server.log and give it to on_output, for the
/// server that runs in this process. Call it on the io loop of the
/// server.
void line_server_log(tref event);

/// Give the session log line @p line of @p session to on_output, in the
/// form of the debug lines, for the server that runs in this process.
/// Call it on the io loop of the server.
void line_server_debug(tref line, const std::string& session);

} // namespace idni

#endif // __IDNI__PARSER__NET__LINE_SERVER_H__
