// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "net/line_client.h"

#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_HEADER_ONLY
#define BOOST_SYSTEM_HEADER_ONLY
#endif

#include <boost/asio.hpp>

using namespace idni;

namespace ba = boost::asio;
namespace json = idni::format::json;
using ba::ip::tcp;
using boost::system::error_code;

namespace {

// One accepted tcp connection, served by @p serve on a thread of its own.
struct tcp_peer {
	ba::io_context io;
	tcp::acceptor acceptor{ io,
		tcp::endpoint(ba::ip::make_address("127.0.0.1"), 0) };
	std::thread thread;

	~tcp_peer() { if (thread.joinable()) thread.join(); }

	template <typename Serve>
	void start(Serve serve) {
		thread = std::thread([this, serve]() mutable {
			error_code ec;
			auto sock = acceptor.accept(ec);
			if (!ec) serve(sock);
		});
	}
	uint16_t port() const { return acceptor.local_endpoint().port(); }
};

connect_target tcp_target(uint16_t port) {
	return { .kind = line_transport_kind::tcp, .host = "127.0.0.1",
		.port = port };
}

// The first line of the client, without its line end.
std::string peer_read(tcp::socket& sock) {
	ba::streambuf buf;
	error_code ec;
	size_t n = ba::read_until(sock, buf, '\n', ec);
	if (ec) return "<error>";
	return std::string(ba::buffers_begin(buf.data()),
		ba::buffers_begin(buf.data()) + static_cast<std::ptrdiff_t>(n - 1));
}

void peer_write(tcp::socket& sock, std::string_view text) {
	error_code ec;
	ba::write(sock, ba::buffer(text.data(), text.size()), ec);
}

std::string printed(const diagnostics::report& rep) {
	std::ostringstream os;
	rep.print(os);
	return os.str();
}

} // namespace

TEST_SUITE("line client: open") {
	TEST_CASE("the first line holds cmd and the open fields") {
		tcp_peer p;
		std::string got;
		p.start([&got](tcp::socket& s) {
			got = peer_read(s);
			peer_write(s, "{\"hello\":true}\n");
		});
		connect_options opt;
		opt.target = tcp_target(p.port());
		opt.open_fields = json::tree::geth(json::tree::build_object({
			json::tree::build_pair("session",
				json::tree::build_string("abc")) }));
		htref hello;
		auto r = line_client_open(opt, "new", hello);
		REQUIRE(r.has_value());
		REQUIRE(hello);
		CHECK(json::tree::get(hello).to_str() == "{\"hello\":true}");
		r.value()->close();
		p.thread.join();
		CHECK(got == "{\"cmd\":\"new\",\"session\":\"abc\"}");
	}
	TEST_CASE("a server that closes without an answer gives the lost connection") {
		tcp_peer p;
		p.start([](tcp::socket& s) {
			peer_read(s);
			error_code ec;
			s.close(ec);
		});
		connect_options opt;
		opt.target = tcp_target(p.port());
		htref hello;
		auto r = line_client_open(opt, "new", hello);
		CHECK_FALSE(r.has_value());
		CHECK(printed(r.report()).find(
			parser_strings::messages::net_connection_lost)
			!= std::string::npos);
		CHECK_FALSE(hello);
	}
	TEST_CASE("an answer that is no JSON gives a parse error") {
		tcp_peer p;
		p.start([](tcp::socket& s) {
			peer_read(s);
			peer_write(s, "not json\n");
		});
		connect_options opt;
		opt.target = tcp_target(p.port());
		htref hello;
		auto r = line_client_open(opt, "new", hello);
		CHECK_FALSE(r.has_value());
		CHECK(r.report().has_error());
		CHECK_FALSE(hello);
	}
	TEST_CASE("a target with no server gives the connect error") {
		uint16_t port = 0;
		{
			tcp_peer p;
			port = p.port();
		}
		connect_options opt;
		opt.target = tcp_target(port);
		htref hello;
		auto r = line_client_open(opt, "new", hello);
		CHECK_FALSE(r.has_value());
		CHECK(printed(r.report()).find(
			parser_strings::messages::connect_server_failed)
			!= std::string::npos);
	}
	TEST_CASE("a long hello passes, a later line over opt.limits is too_long") {
		tcp_peer p;
		const std::string big(4096, 'x');
		p.start([&big](tcp::socket& s) {
			peer_read(s);
			peer_write(s, "{\"hello\":\"" + big + "\"}\n"
				"{\"a\":1}\n{\"b\":\"" + big + "\"}\n{\"c\":2}\n");
			peer_read(s);
		});
		connect_options opt;
		opt.target = tcp_target(p.port());
		opt.limits.max_line = 64;
		htref hello;
		auto r = line_client_open(opt, "new", hello);
		REQUIRE(r.has_value());
		auto& link = r.value();
		std::string l;
		CHECK(link->read_line(l) == read_status::ok);
		CHECK(l == "{\"a\":1}");
		CHECK(link->read_line(l) == read_status::too_long);
		link->close();
	}
	TEST_CASE("a hello that never comes ends at the connect timeout") {
		tcp_peer p;
		p.start([](tcp::socket& s) {
			peer_read(s);
			peer_read(s);
		});
		connect_options opt;
		opt.target = tcp_target(p.port());
		opt.limits.connect_timeout_seconds = 1;
		htref hello;
		const auto t0 = std::chrono::steady_clock::now();
		auto r = line_client_open(opt, "new", hello);
		CHECK_FALSE(r.has_value());
		CHECK(printed(r.report()).find(
			parser_strings::messages::net_read_timeout)
			!= std::string::npos);
		CHECK(std::chrono::steady_clock::now() - t0
			< std::chrono::seconds(10));
	}
	TEST_CASE("a connect that gets no answer ends at the connect timeout") {
		connect_options opt;
		// TEST-NET-1 is never routed, so the connect waits or fails
		opt.target = { .kind = line_transport_kind::tcp,
			.host = "192.0.2.1", .port = 9 };
		opt.limits.connect_timeout_seconds = 1;
		htref hello;
		const auto t0 = std::chrono::steady_clock::now();
		auto r = line_client_open(opt, "new", hello);
		CHECK_FALSE(r.has_value());
		CHECK(std::chrono::steady_clock::now() - t0
			< std::chrono::seconds(10));
	}
}
