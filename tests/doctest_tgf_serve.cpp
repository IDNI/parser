// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Loopback tests for the tgf TCP server. The fixture starts the tgf binary
// as a process on every platform and reads its listening line; the test
// then connects to the port with plain client sockets. No test forks.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "serve/tgf_serve_test_fixture.h"
#include "../src/tgf/tgf_cli.h"
#include "../src/tgf/tgf_serve.h"
#include "format/json/json.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// The dep script ships headers only, so Boost.System must stay header-only.
#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_HEADER_ONLY
#define BOOST_SYSTEM_HEADER_ONLY
#endif

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace idni;
using namespace idni::format;
using tgf_serve_test::child_process;
using tgf_serve_test::grammar_path;
using tgf_serve_test::member;
using tgf_serve_test::parse_line;
using tgf_serve_test::report_contains_key;
using tgf_serve_test::serve_fixture;
using tgf_serve_test::tgf_binary_path;

namespace ba = boost::asio;
using ba::ip::tcp;
using boost::system::error_code;
using jt = json::tree;

// One TCP client with one line at a time. Every read has a deadline, so a
// server that stops answering fails the test instead of hanging it.
struct serve_client {
	explicit serve_client(uint16_t port) {
		error_code ec;
		sock.connect(tcp::endpoint(
			ba::ip::make_address("127.0.0.1"), port), ec);
		REQUIRE_FALSE(ec);
		sock.non_blocking(true, ec);
		REQUIRE_FALSE(ec);
	}

	void send(const std::string& line) {
		send_raw(line + "\n");
	}

	// Write the exact bytes in one stream of writes, so the server can
	// read several pipelined lines in one read.
	void send_raw(const std::string& bytes) {
		size_t off = 0;
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::milliseconds(10000);
		while (off != bytes.size()) {
			error_code ec;
			size_t n = sock.write_some(ba::buffer(
				bytes.data() + off, bytes.size() - off), ec);
			if (!ec) { off += n; continue; }
			if (ec == ba::error::would_block
					|| ec == ba::error::try_again) {
				if (std::chrono::steady_clock::now() >= deadline)
					break;
				std::this_thread::sleep_for(
					std::chrono::milliseconds(2));
				continue;
			}
			break;
		}
		REQUIRE(off == bytes.size());
	}

	// One line, or "" when the server sends nothing before the deadline.
	std::string read_line(int timeout_ms = 10000) {
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::milliseconds(timeout_ms);
		while (in_buf.find('\n') == std::string::npos)
			if (!read_until(deadline)) return {};
		size_t nl = in_buf.find('\n');
		std::string line = in_buf.substr(0, nl);
		in_buf.erase(0, nl + 1);
		if (!line.empty() && line.back() == '\r') line.pop_back();
		return line;
	}

	// True when the peer closed the connection. A deadline that passes
	// with no byte means the connection is still open.
	bool at_eof(int timeout_ms = 2000) {
		if (!in_buf.empty()) return false;
		if (eof_seen) return true;
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::milliseconds(timeout_ms);
		if (read_until(deadline)) return false;
		return eof_seen;
	}

	// Read and discard every byte until the peer closes, or the deadline
	// passes. False means the peer still holds the connection open.
	bool drain_to_eof(int timeout_ms = 15000) {
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::milliseconds(timeout_ms);
		for (;;) {
			if (eof_seen) return true;
			in_buf.clear();
			if (!read_until(deadline)) return eof_seen;
		}
	}

	void close() {
		error_code ec;
		sock.close(ec);
	}

	// Read once, waiting no longer than @p deadline for a byte. False on
	// the deadline or a closed peer; a closed peer sets eof_seen.
	bool read_until(std::chrono::steady_clock::time_point deadline) {
		for (;;) {
			error_code ec;
			char tmp[4096];
			size_t n = sock.read_some(ba::buffer(tmp), ec);
			if (!ec) {
				if (n == 0) { eof_seen = true; return false; }
				in_buf.append(tmp, n);
				return true;
			}
			if (ec == ba::error::would_block
					|| ec == ba::error::try_again) {
				if (std::chrono::steady_clock::now() >= deadline)
					return false;
				std::this_thread::sleep_for(
					std::chrono::milliseconds(2));
				continue;
			}
			eof_seen = true;
			return false;
		}
	}

	ba::io_context io;
	tcp::socket sock{io};
	std::string in_buf;
	bool eof_seen = false;
};

static std::string hello_session(const htref& v) {
	auto hello = member(v, "hello");
	REQUIRE(hello != nullptr);
	auto s = member(hello, "session");
	REQUIRE(s != nullptr);
	return std::string(s->get_string());
}



