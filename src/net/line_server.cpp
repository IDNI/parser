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
#include "line_support.h"

#include <atomic>
#include <csignal>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
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

#include "parser_strings.h"

namespace idni {

namespace {

namespace ba = boost::asio;
using ba::ip::tcp;
using boost::system::error_code;
using jt = format::json::tree;

std::string_view kind_name(line_transport_kind k) {
	switch (k) {
	case line_transport_kind::local_socket: return "local_socket";
	case line_transport_kind::tcp: return "tcp";
	case line_transport_kind::websocket: return "websocket";
	}
	return "";
}

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

	void output(const std::string& line) const;
	void print_start();

	line_server_options opt;
	ba::io_context io;
	// one bound address for each entry of opt.listen, in its order
	std::vector<listen_address> bound;
	// tcp and websocket
	std::vector<tcp_listener> tcp_listeners;
	ba::signal_set signals;
	diagnostics::report report;
	bool stopping = false;
};

std::mutex running_mutex;
std::atomic<line_server*> running{ nullptr };

line_server::line_server(const line_server_options& o)
	: opt(o), io(), signals(io)
{
}

void line_server::output(const std::string& line) const {
	if (opt.on_output) opt.on_output(line);
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

bool line_server::start() {
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
	error_code ec;
	for (auto& l : tcp_listeners) l.acceptor->close(ec);
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

} // namespace idni
