// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "net/line_reader.h"
#include "net/line_server.h"
#include "net/line_client_test.h"

#include <atomic>
#include <csignal>
#include <cstring>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace idni;
using jt = format::json::tree;
using line_client_test::tcp_client;
namespace ba = boost::asio;
using ba::ip::tcp;

// A stream buffer that the server thread writes and the test reads.
// xsputn of stringbuf calls overflow, so the mutex is recursive.
struct locked_buf : std::stringbuf {
	std::recursive_mutex m;
	std::string text() {
		std::lock_guard lock(m);
		return str();
	}
protected:
	int_type overflow(int_type c) override {
		std::lock_guard lock(m);
		return std::stringbuf::overflow(c);
	}
	std::streamsize xsputn(const char* s, std::streamsize n) override {
		std::lock_guard lock(m);
		return std::stringbuf::xsputn(s, n);
	}
};

// Answers each line with the request plus "status":"ok".
struct echo_handler : session_handler {
	explicit echo_handler(std::atomic<int>& c) : closes(c) {}
	std::atomic<int>& closes;
	void on_open(tref first_line, reply_sink& out) override {
		out.write(jt::set_member(first_line, "status",
			jt::build_string("ok")));
	}
	void on_line(tref request, reply_sink& out) override {
		out.write(jt::set_member(request, "status",
			jt::build_string("ok")));
	}
	void on_close() override { ++closes; }
};

// A server on a thread of the test, with its output lines captured.
struct server_thread {
	locked_buf out;
	std::thread thread;
	std::atomic<bool> done{ false };
	std::optional<int> value;
	std::string report_text;
	std::atomic<int> closes{ 0 };

	explicit server_thread(line_server_options o) {
		if (!o.make_handler)
			o.make_handler = [this](const line_session&) {
				return std::make_unique<echo_handler>(closes);
			};
		o.on_output = [this](std::string_view line) {
			std::lock_guard lock(out.m);
			out.sputn(line.data(), static_cast<std::streamsize>(
				line.size()));
			out.sputc('\n');
		};
		thread = std::thread([this, o] {
			auto r = line_server_run(o);
			if (r.has_value()) value = r.value();
			std::ostringstream os;
			r.report().print(os, term::colors(false));
			report_text = os.str();
			done = true;
		});
	}
	~server_thread() { stop(); }