TEST_SUITE("tgf serve: report keys") {
	TEST_CASE("only keys in report children match") {
		using tgf_serve_test::json_node_has_key;
		auto node = parse_line(R"({"key":"root","children":[
			{"key":17},{"key":"first"},
			{"children":[{"key":"nested"}]},null,
			{"children":{"key":"not-an-array"}},
			{"other":{"key":"unrelated"}}]})");
		const jt& tree = jt::get(node);
		CHECK(json_node_has_key(tree, "root"));
		CHECK(json_node_has_key(tree, "first"));
		CHECK(json_node_has_key(tree, "nested"));
		CHECK_FALSE(json_node_has_key(tree, "17"));
		CHECK_FALSE(json_node_has_key(tree, "not-an-array"));
		CHECK_FALSE(json_node_has_key(tree, "unrelated"));
		CHECK_FALSE(json_node_has_key(tree, "absent"));
		CHECK_FALSE(json_node_has_key(jt::get(jt::_null()), "root"));
	}

	TEST_CASE("deep report children use no recursive calls") {
		bintree<json::node>::gc_pause pause;
		tref node = jt::build_object({
			jt::build_pair("key", jt::build_string("leaf")) });
		for (size_t depth = 0; depth < 10000; ++depth)
			node = jt::build_object({ jt::build_pair("children",
				jt::build_array({ node })) });
		CHECK(tgf_serve_test::json_node_has_key(jt::get(node), "leaf"));
		CHECK_FALSE(tgf_serve_test::json_node_has_key(jt::get(node), "absent"));
	}
}

TEST_SUITE("tgf serve: sessions") {
	TEST_CASE("new session, a request, quit and attach again") {
		serve_fixture f;
		std::string id;
		{
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			auto hello = parse_line(c.read_line());
			REQUIRE(member(hello, "hello") != nullptr);
			id = hello_session(hello);
			CHECK(id.size() == 32);

			c.send(R"({"id":1,"cmd":"version"})");
			auto v = parse_line(c.read_line());
			CHECK(member(v, "cmd")->get_string() == "version");
			CHECK(member(v, "status")->get_string() == "ok");
			CHECK(member(member(v, "result"), "version") != nullptr);

			// The child answers quit itself, so the answer carries
			// the state, and the parent then closes the socket.
			c.send(R"({"id":2,"cmd":"quit"})");
			auto q = parse_line(c.read_line());
			CHECK(member(q, "cmd")->get_string() == "quit");
			CHECK(member(q, "status")->get_string() == "quit");
			CHECK(member(q, "id")->get_number() == 2);
			CHECK(member(q, "state") != nullptr);
			CHECK(c.at_eof());
		}
		// The session outlived the connection.
		serve_client c2(f.bound_port());
		c2.send("{\"cmd\":\"attach\",\"session\":\"" + id + "\"}");
		auto h = parse_line(c2.read_line());
		CHECK(member(h, "cmd")->get_string() == "hello");
		CHECK(member(h, "status")->get_string() == "ok");
		CHECK(hello_session(h) == id);
	}

	TEST_CASE("quit inside eval keeps the connection and the session") {
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		c.send(R"({"id":1,"cmd":"eval","src":"quit"})");
		auto q = parse_line(c.read_line());
		CHECK(member(q, "status")->get_string() == "quit");
		// The connection stays: only a plain quit request closes it.
		CHECK_FALSE(c.at_eof());
		c.send(R"({"id":2,"cmd":"version"})");
		auto v = parse_line(c.read_line());
		CHECK(member(v, "status")->get_string() == "ok");
		c.send(R"({"id":3,"cmd":"end-session"})");
		auto e = parse_line(c.read_line());
		CHECK(member(e, "status")->get_string() == "quit");
	}

	TEST_CASE("end-session removes the session") {
		serve_fixture f;
		std::string id;
		{
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			id = hello_session(parse_line(c.read_line()));
			c.send(R"({"id":3,"cmd":"end-session"})");
			auto q = parse_line(c.read_line());
			CHECK(member(q, "cmd")->get_string() == "quit");
			CHECK(member(q, "status")->get_string() == "quit");
			// The parent answers end-session with no state.
			CHECK(member(q, "state") == nullptr);
			CHECK(c.at_eof());
		}
		serve_client c2(f.bound_port());
		c2.send("{\"cmd\":\"attach\",\"session\":\"" + id + "\"}");
		auto e = parse_line(c2.read_line());
		CHECK(member(e, "status")->get_string() == "error");
		CHECK(report_contains_key(e, "Unknown session id"));
	}

	TEST_CASE("an unknown session id gives an error") {
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"attach","session":"0123456789abcdef0123456789abcdef"})");
		auto e = parse_line(c.read_line());
		CHECK(member(e, "status")->get_string() == "error");
		CHECK(report_contains_key(e, "Unknown session id"));
		CHECK(c.at_eof());
	}

	TEST_CASE("a session refuses a second attach") {
		serve_fixture f;
		serve_client c1(f.bound_port());
		c1.send(R"({"cmd":"new"})");
		std::string id = hello_session(parse_line(c1.read_line()));
		serve_client c2(f.bound_port());
		c2.send("{\"cmd\":\"attach\",\"session\":\"" + id + "\"}");
		auto e = parse_line(c2.read_line());
		CHECK(member(e, "status")->get_string() == "error");
		CHECK(report_contains_key(e, "Session already has a client"));
	}

	TEST_CASE("session ids are distinct random hex") {
		serve_fixture f;
		serve_client c1(f.bound_port());
		c1.send(R"({"cmd":"new"})");
		std::string a = hello_session(parse_line(c1.read_line()));
		serve_client c2(f.bound_port());
		c2.send(R"({"cmd":"new"})");
		std::string b = hello_session(parse_line(c2.read_line()));
		CHECK(a.size() == 32);
		CHECK(b.size() == 32);
		CHECK(a != b);
		for (char ch : a + b)
			CHECK(((ch >= '0' && ch <= '9')
				|| (ch >= 'a' && ch <= 'f')));
	}

	TEST_CASE("a pipelined quit before the hello still closes after"
		" its answer") {
		serve_fixture f;
		serve_client c(f.bound_port());
		// One write: the server handles the quit before the child's
		// unsolicited hello arrives, so the counts must stay aligned.
		c.send_raw("{\"cmd\":\"new\"}\n"
			"{\"id\":7,\"cmd\":\"quit\"}\n");
		auto hello = parse_line(c.read_line());
		CHECK(member(hello, "hello") != nullptr);
		auto q = parse_line(c.read_line());
		CHECK(member(q, "cmd")->get_string() == "quit");
		CHECK(member(q, "id")->get_number() == 7);
		CHECK(c.at_eof());
	}

	TEST_CASE("many end-session rounds leave no stale child fd") {
		// Each end-session closes one child pipe. A closed descriptor
		// that stays in the fork list can make a later child close a
		// reused descriptor, so a new session must still answer.
		serve_fixture f(64);
		for (int i = 0; i != 24; ++i) {
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			REQUIRE(member(parse_line(c.read_line()), "hello")
				!= nullptr);
			c.send(R"({"id":1,"cmd":"end-session"})");
			auto q = parse_line(c.read_line());
			REQUIRE(member(q, "status")->get_string() == "quit");
			REQUIRE(c.at_eof());
		}
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		c.send(R"({"id":1,"cmd":"version"})");
		auto v = parse_line(c.read_line());
		CHECK(member(v, "status")->get_string() == "ok");
		CHECK(member(v, "cmd")->get_string() == "version");
	}
}

