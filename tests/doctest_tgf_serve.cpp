// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Loopback tests for the tgf TCP server. The fixture starts the tgf binary
// as a process on every platform and reads its listening line; the test
// then connects to the port with plain client sockets. No test forks.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "serve/tgf_serve_test_fixture.h"
#include "../src/tgf/tgf_cli.h"
#include "../src/tgf/tgf_serve.h"
#include "format/json/json.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

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
using tgf_serve_test::json_options;
using tgf_serve_test::serve_fixture;
using tgf_serve_test::tgf_binary_path;

namespace ba = boost::asio;
using ba::ip::tcp;
using boost::system::error_code;

static json::value parse_line(const std::string& line) {
	auto p = json::parse(line);
	REQUIRE(p.has_value());
	return p.value();
}

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

static std::string hello_session(const json::value& v) {
	auto hello = v.find("hello");
	REQUIRE(hello != nullptr);
	auto s = hello->find("session");
	REQUIRE(s != nullptr);
	return std::string(s->as_string());
}

static bool json_node_has_key(const json::value& node,
	const std::string& key)
{
	if (auto k = node.find("key"); k && k->is_string()
			&& k->as_string() == key) return true;
	if (auto ch = node.find("children"); ch && ch->is_array())
		for (const auto& c : *ch)
			if (json_node_has_key(c, key)) return true;
	return false;
}

static bool report_contains_key(const json::value& resp,
	const std::string& key)
{
	auto rep = resp.find("report");
	if (!rep) return false;
	auto nodes = rep->find("nodes");
	if (!nodes || !nodes->is_array()) return false;
	for (const auto& n : *nodes)
		if (json_node_has_key(n, key)) return true;
	return false;
}

TEST_SUITE("tgf serve: sessions") {
	TEST_CASE("new session, a request, quit and attach again") {
		serve_fixture f;
		std::string id;
		{
			serve_client c(f.bound_port());
			c.send(R"({"cmd":"new"})");
			auto hello = parse_line(c.read_line());
			REQUIRE(hello.find("hello") != nullptr);
			id = hello_session(hello);
			CHECK(id.size() == 32);

			c.send(R"({"id":1,"cmd":"version"})");
			auto v = parse_line(c.read_line());
			CHECK(v.find("cmd")->as_string() == "version");
			CHECK(v.find("status")->as_string() == "ok");
			CHECK(v.find("result")->find("version") != nullptr);

			// The child answers quit itself, so the answer carries
			// the state, and the parent then closes the socket.
			c.send(R"({"id":2,"cmd":"quit"})");
			auto q = parse_line(c.read_line());
			CHECK(q.find("cmd")->as_string() == "quit");
			CHECK(q.find("status")->as_string() == "quit");
			CHECK(q.find("id")->as_number() == 2);
			CHECK(q.find("state") != nullptr);
			CHECK(c.at_eof());
		}
		// The session outlived the connection.
		serve_client c2(f.bound_port());
		c2.send("{\"cmd\":\"attach\",\"session\":\"" + id + "\"}");
		auto h = parse_line(c2.read_line());
		CHECK(h.find("cmd")->as_string() == "hello");
		CHECK(h.find("status")->as_string() == "ok");
		CHECK(hello_session(h) == id);
	}

	TEST_CASE("quit inside eval keeps the connection and the session") {
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(parse_line(c.read_line()).find("hello") != nullptr);
		c.send(R"({"id":1,"cmd":"eval","src":"quit"})");
		auto q = parse_line(c.read_line());
		CHECK(q.find("status")->as_string() == "quit");
		// The connection stays: only a plain quit request closes it.
		CHECK_FALSE(c.at_eof());
		c.send(R"({"id":2,"cmd":"version"})");
		auto v = parse_line(c.read_line());
		CHECK(v.find("status")->as_string() == "ok");
		c.send(R"({"id":3,"cmd":"end-session"})");
		auto e = parse_line(c.read_line());
		CHECK(e.find("status")->as_string() == "quit");
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
			CHECK(q.find("cmd")->as_string() == "quit");
			CHECK(q.find("status")->as_string() == "quit");
			// The parent answers end-session with no state.
			CHECK(q.find("state") == nullptr);
			CHECK(c.at_eof());
		}
		serve_client c2(f.bound_port());
		c2.send("{\"cmd\":\"attach\",\"session\":\"" + id + "\"}");
		auto e = parse_line(c2.read_line());
		CHECK(e.find("status")->as_string() == "error");
		CHECK(report_contains_key(e, "Unknown session id"));
	}

	TEST_CASE("an unknown session id gives an error") {
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"attach","session":"0123456789abcdef0123456789abcdef"})");
		auto e = parse_line(c.read_line());
		CHECK(e.find("status")->as_string() == "error");
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
		CHECK(e.find("status")->as_string() == "error");
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
		CHECK(hello.find("hello") != nullptr);
		auto q = parse_line(c.read_line());
		CHECK(q.find("cmd")->as_string() == "quit");
		CHECK(q.find("id")->as_number() == 7);
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
			REQUIRE(parse_line(c.read_line()).find("hello")
				!= nullptr);
			c.send(R"({"id":1,"cmd":"end-session"})");
			auto q = parse_line(c.read_line());
			REQUIRE(q.find("status")->as_string() == "quit");
			REQUIRE(c.at_eof());
		}
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(parse_line(c.read_line()).find("hello") != nullptr);
		c.send(R"({"id":1,"cmd":"version"})");
		auto v = parse_line(c.read_line());
		CHECK(v.find("status")->as_string() == "ok");
		CHECK(v.find("cmd")->as_string() == "version");
	}
}

