// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Asio includes windows.h, so the Windows defines come first.
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "line_server.h"
#include "line_reader.h"
#include "line_support.h"
#include "utility/filesystem.h"

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <deque>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

// The dep script ships headers only, so Boost.System must stay header-only.
#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_HEADER_ONLY
#define BOOST_SYSTEM_HEADER_ONLY
#endif

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>

#include "parser_strings.h"

namespace idni {

namespace {

namespace ba = boost::asio;
using ba::ip::tcp;
using boost::system::error_code;
using jt = format::json::tree;
using format::json::string_member;

using conn_socket = std::variant<tcp::socket>;

// The idle timer ticks at this interval.
inline constexpr int tick_ms = 200;

std::string_view kind_name(line_transport_kind k) {
	switch (k) {
	case line_transport_kind::local_socket: return "local_socket";
	case line_transport_kind::tcp: return "tcp";
	case line_transport_kind::websocket: return "websocket";
	}
	return "";
}

// The error answer to a line that reaches no handler.
tref error_response(const diagnostics::report& rep) {
	return jt::build_object({
		jt::build_pair("id", jt::_null()),
		jt::build_pair("status", jt::build_string("error")),
		jt::build_pair("report", jt::build_report(rep, true)) });
}

// The text of a log field, empty when the field is absent.
std::string field_text(tref v, std::string_view key) {
	const std::string* f = string_member(v, key);
	return f ? *f : std::string();
}

struct line_server;
struct server_connection;

// The sink of one session. It lives as long as its connection, so a
// handler may keep it and write later from the io loop.
struct connection_sink : reply_sink {
	explicit connection_sink(server_connection& conn) : c(conn) {}
	server_connection& c;
	void write(tref reply) override;
	void close() override;
	void log(tref event) override;
};

// One connection is one session.
struct server_connection : std::enable_shared_from_this<server_connection> {
	server_connection(line_server* s, conn_socket sock, peer_info peer);

	line_server* srv;
	line_session session;
	conn_socket socket;
	std::string line;
	bool dropping = false;
	std::deque<std::string> out;
	// Closes a connection that does not drain its output in time.
	ba::steady_timer write_timer;
	bool closed = false;
	bool close_when_drained = false;
	bool reading = false;
	// True while on_open or on_line runs, so a close waits for its return.
	bool in_handler = false;
	std::array<char, 8192> read_buf{};
	fs::file_ostream log;
	bool log_ok = false;
	connection_sink sink{ *this };

	void start();
	void read_some();
	void on_read(error_code ec, size_t n);
	void handle_line(std::string& l);
	void handle_first_line(std::string& l);
	bool start_session();
	diagnostics::result<tref> parse_line(std::string& l);
	bool logs_lines() const;
	void log_request(tref request);
	void handle_too_long();
	void send(std::string text);
	void send_line(tref v);
	/// Log and send one line that is JSON text already.
	void send_text(std::string text);
	/// Answer with the error @p msg and close after the answer.
	void refuse(diagnostics::code c, std::string_view msg,
		std::initializer_list<diagnostics::attr_in> attrs = {});
	void write_next();
	void close();
	void close_after_flush();
	void arm_write_timer();
	void log_write(tref v);
	void log_store(tref v);
	void log_session_event(std::string_view event);
};

struct tcp_listener {
	std::unique_ptr<tcp::acceptor> acceptor;
	listen_address address;
};

struct line_server {
	explicit line_server(const line_server_options& o);

	bool start();
	void run();
	void stop();
	void close_all();

	bool listen_tcp(const listen_address& a);
	void accept_tcp(size_t i);
	void on_accept(conn_socket sock, peer_info peer);
	void retry_accept(std::function<void()> accept);
	bool full(line_transport_kind k) const;
	void open_session_log(server_connection& c);
	void start_idle_timer();
	void check_idle();

	void log_failure(diagnostics::report detail = {});
	void log_event(tref v, server_connection* c);

	void output(const std::string& line) const;
	void print_event(tref v);
	void print_debug(tref v, const std::string& session);
	void print_start();

