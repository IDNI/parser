// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "net/line_transport.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_HEADER_ONLY
#define BOOST_SYSTEM_HEADER_ONLY
#endif

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

using namespace idni;

namespace ba = boost::asio;
namespace beast = boost::beast;
using ba::ip::tcp;
using boost::system::error_code;

// One accepted connection, served by @p serve on a thread of its own.
template <typename Acceptor>
struct one_peer {
	ba::io_context io;
	Acceptor acceptor;
	std::thread thread;

	template <typename Endpoint>
	explicit one_peer(const Endpoint& ep) : acceptor(io, ep) {}
	~one_peer() { if (thread.joinable()) thread.join(); }

	template <typename Serve>
	void start(Serve serve) {
		thread = std::thread([this, serve]() mutable {
			error_code ec;
			auto sock = acceptor.accept(ec);
			if (!ec) serve(sock);
		});
	}
};

using tcp_peer = one_peer<tcp::acceptor>;

static tcp_peer* make_tcp_peer() {
	return new tcp_peer(tcp::endpoint(ba::ip::make_address("127.0.0.1"), 0));
}

static connect_target tcp_target(uint16_t port) {
	return { .kind = line_transport_kind::tcp, .host = "127.0.0.1",
		.port = port };
}

// The next line that @p sock sends, without its line end.
template <typename Socket>
static std::string peer_read(Socket& sock, ba::streambuf& buf) {
	error_code ec;
	size_t n = ba::read_until(sock, buf, '\n', ec);
	if (ec) return "<error>";
	std::string line(ba::buffers_begin(buf.data()),
		ba::buffers_begin(buf.data()) + static_cast<std::ptrdiff_t>(n - 1));
	buf.consume(n);
	return line;
}

template <typename Socket>
static void peer_write(Socket& sock, std::string_view text) {
	error_code ec;
	ba::write(sock, ba::buffer(text.data(), text.size()), ec);
}

TEST_SUITE("line transport: tcp") {
	TEST_CASE("one line each way and a second line") {
		std::unique_ptr<tcp_peer> p(make_tcp_peer());
		std::string got, second;
		p->start([&got, &second](tcp::socket& s) {
			ba::streambuf buf;
			got = peer_read(s, buf);
			peer_write(s, "world\r\n");
			second = peer_read(s, buf);
		});
		auto t = line_connect(tcp_target(p->acceptor.local_endpoint().port()));
		REQUIRE(t.has_value());
		auto& link = *t.value();
		CHECK(link.send_line("hello"));
		std::string line;
		CHECK(link.read_line(line) == read_status::ok);
		CHECK(line == "world");
		CHECK(link.send_line("s3cret token"));
		link.close();
		p.reset();
		CHECK(got == "hello");
		CHECK(second == "s3cret token");
	}
	TEST_CASE("a line over the limit gives too_long, the next line reads") {
		std::unique_ptr<tcp_peer> p(make_tcp_peer());
		p->start([](tcp::socket& s) {
			peer_write(s, "0123456789abcdef0123\nshort\n12345678\n");
		});
		auto t = line_connect(tcp_target(p->acceptor.local_endpoint().port()),
			{ .max_line = 8 });
		REQUIRE(t.has_value());
		auto& link = *t.value();
		std::string line;
		CHECK(link.read_line(line) == read_status::too_long);
		CHECK(link.read_line(line) == read_status::ok);
		CHECK(line == "short");
		CHECK(link.read_line(line) == read_status::ok);
		CHECK(line == "12345678");
	}
	TEST_CASE("a closed peer gives closed") {
		std::unique_ptr<tcp_peer> p(make_tcp_peer());
		p->start([](tcp::socket& s) {
			error_code ec;
			s.close(ec);
		});
		auto t = line_connect(tcp_target(p->acceptor.local_endpoint().port()));
		REQUIRE(t.has_value());
		p.reset();
		std::string line;
		CHECK(t.value()->read_line(line) == read_status::closed);
		CHECK_FALSE(t.value()->send_line("late"));
	}
	TEST_CASE("a read past the timeout gives timeout") {
		std::unique_ptr<tcp_peer> p(make_tcp_peer());
		bool release = false;
		std::mutex m;
		std::condition_variable cv;
		p->start([&](tcp::socket&) {
			std::unique_lock lock(m);
			cv.wait(lock, [&release] { return release; });
		});
		auto t = line_connect(tcp_target(p->acceptor.local_endpoint().port()),
			{ .timeout_seconds = 1 });
		REQUIRE(t.has_value());
		const auto start = std::chrono::steady_clock::now();
		std::string line;
		CHECK(t.value()->read_line(line) == read_status::timeout);
		CHECK(std::chrono::steady_clock::now() - start
			< std::chrono::seconds(10));
		{
			std::lock_guard lock(m);
			release = true;
		}
		cv.notify_one();
	}
	TEST_CASE("no server gives an error") {
		uint16_t port = 0;
		{
			std::unique_ptr<tcp_peer> p(make_tcp_peer());
			port = p->acceptor.local_endpoint().port();
		}
		auto t = line_connect(tcp_target(port));
		CHECK_FALSE(t.has_value());
	}
}