TEST_SUITE("tgf serve: limits") {
	TEST_CASE("a line over the limit keeps the connection") {
		serve_fixture f(16, 18);
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(parse_line(c.read_line()).find("hello") != nullptr);
		// 22 bytes: over the 18 byte limit.
		c.send(R"({"id":1,"cmd":"version"})");
		auto e = parse_line(c.read_line());
		CHECK(e.find("status")->as_string() == "error");
		// The server builds this answer itself, so it has no state.
		CHECK(e.find("state") == nullptr);
		CHECK(report_contains_key(e, "Line is longer than the limit"));
		// The connection is still there.
		c.send(R"({"cmd":"version"})");
		auto v = parse_line(c.read_line());
		CHECK(v.find("status")->as_string() == "ok");
		CHECK(v.find("cmd")->as_string() == "version");
	}

	TEST_CASE("a padded quit is still intercepted") {
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(parse_line(c.read_line()).find("hello") != nullptr);
		// The parent parses the whole line for the command name, so
		// the 6000 padding bytes must not hide the quit.
		std::string pad(6000, 'x');
		c.send("{\"id\":5,\"cmd\":\"quit\",\"pad\":\""
			+ pad + "\"}");
		auto q = parse_line(c.read_line());
		CHECK(q.find("status")->as_string() == "quit");
		CHECK(q.find("id")->as_number() == 5);
		CHECK(c.at_eof());
	}

	TEST_CASE("max sessions refuses the second session") {
		serve_fixture f(1);
		serve_client c1(f.bound_port());
		c1.send(R"({"cmd":"new"})");
		REQUIRE(parse_line(c1.read_line()).find("hello") != nullptr);
		serve_client c2(f.bound_port());
		c2.send(R"({"cmd":"new"})");
		auto e = parse_line(c2.read_line());
		CHECK(e.find("status")->as_string() == "error");
		CHECK(report_contains_key(e, "Too many sessions"));
		CHECK(c2.at_eof());
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
		CHECK(h.find("cmd")->as_string() == "hello");
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
			REQUIRE(parse_line(c.read_line()).find("hello")
				!= nullptr);
			c.send("{\"id\":1,\"cmd\":\"parse\",\"input\":\""
				+ digits + "\"}");
			c.send(R"({"id":2,"cmd":"end-session"})");
			auto e = parse_line(c.read_line());
			CHECK(e.find("cmd")->as_string() == "quit");
		}
		bool ok = false;
		for (int i = 0; i != 40 && !ok; ++i) {
			serve_client c2(f.bound_port());
			c2.send(R"({"cmd":"new"})");
			auto h = parse_line(c2.read_line());
			if (h.find("hello") != nullptr) { ok = true; break; }
			std::this_thread::sleep_for(
				std::chrono::milliseconds(100));
		}
		CHECK(ok);
	}

	TEST_CASE("a connection that does not read its output is closed") {
		// A short deadline keeps the test fast; the default is 30 seconds.
		serve_fixture f(16, 16777216, "", true, "",
			{ "--write-timeout", "1" });
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(parse_line(c.read_line()).find("hello") != nullptr);
		std::string digits(8000, '1');
		// Queue large answers, then a quit whose answer must follow
		// them. The client never reads, so the socket stalls and the
		// write deadline closes the connection.
		for (int i = 0; i != 8; ++i)
			c.send("{\"id\":" + std::to_string(i + 1)
				+ ",\"cmd\":\"parse\",\"input\":\""
				+ digits + "\"}");
		c.send(R"({"id":99,"cmd":"quit"})");
		CHECK(c.drain_to_eof(6000));
	}
}

