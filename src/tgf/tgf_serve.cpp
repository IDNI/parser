// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// The handle list of a child process needs Vista or later, and this must be
// set before any Windows SDK header.
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include "tgf_serve.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "parser_strings.h"

#include <array>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <utility>

#ifdef _WIN32
// Keep windows.h from pulling in the older winsock.h, which conflicts with
// the winsock2.h that Boost.Asio includes.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/random.h>
#endif
#endif

// The dep script ships headers only, so Boost.System must stay header-only.
#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_HEADER_ONLY
#define BOOST_SYSTEM_HEADER_ONLY
#endif

#include <boost/asio.hpp>
#ifdef _WIN32
#include <boost/asio/windows/stream_handle.hpp>
#else
#include <boost/asio/posix/stream_descriptor.hpp>
#endif
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>

#include "format/json/json.h"
#include "parser_strings.h"
#include "utility/diagnostics.h"
#include "utility/win_args.h"

namespace idni {

namespace ba = boost::asio;
using ba::ip::tcp;
using boost::system::error_code;
using format::json::value;

// The relay end of one child pipe: a POSIX file descriptor or an
// overlapped Windows named-pipe handle. Both support the same Asio stream
// operations, so the relay code below is shared.
#ifdef _WIN32
using child_pipe = ba::windows::stream_handle;
#else
using child_pipe = ba::posix::stream_descriptor;
#endif

// Child output kept while no client is attached.
inline constexpr size_t serve_detached_limit = 16u * 1024 * 1024;
// One child output line stays bounded, like one request line, so a lost
// newline cannot grow the buffer without limit.
inline constexpr size_t serve_child_line_limit = 16u * 1024 * 1024;
// One stderr line of a child stays bounded, so a lost newline cannot grow
// the buffer without limit.
inline constexpr size_t serve_stderr_line_limit = 1u * 1024 * 1024;
// A child is given this long to exit after end-session.
inline constexpr int serve_end_timeout_ms = 5000;
// A client is not read from while this many bytes wait in its output.
inline constexpr size_t serve_conn_out_limit = 1u * 1024 * 1024;
// A client is not read from while this many requests wait for the child.
inline constexpr size_t serve_open_requests_limit = 64;
// The idle timer ticks at this interval.
inline constexpr int serve_tick_ms = 200;

struct serve_connection;
struct serve_session;
struct serve_server;

// One line of a value, with the trailing newline of the protocol.
static std::string json_to_line(const value& v) {
	std::ostringstream os;
	format::json::print(v, os);
	os << '\n';
	return os.str();
}

// The log time: UTC ISO 8601 with milliseconds, for example
// "2026-09-27T10:15:03.123Z".
static std::string log_timestamp() {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
	std::time_t secs = system_clock::to_time_t(now);
	std::tm* tmv = std::gmtime(&secs);
	char buf[32] = {};
	if (tmv) std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", tmv);
	char msbuf[8] = {};
	std::snprintf(msbuf, sizeof msbuf, ".%03dZ",
		static_cast<int>(ms.count()));
	return std::string(buf) + msbuf;
}

// The text of a log `in`, `out` or `stderr` entry.
static value log_text(const char* dir, const std::string& text) {
	value v = value::object();
	v.set("t", value::string(log_timestamp()));
	v.set("dir", value::string(dir));
	v.set("line", value::string(text));
	return v;
}

// A server event entry. The caller appends its own fields.
static value log_event_base(const char* event) {
	value v = value::object();
	v.set("t", value::string(log_timestamp()));
	v.set("dir", value::string("event"));
	v.set("event", value::string(event));
	return v;
}

// Append one JSON line. A failed write reports false; the caller turns the
// log off and records the failure once.
static bool log_append(std::ofstream& os, const value& v) {
	if (!os.is_open()) return false;
	std::ostringstream line;
	format::json::print(v, line);
	line << '\n';
	os << line.str();
	os.flush();
	return os.good();
}

// The log directory when the caller names none.
static std::string default_log_dir() {
#ifdef _WIN32
	const char* home = std::getenv("USERPROFILE");
#else
	const char* home = std::getenv("HOME");
#endif
	if (!home || !*home) return {};
	return std::string(home) + "/.tau/tgf/logs";
}

// Create every missing component of @p dir. An existing component is not
// an error, so the caller can call this on every start. A failed create is
// left to the caller's open of the log, which reports it.
static void make_dirs(const std::string& dir) {
	if (dir.empty()) return;
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);
}

// The start time in a file name: local time, no separators but one dash.
static std::string log_start_time() {
	std::time_t now = std::time(nullptr);
	std::tm* tmv = std::localtime(&now);
	char buf[32] = {};
	if (tmv) std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", tmv);
	return buf;
}

// A response the server builds itself: id, status and the report, with no
// state. Only the server answers without state.
static value serve_error_response(const value& id,
	const diagnostics::report& r)
{
	value v = value::object();
	v.set("id", id);
	v.set("status", value::string("error"));
	v.set("report", format::json::to_value(r, true));
	return v;
}

// The `quit` answer of the end-session request: no state.
static value serve_quit_response(const value& id) {
	value v = value::object();
	v.set("id", id);
	v.set("cmd", value::string("quit"));
	v.set("status", value::string("quit"));
	return v;
}

static std::optional<std::string> json_string_field(const value& q,
	const char* name)
{
	auto f = q.find(name);
	if (!f || !f->is_string()) return std::nullopt;
	return std::string(f->as_string());
}

// Split @p data into lines. A line over @p limit is reported once through
// @p on_too_long, then its tail is dropped up to the next newline, so the
// kept bytes stay bounded by the limit.
template <typename OnLine, typename OnTooLong>
static void feed_lines(std::string& line, bool& dropping, size_t limit,
	const char* data, size_t n, OnLine on_line, OnTooLong on_too_long)
{
	for (size_t i = 0; i != n; ++i) {
		char c = data[i];
		if (dropping) {
			if (c == '\n') dropping = false;
			continue;
		}
		if (c == '\n') {
			on_line(line);
			line.clear();
			continue;
		}
		if (limit != 0 && line.size() >= limit) {
			line.clear();
			dropping = true;
			on_too_long();
			continue;
		}
		line.push_back(c);
	}
}