#ifndef _WIN32
TEST_SUITE("line transport: local socket") {
	using local = ba::local::stream_protocol;

	static std::string socket_path() {
		return (std::filesystem::temp_directory_path()
			/ ("line_transport_" + std::to_string(::getpid()) + ".sock"))
			.string();
	}

	TEST_CASE("one line each way and a second line") {
		const std::string path = socket_path();
		std::filesystem::remove(path);
		std::string got, second;
		{
			one_peer<local::acceptor> p{ local::endpoint(path) };
			p.start([&got, &second](local::socket& s) {
				ba::streambuf buf;
				got = peer_read(s, buf);
				peer_write(s, "world\n");
				second = peer_read(s, buf);
			});
			auto t = line_connect({ .kind = line_transport_kind::local_socket,
				.path = path });
			REQUIRE(t.has_value());
			auto& link = *t.value();
			CHECK(link.send_line("hello"));
			std::string line;
			CHECK(link.read_line(line) == read_status::ok);
			CHECK(line == "world");
			CHECK(link.send_line("pass"));
			link.close();
		}
		std::filesystem::remove(path);
		CHECK(got == "hello");
		CHECK(second == "pass");
	}
	static connect_target local_target(const std::string& path) {
		return { .kind = line_transport_kind::local_socket, .path = path };
	}

	TEST_CASE("a line over the limit gives too_long, the next line reads") {
		const std::string path = socket_path();
		std::filesystem::remove(path);
		{
			one_peer<local::acceptor> p{ local::endpoint(path) };
			p.start([](local::socket& s) {
				peer_write(s, "0123456789abcdef0123\nshort\n");
			});
			auto t = line_connect(local_target(path), { .max_line = 8 });
			REQUIRE(t.has_value());
			std::string line;
			CHECK(t.value()->read_line(line) == read_status::too_long);
			CHECK(t.value()->read_line(line) == read_status::ok);
			CHECK(line == "short");
		}
		std::filesystem::remove(path);
	}
	TEST_CASE("a closed peer gives closed") {
		const std::string path = socket_path();
		std::filesystem::remove(path);
		{
			one_peer<local::acceptor> p{ local::endpoint(path) };
			p.start([](local::socket& s) {
				error_code ec;
				s.close(ec);
			});
			auto t = line_connect(local_target(path));
			REQUIRE(t.has_value());
			std::string line;
			CHECK(t.value()->read_line(line) == read_status::closed);
		}
		std::filesystem::remove(path);
	}
	TEST_CASE("a read past the timeout gives timeout") {
		const std::string path = socket_path();
		std::filesystem::remove(path);
		{
			bool release = false;
			std::mutex m;
			std::condition_variable cv;
			one_peer<local::acceptor> p{ local::endpoint(path) };
			p.start([&](local::socket&) {
				std::unique_lock lock(m);
				cv.wait(lock, [&release] { return release; });
			});
			auto t = line_connect(local_target(path),
				{ .timeout_seconds = 1 });
			REQUIRE(t.has_value());
			std::string line;
			CHECK(t.value()->read_line(line) == read_status::timeout);
			{
				std::lock_guard lock(m);
				release = true;
			}
			cv.notify_one();
		}
		std::filesystem::remove(path);
	}
	TEST_CASE("a path over the system limit names the path") {
		const std::string path = "/tmp/" + std::string(200, 'x');
		auto t = line_connect({ .kind = line_transport_kind::local_socket,
			.path = path });
		REQUIRE_FALSE(t.has_value());
		std::ostringstream os;
		t.report().print(os, term::colors(false));
		CHECK(os.str().find("over the system limit") != std::string::npos);
		CHECK(os.str().find(path) != std::string::npos);
	}
}
#endif