TEST_SUITE("tgf serve: no server paths") {
	TEST_CASE("a session refuses a file path") {
		serve_fixture f;
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		REQUIRE(parse_line(c.read_line()).find("hello") != nullptr);
		c.send(R"({"id":1,"cmd":"parse file","file":"/etc/passwd"})");
		auto e = parse_line(c.read_line());
		CHECK(e.find("status")->as_string() == "error");
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
			REQUIRE(parse_line(c.read_line()).find("hello")
				!= nullptr);
			c.send(R"({"id":1,"cmd":"version"})");
			REQUIRE(parse_line(c.read_line()).find("status")
				->as_string() == "ok");
			c.send(R"({"id":2,"cmd":"end-session"})");
			REQUIRE(parse_line(c.read_line()).find("status")
				->as_string() == "quit");
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
		std::string all((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		CHECK(all.find("\"dir\":\"in\"") != std::string::npos);
		CHECK(all.find("\"dir\":\"out\"") != std::string::npos);
		CHECK(all.find("\"event\":\"new\"") != std::string::npos);
		CHECK(all.find("\"event\":\"end-session\"") != std::string::npos);
		// The first line of the connection is logged as in too.
		CHECK(all.find("\\\"cmd\\\":\\\"new\\\"")
			!= std::string::npos);
		// The child exit is logged exactly once.
		size_t pos = 0, exits = 0;
		while ((pos = all.find("\"event\":\"child-exit\"", pos))
				!= std::string::npos) { ++exits; ++pos; }
		CHECK(exits == 1);
		// Windows refuses to delete a file an open stream still holds.
		in.close();
		std::filesystem::remove_all(dir);
	}
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

		json::value g = json::value::object();
		g.set("name", json::value::string("tiny.tgf"));
		g.set("source", json::value::string(source));
		json::value opts = json::value::object();
		opts.set("print-terminals", json::value::boolean(false));
		json::value init = json::value::object();
		init.set("grammar", std::move(g));
		init.set("start", json::value::string("start"));
		init.set("options", std::move(opts));
		json::value line = json::value::object();
		line.set("init", std::move(init));
		std::ostringstream os;
		json::print(line, os);
		REQUIRE(proc.write_stdin(os.str()));

		std::string hello_line;
		REQUIRE(proc.read_stdout_line(hello_line, 30000));
		auto hello = parse_line(hello_line);
		auto h = hello.find("hello");
		REQUIRE(h != nullptr);
		auto name = h->find("grammar");
		REQUIRE(name != nullptr);
		CHECK(name->as_string() == "tiny.tgf");
		auto start = h->find("start");
		REQUIRE(start != nullptr);
		CHECK(start->as_string() == "start");
		auto dirs = h->find("directives");
		REQUIRE(dirs != nullptr);
		REQUIRE_FALSE(dirs->is_null());
		auto cc = dirs->find("use_char_classes");
		REQUIRE(cc != nullptr);
		REQUIRE(cc->is_array());
		REQUIRE(cc->size() == 1);
		CHECK((*cc)[0].as_string() == "digit");
		auto o = h->find("options");
		REQUIRE(o != nullptr);
		auto pt = o->find("print-terminals");
		REQUIRE(pt != nullptr);
		CHECK_FALSE(pt->as_bool());
	}

	TEST_CASE("the init line sets the line limit") {
		child_process proc;
		std::vector<std::string> args = {
			"repl", "--json", "--init-stdin" };
		REQUIRE(proc.start(tgf_binary_path(), args));

		json::value init = json::value::object();
		init.set("max_line", json::value::number(4096));
		json::value line = json::value::object();
		line.set("init", std::move(init));
		std::ostringstream os;
		json::print(line, os);
		REQUIRE(proc.write_stdin(os.str()));

		std::string hello_line;
		REQUIRE(proc.read_stdout_line(hello_line, 30000));
		REQUIRE(parse_line(hello_line).find("hello") != nullptr);

		// 5000 bytes: over the init limit of 4096.
		std::string pad(5000, 'x');
		REQUIRE(proc.write_stdin(
			"{\"id\":1,\"cmd\":\"version\",\"pad\":\""
			+ pad + "\"}"));
		std::string resp;
		REQUIRE(proc.read_stdout_line(resp, 30000));
		auto e = parse_line(resp);
		CHECK(e.find("status")->as_string() == "error");
		// The child answers its own line-too-long error with state.
		CHECK(e.find("state") != nullptr);

		// The loop goes on with the new limit applied.
		REQUIRE(proc.write_stdin(R"({"id":2,"cmd":"version"})"));
		REQUIRE(proc.read_stdout_line(resp, 30000));
		CHECK(parse_line(resp).find("status")->as_string() == "ok");
	}
}

TEST_SUITE("tgf serve: options") {
	TEST_CASE("serve forwards --productions to the session") {
		serve_fixture f(16, 16777216, "", true, "",
			{ "--productions", "guard1" });
		serve_client c(f.bound_port());
		c.send(R"({"cmd":"new"})");
		auto hello = parse_line(c.read_line());
		auto h = hello.find("hello");
		REQUIRE(h != nullptr);
		auto o = h->find("options");
		REQUIRE(o != nullptr);
		auto ep = o->find("enabled-productions");
		REQUIRE(ep != nullptr);
		REQUIRE(ep->is_array());
		bool found = false;
		for (const auto& e : *ep)
			if (e.is_string() && e.as_string() == "guard1")
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
		REQUIRE(parse_line(c.read_line()).find("hello") != nullptr);
		c.send(R"({"id":1,"cmd":"reload"})");
		auto v = parse_line(c.read_line());
		CHECK(v.find("status")->as_string() == "ok");
		auto result = v.find("result");
		REQUIRE(result != nullptr);
		CHECK(result->find("loaded")->as_bool());
		CHECK(result->find("grammar")->as_string() == grammar_path());
		// The reloaded grammar is the same text, and it still parses.
		c.send(R"({"id":2,"cmd":"grammar"})");
		auto g = parse_line(c.read_line());
		CHECK(g.find("status")->as_string() == "ok");
		auto data = g.find("result");
		REQUIRE(data != nullptr);
		CHECK(data->find("file")->as_string() == grammar_path());
		CHECK(data->find("source")->as_string() == source);
		c.send(R"({"id":3,"cmd":"parse","input":"12"})");
		auto p = parse_line(c.read_line());
		CHECK(p.find("status")->as_string() == "ok");
		auto tree = p.find("result")->find("tree");
		REQUIRE(tree != nullptr);
		CHECK(tree->find("symbol")->as_string() == "start");
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
			REQUIRE(parse_line(c.read_line()).find("hello")
				!= nullptr);
			c.send(R"({"id":1,"cmd":"grammar"})");
			auto g = parse_line(c.read_line());
			CHECK(g.find("status")->as_string() == "ok");
			auto data = g.find("result");
			REQUIRE(data != nullptr);
			CHECK(data->find("file")->as_string() == path);
			CHECK(data->find("source")->as_string() == original);
			c.send(R"({"id":2,"cmd":"reload"})");
			auto v = parse_line(c.read_line());
			CHECK(v.find("status")->as_string() == "ok");
			CHECK(v.find("result")->find("loaded")->as_bool());
			// The reload kept the loaded bytes, not the changed file.
			c.send(R"({"id":3,"cmd":"grammar"})");
			auto g2 = parse_line(c.read_line());
			auto data2 = g2.find("result");
			REQUIRE(data2 != nullptr);
			CHECK(data2->find("source")->as_string() == original);
		}
		std::filesystem::remove_all(dir);
	}
}
