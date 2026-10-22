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

#include <array>
#include <chrono>
#include <csignal>
#include <deque>
#include <exception>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
// Keep windows.h from pulling in the older winsock.h, which conflicts with
// the winsock2.h that Boost.Asio includes.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
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
#include "net/line_reader.h"
#include "net/line_server.h"
#include "net/line_server_asio.h"
#include "net/line_support.h"
#include "parser_strings.h"
#include "utility/diagnostics.h"
#include "utility/filesystem.h"
#include "utility/win_args.h"

namespace idni {

namespace ba = boost::asio;
using boost::system::error_code;
using jt = format::json::tree;
using format::json::string_member;
using tp = tgf_protocol;

// The relay end of one child pipe: a POSIX file descriptor or an
// overlapped Windows named-pipe handle. Both support the same Asio stream
// operations, so the relay code below is shared.
#ifdef _WIN32
using child_pipe = ba::windows::stream_handle;
#else
using child_pipe = ba::posix::stream_descriptor;
#endif

// The idle timer ticks at this interval.
inline constexpr int serve_tick_ms = 200;
// The log event of a child that exits.
inline constexpr const char* serve_child_exit_event = "child-exit";

struct tgf_child;
struct tgf_serve_state;
struct tgf_session_relay;

// The error answer to a line that reaches no session.
static tref serve_error_response(const diagnostics::report& rep) {
	return tgf_repl_evaluator_base::error_response(jt::_null(), rep);
}

// The `quit` answer of the end-session request: no state.
static tref serve_quit_response(tref id) {
	return jt::build_object({
		jt::build_pair(tp::id, id),
		jt::build_pair(tp::cmd, jt::build_string(
			tgf_repl_evaluator_base::name_of(
				tgf_repl_parser::quit_cmd))),
		jt::build_pair(tp::status, jt::build_string(
			tgf_repl_evaluator_base::status_name(cmd_status::quit))) });
}

// A line server session that relays each line to a tgf child process.
struct tgf_session_relay : session_handler {
	explicit tgf_session_relay(tgf_serve_state& s) : state(s) {}
	~tgf_session_relay() override;

	// the table of children by session id
	tgf_serve_state& state;
	// pipes, pid or process handle, detached buffer
	std::shared_ptr<tgf_child> child;
	// valid from on_open until on_close returns
	reply_sink* out = nullptr;

	void on_open(tref first_line, reply_sink& o) override;
	void on_line(tref request, reply_sink& o) override;
	void on_close() override;
	void on_server_event(tref event) override;

	void send(tref v);
	/// Send one line of the child as it is.
	void send_text(const std::string& line);
	/// Answer with the error @p msg and close after the answer.
	void refuse(diagnostics::code c, std::string_view msg);
	void close_after_flush();
	void detach();
};

// One grammar session: a child process, its pipes and the attached relay.
struct tgf_child : std::enable_shared_from_this<tgf_child> {
	tgf_child(ba::io_context& io, tgf_serve_state* s, std::string sid);

	std::string id;
	tgf_serve_state* st;
#ifdef _WIN32
	// Owned process handle; null once the child was reaped.
	HANDLE process = nullptr;
#else
	pid_t pid = -1;
#endif
	child_pipe to_child;
	child_pipe from_child;
	child_pipe child_stderr;
	// null while no client is attached
	tgf_session_relay* relay = nullptr;
	std::string line;
	bool dropping = false;
	std::string err_line;
	bool err_dropping = false;
	bool ended = false;
	bool shutting_down = false;
	std::array<char, 8192> read_buf{};
	std::array<char, 4096> err_buf{};
	std::deque<std::string> out;
	// bytes in out
	size_t out_bytes = 0;
	// True while the read of the client stops for this child.
	bool read_paused = false;
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
	std::chrono::steady_clock::time_point last_activity;
	// The last child output, the idle reference while a request waits.
	std::chrono::steady_clock::time_point last_child_output;
	// Stops a child that does not exit after end-session.
	ba::steady_timer end_timer;
	fs::file_ostream log;
	bool log_ok = false;