TEST_SUITE("tgf serve: limits") {
	TEST_CASE("a line over the limit keeps the connection") {
		serve_fixture f(16, 18);
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		// 22 bytes: over the 18 byte limit.
		c.send(R"({"id":1,"cmd":"version"})");
		auto e = parse_line(c.read_line());
		CHECK(member(e, "status")->get_string() == "error");
		// The server builds this answer itself, so it has no state.
		CHECK(member(e, "state") == nullptr);
		CHECK(report_contains_key(e, "Line is longer than the limit"));
		// The connection is still there.
		c.send(R"({"cmd":"version"})");
		auto v = parse_line(c.read_line());
		CHECK(member(v, "status")->get_string() == "ok");
		CHECK(member(v, "cmd")->get_string() == "version");
	}

	TEST_CASE("a connection with no first line is closed") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--first-line-timeout", "1" });
		serve_client c(f.bound_port());
		CHECK(c.at_eof(5000));
	}

	TEST_CASE("a line over the limit goes into the session log") {
		std::string dir = tgf_serve_test::make_temp_dir(
			"tgf_serve_long_");
		{
			serve_fixture f(16, 18, dir, false);
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			REQUIRE(member(parse_line(c.read_line()), "hello")
				!= nullptr);
			c.send(R"({"id":1,"cmd":"version"})");
			CHECK(member(parse_line(c.read_line()), "status")
				->get_string() == "error");
			std::this_thread::sleep_for(
				std::chrono::milliseconds(300));
		}
		std::string session_log, server_log;
		for (const auto& e : std::filesystem::directory_iterator(dir)) {
			std::ifstream in(e.path());
			std::string text((std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());
			if (e.path().filename() == "server.log") server_log = text;
			else session_log = text;
		}
		CHECK(session_log.find("\"event\":\"line-too-long\"")
			!= std::string::npos);
		CHECK(server_log.find("\"event\":\"line-too-long\"")
			!= std::string::npos);
		std::filesystem::remove_all(dir);
	}

	TEST_CASE("a padded quit is still intercepted") {
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		// The parent parses the whole line for the command name, so
		// the 6000 padding bytes must not hide the quit.
		std::string pad(6000, 'x');
		c.send("{\"id\":5,\"cmd\":\"quit\",\"pad\":\""
			+ pad + "\"}");
		auto q = parse_line(c.read_line());
		CHECK(member(q, "status")->get_string() == "quit");
		CHECK(member(q, "id")->get_number() == 5);
		CHECK(c.at_eof());
	}

	TEST_CASE("max sessions refuses the second session") {
		serve_fixture f(1);
		serve_client c1(f.bound_port());
		c1.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c1.read_line()), "hello") != nullptr);
		serve_client c2(f.bound_port());
		c2.send(R"({"cmd":"new"})");
		auto e = parse_line(c2.read_line());
		CHECK(member(e, "status")->get_string() == "error");
		CHECK(report_contains_key(e, "Too many sessions"));
		CHECK(c2.at_eof());
	}

	TEST_CASE("max pending refuses a second silent connection") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--max-pending", "1", "--first-line-timeout", "5" });
		serve_client c1(f.bound_port());
		serve_client c2(f.bound_port());
		c2.send(R"({"cmd":"new"})");
		auto e = parse_line(c2.read_line());
		CHECK(member(e, "status")->get_string() == "error");
		CHECK(c2.at_eof());
		c1.close();
	}

	TEST_CASE("a small answer limit closes a session") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--max-answer", "32" });
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		CHECK(c.at_eof(5000));
	}

	TEST_CASE("a held port: the error names no option, the report has the hint") {
		const std::string hint =
			"Use -p <port> for another port, or -p 0 for a free port";
		serve_fixture f;
		const std::string port = std::to_string(f.bound_port());
		{
			child_process text;
			REQUIRE(text.start(tgf_binary_path(), { grammar_path(),
				"serve", "--port", port, "--no-log" }));
			std::string line, error_line;
			REQUIRE(text.read_stderr_line(error_line, 30000));
			// a pipe is no terminal, so the lines have no colors
			CHECK(error_line == "Port is in use (port=" + port + ")");
			REQUIRE(text.read_stdout_line(line, 30000));
			CHECK(line == hint);
		}
		child_process json_mode;
		REQUIRE(json_mode.start(tgf_binary_path(), { grammar_path(),
			"serve", "--json", "--port", port, "--no-log" }));
		std::string line;
		REQUIRE(json_mode.read_stdout_line(line, 30000));
		auto rep = parse_line(line);
		auto nodes = member(rep, "nodes");
		REQUIRE(nodes != nullptr);
		bool error_seen = false, hint_seen = false;
		for (const jt& n : nodes->children_trees()) {
			auto k = member(n, "key");
			if (!k || !k->is_string()) continue;
			if (k->get_string() == "Port is in use") error_seen = true;
			if (k->get_string() == hint) {
				hint_seen = true;
				CHECK(member(n, "value") == nullptr);
			}
		}
		CHECK(error_seen);
		CHECK(hint_seen);
	}
}