// 16 bytes from the OS entropy source. False is a hard failure.
static bool os_random_bytes(unsigned char* out, size_t n) {
#ifdef _WIN32
	return ::BCryptGenRandom(nullptr, out, static_cast<ULONG>(n),
		BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
#if defined(__linux__)
	// getrandom is the direct Linux source. A missing syscall falls
	// through to /dev/urandom; any other failure is a hard error.
	{
		size_t got = 0;
		while (got != n) {
			ssize_t r = ::getrandom(out + got, n - got, 0);
			if (r < 0) {
				if (errno == EINTR) continue;
				if (errno != ENOSYS) return false;
				break;
			}
			got += static_cast<size_t>(r);
		}
		if (got == n) return true;
		if (got != 0) return false;
	}
#endif
	int fd = ::open("/dev/urandom", O_RDONLY);
	if (fd < 0) return false;
	size_t got = 0;
	while (got != n) {
		ssize_t r = ::read(fd, out + got, n - got);
		if (r <= 0) {
			if (r < 0 && errno == EINTR) continue;
			::close(fd);
			return false;
		}
		got += static_cast<size_t>(r);
	}
	::close(fd);
	return true;
#endif
}

// 128 bits from the OS entropy source, in hex. A failure is a hard error:
// the id is the only protection of an existing session.
static std::optional<std::string> new_session_id() {
	std::array<unsigned char, 16> bytes{};
	if (!os_random_bytes(bytes.data(), bytes.size()))
		return std::nullopt;
	std::string out;
	out.reserve(32);
	for (unsigned char b : bytes) {
		out.push_back("0123456789abcdef"[b >> 4]);
		out.push_back("0123456789abcdef"[b & 0xf]);
	}
	return out;
}

// A client connection. It owns the socket until the session ends or the
// client quits. The first line picks a new or an existing session.
struct serve_connection : std::enable_shared_from_this<serve_connection> {
	serve_connection(ba::io_context& io, serve_server* s);
	~serve_connection();

	tcp::socket socket;
	serve_server* srv;
	std::shared_ptr<serve_session> session;
	std::string line;
	bool dropping = false;
	bool closed = false;
	bool close_when_drained = false;
	bool read_paused = false;
	bool reading = false;
	size_t out_bytes = 0;
	std::array<char, 8192> read_buf{};
	std::deque<std::string> out;
	// Closes a connection that does not drain its output in time.
	ba::steady_timer write_timer;

	void start();
	void read_some();
	void on_read(error_code ec, size_t n);
	void handle_line(std::string& l);
	void handle_first_line(const std::string& l);
	void handle_too_long();
	void send(std::string text);
	void send_line(const value& v);
	void write_next();
	void close();
	void close_after_flush();
	void arm_write_timer();
	void update_read_state();
};

// One grammar session: a child process, its pipes and the attached client.
struct serve_session : std::enable_shared_from_this<serve_session> {
	serve_session(ba::io_context& io, serve_server* s, std::string sid);
	~serve_session();

	std::string id;
	serve_server* srv;
#ifdef _WIN32
	// Owned process handle; null once the child was reaped.
	HANDLE process = nullptr;
	DWORD process_id = 0;
#else
	pid_t pid = -1;
#endif
	child_pipe to_child;
	child_pipe from_child;
	child_pipe child_stderr;
	std::weak_ptr<serve_connection> conn;
	std::string line;
	bool dropping = false;
	std::string err_line;
	bool err_dropping = false;
	bool ended = false;
	bool shutting_down = false;
	std::array<char, 8192> read_buf{};
	std::array<char, 4096> err_buf{};
	std::deque<std::string> out;
	std::vector<std::string> detached;
	size_t detached_bytes = 0;
	size_t open_requests = 0;
	// A forwarded quit closes the connection after its answer. The count
	// says how many answers are still ahead of that answer.
	bool quit_pending = false;
	size_t quit_wait = 0;
	// The exit code of the child, once it is known, and a guard so the
	// child-exit event is written once in shutdown_session.
	int exit_code = -1;
	bool have_exit_code = false;
	bool child_exit_logged = false;
#ifndef _WIN32
	// The child pipe descriptors as numbers, kept after a close: a closed
	// Asio handle reports -1, but the fork list must lose the original
	// number so a later child never closes a reused descriptor.
	int to_child_fd = -1;
	int from_child_fd = -1;
	int child_stderr_fd = -1;
#endif
	std::chrono::steady_clock::time_point last_activity;
	// The last child output, the idle reference while a request waits.
	std::chrono::steady_clock::time_point last_child_output;
	// Stops a child that does not exit after end-session.
	ba::steady_timer end_timer;
	std::ofstream log;
	bool log_ok = false;

	void start_read();
	void on_read(error_code ec, size_t n);
	void start_stderr_read();
	void on_stderr_read(error_code ec, size_t n);
	void deliver(std::string l);
	void attach(const std::shared_ptr<serve_connection>& c);
	void detach(serve_connection* c);
	void send_to_child(std::string text);
	void write_next();
	void start_end_timer();
	void log_write(const value& v);
	void log_event(const value& v);
};

struct serve_server {
	explicit serve_server(const serve_options& o);
	~serve_server();

	bool start();
	void run();
	uint16_t port() const { return bound_port; }

	void start_accept();
	void handle_new(const std::shared_ptr<serve_connection>& conn,
		const std::string& first_line);
	void handle_attach(const std::shared_ptr<serve_connection>& conn,
		const std::string& id, const std::string& first_line);
	void handle_request(const std::shared_ptr<serve_connection>& conn,
		const std::string& line);
	std::shared_ptr<serve_session> fork_session(const std::string& id);
#ifdef _WIN32
	std::shared_ptr<serve_session> start_child(const std::string& id);
#endif
#ifndef _WIN32
	void close_logs_for_child();
#endif
	void shutdown_session(const std::shared_ptr<serve_session>& s);
	void end_session(const std::shared_ptr<serve_session>& s,
		const value& id);
	void on_sigchld();
	void on_stop_signal();
	void reap_children();
	void start_idle_timer();
	void check_idle();

	void log_failure();
	void log_event(const value& v, serve_session* s);
#ifndef _WIN32
	void track_fd(int fd);
	void untrack_fd(int fd);
	std::vector<int> child_fds;
#endif

	serve_options opt;
	ba::io_context io;
	tcp::acceptor acceptor;
	ba::steady_timer idle_timer;
	ba::signal_set sigchld;
	ba::signal_set sigterm;
	std::map<std::string, std::shared_ptr<serve_session>> sessions;
	std::string log_dir;
	std::ofstream server_log;
	bool log_ok = false;
	bool log_failure_reported = false;
	diagnostics::report report;
	uint16_t bound_port = 0;
	bool stopping = false;
	// True while a tick is scheduled, so a new session only arms it once.
	bool idle_tick_running = false;
};

#ifndef _WIN32
// Give up the address space of the child, so one session cannot take the
// whole machine. A failed call leaves the limit unchanged and puts the
// reason in @p r, which the child hello carries.
static void apply_child_limits(size_t mb, diagnostics::report& r) {
	if (mb == 0) return;
	struct rlimit cur;
	if (::getrlimit(RLIMIT_AS, &cur) != 0) {
		r.error(diagnostics::code::runtime_error,
			parser_strings::messages::session_memory_failed);
		return;
	}
	rlim_t want = static_cast<rlim_t>(mb) * 1024 * 1024;
	if (want > cur.rlim_max) {
		r.error(diagnostics::code::runtime_error,
			parser_strings::messages::session_memory_failed);
		return;
	}
	struct rlimit rl = cur;
	rl.rlim_cur = want;
	if (::setrlimit(RLIMIT_AS, &rl) != 0)
		r.error(diagnostics::code::runtime_error,
			parser_strings::messages::session_memory_failed);
}

// The child runs the JSON loop on the pipes and never returns. @p limits
// carries the errors of apply_child_limits() into the first hello.
[[noreturn]] static void child_run(tgf_repl_evaluator* re,
	const std::string& id, size_t max_line, diagnostics::report limits)
{
	if (!re) ::_exit(1);
	re->begin_server_session(id);
	// A forked child keeps the loaded grammar but reload must use the
	// stored text, so a reload never needs a path here too.
	re->make_grammar_source_backed();
	std::cin.clear();
	std::cout.clear();
	tgf_json_loop(*re, std::cin, std::cout, max_line, false,
		std::move(limits));
	std::cout.flush();
	::_exit(0);
}
#else
// The path of the running executable, so a session starts a second tgf.
static std::wstring current_executable() {
	std::wstring buf(512, L'\0');
	for (;;) {
		DWORD n = ::GetModuleFileNameW(nullptr, buf.data(),
			static_cast<DWORD>(buf.size()));
		if (n == 0) return {};
		if (n < buf.size()) return buf.substr(0, n);
		buf.resize(buf.size() * 2);
	}
}
#endif

serve_connection::serve_connection(ba::io_context& io, serve_server* s)
	: socket(io), srv(s), write_timer(io) {}

serve_connection::~serve_connection() = default;

serve_session::serve_session(ba::io_context& io, serve_server* s,
	std::string sid)
	: id(std::move(sid)), srv(s), to_child(io), from_child(io),
	  child_stderr(io), end_timer(io)
{
	last_activity = std::chrono::steady_clock::now();
	last_child_output = last_activity;
}

serve_session::~serve_session() = default;

serve_server::serve_server(const serve_options& o)
	: opt(o), io(), acceptor(io), idle_timer(io), sigchld(io), sigterm(io)
{
	if (opt.max_sessions == 0) opt.max_sessions = 1;
	if (opt.no_log) return;
	log_dir = opt.log_dir.empty() ? default_log_dir() : opt.log_dir;
	if (log_dir.empty()) return;
	make_dirs(log_dir);
	server_log.open(log_dir + "/server.log", std::ios::app);
	if (!server_log.is_open()) { log_failure(); return; }
	log_ok = true;
}

// Stop every child even when io.run() left by an exception, so no session
// outlives the server. The normal path already emptied the sessions.
serve_server::~serve_server() {
	try {
		std::vector<std::shared_ptr<serve_session>> all;
		for (auto& kv : sessions) all.push_back(kv.second);
		for (auto& s : all) shutdown_session(s);
		sessions.clear();
	} catch (...) {
		// A destructor must not throw; a child that stays is better
		// than terminate.
	}
}

void serve_server::log_failure() {
	if (log_failure_reported) return;
	log_failure_reported = true;
	report.error(diagnostics::code::io_error,
		parser_strings::messages::log_write_failed);
}

void serve_server::log_event(const value& v, serve_session* s) {
	if (log_ok && !log_append(server_log, v)) {
		log_ok = false;
		log_failure();
	}
	if (s) s->log_event(v);
}

void serve_session::log_write(const value& v) {
	if (!log_ok) return;
	if (!log_append(log, v)) {
		log_ok = false;
		srv->log_failure();
	}
}

void serve_session::log_event(const value& v) {
	log_write(v);
}

#ifndef _WIN32
void serve_server::close_logs_for_child() {
	// The child keeps no parent log. The fd is shared by the fork, so a
	// close here stops the child from writing into any log and releases
	// the descriptors the fork inherited.
	server_log.close();
	for (auto& kv : sessions) kv.second->log.close();
}

void serve_server::track_fd(int fd) {
	if (fd > STDERR_FILENO) child_fds.push_back(fd);
}

void serve_server::untrack_fd(int fd) {
	for (size_t i = 0; i != child_fds.size(); ++i)
		if (child_fds[i] == fd) {
			child_fds[i] = child_fds.back();
			child_fds.pop_back();
			return;
		}
}
#endif

bool serve_server::start() {
	error_code ec;
	tcp::endpoint ep(ba::ip::make_address("127.0.0.1", ec), opt.port);
	if (ec) {
		report.error(diagnostics::code::invalid_argument,
			parser_strings::messages::bad_address);
		return false;
	}
	acceptor.open(ep.protocol(), ec);
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::bind_failed);
		return false;
	}
	acceptor.set_option(ba::socket_base::reuse_address(true), ec);
	acceptor.bind(ep, ec);
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::bind_failed);
		return false;
	}
	acceptor.listen(ba::socket_base::max_listen_connections, ec);
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::listen_failed);
		return false;
	}
	auto lep = acceptor.local_endpoint(ec);
	if (ec) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::listen_failed);
		return false;
	}
	bound_port = lep.port();
