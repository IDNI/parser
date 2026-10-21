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

#include "line_transport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

// The dep script ships headers only, so Boost.System must stay header-only.
#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_HEADER_ONLY
#define BOOST_SYSTEM_HEADER_ONLY
#endif

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/un.h>
#endif

#include "parser_strings.h"

namespace idni {

namespace {

namespace ba = boost::asio;
namespace beast = boost::beast;
using ba::ip::tcp;
using boost::system::error_code;

using line_streambuf = ba::streambuf;
using message_buffer = beast::flat_buffer;

void drop_trailing_cr(std::string& line) {
	if (!line.empty() && line.back() == '\r') line.pop_back();
}

// Asio retries a blocking read that a socket timeout ends, so each
// operation runs asynchronously and the run of the context is bounded.
template <typename Start, typename Cancel>
error_code run_bounded(ba::io_context& io, size_t seconds, Start start,
	Cancel cancel, size_t& n)
{
	std::optional<error_code> done;
	start([&done, &n](error_code ec, size_t bytes) {
		done = ec;
		n = bytes;
	});
	io.restart();
	if (seconds == 0) io.run();
	else io.run_for(std::chrono::seconds(seconds));
	if (done) return *done;
	cancel();
	io.restart();
	io.run();
	return ba::error::timed_out;
}

// tcp, a Unix domain socket or a Windows named pipe.
template <typename Socket>
struct stream_line_transport final : line_transport {
	ba::io_context io;
	Socket sock{ io };
	line_limits limits;
	// a streambuf cannot move, so a new limit makes a new one
	std::unique_ptr<line_streambuf> buf;

	explicit stream_line_transport(const line_limits& l)
		: limits(l), buf(std::make_unique<line_streambuf>(buffer_max(l))) {}

	// one byte more for the line end; 0 is no limit
	static size_t buffer_max(const line_limits& l) {
		return l.max_line == 0
			? std::numeric_limits<size_t>::max() : l.max_line + 1;
	}

	void set_limits(const line_limits& l) override {
		limits = l;
		if (!buf) return;
		const size_t n = buf->size();
		// read_line checks a buffered line against the new limit itself
		auto next = std::make_unique<line_streambuf>(
			std::max(buffer_max(l), n));
		ba::buffer_copy(next->prepare(n), buf->data());
		next->commit(n);
		buf = std::move(next);
	}
	~stream_line_transport() override { close(); }

	void cancel() {
		error_code ignored;
		sock.cancel(ignored);
	}

	bool write(const std::array<ba::const_buffer, 2>& parts) {
		if (!sock.is_open()) return false;
		size_t n = 0;
		const auto ec = run_bounded(io, limits.timeout_seconds,
			[this, &parts](auto done) {
				ba::async_write(sock, parts, done);
			},
			[this] { cancel(); }, n);
		if (ec) close();
		return !ec;
	}

	bool send_line(const std::string& line) override {
		return write({ ba::buffer(line), ba::buffer("\n", 1) });
	}

	read_status read_line(std::string& line) override {
		line.clear();
		if (!sock.is_open() || !buf) return read_status::closed;
		// The tail of a line over the limit is dropped up to its line
		// end, so the next line still reads.
		bool dropping = false;
		for (;;) {
			size_t n = 0;
			const auto ec = run_bounded(io, limits.timeout_seconds,
				[this](auto done) {
					ba::async_read_until(sock, *buf, '\n', done);
				},
				[this] { cancel(); }, n);
			const auto begin = ba::buffers_begin(buf->data());
			// a line read before set_limits can be longer than the limit
			if (!ec && limits.max_line != 0 && n - 1 > limits.max_line)
				dropping = true;
			if (!ec) {
				if (!dropping) line.assign(begin, begin
					+ static_cast<std::ptrdiff_t>(n - 1));
				buf->consume(n);
				if (dropping) return read_status::too_long;
				drop_trailing_cr(line);
				return read_status::ok;
			}
			if (ec == ba::error::not_found) {
				dropping = true;
				buf->consume(buf->size());
				continue;
			}
			// the last line of a closed peer can lack its line end
			if (ec == ba::error::eof && !dropping && buf->size() > 0) {
				line.assign(begin, begin + static_cast<std::ptrdiff_t>(
					buf->size()));
				buf->consume(buf->size());
				drop_trailing_cr(line);
				return read_status::ok;
			}
			close();
			if (ec == ba::error::timed_out) return read_status::timeout;
			// a peer that closes in the middle of a long line ends it
			return dropping ? read_status::too_long : read_status::closed;
		}
	}