TEST_SUITE("line transport: websocket") {
	TEST_CASE("an echo server round trips two lines") {
		std::unique_ptr<tcp_peer> p(make_tcp_peer());
		p->start([](tcp::socket& s) {
			beast::websocket::stream<tcp::socket> ws(std::move(s));
			error_code ec;
			ws.accept(ec);
			if (ec) return;
			for (;;) {
				beast::flat_buffer buf;
				ws.read(buf, ec);
				if (ec) return;
				ws.text(ws.got_text());
				ws.write(buf.data(), ec);
				if (ec) return;
			}
		});
		auto t = line_connect({ .kind = line_transport_kind::websocket,
			.host = "127.0.0.1", .port = p->acceptor.local_endpoint().port(),
			.ws_path = "/" });
		REQUIRE(t.has_value());
		auto& link = *t.value();
		std::string line;
		CHECK(link.send_line("{\"cmd\":\"ping\"}"));
		CHECK(link.read_line(line) == read_status::ok);
		CHECK(line == "{\"cmd\":\"ping\"}");
		CHECK(link.send_line("tok"));
		CHECK(link.read_line(line) == read_status::ok);
		CHECK(line == "tok");
		link.close();
	}
	TEST_CASE("a close frame gives closed") {
		std::unique_ptr<tcp_peer> p(make_tcp_peer());
		p->start([](tcp::socket& s) {
			beast::websocket::stream<tcp::socket> ws(std::move(s));
			error_code ec;
			ws.accept(ec);
			if (!ec) ws.close(beast::websocket::close_code::normal, ec);
		});
		auto t = line_connect({ .kind = line_transport_kind::websocket,
			.host = "127.0.0.1", .port = p->acceptor.local_endpoint().port(),
			.ws_path = "/" });
		REQUIRE(t.has_value());
		std::string line;
		CHECK(t.value()->read_line(line) == read_status::closed);
	}
	TEST_CASE("a read past the timeout gives timeout") {
		std::unique_ptr<tcp_peer> p(make_tcp_peer());
		bool release = false;
		std::mutex m;
		std::condition_variable cv;
		p->start([&](tcp::socket& s) {
			beast::websocket::stream<tcp::socket> ws(std::move(s));
			error_code ec;
			ws.accept(ec);
			std::unique_lock lock(m);
			cv.wait(lock, [&release] { return release; });
		});
		auto t = line_connect({ .kind = line_transport_kind::websocket,
			.host = "127.0.0.1", .port = p->acceptor.local_endpoint().port(),
			.ws_path = "/" }, { .timeout_seconds = 1 });
		REQUIRE(t.has_value());
		std::string line;
		CHECK(t.value()->read_line(line) == read_status::timeout);
		{
			std::lock_guard lock(m);
			release = true;
		}
		cv.notify_one();
	}
	TEST_CASE("a message over the limit gives too_long") {
		std::unique_ptr<tcp_peer> p(make_tcp_peer());
		p->start([](tcp::socket& s) {
			beast::websocket::stream<tcp::socket> ws(std::move(s));
			error_code ec;
			ws.accept(ec);
			if (ec) return;
			ws.text(true);
			ws.write(ba::buffer(std::string("0123456789abcdef")), ec);
			// read on, so the close of the client gets its answer
			beast::flat_buffer buf;
			while (!ec) ws.read(buf, ec);
		});
		auto t = line_connect({ .kind = line_transport_kind::websocket,
			.host = "127.0.0.1", .port = p->acceptor.local_endpoint().port(),
			.ws_path = "/" }, { .max_line = 8 });
		REQUIRE(t.has_value());
		std::string line;
		CHECK(t.value()->read_line(line) == read_status::too_long);
	}
}