#ifndef _WIN32
	track_fd(acceptor.native_handle());
#endif
#ifdef _WIN32
	// --session-memory needs setrlimit, which Windows has no equivalent
	// of; the server warns once and starts the child without a limit.
	if (opt.session_memory_mb != 0)
		report.warning(parser_strings::messages::session_memory_unsupported);
#endif
	return true;
}

void serve_server::run() {
	start_accept();
	// The idle tick starts with the first session, so an idle server
	// with no session does not wake up.
	error_code ec;
#ifndef _WIN32
	sigchld.add(SIGCHLD, ec);
	if (!ec)
		sigchld.async_wait([this](error_code e, int) {
			if (!e) on_sigchld();
		});
#endif
	sigterm.add(SIGINT, ec);
	sigterm.add(SIGTERM, ec);
	if (!ec)
		sigterm.async_wait([this](error_code e, int) {
			if (!e) on_stop_signal();
		});
	io.run();
	// The io_context stopped: end every session and its child.
	std::vector<std::shared_ptr<serve_session>> all;
	for (auto& kv : sessions) all.push_back(kv.second);
	for (auto& s : all) shutdown_session(s);
}

void serve_server::start_accept() {
	auto conn = std::make_shared<serve_connection>(io, this);
	acceptor.async_accept(conn->socket,
		[this, conn](error_code ec) {
			if (!ec) {
#ifndef _WIN32
				track_fd(conn->socket.native_handle());
#endif
				conn->start();
			}
			if (!stopping) start_accept();
		});
}