	void close() override {
		if (sock.is_open()) {
			error_code ignored;
			if constexpr (requires { sock.shutdown(
					Socket::shutdown_both, ignored); })
				sock.shutdown(Socket::shutdown_both, ignored);
			sock.close(ignored);
		}
		buf.reset();
	}
};

// One text message is one line.
struct websocket_line_transport final : line_transport {
	ba::io_context io;
	beast::websocket::stream<tcp::socket> ws{ io };
	line_limits limits;
	std::optional<message_buffer> buf;

	explicit websocket_line_transport(const line_limits& l) : limits(l) {
		buf.emplace();
		ws.read_message_max(l.max_line);
	}

	void set_limits(const line_limits& l) override {
		limits = l;
		ws.read_message_max(l.max_line);
	}
	~websocket_line_transport() override { close(); }

	void cancel() {
		error_code ignored;
		beast::get_lowest_layer(ws).cancel(ignored);
	}

	bool handshake(const std::string& host, const std::string& path) {
		size_t n = 0;
		const auto ec = run_bounded(io, limits.connect_timeout_seconds,
			[this, &host, &path](auto done) {
				ws.async_handshake(host, path,
					[done](error_code e) mutable { done(e, 0); });
			},
			[this] { cancel(); }, n);
		if (ec) return false;
		ws.text(true);
		return true;
	}

	bool write(ba::const_buffer message) {
		if (!ws.is_open()) return false;
		size_t n = 0;
		const auto ec = run_bounded(io, limits.timeout_seconds,
			[this, message](auto done) {
				ws.async_write(message, done);
			},
			[this] { cancel(); }, n);
		if (ec) close();
		return !ec;
	}

	bool send_line(const std::string& line) override {
		return write(ba::buffer(line));
	}

	read_status read_line(std::string& line) override {
		line.clear();
		if (!ws.is_open() || !buf) return read_status::closed;
		size_t n = 0;
		const auto ec = run_bounded(io, limits.timeout_seconds,
			[this](auto done) { ws.async_read(*buf, done); },
			[this] { cancel(); }, n);
		if (ec) {
			close();
			if (ec == ba::error::timed_out) return read_status::timeout;
			if (ec == beast::websocket::error::message_too_big)
				return read_status::too_long;
			// a close frame, an end of file or a lost connection
			return read_status::closed;
		}
		const auto data = buf->data();
		line.assign(static_cast<const char*>(data.data()), data.size());
		buf->consume(buf->size());
		drop_trailing_cr(line);
		return read_status::ok;
	}