	// The output lines once @p n "listening" lines are there.
	std::vector<std::string> wait_listening(size_t n) {
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(10);
		for (;;) {
			std::vector<std::string> lines, hits;
			std::istringstream is(out.text());
			for (std::string l; std::getline(is, l); ) {
				lines.push_back(l);
				if (l.find("listening") != std::string::npos)
					hits.push_back(l);
			}
			if (hits.size() >= n || done
					|| std::chrono::steady_clock::now() > deadline)
				return lines;
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

	// The port of the first tcp address, from the json listening line.
	uint16_t tcp_port() {
		for (const auto& l : wait_listening(1)) {
			auto v = jt::get(std::string_view(l));
			if (!v.has_value()) continue;
			tref li = jt::get(v.value()).find("listening");
			if (!li) continue;
			tref p = jt::get(li).find("port");
			if (p) return static_cast<uint16_t>(jt::get(p).get_number());
		}
		FAIL("no listening port");
		return 0;
	}

	// A stop before the run starts would be lost, so it repeats.
	void stop() {
		if (!thread.joinable()) return;
		while (!done) {
			line_server_stop();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		thread.join();
	}
};

static line_server_options tcp_options() {
	line_server_options o;
	o.listen = { { line_transport_kind::tcp, "", 0 } };
	o.json = true;
	return o;
}

static tcp::endpoint loopback(uint16_t port) {
	return { ba::ip::make_address("127.0.0.1"), port };
}

static bool wait_for(const std::atomic<int>& v, int want) {
	auto deadline = std::chrono::steady_clock::now()
		+ std::chrono::seconds(10);
	while (v.load() != want) {
		if (std::chrono::steady_clock::now() > deadline) return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	return true;
}

static bool has(const std::string& line, std::string_view part) {
	return line.find(part) != std::string::npos;
}

TEST_SUITE("line reader") {
	TEST_CASE("feed_lines gives each line and drops a line over the limit") {
		std::string line;
		bool dropping = false;
		std::vector<std::string> got;
		const char* data = "ab\ncdefgh\nij\nk";
		feed_lines(line, dropping, 4, data, std::strlen(data),
			[&](std::string& l) { got.push_back(l); },
			[&] { got.emplace_back("<too long>"); });
		CHECK(got == std::vector<std::string>{ "ab", "<too long>", "ij" });
		CHECK(line == "k");
	}
}

TEST_SUITE("line server: tcp") {
	TEST_CASE("an echo session: the open answer and two lines") {
		server_thread s(tcp_options());
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"id\":1,\"cmd\":\"open\"}");
		CHECK(c.read_line() == "{\"id\":1,\"cmd\":\"open\",\"status\":\"ok\"}");
		c.send("{\"id\":2,\"cmd\":\"a\"}");
		c.send("{\"id\":3,\"cmd\":\"b\"}");
		CHECK(c.read_line() == "{\"id\":2,\"cmd\":\"a\",\"status\":\"ok\"}");
		CHECK(c.read_line() == "{\"id\":3,\"cmd\":\"b\",\"status\":\"ok\"}");
	}
	TEST_CASE("authenticate refuses: one error line, then the close") {
		auto o = tcp_options();
		o.authenticate = [](tref, const peer_info&) {
			return false;
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\",\"token\":\"x\"}");
		const std::string l = c.read_line();
		CHECK(has(l, "\"status\":\"error\""));
		CHECK(has(l, "Login refused"));
		CHECK(c.at_eof(5000));
		CHECK(s.closes.load() == 0);
	}
	TEST_CASE("authenticate accepts: the handler gets on_open") {
		auto o = tcp_options();
		std::atomic<bool> seen{ false };
		o.authenticate = [&seen](tref first, const peer_info& peer)
		{
			seen = peer.kind == line_transport_kind::tcp
				&& jt::get(first).find("token");
			return true;
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\",\"token\":\"x\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		CHECK(seen.load());
	}
	TEST_CASE("a line over max_line: the error, the next line works") {
		auto o = tcp_options();
		o.max_line = 32;
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		c.send("{\"cmd\":\"" + std::string(100, 'x') + "\"}");
		CHECK(has(c.read_line(), "Line is longer than the limit"));
		c.send("{\"id\":2}");
		CHECK(c.read_line() == "{\"id\":2,\"status\":\"ok\"}");
	}
	TEST_CASE("a line that is no JSON: the error, the connection stays") {
		server_thread s(tcp_options());
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		c.send("no json");
		CHECK(has(c.read_line(), "\"status\":\"error\""));
		c.send("{\"id\":3}");
		CHECK(c.read_line() == "{\"id\":3,\"status\":\"ok\"}");
	}
	TEST_CASE("on_close runs once when the client closes") {
		server_thread s(tcp_options());
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		c.close();
		CHECK(wait_for(s.closes, 1));
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		s.stop();
		CHECK(s.closes.load() == 1);
	}
	TEST_CASE("a stop with one open connection: on_close, the value 0") {
		server_thread s(tcp_options());
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		s.stop();
		CHECK(s.closes.load() == 1);
		REQUIRE(s.value.has_value());
		CHECK(*s.value == 0);
		CHECK(c.at_eof(5000));
	}
	TEST_CASE("no make_handler is an error") {
		line_server_options o = tcp_options();
		std::ostringstream sink;
		auto old = std::cout.rdbuf(sink.rdbuf());
		auto r = line_server_run(o);
		std::cout.rdbuf(old);
		CHECK_FALSE(r.has_value());
	}
	TEST_CASE("a held port: the error has the port and names no option") {
		ba::io_context io;
		tcp::acceptor held(io, loopback(0));
		const uint16_t port = held.local_endpoint().port();
		line_server_options o = tcp_options();
		o.listen = { { line_transport_kind::tcp, "", port } };
		o.make_handler = [](const line_session&) {
			return std::unique_ptr<session_handler>{};
		};
		std::ostringstream sink;
		auto old = std::cout.rdbuf(sink.rdbuf());
		auto r = line_server_run(o);
		std::cout.rdbuf(old);
		REQUIRE_FALSE(r.has_value());
		const auto& rep = r.report();
		bool found = false;
		for (size_t i = 0; i < rep.nodes().size(); ++i) {
			const auto& n = rep.nodes()[i];
			for (uint8_t k = 0; k < n.attr_cnt; ++k) {
				const auto& a = rep.attrs()[n.attr_off + k];
				if (a.key != parser_strings::label::port) continue;
				found = true;
				CHECK(a.value == port);
				CHECK(n.tag == diagnostics::code::io_error);
				const std::string msg = rep.format_message(i);
				CHECK(has(msg, "Port is in use"));
				CHECK_FALSE(has(msg, "-p"));
			}
		}
		CHECK(found);
	}
}

// Asks for the close in on_line and records whether on_close ran inside.
struct closing_handler : session_handler {
	explicit closing_handler(std::atomic<int>& r) : result(r) {}
	std::atomic<int>& result;
	bool inside = false;
	void on_line(tref, reply_sink& out) override {
		inside = true;
		out.close();
		inside = false;
	}
	void on_close() override { result = inside ? 1 : 2; }
};

TEST_SUITE("line server: limits") {
	TEST_CASE("a close asked in on_line runs on_close after on_line") {
		std::atomic<int> result{ 0 };
		auto o = tcp_options();
		o.make_handler = [&result](const line_session&) {
			return std::make_unique<closing_handler>(result);
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		c.send("{\"cmd\":\"bye\"}");
		CHECK(c.at_eof(5000));
		CHECK(wait_for(result, 2));
	}
#ifndef _WIN32
	TEST_CASE("the run gives the SIGPIPE handler of the program back") {
		auto mine = +[](int) {};
		auto before = std::signal(SIGPIPE, mine);
		{
			server_thread s(tcp_options());
			s.tcp_port();
		}
		CHECK(std::signal(SIGPIPE, before) == mine);
	}
#endif
}

TEST_SUITE("line server: start and stop") {
	TEST_CASE("the JSON listening line names the bound port, a stop gives 0") {
		server_thread s(tcp_options());
		const uint16_t port = s.tcp_port();
		REQUIRE(port != 0);
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		CHECK(lines[0] == "{\"listening\":{\"kind\":\"tcp\",\"port\":"
			+ std::to_string(port) + "}}");
		tcp_client c(loopback(port));
		s.stop();
		REQUIRE(s.value.has_value());
		CHECK(*s.value == 0);
	}
	TEST_CASE("the text listening line names the address and the port") {
		auto o = tcp_options();
		o.json = false;
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		CHECK(has(lines[0], " listening on 127.0.0.1:"));
	}
}