	line_server_options opt;
	ba::io_context io;
	// one bound address for each entry of opt.listen, in its order
	std::vector<listen_address> bound;
	// tcp and websocket
	std::vector<tcp_listener> tcp_listeners;
	ba::steady_timer idle_timer;
	ba::signal_set signals;
	std::map<std::string, std::shared_ptr<server_connection>> connections;
	fs::file_ostream server_log;
	bool log_ok = false;
	bool log_failure_reported = false;
	diagnostics::report report;
	bool stopping = false;
	// True while a tick is scheduled, so a new connection only arms it once.
	bool idle_tick_running = false;
};

std::mutex running_mutex;
std::atomic<line_server*> running{ nullptr };

server_connection::server_connection(line_server* s, conn_socket sock,
	peer_info peer)
	: srv(s), socket(std::move(sock)), write_timer(s->io)
{
	session.peer = std::move(peer);
	session.last_traffic = std::chrono::steady_clock::now();
}

void server_connection::start() {
	read_some();
}

void server_connection::read_some() {
	if (closed || reading) return;
	reading = true;
	auto self = shared_from_this();
	std::visit([this, self](auto& s) {
		s.async_read_some(ba::buffer(read_buf),
			[self](error_code ec, size_t n) { self->on_read(ec, n); });
	}, socket);
}

void server_connection::on_read(error_code ec, size_t n) {
	reading = false;
	if (closed) return;
	if (ec) { close(); return; }
	feed_lines(line, dropping, srv->opt.max_line, read_buf.data(), n,
		[this](std::string& l) { handle_line(l); },
		[this] { handle_too_long(); });
	// The lines are answered and logged, so no JSON tref is left.
	jt::gc_if_grown();
	if (!closed && !close_when_drained) read_some();
}

void server_connection::handle_line(std::string& l) {
	if (closed || close_when_drained) return;
	if (!l.empty() && l.back() == '\r') l.pop_back();
	if (l.empty()) return;
	session.last_traffic = std::chrono::steady_clock::now();
	if (!session.authenticated) { handle_first_line(l); return; }
	auto req = parse_line(l);
	if (!req.has_value()) {
		send_line(error_response(req.report()));
		return;
	}
	log_request(req.value());
	in_handler = true;
	session.handler->on_line(req.value(), sink);
	in_handler = false;
}

// A line that does not parse is logged as it came.
diagnostics::result<tref> server_connection::parse_line(std::string& l) {
	auto req = jt::get(std::string_view(l));
	if (!req.has_value() && session.authenticated)
		log_write(line_log::text("in", l));
	return req;
}

void server_connection::handle_first_line(std::string& l) {
	auto req = parse_line(l);
	if (!req.has_value()) {
		send_line(error_response(req.report()));
		close_after_flush();
		return;
	}
	const auto& opt = srv->opt;
	const tref first = req.value();
	if (opt.authenticate
		&& !opt.authenticate(first, session.peer))
	{
		refuse(diagnostics::code::invalid_argument,
			parser_strings::messages::net_login_refused);
		return;
	}
	if (!start_session()) return;
	log_request(first);
	log_session_event("new");
	in_handler = true;
	session.handler->on_open(first, sink);
	in_handler = false;
}

// False refuses the connection.
bool server_connection::start_session() {
	session.authenticated = true;
	session.handler = srv->opt.make_handler(session);
	if (!session.handler) {
		session.authenticated = false;
		refuse(diagnostics::code::runtime_error,
			parser_strings::messages::session_start_failed);
		return false;
	}
	srv->open_session_log(*this);
	return true;
}

void server_connection::handle_too_long() {
	if (closed || close_when_drained) return;
	if (session.authenticated) log_session_event("line-too-long");
	diagnostics::report rep;
	rep.error(diagnostics::code::out_of_range,
		parser_strings::messages::line_too_long);
	send_line(error_response(rep));
}

void server_connection::refuse(diagnostics::code c, std::string_view msg,
	std::initializer_list<diagnostics::attr_in> attrs)
{
	diagnostics::report rep;
	rep.error(c, msg, attrs);
	send_line(error_response(rep));
	close_after_flush();
}

void server_connection::send(std::string text) {
	if (closed) return;
	out.push_back(std::move(text));
	if (out.size() == 1) {
		arm_write_timer();
		write_next();
	}
}

void server_connection::send_line(tref v) {
	send_text(jt::get(v).to_str());
}

void server_connection::send_text(std::string text) {
	session.last_traffic = std::chrono::steady_clock::now();
	if (session.authenticated) log_write(line_log::text("out", text));
	text += '\n';
	send(std::move(text));
}

void server_connection::write_next() {
	if (closed || out.empty()) return;
	auto self = shared_from_this();
	auto on_written = [self](error_code ec, size_t) {
		if (ec) { self->close(); return; }
		self->out.pop_front();
		if (self->out.empty() && self->close_when_drained) {
			self->close();
			return;
		}
		// The write made progress, so the deadline starts again.
		self->arm_write_timer();
		self->write_next();
	};
	std::visit([this, &on_written](auto& s) {
		ba::async_write(s, ba::buffer(out.front()),
			std::move(on_written));
	}, socket);
}

void server_connection::close_after_flush() {
	if (closed) return;
	close_when_drained = true;
	if (!out.empty()) return;
	// on_close must not run while the handler still runs
	if (in_handler)
		ba::post(srv->io, [self = shared_from_this()] { self->close(); });
	else close();
}

// A connection that makes no write progress for the deadline is closed,
// so a stalled client cannot hold a queued answer open forever.
void server_connection::arm_write_timer() {
	if (closed) return;
	// The deadline holds only while output waits, so an idle client stays.
	if (out.empty()) { write_timer.cancel(); return; }
	if (srv->opt.write_timeout_seconds == 0) return;
	write_timer.expires_after(
		std::chrono::seconds(srv->opt.write_timeout_seconds));
	auto self = shared_from_this();
	write_timer.async_wait([self](error_code ec) {
		if (ec) return;
		self->close();
	});
}

void server_connection::close() {
	if (closed) return;
	closed = true;
	line.clear();
	// Some Boost versions have no cancel(ec), and a timer cancel reports no error.
	write_timer.cancel();
	std::visit([](auto& s) {
		error_code ec;
		using socket_t = std::decay_t<decltype(s)>;
		if constexpr (requires {
				s.shutdown(socket_t::shutdown_both, ec); })
			s.shutdown(socket_t::shutdown_both, ec);
		s.close(ec);
	}, socket);
	if (session.handler) {
		session.handler->on_close();
		log_session_event("detach");
	}
	log.close();
	// the map can hold the last reference
	auto keep = shared_from_this();
	srv->connections.erase(session.id);
}

void connection_sink::write(tref reply) { c.send_line(reply); }

void connection_sink::close() { c.close_after_flush(); }

void connection_sink::log(tref event) {
	c.srv->log_event(event, &c);
}

void server_connection::log_session_event(std::string_view event) {
	srv->log_event(line_log::session_event(event, session.id), this);
}

bool server_connection::logs_lines() const {
	return srv->opt.log_lines;
}

void server_connection::log_request(tref request) {
	if (logs_lines()) log_write(line_log::text("in", jt::get(request).to_str()));
}

void server_connection::log_write(tref v) {
	// The copy runs also with no log folder, so it comes before the check.
	if (!logs_lines()) return;
	if (srv->opt.debug_sessions) srv->print_debug(v, session.id);
	log_store(v);
}

void server_connection::log_store(tref v) {
	if (!log_ok) return;
	if (!line_log::append(log, v)) {
		log_ok = false;
		srv->log_failure();
	}
}

line_server::line_server(const line_server_options& o)
	: opt(o), io(), idle_timer(io), signals(io)
{
	if (opt.max_sessions == 0) opt.max_sessions = 1;
	if (opt.log_dir.empty()) return;
	// A log line holds private request data, so the folder and the logs
	// are readable by the owner only.
	auto dirs = fs::create_private_dirs(fs::to_path(opt.log_dir));
	if (!dirs.has_value()) {
		log_failure(std::move(dirs).report());
		return;
	}
	auto opened = fs::open_private_append(
		fs::to_path(opt.log_dir + "/server.log"));
	if (!opened.has_value()) {
		log_failure(std::move(opened).report());
		return;
	}
	server_log = std::move(opened).value();
	log_ok = true;
}

void line_server::log_failure(diagnostics::report detail) {
	if (log_failure_reported) return;
	log_failure_reported = true;
	if (detail.has_error()) {
		report.append(std::move(detail));
		return;
	}
	report.error(diagnostics::code::io_error,
		parser_strings::messages::log_write_failed);
}

void line_server::log_event(tref v, server_connection* c) {
	if (log_ok && !line_log::append(server_log, v)) {
		log_ok = false;
		log_failure();
	}
	if (c) c->log_store(v);
	print_event(v);
}

// The start of a text line about a session, shared by events and the
// session log copy, so both sort and read alike.
std::string session_prefix(tref v, const std::string& session) {
	return field_text(v, "t") + " session " + session + ' ';
}

void line_server::output(const std::string& line) const {
	if (opt.on_output) opt.on_output(line);
}

void line_server::print_event(tref v) {
	if (opt.json) { output(jt::get(v).to_str()); return; }
	const std::string session = field_text(v, "session");
	std::string l = session.empty() ? field_text(v, "t") + ' '
		: session_prefix(v, session);
	l += field_text(v, "event");
	if (const std::string kind = field_text(v, "kind"); !kind.empty())
		l += ' ' + kind;
	for (std::string_view key : { "exit", "count" })
		if (tref f = jt::get(v).find(key); f && jt::get(f).is_number())
			l += ' ' + std::to_string(static_cast<long long>(
				jt::get(f).get_number()));
	output(l);
}

void line_server::print_debug(tref v, const std::string& session) {
	if (opt.json) {
		output(jt::get(jt::set_member(v, "session",
			jt::build_string(session))).to_str());
		return;
	}
	std::string dir = field_text(v, "dir");
	// "in" pads to the width of "out", so the payloads line up.
	if (dir.size() < 3) dir.resize(3, ' ');
	output(session_prefix(v, session) + dir + ' ' + field_text(v, "line"));
}

void line_server::print_start() {
	if (opt.json) {
		for (const auto& a : bound) {
			trefs m{ jt::build_pair("kind",
				jt::build_string(kind_name(a.kind))) };
			if (a.kind == line_transport_kind::local_socket)
				m.push_back(jt::build_pair("path",
					jt::build_string(a.path)));
			else m.push_back(jt::build_pair("port", jt::build_number(
				static_cast<double>(a.port))));
			if (a.kind == line_transport_kind::websocket)
				m.push_back(jt::build_pair("path",
					jt::build_string(a.ws_path)));
			output(jt::get(jt::build_object({
				jt::build_pair("listening", jt::build_object(m)) }))
				.to_str());
		}
		return;
	}
	const std::string t = line_log::timestamp();
	for (const auto& a : bound) {
		const std::string host_port = "127.0.0.1:"
			+ std::to_string(a.port);
		if (a.kind == line_transport_kind::local_socket)
			output(t + " listening on " + a.path);
		else if (a.kind == line_transport_kind::websocket)
			output(t + " listening on ws://" + host_port + a.ws_path);
		else output(t + " listening on " + host_port);
	}
	output(log_ok ? t + " logs in " + opt.log_dir : t + " logs off");
}

void line_server::open_session_log(server_connection& c) {
	if (opt.log_dir.empty()) return;
	auto opened = line_log::open_session(opt.log_dir, c.session.id);
	c.log_ok = opened.has_value();
	if (c.log_ok) c.log = std::move(opened).value();
	else log_failure(std::move(opened).report());
}

bool line_server::listen_tcp(const listen_address& a) {
	error_code ec;
	tcp::endpoint ep(ba::ip::address_v4::loopback(), a.port);
	auto acc = std::make_unique<tcp::acceptor>(io);
	acc->open(ep.protocol(), ec);
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::bind_failed);
		return false;
	}
#ifdef _WIN32
	// Without it a second process can bind the same port and take clients.
	using exclusive_address = ba::detail::socket_option::boolean<
		SOL_SOCKET, SO_EXCLUSIVEADDRUSE>;
	acc->set_option(exclusive_address(true), ec);
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::bind_failed);
		return false;
	}