TEST_SUITE("tgf serve: detached buffer") {
	TEST_CASE("the buffer goes out before the new hello") {
		serve_fixture f(16, 100000);
		std::string id;
		std::string digits(8000, '1');
		{
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			id = hello_session(parse_line(c.read_line()));
			// A parse big enough that the child answers after the
			// parent has detached the closed socket.
			c.send("{\"id\":1,\"cmd\":\"parse\",\"input\":\""
				+ digits + "\"}");
			c.close();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		serve_client c2(f.bound_port());
		c2.send("{\"cmd\":\"attach\",\"session\":\"" + id + "\"}");
		// The buffered response is a large tree; check its prefix and
		// do not parse the whole line again.
		auto buffered = c2.read_line();
		CHECK(buffered.rfind(
			"{\"id\":1,\"cmd\":\"parse\",\"status\":\"ok\",", 0)
			== 0);
		auto h = parse_line(c2.read_line());
		CHECK(member(h, "cmd")->get_string() == "hello");
	}

	TEST_CASE("a small detached buffer drops the old answer") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--detached-buffer", "1" });
		std::string id;
		{
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			id = hello_session(parse_line(c.read_line()));
			c.send("{\"id\":1,\"cmd\":\"parse\",\"input\":\""
				+ std::string(50, '1') + "\"}");
			c.close();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		serve_client c2(f.bound_port());
		c2.send("{\"cmd\":\"attach\",\"session\":\"" + id + "\"}");
		auto hello = parse_line(c2.read_line());
		CHECK(member(hello, "cmd")->get_string() == "hello");
	}
}

TEST_SUITE("tgf serve: backpressure") {
	TEST_CASE("end-session drains a pending child answer") {
		// One session slot: it is free again only when the child exits
		// and leaves the table, which needs the drained pipe.
		serve_fixture f(1);
		std::string digits(8000, '1');
		{
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			REQUIRE(member(parse_line(c.read_line()), "hello")
				!= nullptr);
			c.send("{\"id\":1,\"cmd\":\"parse\",\"input\":\""
				+ digits + "\"}");
			c.send(R"({"id":2,"cmd":"end-session"})");
			auto e = parse_line(c.read_line());
			CHECK(member(e, "cmd")->get_string() == "quit");
		}
		bool ok = false;
		for (int i = 0; i != 40 && !ok; ++i) {
			serve_client c2(f.bound_port());
			c2.send(R"({"cmd":"new"})");
			auto h = parse_line(c2.read_line());
			if (member(h, "hello") != nullptr) { ok = true; break; }
			std::this_thread::sleep_for(
				std::chrono::milliseconds(100));
		}
		CHECK(ok);
	}

	TEST_CASE("a client that sends faster than the child answers waits") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--max-open-requests", "2", "--debug-sessions" });
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		// A small request with a large answer: the child is the slow end.
		const std::string req = "{\"cmd\":\"parse\",\"input\":\""
			+ std::string(2000, '1') + "\"}\n";
		std::string many;
		for (int i = 0; i != 200; ++i) many += req;
		size_t off = 0, in = 0, out = 0, worst = 0;
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(3);
		while (std::chrono::steady_clock::now() < deadline) {
			error_code ec;
			if (off != many.size()) off += c.sock.write_some(
				ba::buffer(many.data() + off, many.size() - off), ec);
			while (!c.read_line(1).empty()) {}
			std::string l;
			while (f.proc.read_stdout_line(l, 1)) {
				if (l.find("\"dir\":\"in\"") != std::string::npos) ++in;
				if (l.find("\"dir\":\"out\"") != std::string::npos) ++out;
				// the first in line is the new request
				if (in > out + 1) worst = std::max(worst, in - out - 1);
			}
		}
		CHECK(in > 2);
		// two open requests and the rest of one read of the server
		CHECK(worst <= 5);
	}

	TEST_CASE("a connection that does not read its output is closed") {
		// A short deadline keeps the test fast; the default is 30 seconds.
		serve_fixture f(16, 16777216, "", true, "",
			{ "--write-timeout", "1" });
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		std::string digits(8000, '1');
		// Queue large answers and a quit that waits behind them. The client
		// never reads, so the server's write deadline, the one under test,
		// closes the socket. The client deadline only bounds a slow host.
		for (int i = 0; i != 8; ++i)
			c.send("{\"id\":" + std::to_string(i + 1)
				+ ",\"cmd\":\"parse\",\"input\":\""
				+ digits + "\"}");
		c.send(R"({"id":99,"cmd":"quit"})");
		CHECK(c.drain_to_eof(30000));
	}

	TEST_CASE("--debug-sessions copies the session log lines to stdout") {
		for (bool debug : { true, false }) {
			std::vector<std::string> extra;
			if (debug) extra.push_back("--debug-sessions");
			serve_fixture f(16, 16777216, "", true, "", extra);
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
			c.send(R"({"id":7,"cmd":"version"})");
			REQUIRE_FALSE(c.read_line().empty());
			c.close();
			// Read the server stdout up to the detach of the client.
			int in_version = 0, out_version = 0, copies = 0;
			bool detached = false;
			std::string line;
			while (!detached && f.proc.read_stdout_line(line, 10000)) {
				auto v = parse_line(line);
				if (auto e = member(v, "event"); e
						&& e->get_string() == "detach")
					detached = true;
				auto text = member(v, "line");
				if (!text) continue;
				++copies;
				CHECK(member(v, "session") != nullptr);
				std::string dir = member(v, "dir")->get_string();
				std::string payload = text->get_string();
				// The hello carries a version too; only the request
				// and its answer carry the id.
				if (payload.find("\"id\":7") == std::string::npos)
					continue;
				if (dir == "in") ++in_version;
				if (dir == "out") ++out_version;
			}
			CHECK(detached);
			CHECK(in_version == (debug ? 1 : 0));
			CHECK(out_version == (debug ? 1 : 0));
			if (!debug) CHECK(copies == 0);
		}
	}

	TEST_CASE("an idle connection outlives the write timeout") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--write-timeout", "1" });
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		// No output waits, so the deadline must not close the socket.
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		c.send(R"({"id":1,"cmd":"version"})");
		std::string resp = c.read_line();
		REQUIRE_FALSE(resp.empty());
		CHECK(member(parse_line(resp), "status")->get_string() == "ok");
	}
}