void serve_server::handle_new(
	const std::shared_ptr<serve_connection>& conn,
	const std::string& first_line)
{
	if (sessions.size() >= opt.max_sessions) {
		diagnostics::report rep;
		rep.error(diagnostics::code::out_of_range,
			parser_strings::messages::too_many_sessions);
		conn->send_line(serve_error_response(value::null(), rep));
		conn->close_after_flush();
		return;
	}
	std::optional<std::string> sid = new_session_id();
	while (sid.has_value() && sessions.find(*sid) != sessions.end())
		sid = new_session_id();
	if (!sid.has_value()) {
		diagnostics::report rep;
		rep.error(diagnostics::code::runtime_error,
			parser_strings::messages::entropy_failed);
		conn->send_line(serve_error_response(value::null(), rep));
		conn->close_after_flush();
		return;
	}
	std::string id = std::move(*sid);
	auto s = fork_session(id);
	if (!s) {
		diagnostics::report rep;
		rep.error(diagnostics::code::runtime_error,
			parser_strings::messages::session_start_failed);
		conn->send_line(serve_error_response(value::null(), rep));
		conn->close_after_flush();
		return;
	}
	sessions[id] = s;
	// The idle tick runs while a session exists, also on Windows where
	// it polls the child handles.
	start_idle_timer();
	conn->session = s;
	s->attach(conn);
	// The child writes one unsolicited hello, so count it as an open
	// request: a request that arrives before the hello must not shift
	// the count.
	s->open_requests = 1;
	s->start_read();
	s->start_stderr_read();
	// The log holds the first line of the connection too.
	s->log_write(log_text("in", first_line));
	value ev = log_event_base("new");
	ev.set("session", value::string(id));
	log_event(ev, s.get());
}

void serve_server::handle_attach(
	const std::shared_ptr<serve_connection>& conn, const std::string& id,
	const std::string& first_line)
{
	auto it = sessions.find(id);
	if (it == sessions.end() || it->second->ended) {
		diagnostics::report rep;
		rep.error(diagnostics::code::invalid_argument,
			parser_strings::messages::unknown_session);
		conn->send_line(serve_error_response(value::null(), rep));
		conn->close_after_flush();
		return;
	}
	auto s = it->second;
	if (auto cur = s->conn.lock()) {
		diagnostics::report rep;
		rep.error(diagnostics::code::invalid_argument,
			parser_strings::messages::session_attached);
		conn->send_line(serve_error_response(value::null(), rep));
		conn->close_after_flush();
		return;
	}
	conn->session = s;
	s->attach(conn);
	s->log_write(log_text("in", first_line));
	value ev = log_event_base("attach");
	ev.set("session", value::string(id));
	log_event(ev, s.get());
	// A re-attach asks the child for a fresh hello after the buffer.
	++s->open_requests;
	s->send_to_child("{\"cmd\":\"hello\"}\n");
}

void serve_server::handle_request(
	const std::shared_ptr<serve_connection>& conn, const std::string& line)
{
	auto s = conn->session;
	if (!s || s->ended) { conn->close(); return; }
	s->last_activity = std::chrono::steady_clock::now();
	s->log_write(log_text("in", line));
	// The parent owns quit and end-session, so it reads the command of
	// every line to intercept them. Every other line goes to the child.
	auto req = format::json::parse(line);
	if (req.has_value()) {
		auto cmd = json_string_field(req.value(), "cmd");
		value id = value::null();
		if (auto i = req.value().find("id"); i) id = *i;
		if (cmd.has_value() && cmd.value() == "quit") {
			// The child answers quit and keeps the session.
			// The parent closes the connection after that
			// answer, so it counts the answers in order.
			value ev = log_event_base("quit");
			ev.set("session", value::string(s->id));
			log_event(ev, s.get());
			++s->open_requests;
			s->quit_pending = true;
			s->quit_wait = s->open_requests;
			s->send_to_child(line + "\n");
			conn->update_read_state();
			return;
		}
		if (cmd.has_value() && cmd.value() == "end-session") {
			end_session(s, id);
			return;
		}
	}
	++s->open_requests;
	s->send_to_child(line + "\n");
	conn->update_read_state();
}

#ifndef _WIN32
std::shared_ptr<serve_session> serve_server::fork_session(
	const std::string& id)
{
	int p2c[2] = { -1, -1 };
	int c2p[2] = { -1, -1 };
	int e2p[2] = { -1, -1 };
	if (::pipe(p2c) != 0) return nullptr;
	if (::pipe(c2p) != 0) {
		::close(p2c[0]);
		::close(p2c[1]);
		return nullptr;
	}
	if (::pipe(e2p) != 0) {
		::close(p2c[0]);
		::close(p2c[1]);
		::close(c2p[0]);
		::close(c2p[1]);
		return nullptr;
	}
	// The parent holds one thread, so the fork inherits no second one.
	std::cout.flush();
	io.notify_fork(ba::io_context::fork_prepare);
	pid_t pid = ::fork();
	if (pid < 0) {
		io.notify_fork(ba::io_context::fork_parent);
		::close(p2c[0]);
		::close(p2c[1]);
		::close(c2p[0]);
		::close(c2p[1]);
		::close(e2p[0]);
		::close(e2p[1]);
		return nullptr;
	}
	if (pid == 0) {
		io.notify_fork(ba::io_context::fork_child);
		// Copy the child pipe ends to fresh descriptors first, so the
		// dup2 order onto 0, 1 and 2 can never clobber another end.
		int in_fd = ::fcntl(p2c[0], F_DUPFD, 3);
		int out_fd = ::fcntl(c2p[1], F_DUPFD, 3);
		int err_fd = ::fcntl(e2p[1], F_DUPFD, 3);
		if (in_fd < 0 || out_fd < 0 || err_fd < 0) ::_exit(1);
		// The child keeps no listener, no socket and no parent log.
		for (int fd : child_fds) ::close(fd);
		close_logs_for_child();
		// The original pipe fds are no longer needed.
		::close(p2c[0]);
		::close(p2c[1]);
		::close(c2p[0]);
		::close(c2p[1]);
		::close(e2p[0]);
		::close(e2p[1]);
		if (::dup2(in_fd, STDIN_FILENO) < 0) ::_exit(1);
		if (::dup2(out_fd, STDOUT_FILENO) < 0) ::_exit(1);
		if (::dup2(err_fd, STDERR_FILENO) < 0) ::_exit(1);
		::close(in_fd);
		::close(out_fd);
		::close(err_fd);
		// The server catches these signals with Asio handlers. The
		// child must die on a signal instead, so restore the defaults.
		std::signal(SIGTERM, SIG_DFL);
		std::signal(SIGINT, SIG_DFL);
		std::signal(SIGCHLD, SIG_DFL);
		diagnostics::report limits;
		apply_child_limits(opt.session_memory_mb, limits);
		child_run(opt.evaluator, id, opt.max_line, std::move(limits));
	}
	io.notify_fork(ba::io_context::fork_parent);
	::close(p2c[0]);
	::close(c2p[1]);
	::close(e2p[1]);
	auto s = std::make_shared<serve_session>(io, this, id);
	s->pid = pid;
	error_code ec;
	s->to_child.assign(p2c[1], ec);
	if (ec) {
		::close(p2c[1]);
		::close(c2p[0]);
		::close(e2p[0]);
		return nullptr;
	}
	s->from_child.assign(c2p[0], ec);
	if (ec) {
		::close(c2p[0]);
		::close(e2p[0]);
		return nullptr;
	}
	s->child_stderr.assign(e2p[0], ec);
	if (ec) {
		::close(e2p[0]);
		return nullptr;
	}
	s->to_child_fd = s->to_child.native_handle();
	s->from_child_fd = s->from_child.native_handle();
	s->child_stderr_fd = s->child_stderr.native_handle();
	track_fd(s->to_child_fd);
	track_fd(s->from_child_fd);
	track_fd(s->child_stderr_fd);
	if (log_ok) {
		std::string name = log_dir + "/" + log_start_time()
			+ "-" + id + ".log";
		s->log.open(name, std::ios::app);
		s->log_ok = s->log.is_open();
		if (!s->log_ok) log_failure();
	}
	return s;
}