#else
	// On Windows this option lets a second server take a busy port.
	acc->set_option(ba::socket_base::reuse_address(true), ec);
#endif
	acc->bind(ep, ec);
	if (ec == ba::error::address_in_use) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::port_in_use,
			{ { parser_strings::label::port, size_t{ a.port } } });
		return false;
	}
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::bind_failed);
		return false;
	}
	acc->listen(ba::socket_base::max_listen_connections, ec);
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::listen_failed);
		return false;
	}
	auto lep = acc->local_endpoint(ec);
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::listen_failed);
		return false;
	}
	listen_address b = a;
	b.port = lep.port();
	bound.push_back(b);
	tcp_listeners.push_back({ std::move(acc), std::move(b) });
	return true;
}

void line_server::retry_accept(std::function<void()> accept) {
	auto t = std::make_shared<ba::steady_timer>(io,
		std::chrono::milliseconds(opt.accept_retry_ms));
	t->async_wait([this, t, accept = std::move(accept)](error_code ec) {
		if (!ec && !stopping) accept();
	});
}

// True when a new connection is over the session limit.
bool line_server::full(line_transport_kind) const {
	return connections.size() >= opt.max_sessions;
}

void line_server::accept_tcp(size_t i) {
	tcp_listeners[i].acceptor->async_accept(
		[this, i](error_code ec, tcp::socket s) {
			if (stopping) return;
			if (ec) {
				retry_accept([this, i] { accept_tcp(i); });
				return;
			}
			on_accept(conn_socket(std::move(s)),
				peer_info{ .kind = line_transport_kind::tcp });
			accept_tcp(i);
		});
}