	void start_read();
	void on_read(error_code ec, size_t n);
	void start_stderr_read();
	void on_stderr_read(error_code ec, size_t n);
	void deliver(std::string l);
	void attach(tgf_session_relay* r);
	void send_to_child(std::string text);
	void write_next();
	/// Stop or start the read of the client by the open requests and the
	/// queued bytes.
	void update_read_state();
	void start_end_timer();
	void log_write(tref v);
	void log_event(tref v);
	void log_store(tref v);
};

// The tgf side of the server: the children, their table, the logs and the
// events. Its Asio objects live on the loop of the line server.
struct tgf_serve_state {
	explicit tgf_serve_state(const serve_options& o);
	~tgf_serve_state();

	/// Take the loop of the line server at the first session.
	void bind_io();
	/// The stop of the line server: end every child and give up the loop.
	void stop();

	void handle_new(tgf_session_relay& r, const std::string& first_line);
	void handle_attach(tgf_session_relay& r, const std::string& id,
		const std::string& first_line);
	std::shared_ptr<tgf_child> fork_session(const std::string& id);
#ifdef _WIN32
	std::shared_ptr<tgf_child> start_child(const std::string& id);
#endif
	/// End the child of @p s. With @p wait the call waits for the exit,
	/// for the stop, when the loop runs no more.
	void shutdown_session(const std::shared_ptr<tgf_child>& s,
		bool wait = false);
	/// Write the exit of a reaped child and close its log.
	void finish_child(const std::shared_ptr<tgf_child>& s);
	void end_session(const std::shared_ptr<tgf_child>& s, tref id);
	void on_sigchld();
	void reap_children();
	void start_idle_timer();
	void check_idle();

	void log_failure(diagnostics::report detail = {});
	void open_session_log(tgf_child& s);
	void log_event(tref v, tgf_child* s);

