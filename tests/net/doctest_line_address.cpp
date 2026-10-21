// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "net/line_address.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

using namespace idni;

// The rules of a program "demo" with a default socket.
static connect_address_rules demo_rules(std::string socket = "/run/demo.sock")
{
	return { "demo", std::move(socket), "localhost", 4000 };
}

// The target of a text that must parse.
static connect_target parsed(const std::string& text,
	const connect_address_rules& rules = demo_rules())
{
	auto r = parse_connect_address(text, rules);
	REQUIRE(r.has_value());
	return r.value();
}

// The report text of a failed parse, without colors.
static std::string refused(const std::string& text,
	const connect_address_rules& rules = demo_rules())
{
	auto r = parse_connect_address(text, rules);
	REQUIRE_FALSE(r.has_value());
	std::ostringstream os;
	r.report().print(os, term::colors(false));
	return os.str();
}

static std::string home() {
	auto h = expand_home("~");
	REQUIRE(h.has_value());
	return h.value();
}

TEST_SUITE("line address: local socket") {
	TEST_CASE("an empty text gives the default socket") {
		auto t = parsed("");
		CHECK(t.kind == line_transport_kind::local_socket);
		CHECK(t.path == "/run/demo.sock");
	}
	TEST_CASE("an empty text with no default socket is an error") {
		CHECK(refused("", demo_rules("")).find("Invalid server address")
			!= std::string::npos);
	}
	TEST_CASE("an absolute path") {
		auto t = parsed("/tmp/x.sock");
		CHECK(t.kind == line_transport_kind::local_socket);
		CHECK(t.path == "/tmp/x.sock");
	}
	TEST_CASE("the tilde is the home folder") {
		CHECK_FALSE(home().empty());
		CHECK(home().front() != '~');
		CHECK(parsed("~/x.sock").path == home() + "/x.sock");
		CHECK(parsed("~").path == home());
		CHECK(parsed("demo+unix://~/y.sock").path == home() + "/y.sock");
		CHECK(refused("~bob/x.sock").find("~bob/x.sock")
			!= std::string::npos);
	}
	TEST_CASE("the unix scheme of the program") {
		auto t = parsed("demo+unix:///tmp/s");
		CHECK(t.kind == line_transport_kind::local_socket);
		CHECK(t.path == "/tmp/s");
		refused("demo+unix://");
		refused("other+unix:///tmp/s");
	}
}

#ifdef _WIN32
TEST_SUITE("line address: named pipe") {
	TEST_CASE("a pipe name with two backslashes is a local socket") {
		auto t = parsed("\\\\.\\pipe\\demo");
		CHECK(t.kind == line_transport_kind::local_socket);
		CHECK(t.path == "\\\\.\\pipe\\demo");
	}
}
#endif

TEST_SUITE("line address: tcp") {
	TEST_CASE("the host and port forms") {
		auto ok = [](const std::string& text, const std::string& host,
			uint16_t port)
		{
			auto t = parsed(text);
			CHECK(t.kind == line_transport_kind::tcp);
			CHECK(t.host == host);
			CHECK(t.port == port);
		};
		ok("demo://localhost:4101", "localhost", 4101);
		ok("demo://localhost:4101/", "localhost", 4101);
		ok("demo://localhost", "localhost", 4000);
		ok("localhost:4101", "localhost", 4101);
		ok("example.org", "example.org", 4000);
		ok("127.0.0.1:65535", "127.0.0.1", 65535);
		ok("[::1]:4101", "::1", 4101);
		ok("[::1]", "::1", 4000);
	}
	TEST_CASE("bad forms name the text") {
		for (const char* bad : { "localhost:0", "localhost:65536",
			"localhost:99999", "localhost:", "localhost:4x",
			"demo://localhost/x", "demo://user@localhost",
			"localhost?a", "localhost#a", "::1:4000", "[::1",
			"[]:4000", "[::1]x", "[::g]", "local host", "demo://",
			"demo:///", "ws://", "ws:///x" })
		{
			CHECK(refused(bad).find(bad) != std::string::npos);
		}
	}
	TEST_CASE("a scheme of another program is an error") {
		refused("tgf://localhost:4000");
		refused("http://localhost");
		refused("ftp://localhost");
	}
}

TEST_SUITE("line address: websocket") {
	TEST_CASE("the ws forms") {
		auto t = parsed("ws://localhost:8080/api/v1");
		CHECK(t.kind == line_transport_kind::websocket);
		CHECK(t.host == "localhost");
		CHECK(t.port == 8080);
		CHECK(t.ws_path == "/api/v1");
		t = parsed("ws://127.0.0.1");
		CHECK(t.kind == line_transport_kind::websocket);
		CHECK(t.port == 4000);
		CHECK(t.ws_path == "/");
		t = parsed("ws://[::1]:9/");
		CHECK(t.host == "::1");
		CHECK(t.port == 9);
		CHECK(t.ws_path == "/");
	}
	TEST_CASE("bad ws forms") {
		refused("ws://localhost:0/");
		refused("ws://localhost/a b");
		refused("ws://localhost/a#b");
		refused("ws://user@localhost/");
	}
}