void line_server::on_accept(conn_socket sock, peer_info peer) {
	auto c = std::make_shared<server_connection>(this, std::move(sock),
		std::move(peer));
	if (full(c->session.peer.kind)) {
		c->refuse(diagnostics::code::out_of_range, opt.busy_message);
		return;
	}
	std::optional<std::string> sid = new_session_id();
	while (sid.has_value() && connections.find(*sid) != connections.end())
		sid = new_session_id();
	if (!sid.has_value()) {
		c->refuse(diagnostics::code::runtime_error,
			parser_strings::messages::entropy_failed);
		return;
	}
	c->session.id = std::move(*sid);
	connections[c->session.id] = c;
	start_idle_timer();
	c->start();
}

bool line_server::start() {
	if (report.has_error()) return false;
	for (const auto& a : opt.listen) {
		switch (a.kind) {
		case line_transport_kind::tcp:
		case line_transport_kind::websocket:
			if (!listen_tcp(a)) return false;
			break;
		case line_transport_kind::local_socket:
			break;
		}
	}
	return true;
}

void line_server::run() {
	for (size_t i = 0; i != tcp_listeners.size(); ++i) accept_tcp(i);
	error_code ec;
	signals.add(SIGINT, ec);
	signals.add(SIGTERM, ec);
	if (!ec)
		signals.async_wait([this](error_code e, int) {
			if (!e) stop();
		});
	io.run();
	close_all();
}