	serve_options opt;
	// empty turns every log off
	std::string log_dir;
	ba::io_context* io = nullptr;
	std::unique_ptr<ba::steady_timer> idle_timer;
	std::unique_ptr<ba::signal_set> sigchld;
	std::map<std::string, std::shared_ptr<tgf_child>> sessions;
	// Ended sessions whose child still runs. The reap writes their exit.
	std::vector<std::shared_ptr<tgf_child>> reaping;
	bool log_failure_reported = false;
	diagnostics::report report;
	bool stopped = false;
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

// The line server does not list its descriptors, so the child closes
// every descriptor above stderr: listeners, client sockets and logs.
static void close_inherited_fds() {
#ifdef SYS_close_range
	if (::syscall(SYS_close_range, 3u, ~0u, 0u) == 0) return;
#endif
	long max = ::sysconf(_SC_OPEN_MAX);
	if (max < 0 || max > (1L << 20)) max = 1L << 20;
	for (long fd = 3; fd < max; ++fd) ::close(static_cast<int>(fd));
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

// The exit code of a wait status: the code, or 128 plus the signal.
static int exit_code_of(int status) {
	return WIFEXITED(status) ? WEXITSTATUS(status)
		: (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1);
}

// Stop the child process with no Asio call.
static void kill_child(tgf_child& s) {
	if (s.pid <= 0) return;
	::kill(s.pid, SIGKILL);
	int status = 0;
	if (::waitpid(s.pid, &status, 0) == s.pid) {
		s.exit_code = exit_code_of(status);
		s.have_exit_code = true;
	}
	s.pid = -1;
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

// Stop the child process with no Asio call.
static void kill_child(tgf_child& s) {
	if (!s.process) return;
	::TerminateProcess(s.process, 1);
	DWORD code = 0;
	if (::WaitForSingleObject(s.process, 1000) == WAIT_OBJECT_0
			&& ::GetExitCodeProcess(s.process, &code)) {
		s.exit_code = static_cast<int>(code);
		s.have_exit_code = true;
	}
	::CloseHandle(s.process);
	s.process = nullptr;
}
#endif

tgf_child::tgf_child(ba::io_context& io, tgf_serve_state* s,
	std::string sid)
	: id(std::move(sid)), st(s), to_child(io), from_child(io),
	  child_stderr(io), end_timer(io)
{
	last_activity = std::chrono::steady_clock::now();
	last_child_output = last_activity;
}

tgf_serve_state::tgf_serve_state(const serve_options& o) : opt(o) {
	if (opt.max_sessions == 0) opt.max_sessions = 1;
#ifdef _WIN32
	// --session-memory needs setrlimit, which Windows has no equivalent
	// of; the server warns once and starts the child without a limit.
	if (opt.session_memory_mb != 0)
		report.warning(parser_strings::messages::session_memory_unsupported);
#endif
	if (opt.no_log) return;
	if (!opt.log_dir.empty()) { log_dir = opt.log_dir; return; }
	auto home = expand_home("~/.tau/tgf/logs");
	if (home.has_value()) log_dir = std::move(home).value();
	else report.append(home.report());
	// The line server makes the folder and owns server.log, before the
	// first session log opens here.
}

void tgf_serve_state::open_session_log(tgf_child& s) {
	if (log_dir.empty()) return;
	auto opened = line_log::open_session(log_dir, s.id);
	s.log_ok = opened.has_value();
	if (s.log_ok) s.log = std::move(opened).value();
	else log_failure(std::move(opened).report());
}

tgf_serve_state::~tgf_serve_state() {
	if (stopped) return;
	// An exception ended the loop before on_stop, so the loop is gone.
	// Stop the processes and leave the Asio objects, which need the loop.
	for (auto& kv : sessions) kill_child(*kv.second);
	static_cast<void>(new std::map<std::string,
		std::shared_ptr<tgf_child>>(std::move(sessions)));
	static_cast<void>(idle_timer.release());
	static_cast<void>(sigchld.release());
}

void tgf_serve_state::bind_io() {
	if (io) return;
	io = &line_server_io();
	idle_timer = std::make_unique<ba::steady_timer>(*io);
#ifndef _WIN32
	sigchld = std::make_unique<ba::signal_set>(*io);
	error_code ec;
	sigchld->add(SIGCHLD, ec);
	if (!ec)
		sigchld->async_wait([this](error_code e, int) {
			if (!e) on_sigchld();
		});
#endif
}

void tgf_serve_state::stop() {
	std::vector<std::shared_ptr<tgf_child>> live;
	for (auto& kv : sessions) live.push_back(kv.second);
	// the loop has ended, so each child is waited for here
	for (auto& s : live) shutdown_session(s, true);
	sessions.clear();
	for (auto& s : std::vector(std::move(reaping))) {
		kill_child(*s);
		finish_child(s);
	}
	reaping.clear();
	idle_timer.reset();
	sigchld.reset();
	io = nullptr;
	stopped = true;
}

void tgf_serve_state::log_failure(diagnostics::report detail) {
	if (log_failure_reported) return;
	log_failure_reported = true;
	if (detail.has_error()) {
		report.append(std::move(detail));
		return;
	}
	report.error(diagnostics::code::io_error,
		parser_strings::messages::log_write_failed);
}

void tgf_serve_state::log_event(tref v, tgf_child* s) {
	line_server_log(v);
	if (s) s->log_event(v);
}

void tgf_child::log_write(tref v) {
	// The copy runs also with --no-log, so it comes before the log check.
	if (st->opt.debug_sessions) line_server_debug(v, id);
	log_store(v);
}

// The server prints an event itself, so it skips the session log copy.
void tgf_child::log_event(tref v) {
	log_store(v);
}

void tgf_child::log_store(tref v) {
	if (!log_ok) return;
	if (!line_log::append(log, v)) {
		log_ok = false;
		st->log_failure();
	}
}

void tgf_serve_state::handle_new(tgf_session_relay& r,
	const std::string& first_line)
{
	if (sessions.size() >= opt.max_sessions) {
		r.refuse(diagnostics::code::out_of_range,
			parser_strings::messages::too_many_sessions);
		return;
	}
	std::optional<std::string> sid = new_session_id();
	while (sid.has_value() && sessions.find(*sid) != sessions.end())
		sid = new_session_id();
	if (!sid.has_value()) {
		r.refuse(diagnostics::code::runtime_error,
			parser_strings::messages::entropy_failed);
		return;
	}
	std::string id = std::move(*sid);
	auto s = fork_session(id);
	if (!s) {
		r.refuse(diagnostics::code::runtime_error,
			parser_strings::messages::session_start_failed);
		return;
	}
	sessions[id] = s;
	// The idle tick runs while a session exists, also on Windows where
	// it polls the child handles.
	start_idle_timer();
	r.child = s;
	s->attach(&r);
	// The child writes one unsolicited hello, so count it as an open
	// request: a request that arrives before the hello must not shift
	// the count.
	s->open_requests = 1;
	s->start_read();
	s->start_stderr_read();
	// The log holds the first line of the connection too.
	s->log_write(line_log::text("in", first_line));
	log_event(line_log::session_event(tp::new_session, id), s.get());
}

void tgf_serve_state::handle_attach(tgf_session_relay& r,
	const std::string& id, const std::string& first_line)
{
	auto it = sessions.find(id);
	if (it == sessions.end() || it->second->ended) {
		r.refuse(diagnostics::code::invalid_argument,
			parser_strings::messages::unknown_session);
		return;
	}
	auto s = it->second;
	if (s->relay) {
		r.refuse(diagnostics::code::invalid_argument,
			parser_strings::messages::session_attached);
		return;
	}
	r.child = s;
	s->attach(&r);
	s->log_write(line_log::text("in", first_line));
	log_event(line_log::session_event(tp::attach, id), s.get());
	// A re-attach asks the child for a fresh hello after the buffer.
	++s->open_requests;
	s->send_to_child(jt::get(jt::build_object({ jt::build_pair(tp::cmd,
		jt::build_string(tp::hello)) })).to_str() + "\n");
}

tgf_session_relay::~tgf_session_relay() {
	detach();
}

void tgf_session_relay::on_open(tref first_line, reply_sink& o) {
	out = &o;
	const std::string text = jt::get(first_line).to_str();
	const std::string* cmd = string_member(first_line, tp::cmd);
	if (cmd && *cmd == tp::new_session) {
		state.handle_new(*this, text);
		return;
	}
	if (cmd && *cmd == tp::attach) {
		const std::string* id = string_member(first_line, tp::session);
		if (!id) {
			refuse(diagnostics::code::invalid_argument,
				parser_strings::messages::missing_field);
			return;
		}
		state.handle_attach(*this, *id, text);
		return;
	}
	refuse(diagnostics::code::invalid_argument,
		parser_strings::messages::unknown_command);
}

void tgf_session_relay::on_line(tref request, reply_sink& o) {
	out = &o;
	auto s = child;
	if (!s || s->ended) { close_after_flush(); return; }
	s->last_activity = std::chrono::steady_clock::now();
	std::string line = jt::get(request).to_str();
	s->log_write(line_log::text("in", line));
	// The parent owns quit and end-session, so it reads the command of
	// every line to intercept them. Every other line goes to the child.
	const std::string* cmd = string_member(request, tp::cmd);
	tref id = jt::_null();
	if (tref i = jt::get(request).find(tp::id); i) id = i;
	if (cmd && tgf_repl_evaluator_base::command_of(*cmd)
			== tgf_repl_parser::quit_cmd) {
		// The child answers quit and keeps the session. The parent
		// closes the connection after that answer, so it counts the
		// answers in order.
		state.log_event(line_log::session_event(
			tgf_repl_evaluator_base::name_of(
				tgf_repl_parser::quit_cmd), s->id), s.get());
		++s->open_requests;
		s->quit_pending = true;
		s->quit_wait = s->open_requests;
		s->send_to_child(line + "\n");
		return;
	}
	if (cmd && *cmd == tp::end_session) {
		state.end_session(s, id);
		return;
	}
	++s->open_requests;
	s->send_to_child(line + "\n");
}

void tgf_session_relay::on_close() {
	if (child && child->relay == this) {
		state.log_event(line_log::session_event("detach", child->id), child.get());
		child->relay = nullptr;
	}
	out = nullptr;
}

void tgf_session_relay::on_server_event(tref event) {
	if (child) child->log_event(event);
}

void tgf_session_relay::detach() {
	if (child && child->relay == this) child->relay = nullptr;
	out = nullptr;
}

void tgf_session_relay::send(tref v) {
	if (out) out->write(v);
}

void tgf_session_relay::send_text(const std::string& line) {
	if (out) out->write(line);
}

void tgf_session_relay::refuse(diagnostics::code c, std::string_view msg)
{
	diagnostics::report rep;
	rep.error(c, msg);
	send(serve_error_response(rep));
	close_after_flush();
}

// The close can run on_close at once, which clears out.
void tgf_session_relay::close_after_flush() {
	if (reply_sink* o = out) o->close();
}

#ifndef _WIN32
std::shared_ptr<tgf_child> tgf_serve_state::fork_session(
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
	io->notify_fork(ba::io_context::fork_prepare);
	pid_t pid = ::fork();
	if (pid < 0) {
		io->notify_fork(ba::io_context::fork_parent);
		::close(p2c[0]);
		::close(p2c[1]);
		::close(c2p[0]);
		::close(c2p[1]);
		::close(e2p[0]);
		::close(e2p[1]);
		return nullptr;
	}
	if (pid == 0) {
		io->notify_fork(ba::io_context::fork_child);
		// Copy the child pipe ends to fresh descriptors first, so the
		// dup2 order onto 0, 1 and 2 can never clobber another end.
		int in_fd = ::fcntl(p2c[0], F_DUPFD, 3);
		int out_fd = ::fcntl(c2p[1], F_DUPFD, 3);
		int err_fd = ::fcntl(e2p[1], F_DUPFD, 3);
		if (in_fd < 0 || out_fd < 0 || err_fd < 0) ::_exit(1);
		if (::dup2(in_fd, STDIN_FILENO) < 0) ::_exit(1);
		if (::dup2(out_fd, STDOUT_FILENO) < 0) ::_exit(1);
		if (::dup2(err_fd, STDERR_FILENO) < 0) ::_exit(1);
		// This also closes the pipe ends and their copies.
		close_inherited_fds();
		// The server catches these signals with Asio handlers. The
		// child must die on a signal instead, so restore the defaults.
		std::signal(SIGTERM, SIG_DFL);
		std::signal(SIGINT, SIG_DFL);
		std::signal(SIGCHLD, SIG_DFL);
		diagnostics::report limits;
		apply_child_limits(opt.session_memory_mb, limits);
		child_run(opt.evaluator, id, opt.max_line, std::move(limits));
	}
	io->notify_fork(ba::io_context::fork_parent);
	::close(p2c[0]);
	::close(c2p[1]);
	::close(e2p[1]);
	auto s = std::make_shared<tgf_child>(*io, this, id);
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
	open_session_log(*s);
	return s;
}

#else // _WIN32

std::shared_ptr<tgf_child> tgf_serve_state::fork_session(
	const std::string& id)
{
	return start_child(id);
}

// The init line of a Windows session child: the grammar text, the start
// symbol, the option values and the line limit. The child applies it
// before its first hello, so the session starts with the state of the
// server.
static tref serve_init_line(const tgf_repl_evaluator& re,
	size_t max_line)
{
	trefs init;
	// A built-in grammar is compiled into the child, so its key is absent
	// and the child keeps it; null means the child starts with none.
	if (re.has_grammar() && !re.has_fixed_grammar())
		init.push_back(jt::build_pair(tp::grammar, jt::build_object({
			jt::build_pair(tp::name, jt::build_string(re.filename())),
			jt::build_pair(tp::source,
				jt::build_string(re.grammar_text())) })));
	else if (!re.has_grammar())
		init.push_back(jt::build_pair(tp::grammar, jt::_null()));
	init.push_back(jt::build_pair(tp::start, re.start_symbol().empty()
		? jt::_null() : jt::build_string(re.start_symbol())));
	init.push_back(jt::build_pair(tp::options, re.option_values()));
	init.push_back(jt::build_pair(tp::max_line,
		jt::build_number(static_cast<double>(max_line))));
	return jt::build_object({
		jt::build_pair(tp::init, jt::build_object(init)) });
}

// A Windows session is a second tgf process. Each pipe has an overlapped
// server end for the relay and a plain client end that the child inherits
// as one of its standard handles.
std::shared_ptr<tgf_child> tgf_serve_state::start_child(
	const std::string& id)
{
	struct pipe_pair { HANDLE parent = nullptr; HANDLE child = nullptr; };
	pipe_pair in, out, err;
	// Another process must neither guess the name nor open the pipe.
	const user_only_security security;
	const std::optional<std::string> nonce = new_session_id();
	if (!security.descriptor() || !nonce) return nullptr;
	const std::wstring base = L"\\\\.\\pipe\\tgf_"
		+ win_widen(*nonce) + L"_";
	auto make_pair = [&base, &security](const wchar_t* tag,
		bool parent_writes, pipe_pair& p) -> bool
	{
		std::wstring name = base + tag;
		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof sa;
		sa.lpSecurityDescriptor = security.descriptor();
		sa.bInheritHandle = FALSE;
		HANDLE server = ::CreateNamedPipeW(name.c_str(),
			(parent_writes ? PIPE_ACCESS_OUTBOUND
				: PIPE_ACCESS_INBOUND) | FILE_FLAG_OVERLAPPED
				| FILE_FLAG_FIRST_PIPE_INSTANCE,
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
	auto s = std::make_shared<tgf_child>(*io, this, id);
	s->process = pi.hProcess;
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
	open_session_log(*s);
	// The child reads the init line before its first hello, so the parent
	// writes it right after the start.
	std::string init_text = jt::get(serve_init_line(*opt.evaluator,
		opt.max_line)).to_str();
	s->log_write(line_log::text("in", init_text));
	s->send_to_child(std::move(init_text) + "\n");
	return s;
}

#endif // _WIN32

void tgf_serve_state::shutdown_session(const std::shared_ptr<tgf_child>& s,
	bool wait)
{
	if (!s || s->shutting_down) return;
	s->shutting_down = true;
	s->ended = true;
	if (s->relay) s->relay->close_after_flush();
	// Some Boost versions have no cancel(ec), and a timer cancel reports no error.
	s->end_timer.cancel();
	bool running = false;
#ifdef _WIN32
	if (s->process) {
		DWORD code = 0;
		if (::WaitForSingleObject(s->process, 0) == WAIT_OBJECT_0
				&& ::GetExitCodeProcess(s->process, &code)) {
			s->exit_code = static_cast<int>(code);
			s->have_exit_code = true;
			::CloseHandle(s->process);
			s->process = nullptr;
		} else if (wait) kill_child(*s);
		else {
			::TerminateProcess(s->process, 1);
			running = true;
		}
	}
#else
	// A pipe read can reach the end of the child output before SIGCHLD
	// arrives, so reap here too and keep the code for the log below.
	if (s->pid > 0 && !s->have_exit_code) {
		int status = 0;
		const pid_t p = ::waitpid(s->pid, &status, WNOHANG);
		if (p == s->pid) {
			s->exit_code = exit_code_of(status);
			s->have_exit_code = true;
			s->pid = -1;
		} else if (p == 0 && wait) kill_child(*s);
		else if (p == 0) {
			// The SIGCHLD reap writes the exit, and the end timer
			// stops a child that ignores SIGTERM.
			::kill(s->pid, SIGTERM);
			running = true;
		}
	}
#endif
	error_code ec;
	s->to_child.close(ec);
	s->from_child.close(ec);
	s->child_stderr.close(ec);
	sessions.erase(s->id);
	if (running) {
		reaping.push_back(s);
		s->start_end_timer();
		start_idle_timer();
		return;
	}
	finish_child(s);
}

void tgf_serve_state::finish_child(const std::shared_ptr<tgf_child>& s) {
	// The exit is written once, whichever of the pipe read or the reap
	// reaches the session first.
	if (s->have_exit_code && !s->child_exit_logged) {
		s->child_exit_logged = true;
		trefs ev = line_log::event_base(serve_child_exit_event);
		ev.push_back(jt::build_pair(tp::session,
			jt::build_string(s->id)));
		ev.push_back(jt::build_pair("exit", jt::build_number(
			static_cast<double>(s->exit_code))));
		log_event(jt::build_object(ev), s.get());
	}
	s->log.close();
	std::erase(reaping, s);
}

// end-session: the parent answers, closes the input of the child and keeps
// the entry until the child exits, so its exit code reaches the log.
void tgf_serve_state::end_session(const std::shared_ptr<tgf_child>& s,
	tref id)
{
	if (!s || s->ended) return;
	s->ended = true;
	tref resp = serve_quit_response(id);
	s->log_write(line_log::text("out", jt::get(resp).to_str()));
	if (tgf_session_relay* r = s->relay) {
		r->send(resp);
		r->close_after_flush();
	}
	log_event(line_log::session_event(tp::end_session, s->id), s.get());
	// Closing the write end hands the child an end of input, so its JSON
	// loop returns without a quit of its own.
	error_code ec;
	s->to_child.close(ec);
	// A child that does not exit within a short time is stopped.
	s->start_end_timer();
}

#ifndef _WIN32
void tgf_serve_state::on_sigchld() {
	reap_children();
	if (!sigchld) return;
	sigchld->async_wait([this](error_code e, int) {
		if (!e) on_sigchld();
	});
}
#endif

void tgf_serve_state::reap_children() {
#ifdef _WIN32
	// The idle tick polls each process handle, which is the only wait a
	// Windows child gives without a second thread.
	auto exited = [](const std::shared_ptr<tgf_child>& s) {
		return s->process && ::WaitForSingleObject(s->process, 0)
			== WAIT_OBJECT_0;
	};
	std::vector<std::shared_ptr<tgf_child>> dead;
	for (auto& kv : sessions) if (exited(kv.second)) dead.push_back(kv.second);
	// shutdown_session reads the exit code before it closes the handle.
	for (auto& s : dead) shutdown_session(s);
	for (auto& s : std::vector(reaping)) {
		if (!exited(s)) continue;
		DWORD code = 0;
		if (::GetExitCodeProcess(s->process, &code)) {
			s->exit_code = static_cast<int>(code);
			s->have_exit_code = true;
		}
		::CloseHandle(s->process);
		s->process = nullptr;
		s->end_timer.cancel();
		finish_child(s);
	}
#else
	for (;;) {
		int status = 0;
		pid_t p = ::waitpid(-1, &status, WNOHANG);
		if (p <= 0) break;
		std::shared_ptr<tgf_child> found;
		for (auto& kv : sessions)
			if (kv.second->pid == p) { found = kv.second; break; }
		for (auto& r : reaping)
			if (!found && r->pid == p) found = r;
		if (!found) continue;
		found->exit_code = exit_code_of(status);
		found->have_exit_code = true;
		// The child is reaped, so do not signal this pid again.
		found->pid = -1;
		if (found->shutting_down) {
			found->end_timer.cancel();
			finish_child(found);
		} else shutdown_session(found);
	}
#endif
}

void tgf_serve_state::start_idle_timer() {
	if (idle_tick_running || !idle_timer) return;
	idle_tick_running = true;
	idle_timer->expires_after(std::chrono::milliseconds(serve_tick_ms));
	idle_timer->async_wait([this](error_code ec) {
		idle_tick_running = false;
		if (ec) return;
#ifdef _WIN32
		reap_children();
#endif
		check_idle();
		// No session means no tick; the next session arms it again.
		if (!sessions.empty() || !reaping.empty()) start_idle_timer();
	});
}

void tgf_serve_state::check_idle() {
	if (opt.idle_timeout_minutes == 0) return;
	auto now = std::chrono::steady_clock::now();
	auto limit = std::chrono::minutes(opt.idle_timeout_minutes);
	std::vector<std::shared_ptr<tgf_child>> dead;
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
		log_event(line_log::session_event("idle-timeout", s->id), s.get());
		shutdown_session(s);
	}
}

void tgf_child::start_read() {
	auto self = shared_from_this();
	from_child.async_read_some(ba::buffer(read_buf),
		[self](error_code ec, size_t n) { self->on_read(ec, n); });
}

void tgf_child::on_read(error_code ec, size_t n) {
	if (ec) { st->shutdown_session(shared_from_this()); return; }
	if (ended) {
		// end-session closed the child input, but a pending answer may
		// still be in the pipe. Drain and discard it so the child never
		// blocks on a full pipe and the session leaves the table.
		feed_lines(line, dropping, st->opt.max_answer,
			read_buf.data(), n, [](std::string&) {}, [] {});
	} else {
		feed_lines(line, dropping, st->opt.max_answer,
			read_buf.data(), n,
			[this](std::string& l) { deliver(std::move(l)); },
			[this] { st->shutdown_session(shared_from_this()); });
	}
	// The answers are relayed and logged, so no JSON tref is left.
	jt::gc_if_grown();
	if (!shutting_down) start_read();
}

void tgf_child::start_stderr_read() {
	if (ended) return;
	auto self = shared_from_this();
	child_stderr.async_read_some(ba::buffer(err_buf),
		[self](error_code ec, size_t n) { self->on_stderr_read(ec, n); });
}

void tgf_child::on_stderr_read(error_code ec, size_t n) {
	if (ec) return;
	feed_lines(err_line, err_dropping, st->opt.max_stderr_line,
		err_buf.data(), n,
		[this](std::string& l) { log_write(line_log::text("stderr", l)); },
		[] {});
	if (!ended) start_stderr_read();
}

void tgf_child::deliver(std::string l) {
	last_activity = std::chrono::steady_clock::now();
	last_child_output = last_activity;
	if (open_requests > 0) --open_requests;
	update_read_state();
	bool close_after = false;
	if (quit_pending) {
		if (quit_wait > 0) --quit_wait;
		if (quit_wait == 0) { quit_pending = false; close_after = true; }
	}
	log_write(line_log::text("out", l));
	if (tgf_session_relay* r = relay) {
		r->send_text(l);
		if (close_after) r->close_after_flush();
		return;
	}
	// The client is gone: keep the line for the next attach.
	detached_bytes += l.size() + 1;
	detached.push_back(std::move(l));
	size_t dropped = 0;
	while (detached_bytes > st->opt.detached_buffer && !detached.empty()) {
		detached_bytes -= detached.front().size() + 1;
		detached.erase(detached.begin());
		++dropped;
	}
	if (dropped) {
		trefs ev = line_log::event_base("dropped-buffer-lines");
		ev.push_back(jt::build_pair(tp::session, jt::build_string(id)));
		ev.push_back(jt::build_pair("count", jt::build_number(
			static_cast<double>(dropped))));
		st->log_event(jt::build_object(ev), this);
	}
}

void tgf_child::attach(tgf_session_relay* r) {
	relay = r;
	if (read_paused && r->out) r->out->pause_read(true);
	last_activity = std::chrono::steady_clock::now();
	if (detached.empty()) return;
	for (auto& l : detached) r->send_text(l);
	detached.clear();
	detached_bytes = 0;
}

void tgf_child::send_to_child(std::string text) {
	if (ended) return;
	out_bytes += text.size();
	out.push_back(std::move(text));
	if (out.size() == 1) write_next();
	update_read_state();
}

// A client that sends faster than the child reads waits, so the queue of
// the child stays bounded.
void tgf_child::update_read_state() {
	const bool full = open_requests >= st->opt.max_open_requests
		|| out_bytes >= st->opt.max_line;
	if (full == read_paused) return;
	read_paused = full;
	if (relay && relay->out) relay->out->pause_read(full);
}

void tgf_child::write_next() {
	if (ended || out.empty()) return;
	auto self = shared_from_this();
	ba::async_write(to_child, ba::buffer(out.front()),
		[self](error_code ec, size_t) {
			if (ec) {
				self->st->shutdown_session(self);
				return;
			}
			self->out_bytes -= self->out.front().size();
			self->out.pop_front();
			self->update_read_state();
			self->write_next();
		});
}

// Stop a child that does not exit after end-session.
void tgf_child::start_end_timer() {
	end_timer.expires_after(
		std::chrono::seconds(st->opt.end_timeout_seconds));
	auto self = shared_from_this();
	end_timer.async_wait([self](error_code ec) {
		if (ec) return;
		if (!self->shutting_down) {
			self->st->shutdown_session(self);
			return;
		}
		// The child ignored the end: force it, and the reap writes the exit.
#ifdef _WIN32
		if (self->process) ::TerminateProcess(self->process, 1);
#else
		if (self->pid > 0) ::kill(self->pid, SIGKILL);
#endif
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
	tgf_serve_state state(opt);
	// with no home folder the user names a log folder or turns logs off
	if (state.report.has_error()) {
		res.report().append(std::move(state.report));
		return res;
	}
	line_server_options o;
	// the caller checks the range of the port
	o.listen = { { line_transport_kind::tcp, "",
		static_cast<uint16_t>(opt.port) } };
	// The relay counts the children, so a connection is no session.
	o.max_sessions = std::numeric_limits<size_t>::max();
	// The relay ends an idle child, also one with no client.
	o.idle_timeout_seconds = 0;
	o.max_line = opt.max_line;
	o.write_timeout_seconds = opt.write_timeout_seconds;
	o.first_line_timeout_seconds = opt.first_line_timeout_seconds;
	o.max_pending = opt.max_pending;
	o.log_dir = state.log_dir;
	o.json = opt.json;
	o.debug_sessions = opt.debug_sessions;
	o.handler_logs = true;
	o.make_handler = [&state](const line_session&) {
		state.bind_io();
		return std::make_unique<tgf_session_relay>(state);
	};
	o.on_stop = [&state] { state.stop(); };
#ifdef _WIN32
	// A JSON line ends with LF only, as format::json::write_line writes it.
	if (opt.json) ::_setmode(::_fileno(stdout), _O_BINARY);
#endif
	o.on_output = [](std::string_view line) {
		std::cout << line << std::endl;
	};
	auto run = line_server_run(o);
	const std::optional<int> value = run.has_value()
		? std::optional<int>(run.value()) : std::nullopt;
	res.report().append(std::move(run).report());
	res.report().append(std::move(state.report));
	if (value && !res.report().has_error()) res.emplace(*value);
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