#else // _WIN32

std::shared_ptr<serve_session> serve_server::fork_session(
	const std::string& id)
{
	return start_child(id);
}

// The init line of a Windows session child: the grammar text, the start
// symbol, the option values and the line limit. The child applies it
// before its first hello, so the session starts with the state of the
// server.
static value serve_init_line(const tgf_repl_evaluator& re,
	size_t max_line)
{
	value init = value::object();
	// A built-in grammar is compiled into the child, so its key is absent
	// and the child keeps it; null means the child starts with none.
	if (re.has_grammar() && !re.has_fixed_grammar()) {
		value g = value::object();
		g.set("name", value::string(re.filename()));
		g.set("source", value::string(re.grammar_text()));
		init.set("grammar", std::move(g));
	} else if (!re.has_grammar()) {
		init.set("grammar", value::null());
	}
	init.set("start", re.start_symbol().empty() ? value::null()
		: value::string(re.start_symbol()));
	init.set("options", re.option_values());
	init.set("max_line", value::number(
		static_cast<double>(max_line)));
	value v = value::object();
	v.set("init", std::move(init));
	return v;
}

// A Windows session is a second tgf process. Each pipe has an overlapped
// server end for the relay and a plain client end that the child inherits
// as one of its standard handles.
std::shared_ptr<serve_session> serve_server::start_child(
	const std::string& id)
{
	struct pipe_pair { HANDLE parent = nullptr; HANDLE child = nullptr; };
	pipe_pair in, out, err;
	static std::atomic<unsigned> counter{0};
	std::wstring base = L"\\\\.\\pipe\\tgf_"
		+ std::to_wstring(::GetCurrentProcessId()) + L"_"
		+ std::to_wstring(counter.fetch_add(1)) + L"_";
	auto make_pair = [&base](const wchar_t* tag, bool parent_writes,
		pipe_pair& p) -> bool
	{
		std::wstring name = base + tag;
		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof sa;
		sa.bInheritHandle = FALSE;
		HANDLE server = ::CreateNamedPipeW(name.c_str(),
			(parent_writes ? PIPE_ACCESS_OUTBOUND
				: PIPE_ACCESS_INBOUND) | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
				| PIPE_REJECT_REMOTE_CLIENTS,
			1, 65536, 65536, 0, &sa);
		if (server == INVALID_HANDLE_VALUE) return false;
		SECURITY_ATTRIBUTES csa{};
		csa.nLength = sizeof csa;
		csa.bInheritHandle = TRUE;
		HANDLE client = ::CreateFileW(name.c_str(),
			parent_writes ? GENERIC_READ : GENERIC_WRITE,
			0, &csa, OPEN_EXISTING, 0, nullptr);
		if (client == INVALID_HANDLE_VALUE) {
			::CloseHandle(server);
			return false;
		}
		p.parent = server;
		p.child = client;
		return true;
	};
	auto close_all = [&] {
		HANDLE hs[6] = { in.parent, in.child, out.parent,
			out.child, err.parent, err.child };
		for (HANDLE h : hs) if (h) ::CloseHandle(h);
		in = out = err = pipe_pair{};
	};
	if (!make_pair(L"in", true, in)
			|| !make_pair(L"out", false, out)
			|| !make_pair(L"err", false, err)) {
		close_all();
		return nullptr;
	}
	std::wstring exe = current_executable();
	if (exe.empty()) { close_all(); return nullptr; }
	std::wstring cmd = win_quote_arg(exe);
	cmd += L" repl --json --session " + win_quote_arg(win_widen(id));
	cmd += L" --init-stdin";
	STARTUPINFOEXW si{};
	si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
	si.StartupInfo.hStdInput = in.child;
	si.StartupInfo.hStdOutput = out.child;
	si.StartupInfo.hStdError = err.child;
	HANDLE inherit[3] = { in.child, out.child, err.child };
	SIZE_T attr_size = 0;
	::InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_size);
	std::vector<char> attr_buf(attr_size);
	LPPROC_THREAD_ATTRIBUTE_LIST attrs =
		reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
			attr_buf.data());
	bool have_attrs = attr_size != 0
		&& ::InitializeProcThreadAttributeList(attrs, 1, 0,
			&attr_size);
	bool attrs_initialized = have_attrs;
	if (have_attrs && !::UpdateProcThreadAttribute(attrs, 0,
			PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherit,
			sizeof inherit, nullptr, nullptr))
		have_attrs = false;
	si.StartupInfo.cb = have_attrs ? sizeof(STARTUPINFOEXW)
		: sizeof(STARTUPINFO);
	si.lpAttributeList = have_attrs ? attrs : nullptr;
	PROCESS_INFORMATION pi{};
	BOOL ok = ::CreateProcessW(exe.c_str(), cmd.data(), nullptr,
		nullptr, TRUE,
		have_attrs ? EXTENDED_STARTUPINFO_PRESENT : 0,
		nullptr, nullptr, &si.StartupInfo, &pi);
	if (attrs_initialized) ::DeleteProcThreadAttributeList(attrs);
	if (!ok) { close_all(); return nullptr; }
	::CloseHandle(pi.hThread);
	// The parent owns only the server ends; the child owns the client ends.
	::CloseHandle(in.child); in.child = nullptr;
	::CloseHandle(out.child); out.child = nullptr;
	::CloseHandle(err.child); err.child = nullptr;
	auto s = std::make_shared<serve_session>(io, this, id);
	s->process = pi.hProcess;
	s->process_id = pi.dwProcessId;
	error_code ec;
	s->to_child.assign(in.parent, ec);
	if (ec) {
		close_all();
		::TerminateProcess(pi.hProcess, 1);
		::CloseHandle(pi.hProcess);
		s->process = nullptr;
		return nullptr;
	}
	in.parent = nullptr;
	s->from_child.assign(out.parent, ec);
	if (ec) {
		close_all();
		::TerminateProcess(pi.hProcess, 1);
		::CloseHandle(pi.hProcess);
		s->process = nullptr;
		return nullptr;
	}
	out.parent = nullptr;
	s->child_stderr.assign(err.parent, ec);
	if (ec) {
		close_all();
		::TerminateProcess(pi.hProcess, 1);
		::CloseHandle(pi.hProcess);
		s->process = nullptr;
		return nullptr;
	}
	err.parent = nullptr;
	if (log_ok) {
		std::string name = log_dir + "/" + log_start_time()
			+ "-" + id + ".log";
		s->log.open(name, std::ios::app);
		s->log_ok = s->log.is_open();
		if (!s->log_ok) log_failure();
	}
	// The child reads the init line before its first hello, so the parent
	// writes it right after the start.
	value init = serve_init_line(*opt.evaluator, opt.max_line);
	std::string init_text = json_to_line(init);
	std::string init_body = init_text;
	if (!init_body.empty() && init_body.back() == '\n')
		init_body.pop_back();
	s->log_write(log_text("in", init_body));
	s->send_to_child(std::move(init_text));
	return s;
}

