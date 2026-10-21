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
	/// A reply that is JSON text already, sent as it is with no parse
	/// and no print.
	virtual void write(std::string line) = 0;
	/// Ends the connection after the queued lines are written.
	virtual void close() = 0;
	/// True stops the read of new request lines, false starts it again.
	/// A handler whose own queue is full uses it as back pressure.
	virtual void pause_read(bool paused) = 0;
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
	/// raw_lines: one request line as it came, with no line end. The view
	/// ends with the call, so the handler copies what it keeps. The
	/// default does nothing.
	virtual void on_text([[maybe_unused]] std::string_view line,
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
	/// With handler_logs, an event of the server about this session, for
	/// example "line-too-long". The server writes it into server.log, and
	/// the handler can copy it into its session log. The default does
	/// nothing.
	virtual void on_server_event([[maybe_unused]] tref event) {}
	/// A request line over max_line. True: the handler wrote the answer.
	/// False: the server writes its error line. The default is false.
	virtual bool on_line_too_long([[maybe_unused]] reply_sink& out) {
		return false;
	}
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

/// Limits of failed logins, for each transport kind. A wrong token
/// counts as one failure. The wait refuses every client of that kind,
/// also a client with the right token, and also a connection that was
/// open before the limit was reached.
struct login_limits {
	/// After this many failures the server refuses new connections of
	/// that kind for the wait time.
	size_t max_failures = 5;
	std::chrono::seconds window{ 60 };
	std::chrono::seconds wait{ 60 };
};

/// What the server does after it answers a request line over max_line.
enum class oversize_policy {
	/// Drop the tail of the line and read the next line.
	keep_open,
	/// Close the connection after the answer.
	reply_and_close,
};

/// What the server does with a new connection at max_sessions or
/// max_pending.
enum class full_policy {
	/// Answer with busy_message, then close.
	busy_message,
	/// Close with no answer.
	close_silent,
};

/// The bytes that the server writes after each reply line, for each
/// transport kind.
struct reply_line_ends {
	std::string local_socket = "\n";
	std::string tcp = "\n";
	/// A websocket message needs no line end.
	std::string websocket{};
	const std::string& of(line_transport_kind kind) const {
		switch (kind) {
		case line_transport_kind::tcp: return tcp;
		case line_transport_kind::websocket: return websocket;
		case line_transport_kind::local_socket: break;
		}
		return local_socket;
	}
};

/// An optional value for each transport kind. No value takes the value
/// of the whole server.
struct kind_values {
	std::optional<size_t> local_socket{};
	std::optional<size_t> tcp{};
	std::optional<size_t> websocket{};
	std::optional<size_t> of(line_transport_kind kind) const {
		switch (kind) {
		case line_transport_kind::tcp: return tcp;
		case line_transport_kind::websocket: return websocket;
		case line_transport_kind::local_socket: break;
		}
		return local_socket;
	}
};

/// The settings of the line server. One server listens on all
/// addresses at once.
struct line_server_options {
	/// Default: one local_socket address. TCP and WebSocket bind the host
	/// of the address, 127.0.0.1 by default.
	std::vector<listen_address> listen{};
	/// local_socket: refuse a peer of another user than the server. On
	/// Windows the pipe access list allows only the user.
	bool require_same_user = true;
	/// websocket: the Origin values that the server accepts. Empty
	/// refuses every browser origin, because any web page can open a
	/// WebSocket to 127.0.0.1.
	std::vector<std::string> allowed_origins{};
	/// websocket: decides the Origin of an upgrade request. When set, it
	/// replaces allowed_origins. True accepts the upgrade. A request with
	/// no Origin header passes without a call.
	std::function<bool(std::string_view origin)> accept_origin{};
	/// websocket: with a value, a refused origin gets the upgrade, this
	/// one text message and the close, and no session. No value answers
	/// a refused origin with the HTTP answer 403.
	std::optional<std::string> origin_refusal_message{};
	/// At this count the server refuses a new connection with busy_message.
	size_t max_sessions = 16;
	/// At this count of connections of one transport kind the server
	/// refuses a new connection of that kind, as at max_sessions.
	/// max_sessions stays the limit of all kinds together.
	kind_values max_sessions_by_kind{};
	/// At this count of connections with no accepted first line, and of
	/// websocket upgrades in flight, the server refuses a new connection
	/// with busy_message.
	size_t max_pending = 16;
	/// Close a connection that sends no first line in this time. 0 turns
	/// the timeout off.
	size_t first_line_timeout_seconds = 30;
	/// websocket: close a client that sends no complete upgrade request in
	/// this time. The same time bounds the websocket close handshake when
	/// close_timeout_seconds has no value.
	size_t upgrade_timeout_seconds = 30;
	/// websocket: the time of the close handshake. The socket closes when
	/// the client does not answer the close frame in this time.
	std::optional<size_t> close_timeout_seconds{};
	/// Stop the read of a client while this many bytes of its output wait.
	size_t max_output_bytes = 1u * 1024 * 1024;
	/// The wait after a failed accept, so a lack of file descriptors does
	/// not keep the loop busy.
	size_t accept_retry_ms = 100;
	std::string busy_message = "the server takes no more sessions";
	/// A websocket upgrade over max_pending gets the HTTP answer 503. With
	/// close_silent an upgrade at max_sessions or at the websocket value
	/// of max_sessions_by_kind also gets it.
	full_policy when_full = full_policy::busy_message;
	login_limits logins{};
	/// End a session with no traffic. 0 turns the timeout off.
	size_t idle_timeout_seconds = 30 * 60;
	/// The idle timeout of each transport kind, in seconds. A kind with no
	/// value takes idle_timeout_seconds. 0 turns it off for that kind.
	kind_values idle_timeout_by_kind{};
	/// Also ends each error line of the server.
	reply_line_ends reply_line_end{};
	/// Largest request line or message. The tail of a longer line is
	/// dropped.
	size_t max_line = 16u * 1024 * 1024;
	/// A local_socket or tcp line over max_line. Beast closes a websocket
	/// with a message over max_line in each policy.
	oversize_policy when_too_long = oversize_policy::keep_open;
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
	/// True: the handler writes the session events and the session
	/// logs itself. The server writes no session event, no session log
	/// and no in or out line. server.log, the listening lines and the
	/// events of reply_sink::log stay.
	bool handler_logs = false;
	/// Give each line to session_handler::on_text as text, with no JSON
	/// parse. Raw lines have no login line, so a tcp or websocket host
	/// other than the loopback makes a server open to the network. The
	/// caller chooses this. authenticate, the login limits and
	/// first_line_timeout_seconds do not apply, and on_open does not run.
	/// An empty line also reaches the handler. The start refuses
	/// raw_lines with authenticate.
	bool raw_lines = false;
	/// websocket: a binary message is one line as it came, as a text
	/// message is. False refuses a binary message and closes. The bytes
	/// need not be UTF-8, so a raw_lines handler decodes them itself.
	bool binary_as_text = false;
	/// Checks the first line of a connection after the peer check. False
	/// counts a failed login and closes it. Empty accepts.
	std::function<bool(tref first_line, const peer_info& peer)>
		authenticate{};
	/// Makes the handler of a new session. Must be set.
	std::function<std::unique_ptr<session_handler>(
		const line_session& session)> make_handler{};
	/// Called at the stop, after the connections closed and before the
	/// listeners close, so a handler can end work that lives outside a
	/// connection.
	std::function<void()> on_stop{};
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

/// Run @p fn on the io loop of the server that runs in this process, from
/// any thread. A handler that waits for work of its own gives the result
/// to its sink this way. False when no server runs.
bool line_server_post(std::function<void()> fn);

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