	void close() override {
		if (ws.is_open()) {
			size_t n = 0;
			// a peer that never answers the close frame cannot hold it
			run_bounded(io, limits.close_timeout_seconds,
				[this](auto done) {
					ws.async_close(beast::websocket::close_code::normal,
						[done](error_code e) mutable { done(e, 0); });
				},
				[this] { cancel(); }, n);
		}
		auto& sock = beast::get_lowest_layer(ws);
		if (sock.is_open()) {
			error_code ignored;
			sock.close(ignored);
		}
		buf.reset();
	}
};

using line_result = diagnostics::result<std::unique_ptr<line_transport>>;

// Resolve the host of @p target and connect @p sock to the first of its
// addresses that takes the connection, within @p seconds.
bool tcp_open(ba::io_context& io, tcp::socket& sock,
	const connect_target& target, size_t seconds, line_result& res)
{
	const std::string& host = target.host;
#ifndef AI_IDN
	for (const char c : host)
		if (static_cast<unsigned char>(c) >= 0x80) {
			res.error(diagnostics::code::invalid_argument,
				parser_strings::messages::connect_idn_unsupported,
				{ { parser_strings::label::value, host } });
			return false;
		}
#endif
	tcp::resolver resolver{ io };
#ifdef AI_IDN
	// The resolver turns a non-ASCII host name into its ASCII form.
	auto flags = static_cast<tcp::resolver::flags>(AI_IDN);
#else
	auto flags = tcp::resolver::flags();
#endif
	bool resolved = false;
	size_t n = 0;
	// localhost can give ::1 first while the server binds 127.0.0.1 only,
	// so every address is tried in turn.
	const error_code ec = run_bounded(io, seconds,
		[&](auto done) {
			resolver.async_resolve(host, std::to_string(target.port),
				flags, [&, done](error_code e,
					tcp::resolver::results_type eps) mutable
				{
					if (e) { done(e, 0); return; }
					resolved = true;
					ba::async_connect(sock, eps,
						[done](error_code c, const tcp::endpoint&)
							mutable { done(c, 0); });
				});
		},
		[&] {
			resolver.cancel();
			error_code ignored;
			sock.cancel(ignored);
		}, n);
	if (ec && !resolved) {
		res.error(diagnostics::code::io_error,
			parser_strings::messages::connect_resolve_failed,
			{ { parser_strings::label::value, host } });
		return false;
	}
	if (ec) {
		std::string where = host.find(':') == std::string::npos
			? host : "[" + host + "]";
		res.error(diagnostics::code::io_error,
			parser_strings::messages::connect_server_failed,
			{ { parser_strings::label::value,
				where + ":" + std::to_string(target.port) } });
		return false;
	}
	return true;
}

line_result local_connect(const std::string& path, const line_limits& limits)
{
	line_result res;
	auto failed = [&res, &path](std::string_view msg) {
		res.error(diagnostics::code::io_error, msg,
			{ { parser_strings::label::value, path } });
		return std::move(res);
	};
#ifdef _WIN32
	// the system limit of a pipe name
	if (path.size() > 256)
		return failed(parser_strings::messages::net_path_too_long);
	const int size = MultiByteToWideChar(CP_UTF8, 0, path.data(),
		static_cast<int>(path.size()), nullptr, 0);
	if (size == 0)
		return failed(parser_strings::messages::connect_server_failed);
	std::wstring name(static_cast<size_t>(size), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, path.data(),
		static_cast<int>(path.size()), name.data(), size);
	// Asio waits on a pipe handle only when it is opened overlapped.
	HANDLE h = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
		nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
	if (h == INVALID_HANDLE_VALUE)
		return failed(parser_strings::messages::connect_server_failed);
	auto t = std::make_unique<stream_line_transport<
		ba::windows::stream_handle>>(limits);
	error_code ec;
	t->sock.assign(h, ec);
	if (ec) {
		CloseHandle(h);
		return failed(parser_strings::messages::connect_server_failed);
	}
#else
	// The endpoint of Asio throws on a path over the limit, so check first.
	if (path.empty() || path.size() >= sizeof(sockaddr_un{}.sun_path))
		return failed(parser_strings::messages::net_path_too_long);
	auto t = std::make_unique<stream_line_transport<
		ba::local::stream_protocol::socket>>(limits);
	error_code ec;
	t->sock.connect(ba::local::stream_protocol::endpoint(path), ec);
	if (ec) return failed(parser_strings::messages::connect_server_failed);
#endif
	return res.with_value(std::move(t));
}

} // namespace

diagnostics::result<std::unique_ptr<line_transport>> line_connect(
	const connect_target& target, const line_limits& limits)
{
	switch (target.kind) {
	case line_transport_kind::local_socket:
		return local_connect(target.path, limits);
	case line_transport_kind::tcp: {
		line_result res;
		auto t = std::make_unique<stream_line_transport<tcp::socket>>(
			limits);
		if (!tcp_open(t->io, t->sock, target,
				limits.connect_timeout_seconds, res))
			return res;
		return res.with_value(std::move(t));
	}
	case line_transport_kind::websocket: {
		line_result res;
		auto t = std::make_unique<websocket_line_transport>(limits);
		if (!tcp_open(t->io, t->ws.next_layer(), target,
				limits.connect_timeout_seconds, res))
			return res;
		std::string host = target.host.find(':') == std::string::npos
			? target.host : "[" + target.host + "]";
		host += ":" + std::to_string(target.port);
		if (!t->handshake(host, target.ws_path)) {
			res.error(diagnostics::code::io_error,
				parser_strings::messages::connect_server_failed,
				{ { parser_strings::label::value,
					"ws://" + host + target.ws_path } });
			return res;
		}
		return res.with_value(std::move(t));
	}
	}
	line_result res;
	res.error(diagnostics::code::invalid_argument,
		parser_strings::messages::net_bad_address);
	return res;
}

} // namespace idni