#endif // _WIN32

void serve_server::shutdown_session(
	const std::shared_ptr<serve_session>& s)
{
	if (!s || s->shutting_down) return;
	s->shutting_down = true;
	s->ended = true;
	if (auto c = s->conn.lock()) c->close_after_flush();
	error_code ec;
	s->end_timer.cancel(ec);
#ifdef _WIN32
	if (s->process) {
		DWORD code = 0;
		// A child that already exited reports its code; a live one is
		// terminated and gets a short time to report as well.
		if (::WaitForSingleObject(s->process, 0) == WAIT_OBJECT_0
				&& ::GetExitCodeProcess(s->process, &code)) {
			s->exit_code = static_cast<int>(code);
			s->have_exit_code = true;
		} else {
			::TerminateProcess(s->process, 1);
			if (::WaitForSingleObject(s->process, 100)
					== WAIT_OBJECT_0
					&& ::GetExitCodeProcess(s->process, &code)) {
				s->exit_code = static_cast<int>(code);
				s->have_exit_code = true;
			}
		}
		::CloseHandle(s->process);
		s->process = nullptr;
	}
#else
	// A pipe read can reach the end of the child output before SIGCHLD
	// arrives, so reap here too and keep the code for the log below.
	if (s->pid > 0 && !s->have_exit_code) {
		int status = 0;
		pid_t p = ::waitpid(s->pid, &status, WNOHANG);
		if (p == 0) {
			// The child is still alive: stop it and reap it, so
			// its exit reaches the log too.
			::kill(s->pid, SIGTERM);
			// A bounded wait, so a stuck child cannot block long.
			for (int i = 0; i != 100; ++i) {
				p = ::waitpid(s->pid, &status, WNOHANG);
				if (p == s->pid) break;
				::usleep(1000);
			}
			// SIGTERM did not stop it within the grace, so force it.
			if (p != s->pid) {
				::kill(s->pid, SIGKILL);
				for (int i = 0; i != 100; ++i) {
					p = ::waitpid(s->pid, &status, WNOHANG);
					if (p == s->pid) break;
					::usleep(1000);
				}
			}
		}
		if (p == s->pid) {
			s->exit_code = WIFEXITED(status) ? WEXITSTATUS(status)
				: (WIFSIGNALED(status)
					? 128 + WTERMSIG(status) : -1);
			s->have_exit_code = true;
			s->pid = -1;
		}
	}
	// The close makes a handle report -1, so untrack the stored numbers.
	untrack_fd(s->to_child_fd);
	untrack_fd(s->from_child_fd);
	untrack_fd(s->child_stderr_fd);
	s->to_child_fd = s->from_child_fd = s->child_stderr_fd = -1;
#endif
	s->to_child.close(ec);
	s->from_child.close(ec);
	s->child_stderr.close(ec);
	// The exit is written once, whichever of the pipe read or the reap
	// reaches the session first.
	if (s->have_exit_code && !s->child_exit_logged) {
		s->child_exit_logged = true;
		value ev = log_event_base("child-exit");
		ev.set("session", value::string(s->id));
		ev.set("exit", value::number(
			static_cast<double>(s->exit_code)));
		log_event(ev, s.get());
	}
#ifndef _WIN32
	// A reaped child has pid -1, so this never signals a reused pid.
	if (s->pid > 0) ::kill(s->pid, SIGTERM);
#endif
	s->log.close();
	sessions.erase(s->id);
}

// end-session: the parent answers, closes the input of the child and keeps
// the entry until the child exits, so its exit code reaches the log.
void serve_server::end_session(const std::shared_ptr<serve_session>& s,
	const value& id)
{
	if (!s || s->ended) return;
	s->ended = true;
	value resp = serve_quit_response(id);
	std::string resp_text = json_to_line(resp);
	if (!resp_text.empty() && resp_text.back() == '\n')
		resp_text.pop_back();
	s->log_write(log_text("out", resp_text));
	if (auto c = s->conn.lock()) {
		c->send(std::move(resp_text) + "\n");
		c->close_after_flush();
	}
	value ev = log_event_base("end-session");
	ev.set("session", value::string(s->id));
	log_event(ev, s.get());
	// Closing the write end hands the child an end of input, so its JSON
	// loop returns without a quit of its own.
	error_code ec;
#ifndef _WIN32
	// Untrack here: the close makes the handle report -1, so
	// shutdown_session cannot recover the number to untrack.
	untrack_fd(s->to_child_fd);
	s->to_child_fd = -1;
#endif
	s->to_child.close(ec);
	// A child that does not exit within a short time is stopped.
	s->start_end_timer();
}