TEST_SUITE("tgf serve: no server paths") {
	TEST_CASE("a session refuses a file path") {
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		c.send(R"({"id":1,"cmd":"parse_file","file":"/etc/passwd"})");
		auto e = parse_line(c.read_line());
		CHECK(member(e, "status")->get_string() == "error");
		CHECK(report_contains_key(e,
			"File paths are not allowed in a server session"));
	}
}

TEST_SUITE("tgf serve: logs") {
	TEST_CASE("a session and the server write a log") {
		std::string dir = tgf_serve_test::make_temp_dir(
			"tgf_serve_log_");
		{
			serve_fixture f(16, 100000, dir, false);
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			REQUIRE(member(parse_line(c.read_line()), "hello")
				!= nullptr);
			c.send(R"({"id":1,"cmd":"version"})");
			REQUIRE(member(parse_line(c.read_line()), "status")
				->get_string() == "ok");
			c.send(R"({"id":2,"cmd":"end-session"})");
			REQUIRE(member(parse_line(c.read_line()), "status")
				->get_string() == "quit");
			// Give the child and the parent a moment to flush.
			std::this_thread::sleep_for(
				std::chrono::milliseconds(300));
		}
		bool have_server_log = false;
		std::string session_log;
		for (const auto& e :
				std::filesystem::directory_iterator(dir)) {
			std::string name = e.path().filename().string();
			if (name == "server.log") { have_server_log = true;
				continue; }
			if (name.size() > 4 && name.substr(name.size() - 4)
					== ".log")
				session_log = e.path().string();
		}
		CHECK(have_server_log);
		REQUIRE_FALSE(session_log.empty());
		std::ifstream in(session_log);
		std::string text((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		CHECK(text.find("\"dir\":\"in\"") != std::string::npos);
		CHECK(text.find("\"dir\":\"out\"") != std::string::npos);
		CHECK(text.find("\"event\":\"new\"") != std::string::npos);
		CHECK(text.find("\"event\":\"end-session\"") != std::string::npos);
		// The first line of the connection is logged as in too.
		CHECK(text.find("\\\"cmd\\\":\\\"new\\\"")
			!= std::string::npos);
		// The child exit is logged exactly once.
		size_t pos = 0, exits = 0;
		while ((pos = text.find("\"event\":\"child-exit\"", pos))
				!= std::string::npos) { ++exits; ++pos; }
		CHECK(exits == 1);
		// Windows refuses to delete a file an open stream still holds.
		in.close();
		std::filesystem::remove_all(dir);
	}
#ifndef _WIN32
	TEST_CASE("--log-dir: a new folder gets 0700, each log gets 0600") {
		const std::string base = tgf_serve_test::make_temp_dir(
			"tgf_serve_modes_");
		const std::string dir = base + "/logs";
		{
			serve_fixture f(16, 100000, dir, false);
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			REQUIRE(member(parse_line(c.read_line()), "hello")
				!= nullptr);
			c.send(R"({"id":1,"cmd":"end-session"})");
			REQUIRE(member(parse_line(c.read_line()), "status")
				->get_string() == "quit");
		}
		auto mode = [](const std::filesystem::path& p) {
			return static_cast<unsigned>(
				std::filesystem::status(p).permissions()) & 0777u;
		};
		CHECK(mode(dir) == 0700);
		size_t logs = 0;
		for (const auto& e : std::filesystem::directory_iterator(dir)) {
			++logs;
			CHECK(mode(e.path()) == 0600);
		}
		CHECK(logs == 2);
		// The line server writes server.log, the events of tgf serve too.
		std::ifstream is(dir + "/server.log");
		const std::string log((std::istreambuf_iterator<char>(is)),
			std::istreambuf_iterator<char>());
		CHECK(log.find("\"event\":\"new\"") != std::string::npos);
		CHECK(log.find("\"event\":\"end-session\"")
			!= std::string::npos);
		is.close();
		std::filesystem::remove_all(base);
	}
#endif
}

TEST_SUITE("tgf serve: init line") {
	TEST_CASE("--init-stdin applies the grammar and the start symbol") {
		std::ifstream in(grammar_path());
		REQUIRE(in.good());
		std::string source((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		child_process proc;
		std::vector<std::string> args = {
			"repl", "--json", "--init-stdin" };
		REQUIRE(proc.start(tgf_binary_path(), args));

		tref g = jt::build_object({
			jt::build_pair("name", jt::build_string("tiny.tgf")),
			jt::build_pair("source", jt::build_string(source)) });
		tref opts = jt::build_object({ jt::build_pair(
			"print-terminals", jt::build_bool(false)) });
		tref init = jt::build_object({
			jt::build_pair("grammar", g),
			jt::build_pair("start", jt::build_string("start")),
			jt::build_pair("options", opts) });
		REQUIRE(proc.write_stdin(jt::get(jt::build_object({
			jt::build_pair("init", init) })).to_str()));

		std::string hello_line;
		REQUIRE(proc.read_stdout_line(hello_line, 30000));
		auto hello = parse_line(hello_line);
		auto h = member(hello, "hello");
		REQUIRE(h != nullptr);
		auto name = member(h, "grammar");
		REQUIRE(name != nullptr);
		CHECK(name->get_string() == "tiny.tgf");
		auto start = member(h, "start");
		REQUIRE(start != nullptr);
		CHECK(start->get_string() == "start");
		auto o = member(h, "options");
		REQUIRE(o != nullptr);
		auto cc = member(o, "char-classes");
		REQUIRE(cc != nullptr);
		REQUIRE(cc->is_array());
		REQUIRE(cc->children_size() == 1);
		CHECK((*cc)[0].get_string() == "digit");
		auto pt = member(o, "print-terminals");
		REQUIRE(pt != nullptr);
		CHECK_FALSE(pt->get_bool());
	}

	TEST_CASE("the init line sets the line limit") {
		child_process proc;
		std::vector<std::string> args = {
			"repl", "--json", "--init-stdin" };
		REQUIRE(proc.start(tgf_binary_path(), args));

		tref init = jt::build_object({
			jt::build_pair("max_line", jt::build_number(4096)) });
		REQUIRE(proc.write_stdin(jt::get(jt::build_object({
			jt::build_pair("init", init) })).to_str()));

		std::string hello_line;
		REQUIRE(proc.read_stdout_line(hello_line, 30000));
		REQUIRE(member(parse_line(hello_line), "hello") != nullptr);

		// 5000 bytes: over the init limit of 4096.
		std::string pad(5000, 'x');
		REQUIRE(proc.write_stdin(
			"{\"id\":1,\"cmd\":\"version\",\"pad\":\""
			+ pad + "\"}"));
		std::string resp;
		REQUIRE(proc.read_stdout_line(resp, 30000));
		auto e = parse_line(resp);
		CHECK(member(e, "status")->get_string() == "error");
		// The child answers its own line-too-long error with state.
		CHECK(member(e, "state") != nullptr);

		// The loop goes on with the new limit applied.
		REQUIRE(proc.write_stdin(R"({"id":2,"cmd":"version"})"));
		REQUIRE(proc.read_stdout_line(resp, 30000));
		CHECK(member(parse_line(resp), "status")->get_string() == "ok");
	}
}

TEST_SUITE("tgf serve: options") {
	TEST_CASE("the default port metadata is 4000") {
		auto port = std::find_if(tgf_option_set().options.begin(),
			tgf_option_set().options.end(), [](const option_spec& option) {
				return option.name == "port";
			});
		REQUIRE(port != tgf_option_set().options.end());
		REQUIRE(std::holds_alternative<std::size_t>(port->fallback));
		CHECK(std::get<std::size_t>(port->fallback) == 4000);
	}

	TEST_CASE("non-default stderr and end limits are accepted") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--max-stderr-line", "32", "--end-timeout", "1" });
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		c.send(R"({"id":1,"cmd":"end-session"})");
		CHECK(member(parse_line(c.read_line()), "status")
			->get_string() == "quit");
	}

	TEST_CASE("serve forwards --enabled-productions to the session") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--enabled-productions", "guard1" });
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		auto hello = parse_line(c.read_line());
		auto h = member(hello, "hello");
		REQUIRE(h != nullptr);
		auto o = member(h, "options");
		REQUIRE(o != nullptr);
		auto ep = member(o, "enabled-productions");
		REQUIRE(ep != nullptr);
		REQUIRE(ep->is_array());
		bool found = false;
		for (const jt& e : ep->children_trees())
			if (e.is_string() && e.get_string() == "guard1")
				found = true;
		CHECK(found);
	}
}

TEST_SUITE("tgf serve: reload") {
	TEST_CASE("a session reloads the stored grammar text") {
		std::ifstream in(grammar_path(), std::ios::binary);
		REQUIRE(in.good());
		std::string source((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(member(parse_line(c.read_line()), "hello") != nullptr);
		c.send(R"({"id":1,"cmd":"reload"})");
		auto v = parse_line(c.read_line());
		CHECK(member(v, "status")->get_string() == "ok");
		auto result = member(v, "result");
		REQUIRE(result != nullptr);
		CHECK(member(result, "loaded")->get_bool());
		CHECK(member(result, "grammar")->get_string() == grammar_path());
		// The reloaded grammar is the same text, and it still parses.
		c.send(R"({"id":2,"cmd":"grammar"})");
		auto g = parse_line(c.read_line());
		CHECK(member(g, "status")->get_string() == "ok");
		auto data = member(g, "result");
		REQUIRE(data != nullptr);
		CHECK(member(data, "file")->get_string() == grammar_path());
		CHECK(member(data, "source")->get_string() == source);
		c.send(R"({"id":3,"cmd":"parse","input":"12"})");
		auto p = parse_line(c.read_line());
		CHECK(member(p, "status")->get_string() == "ok");
		auto tree = member(member(p, "result"), "tree");
		REQUIRE(tree != nullptr);
		CHECK(member(tree, "symbol")->get_string() == "start");
	}

	TEST_CASE("a session keeps the bytes the server loaded") {
		std::string dir = tgf_serve_test::make_temp_dir(
			"tgf_reload_");
		std::string path = dir + "/tiny.tgf";
		std::string original;
		{
			std::ifstream in(grammar_path(), std::ios::binary);
			REQUIRE(in.good());
			original.assign(std::istreambuf_iterator<char>(in),
				std::istreambuf_iterator<char>());
			std::ofstream out(path, std::ios::binary);
			REQUIRE(out.good());
			out.write(original.data(),
				static_cast<std::streamsize>(original.size()));
		}
		{
			serve_fixture f(16, 100000, "", true, path);
			// The grammar file changes after the server loaded it.
			{
				std::ofstream out(path, std::ios::binary);
				REQUIRE(out.good());
				out << "@use char classes digit.\n"
					"start => other.\n";
			}
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			REQUIRE(member(parse_line(c.read_line()), "hello")
				!= nullptr);
			c.send(R"({"id":1,"cmd":"grammar"})");
			auto g = parse_line(c.read_line());
			CHECK(member(g, "status")->get_string() == "ok");
			auto data = member(g, "result");
			REQUIRE(data != nullptr);
			CHECK(member(data, "file")->get_string() == path);
			CHECK(member(data, "source")->get_string() == original);
			c.send(R"({"id":2,"cmd":"reload"})");
			auto v = parse_line(c.read_line());
			CHECK(member(v, "status")->get_string() == "ok");
			CHECK(member(member(v, "result"), "loaded")->get_bool());
			// The reload kept the loaded bytes, not the changed file.
			c.send(R"({"id":3,"cmd":"grammar"})");
			auto g2 = parse_line(c.read_line());
			auto data2 = member(g2, "result");
			REQUIRE(data2 != nullptr);
			CHECK(member(data2, "source")->get_string() == original);
		}
		std::filesystem::remove_all(dir);
	}
}

TEST_SUITE("tgf: colors in a pipe") {

	// The stdout of tgf with @p args, after the lines @p input on stdin.
	static std::string stdout_of(const std::vector<std::string>& args,
		const std::vector<std::string>& input = {})
	{
		child_process proc;
		REQUIRE(proc.start(tgf_binary_path(), args));
		for (const auto& l : input) REQUIRE(proc.write_stdin(l));
		std::string all, line;
		while (proc.read_stdout_line(line, 10000)) all += line + "\n";
		return all;
	}

	TEST_CASE("parse -e into a pipe has no escape byte") {
		const std::string out = stdout_of({ grammar_path(), "parse",
			"-e", "12" });
		CHECK(out.find("12") != std::string::npos);
		CHECK(out.find('\x1b') == std::string::npos);
	}

	TEST_CASE("parse -c true -e into a pipe has an escape byte") {
		const std::string out = stdout_of({ grammar_path(), "parse",
			"-c", "true", "-e", "12" });
		CHECK(out.find('\x1b') != std::string::npos);
	}

	TEST_CASE("set colors true in a REPL session gives colors") {
		const std::string plain = stdout_of({ grammar_path() },
			{ "p 12", "q" });
		CHECK(plain.find("12") != std::string::npos);
		CHECK(plain.find('\x1b') == std::string::npos);
		const std::string colored = stdout_of({ grammar_path() },
			{ "set colors true", "p 12", "q" });
		CHECK(colored.find('\x1b') != std::string::npos);
	}
}