void line_server::stop() {
	stopping = true;
	io.stop();
}

void line_server::close_all() {
	stopping = true;
	std::vector<std::shared_ptr<server_connection>> live;
	for (auto& kv : connections) live.push_back(kv.second);
	for (auto& c : live) c->close();
	error_code ec;
	for (auto& l : tcp_listeners) l.acceptor->close(ec);
	log_ok = false;
	server_log.close();
}

void line_server::start_idle_timer() {
	if (idle_tick_running) return;
	idle_tick_running = true;
	idle_timer.expires_after(std::chrono::milliseconds(tick_ms));
	idle_timer.async_wait([this](error_code ec) {
		idle_tick_running = false;
		if (ec) return;
		check_idle();
		// No connection means no tick; the next connection arms it again.
		if (!connections.empty()) start_idle_timer();
	});
}

void line_server::check_idle() {
	auto now = std::chrono::steady_clock::now();
	std::vector<std::shared_ptr<server_connection>> idle;
	for (auto& kv : connections) {
		const auto& se = kv.second->session;
		const auto quiet = now - se.last_traffic;
		if (se.authenticated && opt.idle_timeout_seconds != 0
				&& quiet >= std::chrono::seconds(opt.idle_timeout_seconds))
			idle.push_back(kv.second);
	}
	for (auto& c : idle) {
		c->log_session_event("idle-timeout");
		c->close();
	}
}

diagnostics::result<int> run_server(const line_server_options& opt) {
	diagnostics::result<int> res;
	if (!opt.make_handler) {
		res.error(diagnostics::code::invalid_argument,
			parser_strings::messages::session_start_failed);
		return res;
	}
#ifndef _WIN32
	// A write to a socket whose reader is gone must not kill the server.
	// The program gets its own handler back at the end of the run.
	struct ignore_sigpipe {
		void (*old)(int) = std::signal(SIGPIPE, SIG_IGN);
		~ignore_sigpipe() {
			if (old != SIG_ERR) std::signal(SIGPIPE, old);
		}
	} sigpipe;
#endif
	line_server srv(opt);
	if (!srv.start()) {
		srv.close_all();
		res.report().append(std::move(srv.report));
		return res;
	}
	{
		std::lock_guard lock(running_mutex);
		running = &srv;
	}
	srv.print_start();
	srv.run();
	{
		std::lock_guard lock(running_mutex);
		running = nullptr;
	}
	diagnostics::report rep = std::move(srv.report);
	if (rep.has_error()) {
		res.report().append(std::move(rep));
		return res;
	}
	res.emplace(0);
	res.report().append(std::move(rep));
	return res;
}

} // namespace

diagnostics::result<int> line_server_run(const line_server_options& opt) {
	diagnostics::result<int> res;
	// Asio reports a failed system call of an internal step by a throw.
	try {
		return run_server(opt);
	} catch (...) {
		{
			std::lock_guard lock(running_mutex);
			running = nullptr;
		}
		res.error(diagnostics::code::runtime_error,
			parser_strings::messages::serve_failed);
		return res;
	}
}

void line_server_stop() {
	std::lock_guard lock(running_mutex);
	if (line_server* s = running.load())
		ba::post(s->io, [s] { s->stop(); });
}

void line_server_debug(tref line, const std::string& session) {
	std::lock_guard lock(running_mutex);
	if (line_server* s = running.load()) s->print_debug(line, session);
}

} // namespace idni