#ifndef _WIN32
void serve_server::on_sigchld() {
	reap_children();
	sigchld.async_wait([this](error_code e, int) {
		if (!e) on_sigchld();
	});
}
#endif

void serve_server::on_stop_signal() {
	stopping = true;
	io.stop();
}

void serve_server::reap_children() {
#ifdef _WIN32
	// The idle tick polls each process handle, which is the only wait a
	// Windows child gives without a second thread.
	std::vector<std::shared_ptr<serve_session>> dead;
	for (auto& kv : sessions) {
		auto& s = kv.second;
		if (s->process && ::WaitForSingleObject(s->process, 0)
				== WAIT_OBJECT_0)
			dead.push_back(s);
	}
	// shutdown_session reads the exit code before it closes the handle.
	for (auto& s : dead) shutdown_session(s);
#else
	for (;;) {
		int status = 0;
		pid_t p = ::waitpid(-1, &status, WNOHANG);
		if (p <= 0) break;
		std::shared_ptr<serve_session> found;
		for (auto& kv : sessions)
			if (kv.second->pid == p) { found = kv.second; break; }
		if (!found) continue;
		found->exit_code = WIFEXITED(status) ? WEXITSTATUS(status)
			: (WIFSIGNALED(status)
				? 128 + WTERMSIG(status) : -1);
		found->have_exit_code = true;
		// The child is reaped, so do not signal this pid again.
		found->pid = -1;
		// shutdown_session writes the child-exit event once.
		shutdown_session(found);
	}
#endif
}

void serve_server::start_idle_timer() {
	if (idle_tick_running) return;
	idle_tick_running = true;
	idle_timer.expires_after(std::chrono::milliseconds(serve_tick_ms));
	idle_timer.async_wait([this](error_code ec) {
		idle_tick_running = false;
		if (ec) return;
#ifdef _WIN32
		reap_children();
#endif
		check_idle();
		// No session means no tick; the next session arms it again.
		if (!sessions.empty()) start_idle_timer();
	});
}

void serve_server::check_idle() {
	if (opt.idle_timeout_minutes == 0) return;
	auto now = std::chrono::steady_clock::now();
	auto limit = std::chrono::minutes(opt.idle_timeout_minutes);
	std::vector<std::shared_ptr<serve_session>> dead;
	for (auto& kv : sessions) {
		auto& s = kv.second;
		if (s->ended) continue;
		// A request that waits on the child measures the idle time
		// from the last child output, so a hung child is ended too.
		auto ref = s->open_requests != 0 ? s->last_child_output
			: s->last_activity;
		if (now - ref >= limit) dead.push_back(s);
	}
	for (auto& s : dead) {
		value ev = log_event_base("idle-timeout");
		ev.set("session", value::string(s->id));
		log_event(ev, s.get());
		shutdown_session(s);
	}
}

void serve_connection::start() {
	read_some();
}

void serve_connection::read_some() {
	if (closed || read_paused || reading) return;
	reading = true;
	auto self = shared_from_this();
	socket.async_read_some(ba::buffer(read_buf),
		[self](error_code ec, size_t n) { self->on_read(ec, n); });
}

void serve_connection::on_read(error_code ec, size_t n) {
	reading = false;
	if (closed) return;
	if (ec) { close(); return; }
	feed_lines(line, dropping, srv->opt.max_line,
		read_buf.data(), n,
		[this](std::string& l) { handle_line(l); },
		[this] { handle_too_long(); });
	if (!closed && !close_when_drained && !read_paused) read_some();
}

void serve_connection::handle_line(std::string& l) {
	if (closed || close_when_drained) return;
	if (!l.empty() && l.back() == '\r') l.pop_back();
	if (l.empty()) return;
	if (session) srv->handle_request(shared_from_this(), l);
	else handle_first_line(l);
}

void serve_connection::handle_first_line(const std::string& l) {
	auto req = format::json::parse(l);
	if (!req.has_value()) {
		send_line(serve_error_response(value::null(), req.report()));
		close_after_flush();
		return;
	}
	auto cmd = json_string_field(req.value(), "cmd");
	if (cmd.has_value() && cmd.value() == "new") {
		srv->handle_new(shared_from_this(), l);
		return;
	}
	if (cmd.has_value() && cmd.value() == "attach") {
		auto id = json_string_field(req.value(), "session");
		if (!id.has_value()) {
			diagnostics::report rep;
			rep.error(diagnostics::code::invalid_argument,
				parser_strings::messages::missing_field);
			send_line(serve_error_response(value::null(), rep));
			close_after_flush();
			return;
		}
		srv->handle_attach(shared_from_this(), id.value(), l);
		return;
	}
	diagnostics::report rep;
	rep.error(diagnostics::code::invalid_argument,
		parser_strings::messages::unknown_command);
	send_line(serve_error_response(value::null(), rep));
	close_after_flush();
}

void serve_connection::handle_too_long() {
	diagnostics::report rep;
	rep.error(diagnostics::code::out_of_range,
		parser_strings::messages::line_too_long);
	if (session) {
		value ev = log_event_base("line-too-long");
		ev.set("session", value::string(session->id));
		srv->log_event(ev, session.get());
	}
	send_line(serve_error_response(value::null(), rep));
}

void serve_connection::send(std::string text) {
	if (closed) return;
	out_bytes += text.size();
	out.push_back(std::move(text));
	if (out.size() == 1) {
		arm_write_timer();
		write_next();
	}
	update_read_state();
}

void serve_connection::send_line(const value& v) {
	std::string text = json_to_line(v);
	if (session) {
		std::string body = text;
		if (!body.empty() && body.back() == '\n') body.pop_back();
		session->log_write(log_text("out", body));
	}
	send(std::move(text));
}

void serve_connection::write_next() {
	if (closed || out.empty()) return;
	auto self = shared_from_this();
	ba::async_write(socket, ba::buffer(out.front()),
		[self](error_code ec, size_t) {
			if (ec) { self->close(); return; }
			self->out_bytes -= self->out.front().size();
			self->out.pop_front();
			if (self->out.empty() && self->close_when_drained) {
				self->close();
				return;
			}
			self->update_read_state();
			// The write made progress, so the deadline starts again.
			self->arm_write_timer();
			self->write_next();
		});
}

void serve_connection::close_after_flush() {
	if (closed) return;
	close_when_drained = true;
	if (out.empty()) close();
}

// A connection that makes no write progress for the deadline is closed,
// so a stalled client cannot hold a queued answer open forever.
void serve_connection::arm_write_timer() {
	if (closed || out.empty()) return;
	if (srv->opt.write_timeout_seconds == 0) return;
	write_timer.expires_after(
		std::chrono::seconds(srv->opt.write_timeout_seconds));
	auto self = shared_from_this();
	write_timer.async_wait([self](error_code ec) {
		if (ec) return;
		self->close();
	});
}

// Read from a client only while its output and the child queue have room.
void serve_connection::update_read_state() {
	if (closed) return;
	bool congested = out_bytes >= serve_conn_out_limit;
	if (session && session->open_requests >= serve_open_requests_limit)
		congested = true;
	if (congested) { read_paused = true; return; }
	if (!read_paused) return;
	read_paused = false;
	read_some();
}

void serve_connection::close() {
	if (closed) return;
	closed = true;
	error_code ec;
	write_timer.cancel(ec);
#ifndef _WIN32
	srv->untrack_fd(socket.native_handle());
#endif
	socket.shutdown(tcp::socket::shutdown_both, ec);
	socket.close(ec);
	if (session) {
		value ev = log_event_base("detach");
		ev.set("session", value::string(session->id));
		srv->log_event(ev, session.get());
		session->detach(this);
		session.reset();
	}
}

void serve_session::start_read() {
	auto self = shared_from_this();
	from_child.async_read_some(ba::buffer(read_buf),
		[self](error_code ec, size_t n) { self->on_read(ec, n); });
}

void serve_session::on_read(error_code ec, size_t n) {
	if (ec) { srv->shutdown_session(shared_from_this()); return; }
	if (ended) {
		// end-session closed the child input, but a pending answer may
		// still be in the pipe. Drain and discard it so the child never
		// blocks on a full pipe and the session leaves the table.
		feed_lines(line, dropping, serve_child_line_limit,
			read_buf.data(), n, [](std::string&) {}, [] {});
	} else {
		feed_lines(line, dropping, serve_child_line_limit,
			read_buf.data(), n,
			[this](std::string& l) { deliver(std::move(l)); },
			[this] { srv->shutdown_session(shared_from_this()); });
	}
	if (!shutting_down) start_read();
}

void serve_session::start_stderr_read() {
	if (ended) return;
	auto self = shared_from_this();
	child_stderr.async_read_some(ba::buffer(err_buf),
		[self](error_code ec, size_t n) { self->on_stderr_read(ec, n); });
}

void serve_session::on_stderr_read(error_code ec, size_t n) {
	if (ec) return;
	feed_lines(err_line, err_dropping, serve_stderr_line_limit,
		err_buf.data(), n,
		[this](std::string& l) { log_write(log_text("stderr", l)); },
		[] {});
	if (!ended) start_stderr_read();
}

void serve_session::deliver(std::string l) {
	last_activity = std::chrono::steady_clock::now();
	last_child_output = last_activity;
	if (open_requests > 0) --open_requests;
	bool close_after = false;
	if (quit_pending) {
		if (quit_wait > 0) --quit_wait;
		if (quit_wait == 0) { quit_pending = false; close_after = true; }
	}
	log_write(log_text("out", l));
	l.push_back('\n');
	auto c = conn.lock();
	if (c && !c->closed) {
		c->send(std::move(l));
		if (close_after) c->close_after_flush();
		return;
	}
	// The client is gone: keep the line for the next attach.
	detached.push_back(std::move(l));
	detached_bytes += detached.back().size();
	size_t dropped = 0;
	while (detached_bytes > serve_detached_limit && !detached.empty()) {
		detached_bytes -= detached.front().size();
		detached.erase(detached.begin());
		++dropped;
	}
	if (dropped) {
		value ev = log_event_base("dropped-buffer-lines");
		ev.set("session", value::string(id));
		ev.set("count", value::number(
			static_cast<double>(dropped)));
		srv->log_event(ev, this);
	}
}

void serve_session::attach(const std::shared_ptr<serve_connection>& c) {
	conn = c;
	last_activity = std::chrono::steady_clock::now();
	if (detached.empty()) return;
	for (auto& l : detached) c->send(l);
	detached.clear();
	detached_bytes = 0;
}

void serve_session::detach(serve_connection* c) {
	auto cur = conn.lock();
	if (cur && cur.get() == c) conn.reset();
}

void serve_session::send_to_child(std::string text) {
	if (ended) return;
	out.push_back(std::move(text));
	if (out.size() == 1) write_next();
}

void serve_session::write_next() {
	if (ended || out.empty()) return;
	auto self = shared_from_this();
	ba::async_write(to_child, ba::buffer(out.front()),
		[self](error_code ec, size_t) {
			if (ec) {
				self->srv->shutdown_session(self);
				return;
			}
			self->out.pop_front();
			self->write_next();
		});
}

// Stop a child that does not exit after end-session.
void serve_session::start_end_timer() {
	end_timer.expires_after(
		std::chrono::milliseconds(serve_end_timeout_ms));
	auto self = shared_from_this();
	end_timer.async_wait([self](error_code ec) {
		if (ec) return;
		self->srv->shutdown_session(self);
	});
}

static diagnostics::result<int> serve_run_impl(const serve_options& opt)
{
	diagnostics::result<int> res;
	if (!opt.evaluator) {
		res.error(diagnostics::code::invalid_argument,
			parser_strings::messages::session_start_failed);
		return res;
	}
	// A write to a pipe whose reader is gone must not kill the server.
#ifndef _WIN32
	std::signal(SIGPIPE, SIG_IGN);
#endif
	serve_server srv(opt);
	if (!srv.start()) {
		res.report().append(std::move(srv.report));
		return res;
	}
	uint16_t port = srv.port();
	value v = value::object();
	v.set("listening", value::number(port));
	json_write_line(std::cout, v);
	srv.run();
	diagnostics::report rep = std::move(srv.report);
	if (rep.has_error()) {
		res.report().append(std::move(rep));
		return res;
	}
	res.emplace(0);
	res.report().append(std::move(rep));
	return res;
}

diagnostics::result<int> tgf_serve_run(const serve_options& opt)
{
	diagnostics::result<int> res;
	try {
		return serve_run_impl(opt);
	} catch (const std::exception&) {
		res.error(diagnostics::code::runtime_error,
			parser_strings::messages::serve_failed);
		return res;
	} catch (...) {
		res.error(diagnostics::code::runtime_error,
			parser_strings::messages::serve_failed);
		return res;
	}
}

} // namespace idni
