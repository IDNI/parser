// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "net/line_reader.h"
#include "net/line_server.h"
#include "net/line_server_asio.h"
#include "net/line_transport.h"
#include "net/line_client_test.h"

#include <atomic>
#include <csignal>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

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

// Keeps the sink of on_open and answers each line 50 ms later, from a
// timer on the io loop of the server.
struct deferred_handler : session_handler {
	explicit deferred_handler(std::atomic<int>& c) : closes(c) {}
	std::atomic<int>& closes;
	reply_sink* sink = nullptr;
	ba::steady_timer timer{ line_server_io() };
	void on_open(tref first_line, reply_sink& out) override {
		sink = &out;
		out.write(jt::set_member(first_line, "status",
			jt::build_string("ok")));
	}
	void on_line(tref, reply_sink&) override {
		timer.expires_after(std::chrono::milliseconds(50));
		timer.async_wait([this](boost::system::error_code ec) {
			if (ec) return;
			sink->write(jt::build_object({ jt::build_pair("deferred",
				jt::build_string("yes")) }));
		});
	}
	void on_close() override {
		timer.cancel();
		sink = nullptr;
		++closes;
	}
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

// A fresh folder name under the temp folder. The folder does not exist.
static std::filesystem::path unique_temp_dir(const std::string& prefix) {
	auto dir = std::filesystem::temp_directory_path() / (prefix
		+ std::to_string(std::chrono::steady_clock::now()
			.time_since_epoch().count()));
	std::filesystem::remove_all(dir);
	return dir;
}

#ifndef _WIN32
// The permission bits of @p p.
static unsigned mode_of(const std::filesystem::path& p) {
	return static_cast<unsigned>(std::filesystem::status(p).permissions())
		& 0777u;
}
#endif

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
	TEST_CASE("max_sessions 1: the second connection gets the busy answer") {
		auto o = tcp_options();
		o.max_sessions = 1;
		server_thread s(o);
		tcp_client c1(loopback(s.tcp_port()));
		c1.send("{\"cmd\":\"open\"}");
		CHECK(has(c1.read_line(), "\"status\":\"ok\""));
		tcp_client c2(loopback(s.tcp_port()));
		const std::string l = c2.read_line();
		CHECK(has(l, "\"status\":\"error\""));
		CHECK(has(l, o.busy_message));
		CHECK(c2.at_eof(5000));
		CHECK_FALSE(c1.at_eof(200));
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
	TEST_CASE("on_stop runs once at the stop, after on_close") {
		std::atomic<int> stops{ 0 };
		std::atomic<int> closes_at_stop{ -1 };
		auto o = tcp_options();
		std::atomic<int>* closes = nullptr;
		o.on_stop = [&] {
			closes_at_stop = closes->load();
			++stops;
		};
		server_thread s(o);
		closes = &s.closes;
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		CHECK(stops.load() == 0);
		s.stop();
		CHECK(stops.load() == 1);
		CHECK(closes_at_stop.load() == 1);
	}
	TEST_CASE("the listening line as text") {
		auto o = tcp_options();
		o.json = false;
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		REQUIRE(lines.size() >= 2);
		CHECK(has(lines[0], " listening on 127.0.0.1:"));
		CHECK(has(lines[1], " logs off"));
	}
	TEST_CASE("a handler keeps the sink and answers from a timer") {
		auto o = tcp_options();
		std::atomic<int> closes{ 0 };
		o.make_handler = [&closes](const line_session&) {
			return std::make_unique<deferred_handler>(closes);
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		c.send("{\"id\":2}");
		CHECK(c.read_line() == "{\"deferred\":\"yes\"}");
		c.close();
		CHECK(wait_for(closes, 1));
	}
	TEST_CASE("a text reply goes out as it is and into the log") {
		struct text_handler : session_handler {
			void on_line(tref, reply_sink& out) override {
				// not compact, so a parse and a print would change it
				out.write(std::string("{\"a\" : 1}"));
			}
		};
		auto o = tcp_options();
		o.debug_sessions = true;
		o.make_handler = [](const line_session&) {
			return std::make_unique<text_handler>();
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		c.send("{\"id\":2}");
		CHECK(c.read_line() == "{\"a\" : 1}");
		c.close();
		s.stop();
		CHECK(has(s.out.text(), "\"dir\":\"out\""));
		CHECK(has(s.out.text(), "{\\\"a\\\" : 1}"));
	}
	TEST_CASE("handler_logs: no session event, no session log") {
		const auto dir = std::filesystem::temp_directory_path()
			/ ("line_server_logs_" + std::to_string(
				std::chrono::steady_clock::now()
					.time_since_epoch().count()));
		std::filesystem::remove_all(dir);
		auto o = tcp_options();
		o.json = false;
		o.log_dir = dir.string();
		o.debug_sessions = true;
		o.handler_logs = true;
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		uint16_t port = 0;
		const auto at = lines[0].rfind(':');
		REQUIRE(at != std::string::npos);
		port = static_cast<uint16_t>(std::stoi(lines[0].substr(at + 1)));
		{
			tcp_client c(loopback(port));
			c.send("{\"cmd\":\"open\"}");
			CHECK(has(c.read_line(), "\"status\":\"ok\""));
			c.send("{\"id\":2}");
			CHECK(c.read_line() == "{\"id\":2,\"status\":\"ok\"}");
		}
		CHECK(wait_for(s.closes, 1));
		s.stop();
		std::istringstream is(s.out.text());
		size_t listening = 0;
		for (std::string l; std::getline(is, l); ) {
			if (has(l, " listening on ")) ++listening;
			CHECK_FALSE(has(l, " session "));
		}
		CHECK(listening == 1);
		std::vector<std::string> files;
		for (const auto& e : std::filesystem::directory_iterator(dir))
			files.push_back(e.path().filename().string());
		CHECK(files == std::vector<std::string>{ "server.log" });
		std::filesystem::remove_all(dir);
	}
	TEST_CASE("a line that does not parse is logged raw") {
		const auto dir = std::filesystem::temp_directory_path()
			/ ("line_server_raw_" + std::to_string(
				std::chrono::steady_clock::now()
					.time_since_epoch().count()));
		std::filesystem::remove_all(dir);
		auto o = tcp_options();
		o.log_dir = dir.string();
		server_thread s(o);
		{
			tcp_client c(loopback(s.tcp_port()));
			c.send("{\"cmd\":\"hi\"}");
			CHECK(has(c.read_line(), "\"status\":\"ok\""));
			c.send("{\"x\":");
			CHECK(has(c.read_line(), "Unexpected end of file at 1:6 (6)"));
		}
		CHECK(wait_for(s.closes, 1));
		s.stop();
		std::string session_log;
		for (const auto& e : std::filesystem::directory_iterator(dir))
			if (e.path().filename() != "server.log") {
				std::ifstream is(e.path());
				session_log.assign(std::istreambuf_iterator<char>(is), {});
			}
		CHECK(has(session_log, R"("line":"{\"x\":")"));
		CHECK_FALSE(has(session_log, "unparsed"));
		std::filesystem::remove_all(dir);
	}
	TEST_CASE("line_server_log with no running server prints and writes nothing") {
		std::ostringstream sink;
		auto old = std::cout.rdbuf(sink.rdbuf());
		line_server_log(jt::build_object({ jt::build_pair(
			"event", jt::build_string("lost")) }));
		std::cout.rdbuf(old);
		CHECK(sink.str().empty());
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
	TEST_CASE("line_server_log: the event goes into server.log beside the server events") {
		struct logging_handler : session_handler {
			void on_line(tref, reply_sink& out) override {
				line_server_log(jt::build_object({ jt::build_pair(
					"event", jt::build_string("handler-event")) }));
				out.write(std::string("{\"status\":\"ok\"}"));
			}
		};
		const auto dir = unique_temp_dir("line_server_log_call_");
		auto o = tcp_options();
		o.log_dir = dir.string();
		o.make_handler = [](const line_session&) {
			return std::make_unique<logging_handler>();
		};
		server_thread s(o);
		{
			tcp_client c(loopback(s.tcp_port()));
			c.send("{\"cmd\":\"open\"}");
			c.send("{\"id\":1}");
			CHECK(c.read_line() == "{\"status\":\"ok\"}");
		}
		s.stop();
		std::ifstream is(dir / "server.log");
		const std::string log((std::istreambuf_iterator<char>(is)), {});
		CHECK(has(log, "\"event\":\"new\""));
		CHECK(has(log, "\"event\":\"handler-event\""));
		CHECK(has(s.out.text(), "handler-event"));
		std::filesystem::remove_all(dir);
	}
#ifndef _WIN32
	TEST_CASE("logs: a new folder gets 0700, each log gets 0600") {
		const auto dir = unique_temp_dir("line_server_modes_");
		const auto sub = dir / "logs";
		auto o = tcp_options();
		o.log_dir = sub.string();
		server_thread s(o);
		{
			tcp_client c(loopback(s.tcp_port()));
			c.send("{\"cmd\":\"open\"}");
			CHECK(has(c.read_line(), "\"status\":\"ok\""));
		}
		CHECK(wait_for(s.closes, 1));
		s.stop();
		CHECK(mode_of(dir) == 0700);
		CHECK(mode_of(sub) == 0700);
		size_t logs = 0;
		for (const auto& e : std::filesystem::directory_iterator(sub)) {
			++logs;
			CHECK(mode_of(e.path()) == 0600);
		}
		CHECK(logs == 2);
		std::filesystem::remove_all(dir);
	}
	TEST_CASE("logs: a server.log with 0644 gets 0600, the folder keeps its mode") {
		const auto dir = unique_temp_dir("line_server_narrow_");
		std::filesystem::create_directories(dir);
		std::filesystem::permissions(dir, std::filesystem::perms(0755),
			std::filesystem::perm_options::replace);
		const auto log = dir / "server.log";
		std::ofstream(log) << "{}\n";
		std::filesystem::permissions(log, std::filesystem::perms(0644),
			std::filesystem::perm_options::replace);
		auto o = tcp_options();
		o.log_dir = dir.string();
		server_thread s(o);
		REQUIRE(s.wait_listening(1).size() >= 1);
		s.stop();
		CHECK(mode_of(log) == 0600);
		CHECK(mode_of(dir) == 0755);
		std::filesystem::remove_all(dir);
	}
#endif
}

static line_server_options ws_options(std::string ws_path = "/ws") {
	line_server_options o;
	o.listen = { { .kind = line_transport_kind::websocket, .port = 0,
		.ws_path = std::move(ws_path) } };
	o.json = true;
	return o;
}

static std::unique_ptr<line_transport> ws_connect(uint16_t port,
	size_t max_line = 0)
{
	auto t = line_connect({ .kind = line_transport_kind::websocket,
		.host = "127.0.0.1", .port = port, .ws_path = "/ws" },
		{ .max_line = max_line, .timeout_seconds = 10 });
	REQUIRE(t.has_value());
	return std::move(t.value());
}

// A raw upgrade request, so the test sees the HTTP answer of a refusal.
static std::string upgrade_request(std::string_view target,
	std::string_view origin)
{
	std::string r = "GET " + std::string(target) + " HTTP/1.1\r\n"
		"Host: 127.0.0.1\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
		"Sec-WebSocket-Version: 13\r\n";
	if (!origin.empty()) r += "Origin: " + std::string(origin) + "\r\n";
	return r + "\r\n";
}

// The rest of an HTTP answer, up to the close of the server.
// One masked text frame of up to 125 bytes. The mask key 0 leaves the
// payload as it is.
static std::string text_frame(const std::string& payload) {
	REQUIRE(payload.size() <= 125);
	return std::string{ char(0x81), char(0x80 | payload.size()),
		0, 0, 0, 0 } + payload;
}

// Answers each line with the request and a large "pad" member.
struct large_reply_handler : session_handler {
	explicit large_reply_handler(std::atomic<int>& c) : closes(c) {}
	std::atomic<int>& closes;
	void on_line(tref request, reply_sink& out) override {
		out.write(jt::set_member(request, "pad",
			jt::build_string(std::string(64 * 1024, 'x'))));
	}
	void on_close() override { ++closes; }
};

static std::string read_to_close(tcp_client& c) {
	const auto deadline = std::chrono::steady_clock::now()
		+ std::chrono::seconds(10);
	while (c.read_until(deadline)) {}
	return c.in_buf;
}

TEST_SUITE("line server: websocket") {
	TEST_CASE("an echo session: the listening line, the open answer and two lines") {
		server_thread s(ws_options());
		const uint16_t port = s.tcp_port();
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		CHECK(lines[0] == "{\"listening\":{\"kind\":\"websocket\",\"port\":"
			+ std::to_string(port) + ",\"path\":\"/ws\"}}");
		auto t = ws_connect(port);
		std::string l;
		CHECK(t->send_line("{\"id\":1,\"cmd\":\"open\"}"));
		REQUIRE(t->read_line(l) == read_status::ok);
		CHECK(l == "{\"id\":1,\"cmd\":\"open\",\"status\":\"ok\"}");
		CHECK(t->send_line("{\"id\":2,\"cmd\":\"a\"}"));
		CHECK(t->send_line("{\"id\":3,\"cmd\":\"b\"}"));
		REQUIRE(t->read_line(l) == read_status::ok);
		CHECK(l == "{\"id\":2,\"cmd\":\"a\",\"status\":\"ok\"}");
		REQUIRE(t->read_line(l) == read_status::ok);
		CHECK(l == "{\"id\":3,\"cmd\":\"b\",\"status\":\"ok\"}");
		t->close();
		CHECK(wait_for(s.closes, 1));
	}
	TEST_CASE("the listening line as text") {
		auto o = ws_options();
		o.json = false;
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		CHECK(has(lines[0], " listening on ws://127.0.0.1:"));
		CHECK(has(lines[0], "/ws"));
	}
	TEST_CASE("an allowed origin passes, the handler sees it") {
		auto o = ws_options();
		o.allowed_origins = { "http://good.example" };
		std::atomic<bool> seen{ false };
		o.authenticate = [&seen](tref, const peer_info& peer) {
			seen = peer.kind == line_transport_kind::websocket
				&& peer.origin == "http://good.example";
			return true;
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send_raw(upgrade_request("/ws", "http://good.example"));
		CHECK(c.read_line() == "HTTP/1.1 101 Switching Protocols");
		c.send_raw(text_frame("{\"cmd\":\"open\"}"));
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(10);
		while (!seen && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		CHECK(seen.load());
	}
	TEST_CASE("an origin that is not allowed gets 403 and the close") {
		auto o = ws_options();
		o.allowed_origins = { "http://good.example" };
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send_raw(upgrade_request("/ws", "http://evil.example"));
		CHECK(c.read_line() == "HTTP/1.1 403 Forbidden");
		CHECK(has(read_to_close(c), "The WebSocket origin is not allowed"));
		CHECK(c.eof_seen);
	}
	TEST_CASE("accept_origin decides the origin in place of the list") {
		auto o = ws_options();
		o.allowed_origins = { "http://listed.example" };
		std::vector<std::string> asked;
		std::mutex m;
		// localhost with any port, as a node accepts the page of a wallet
		o.accept_origin = [&](std::string_view origin) {
			std::lock_guard g(m);
			asked.emplace_back(origin);
			return origin.starts_with("http://localhost:");
		};
		server_thread s(o);
		for (const char* port : { "8000", "51234" }) {
			tcp_client c(loopback(s.tcp_port()));
			c.send_raw(upgrade_request("/ws",
				std::string("http://localhost:") + port));
			CHECK(c.read_line() == "HTTP/1.1 101 Switching Protocols");
		}
		for (const char* origin : { "http://listed.example",
				"http://evil.example" })
		{
			tcp_client c(loopback(s.tcp_port()));
			c.send_raw(upgrade_request("/ws", origin));
			CHECK(c.read_line() == "HTTP/1.1 403 Forbidden");
		}
		tcp_client none(loopback(s.tcp_port()));
		none.send_raw(upgrade_request("/ws", ""));
		CHECK(none.read_line() == "HTTP/1.1 101 Switching Protocols");
		std::lock_guard g(m);
		CHECK(asked == std::vector<std::string>{ "http://localhost:8000",
			"http://localhost:51234", "http://listed.example",
			"http://evil.example" });
	}
	TEST_CASE("origin_refusal_message: the upgrade, one message, the close") {
		auto o = ws_options();
		o.allowed_origins = { "http://good.example" };
		o.origin_refusal_message = "error disallowed_origin";
		std::atomic<int> sessions{ 0 };
		o.make_handler = [&sessions](const line_session&) {
			++sessions;
			return std::make_unique<session_handler>();
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send_raw(upgrade_request("/ws", "http://evil.example"));
		CHECK(c.read_line() == "HTTP/1.1 101 Switching Protocols");
		const auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(10);
		while (c.in_buf.find(char(0x88)) == std::string::npos
			&& c.read_until(deadline)) {}
		// a browser answers the close frame with its own
		c.send_raw(std::string{ char(0x88), char(0x80), 0, 0, 0, 0 });
		const std::string rest = read_to_close(c);
		const std::string text = "error disallowed_origin";
		// the header of one unmasked text frame, then the text
		CHECK(has(rest, std::string{ char(0x81), char(text.size()) } + text));
		const size_t close_at = rest.find(char(0x88));
		REQUIRE(close_at != std::string::npos);
		CHECK(close_at > rest.find(text));
		CHECK(c.eof_seen);
		tcp_client good(loopback(s.tcp_port()));
		good.send_raw(upgrade_request("/ws", "http://good.example"));
		CHECK(good.read_line() == "HTTP/1.1 101 Switching Protocols");
		good.send_raw(text_frame("{\"cmd\":\"open\"}"));
		CHECK(wait_for(sessions, 1));
		CHECK(sessions.load() == 1);
	}
	TEST_CASE("an empty allowed_origins refuses every browser origin") {
		server_thread s(ws_options());
		tcp_client c(loopback(s.tcp_port()));
		c.send_raw(upgrade_request("/ws", "http://127.0.0.1"));
		CHECK(c.read_line() == "HTTP/1.1 403 Forbidden");
	}
	TEST_CASE("a target that is not ws_path gets 404") {
		server_thread s(ws_options());
		tcp_client c(loopback(s.tcp_port()));
		c.send_raw(upgrade_request("/other", ""));
		CHECK(c.read_line() == "HTTP/1.1 404 Not Found");
		read_to_close(c);
		CHECK(c.eof_seen);
	}
	TEST_CASE("a client that never reads: the write timer closes it") {
		auto o = ws_options();
		o.write_timeout_seconds = 1;
		std::atomic<int> closes{ 0 };
		o.make_handler = [&closes](const line_session&) {
			return std::make_unique<large_reply_handler>(closes);
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send_raw(upgrade_request("/ws", ""));
		CHECK(c.read_line() == "HTTP/1.1 101 Switching Protocols");
		c.send_raw(text_frame("{\"cmd\":\"open\"}"));
		// 256 answers of 64 KiB fill the socket buffers and the output
		// queue, so the writes stall.
		std::string frames;
		for (int i = 0; i != 256; ++i)
			frames += text_frame("{\"id\":" + std::to_string(i) + "}");
		c.send_raw(frames);
		CHECK(wait_for(closes, 1));
	}
	TEST_CASE("a message over max_line closes the connection") {
		auto o = ws_options();
		o.max_line = 32;
		server_thread s(o);
		auto t = ws_connect(s.tcp_port());
		std::string l;
		CHECK(t->send_line("{\"cmd\":\"open\"}"));
		REQUIRE(t->read_line(l) == read_status::ok);
		CHECK(has(l, "\"status\":\"ok\""));
		t->send_line("{\"cmd\":\"" + std::string(100, 'x') + "\"}");
		CHECK(t->read_line(l) == read_status::closed);
		CHECK(wait_for(s.closes, 1));
	}
}

#ifndef _WIN32
TEST_SUITE("line server: local socket") {
	using line_client_test::local_client;
	using local = ba::local::stream_protocol;

	static std::string socket_path(const char* tag) {
		return (std::filesystem::temp_directory_path()
			/ (std::string("line_server_") + tag + "_"
				+ std::to_string(::getpid()) + ".sock")).string();
	}

	TEST_CASE("an echo session, the mode 0600 and the listening lines") {
		const std::string path = socket_path("echo");
		std::filesystem::remove(path);
		line_server_options o;
		o.listen = { { line_transport_kind::local_socket, path, 0 },
			{ line_transport_kind::tcp, "", 0 } };
		o.json = true;
		server_thread s(o);
		const auto lines = s.wait_listening(2);
		REQUIRE(lines.size() >= 2);
		CHECK(lines[0] == "{\"listening\":{\"kind\":\"local_socket\","
			"\"path\":\"" + path + "\"}}");
		CHECK(has(lines[1], "{\"listening\":{\"kind\":\"tcp\",\"port\":"));
		struct stat st{};
		REQUIRE(::stat(path.c_str(), &st) == 0);
		CHECK((st.st_mode & 0777) == 0600);
		{
			local_client c{ local::endpoint(path) };
			c.send("{\"id\":1,\"cmd\":\"open\"}");
			CHECK(c.read_line()
				== "{\"id\":1,\"cmd\":\"open\",\"status\":\"ok\"}");
			c.send("{\"id\":2}");
			CHECK(c.read_line() == "{\"id\":2,\"status\":\"ok\"}");
		}
		s.stop();
		CHECK_FALSE(std::filesystem::exists(path));
	}
	TEST_CASE("the listening line of a socket as text") {
		const std::string path = socket_path("text");
		line_server_options o;
		o.listen = { { line_transport_kind::local_socket, path, 0 } };
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		CHECK(has(lines[0], " listening on " + path));
	}
	TEST_CASE("a stale socket file goes, a live server keeps its file") {
		const std::string path = socket_path("stale");
		std::filesystem::remove(path);
		{
			// a bound socket that nobody accepts on leaves the file
			ba::io_context io;
			local::acceptor a(io, local::endpoint(path));
		}
		REQUIRE(std::filesystem::exists(path));
		line_server_options o;
		o.listen = { { line_transport_kind::local_socket, path, 0 } };
		o.json = true;
		server_thread s(o);
		REQUIRE(s.wait_listening(1).size() >= 1);
		line_server_options o2 = o;
		o2.make_handler = [](const line_session&) {
			return std::unique_ptr<session_handler>{};
		};
		std::ostringstream sink;
		auto old = std::cout.rdbuf(sink.rdbuf());
		auto r = line_server_run(o2);
		std::cout.rdbuf(old);
		CHECK_FALSE(r.has_value());
	}
	TEST_CASE("a path over the system limit names the path") {
		const std::string path = "/tmp/" + std::string(200, 'x');
		line_server_options o;
		o.listen = { { line_transport_kind::local_socket, path, 0 } };
		o.make_handler = [](const line_session&) {
			return std::unique_ptr<session_handler>{};
		};
		std::ostringstream sink;
		auto old = std::cout.rdbuf(sink.rdbuf());
		auto r = line_server_run(o);
		std::cout.rdbuf(old);
		REQUIRE_FALSE(r.has_value());
		std::ostringstream os;
		r.report().print(os, term::colors(false));
		CHECK(has(os.str(), "over the system limit"));
		CHECK(has(os.str(), path));
	}
}
#endif

#ifdef _WIN32
TEST_SUITE("line server: named pipe") {
	static std::string pipe_name(const char* tag) {
		return std::string("\\\\.\\pipe\\line_server_") + tag + "_"
			+ std::to_string(::GetCurrentProcessId());
	}

	TEST_CASE("an echo session, then a second client on the same pipe") {
		const std::string name = pipe_name("echo");
		line_server_options o;
		o.listen = { { line_transport_kind::local_socket, name, 0 } };
		o.json = true;
		server_thread s(o);
		REQUIRE_FALSE(s.wait_listening(1).empty());
		for (int i = 0; i != 2; ++i) {
			auto t = line_connect({ .kind = line_transport_kind::local_socket,
				.path = name }, { .timeout_seconds = 10 });
			REQUIRE(t.has_value());
			auto& link = *t.value();
			std::string l;
			CHECK(link.send_line("{\"id\":1,\"cmd\":\"open\"}"));
			REQUIRE(link.read_line(l) == read_status::ok);
			CHECK(l == "{\"id\":1,\"cmd\":\"open\",\"status\":\"ok\"}");
			link.close();
		}
		CHECK(wait_for(s.closes, 2));
	}

	TEST_CASE("a pipe name that another process holds is refused") {
		const std::string name = pipe_name("held");
		HANDLE held = ::CreateNamedPipeA(name.c_str(),
			PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES, 512, 512, 0, nullptr);
		REQUIRE(held != INVALID_HANDLE_VALUE);
		line_server_options o;
		o.listen = { { line_transport_kind::local_socket, name, 0 } };
		o.make_handler = [](const line_session&) {
			return std::unique_ptr<session_handler>{};
		};
		auto r = line_server_run(o);
		::CloseHandle(held);
		CHECK_FALSE(r.has_value());
		std::ostringstream os;
		r.report().print(os, term::colors(false));
		CHECK(has(os.str(), name));
	}
}
#endif

// The token check of a program.
static void token_login(line_server_options& o, const std::string& token) {
	o.authenticate = [&token](tref first, const peer_info&) {
		const std::string* t = format::json::string_member(first, "token");
		return t && *t == token;
	};
}

static line_server_options login_options(const std::string& token) {
	auto o = tcp_options();
	o.logins = { .max_failures = 3, .window = std::chrono::seconds(1),
		.wait = std::chrono::seconds(1) };
	token_login(o, token);
	return o;
}

// A first line with @p token, and the answer of the server.
template <typename Client>
static std::string login(Client& c, std::string_view token) {
	c.send("{\"cmd\":\"open\",\"token\":\"" + std::string(token) + "\"}");
	return c.read_line();
}

static void fail_logins(uint16_t port, int count) {
	for (int i = 0; i != count; ++i) {
		tcp_client c(loopback(port));
		CHECK(has(login(c, "wrong"), "Login refused"));
		CHECK(c.at_eof(5000));
	}
}

TEST_SUITE("line server: login limits") {
	TEST_CASE("three failures: the next connection waits, then a login passes") {
		const std::string token = "s3cret";
		server_thread s(login_options(token));
		const uint16_t port = s.tcp_port();
		fail_logins(port, 3);
		{
			tcp_client c(loopback(port));
			const std::string l = c.read_line();
			CHECK(has(l, "\"status\":\"error\""));
			CHECK(has(l, "Too many failed logins"));
			CHECK(c.at_eof(5000));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1200));
		tcp_client c(loopback(port));
		CHECK(has(login(c, "s3cret"), "\"status\":\"ok\""));
		const std::string out = s.out.text();
		CHECK(has(out, "\"event\":\"login-refused\""));
		CHECK(has(out, "\"event\":\"login-wait\""));
		CHECK(has(out, "\"kind\":\"tcp\""));
	}
	TEST_CASE("connections opened before the limit cannot guess after it") {
		const std::string token = "s3cret";
		server_thread s(login_options(token));
		const uint16_t port = s.tcp_port();
		std::vector<std::unique_ptr<tcp_client>> open;
		for (size_t i = 0; i != 4; ++i)
			open.push_back(std::make_unique<tcp_client>(loopback(port)));
		for (size_t i = 0; i != 3; ++i)
			CHECK(has(login(*open[i], "wrong"), "Login refused"));
		const std::string l = login(*open[3], "s3cret");
		CHECK(has(l, "\"status\":\"error\""));
		CHECK(has(l, "Too many failed logins"));
		CHECK(open[3]->at_eof(5000));
	}
	TEST_CASE("max_failures 0 never refuses") {
		const std::string token = "s3cret";
		auto o = login_options(token);
		o.logins.max_failures = 0;
		server_thread s(o);
		const uint16_t port = s.tcp_port();
		fail_logins(port, 5);
		tcp_client c(loopback(port));
		CHECK(has(login(c, "s3cret"), "\"status\":\"ok\""));
	}
	TEST_CASE("the events as text name the kind") {
		const std::string token = "s3cret";
		auto o = login_options(token);
		o.json = false;
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		const auto at = lines[0].rfind(':');
		REQUIRE(at != std::string::npos);
		const auto port = static_cast<uint16_t>(
			std::stoi(lines[0].substr(at + 1)));
		fail_logins(port, 3);
		{
			tcp_client c(loopback(port));
			CHECK(has(c.read_line(), "Too many failed logins"));
		}
		const std::string out = s.out.text();
		CHECK(has(out, " login-refused tcp\n"));
		CHECK(has(out, " login-wait tcp\n"));
	}
#ifndef _WIN32
	TEST_CASE("the tcp counter does not block the local socket") {
		const std::string token = "s3cret";
		const std::string path = (std::filesystem::temp_directory_path()
			/ ("line_server_login_" + std::to_string(::getpid())
				+ ".sock")).string();
		std::filesystem::remove(path);
		auto o = login_options(token);
		o.listen = { { line_transport_kind::local_socket, path, 0 },
			{ line_transport_kind::tcp, "", 0 } };
		server_thread s(o);
		REQUIRE(s.wait_listening(2).size() >= 2);
		const uint16_t port = s.tcp_port();
		fail_logins(port, 3);
		{
			tcp_client c(loopback(port));
			CHECK(has(c.read_line(), "Too many failed logins"));
		}
		line_client_test::local_client c{
			ba::local::stream_protocol::endpoint(path) };
		CHECK(has(login(c, "s3cret"), "\"status\":\"ok\""));
	}
#endif
}

// Answers each line with a JSON string of @p size bytes and counts the lines.
struct big_reply_handler : session_handler {
	big_reply_handler(std::atomic<int>& l, size_t size)
		: lines(l), reply("\"" + std::string(size, 'x') + "\"") {}
	std::atomic<int>& lines;
	std::string reply;
	void on_line(tref, reply_sink& out) override {
		++lines;
		out.write(reply);
	}
};

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

// Answers each line from a thread of its own through line_server_post.
struct posting_handler : session_handler {
	std::shared_ptr<std::atomic<reply_sink*>> sink
		= std::make_shared<std::atomic<reply_sink*>>(nullptr);
	std::vector<std::thread> workers;
	void on_open(tref, reply_sink& out) override {
		*sink = &out;
	}
	void on_line(tref, reply_sink&) override {
		workers.emplace_back([s = sink] {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			line_server_post([s] {
				if (reply_sink* o = s->load()) o->write(
					jt::build_object({ jt::build_pair("posted",
						jt::build_string("yes")) }));
			});
		});
	}
	void on_close() override { *sink = nullptr; }
	~posting_handler() override {
		for (auto& w : workers) w.join();
	}
};

// Records the events that the server gives with handler_logs.
struct event_handler : session_handler {
	explicit event_handler(std::atomic<int>& e) : events(e) {}
	std::atomic<int>& events;
	void on_server_event(tref ev) override {
		const std::string* name = format::json::string_member(ev, "event");
		if (name && *name == "line-too-long") ++events;
	}
};

static std::string binary_frame(const std::string& payload) {
	REQUIRE(payload.size() <= 125);
	return std::string{ char(0x82), char(0x80 | payload.size()),
		0, 0, 0, 0 } + payload;
}

TEST_SUITE("line server: limits") {
	TEST_CASE("a connection with no first line is closed after the timeout") {
		auto o = tcp_options();
		o.first_line_timeout_seconds = 1;
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		CHECK(c.at_eof(5000));
		CHECK(has(s.out.text(), "\"event\":\"first-line-timeout\""));
	}
	TEST_CASE("max_pending: a silent connection takes the place") {
		auto o = tcp_options();
		o.max_pending = 1;
		server_thread s(o);
		const uint16_t port = s.tcp_port();
		tcp_client first(loopback(port));
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		tcp_client second(loopback(port));
		CHECK(has(second.read_line(), "the server takes no more sessions"));
		CHECK(second.at_eof(5000));
		first.send("{\"cmd\":\"open\"}");
		CHECK(has(first.read_line(), "\"status\":\"ok\""));
		tcp_client third(loopback(port));
		third.send("{\"cmd\":\"open\"}");
		CHECK(has(third.read_line(), "\"status\":\"ok\""));
	}
	TEST_CASE("max_pending counts a websocket upgrade in flight") {
		auto o = ws_options();
		o.max_pending = 1;
		server_thread s(o);
		const uint16_t port = s.tcp_port();
		tcp_client first(loopback(port));
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		tcp_client second(loopback(port));
		second.send_raw(upgrade_request("/ws", ""));
		CHECK(second.read_line() == "HTTP/1.1 503 Service Unavailable");
	}
	TEST_CASE("idle_timeout_seconds ends a quiet session") {
		auto o = tcp_options();
		o.idle_timeout_seconds = 1;
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		CHECK(c.at_eof(5000));
		CHECK(has(s.out.text(), "\"event\":\"idle-timeout\""));
	}
	TEST_CASE("close_timeout_seconds bounds the close handshake") {
		for (const bool close_time : { true, false }) {
			CAPTURE(close_time);
			auto o = ws_options();
			o.idle_timeout_seconds = 1;
			o.upgrade_timeout_seconds = close_time ? 30 : 4;
			if (close_time) o.close_timeout_seconds = 1;
			server_thread s(o);
			tcp_client c(loopback(s.tcp_port()));
			c.send_raw(upgrade_request("/ws", ""));
			CHECK(c.read_line() == "HTTP/1.1 101 Switching Protocols");
			c.send_raw(text_frame("{\"cmd\":\"open\"}"));
			// The client never answers the close frame of the idle end.
			const auto start = std::chrono::steady_clock::now();
			const std::string rest = read_to_close(c);
			const auto took = std::chrono::steady_clock::now() - start;
			CHECK(c.eof_seen);
			CHECK(rest.find(char(0x88)) != std::string::npos);
			if (close_time) CHECK(took < std::chrono::seconds(4));
			else CHECK(took >= std::chrono::seconds(4));
		}
	}
	TEST_CASE("a client that does not read stops the read of its lines") {
		std::atomic<int> lines{ 0 };
		auto o = tcp_options();
		o.max_output_bytes = 64 * 1024;
		o.write_timeout_seconds = 0;
		o.make_handler = [&lines](const line_session&) {
			return std::make_unique<big_reply_handler>(lines,
				256 * 1024);
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		// The lines span many reads, and the pause holds between two reads.
		std::string many;
		const std::string pad(1000, 'p');
		for (int i = 0; i != 100; ++i)
			many += "{\"cmd\":\"x\",\"pad\":\"" + pad + "\"}\n";
		c.send_raw(many);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		CHECK(lines.load() < 100);
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(30);
		size_t got = 0;
		while (got != 100 && std::chrono::steady_clock::now() < deadline)
			if (!c.read_line(1000).empty()) ++got;
		CHECK(got == 100);
		CHECK(lines.load() == 100);
	}
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
	TEST_CASE("line_server_post answers from another thread") {
		auto o = tcp_options();
		o.make_handler = [](const line_session&) {
			return std::make_unique<posting_handler>();
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		c.send("{\"cmd\":\"work\"}");
		CHECK(c.read_line() == "{\"posted\":\"yes\"}");
	}
	TEST_CASE("line_server_post with no running server is false") {
		CHECK_FALSE(line_server_post([] {}));
	}
	TEST_CASE("handler_logs: a long line reaches the handler as an event") {
		std::atomic<int> events{ 0 };
		auto o = tcp_options();
		o.handler_logs = true;
		o.max_line = 16;
		o.make_handler = [&events](const line_session&) {
			return std::make_unique<event_handler>(events);
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		c.send(std::string(64, 'x'));
		CHECK(has(c.read_line(), "\"status\":\"error\""));
		CHECK(wait_for(events, 1));
		CHECK(has(s.out.text(), "\"event\":\"line-too-long\""));
	}
	TEST_CASE("a binary websocket message gets the error and the close") {
		server_thread s(ws_options());
		tcp_client c(loopback(s.tcp_port()));
		c.send_raw(upgrade_request("/ws", ""));
		CHECK(c.read_line() == "HTTP/1.1 101 Switching Protocols");
		c.send_raw(text_frame("{\"cmd\":\"open\"}"));
		c.send_raw(binary_frame("{\"cmd\":\"x\"}"));
		const std::string rest = read_to_close(c);
		CHECK(has(rest, "\"status\":\"error\""));
		// the close frame of the server
		CHECK(rest.find(char(0x88)) != std::string::npos);
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

// Answers each raw line with "got:" and the line, and counts on_line.
struct text_handler : session_handler {
	explicit text_handler(std::atomic<int>& j) : json_lines(j) {}
	std::atomic<int>& json_lines;
	void on_line(tref, reply_sink&) override { ++json_lines; }
	void on_text(std::string_view line, reply_sink& out) override {
		out.write("got:" + std::string(line));
	}
};

static line_server_options raw_options(line_server_options o,
	std::atomic<int>& json_lines)
{
	o.raw_lines = true;
	o.make_handler = [&json_lines](const line_session&) {
		return std::make_unique<text_handler>(json_lines);
	};
	return o;
}

TEST_SUITE("line server: raw lines") {
	TEST_CASE("raw_lines: no login line, each line reaches on_text as text") {
		std::atomic<int> json_lines{ 0 };
		server_thread s(raw_options(tcp_options(), json_lines));
		tcp_client c(loopback(s.tcp_port()));
		c.send("hello version=1");
		CHECK(c.read_line() == "got:hello version=1");
		c.send_raw("{\"id\":1}\r\n\nno json\n");
		CHECK(c.read_line() == "got:{\"id\":1}");
		CHECK(c.read_line() == "got:");
		CHECK(c.read_line() == "got:no json");
		CHECK(json_lines.load() == 0);
	}
	TEST_CASE("raw_lines: no first line timeout") {
		std::atomic<int> json_lines{ 0 };
		auto o = raw_options(tcp_options(), json_lines);
		o.first_line_timeout_seconds = 1;
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		CHECK_FALSE(c.at_eof(1500));
		c.send("late");
		CHECK(c.read_line() == "got:late");
	}
	TEST_CASE("binary_as_text: a binary message reaches on_text as it came") {
		std::atomic<int> json_lines{ 0 };
		auto o = raw_options(ws_options(), json_lines);
		o.binary_as_text = true;
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send_raw(upgrade_request("/ws", ""));
		CHECK(c.read_line() == "HTTP/1.1 101 Switching Protocols");
		c.send_raw(binary_frame("b\xffn"));
		c.send_raw(text_frame("t"));
		const auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(10);
		while (!has(c.in_buf, "got:t") && c.read_until(deadline)) {}
		CHECK(has(c.in_buf, "got:b\xffn"));
		CHECK(has(c.in_buf, "got:t"));
		CHECK_FALSE(c.eof_seen);
		CHECK(json_lines.load() == 0);
	}
	TEST_CASE("raw_lines over websocket: one message is one line") {
		std::atomic<int> json_lines{ 0 };
		server_thread s(raw_options(ws_options(), json_lines));
		auto t = ws_connect(s.tcp_port());
		std::string l;
		CHECK(t->send_line("getmempool"));
		REQUIRE(t->read_line(l) == read_status::ok);
		CHECK(l == "got:getmempool");
	}
	TEST_CASE("raw_lines with authenticate: the start fails") {
		std::atomic<int> json_lines{ 0 };
		auto o = raw_options(tcp_options(), json_lines);
		o.authenticate = [](tref, const peer_info&) {
			return true;
		};
		auto r = line_server_run(o);
		REQUIRE_FALSE(r.has_value());
		std::ostringstream os;
		r.report().print(os, term::colors(false));
		CHECK(has(os.str(), "raw_lines and authenticate"));
	}
	TEST_CASE("raw_lines logs each line as text") {
		std::atomic<int> json_lines{ 0 };
		auto o = raw_options(tcp_options(), json_lines);
		o.debug_sessions = true;
		server_thread s(o);
		{
			tcp_client c(loopback(s.tcp_port()));
			c.send("getmempool");
			CHECK(c.read_line() == "got:getmempool");
		}
		s.stop();
		const std::string out = s.out.text();
		CHECK(has(out, "\"event\":\"new\""));
		CHECK(has(out, R"("dir":"in","line":"getmempool")"));
	}
}

TEST_SUITE("line server: listen host") {
	TEST_CASE("the default host: 127.0.0.1, no host in the listening line") {
		CHECK(listen_address{}.host == "127.0.0.1");
		server_thread s(tcp_options());
		const uint16_t port = s.tcp_port();
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		CHECK(lines[0] == "{\"listening\":{\"kind\":\"tcp\",\"port\":"
			+ std::to_string(port) + "}}");
		tcp_client c(loopback(port));
		c.send("{\"id\":1}");
		CHECK(c.read_line() == "{\"id\":1,\"status\":\"ok\"}");
	}
	TEST_CASE("an IPv6 host: the server binds it and names it") {
		auto o = tcp_options();
		o.listen[0].host = "::1";
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		if (s.done) {
			MESSAGE("no IPv6 loopback: " << s.report_text);
			return;
		}
		REQUIRE_FALSE(lines.empty());
		CHECK(has(lines[0], "\"host\":\"::1\""));
		tcp_client c({ ba::ip::make_address("::1"), s.tcp_port() });
		c.send("{\"id\":1}");
		CHECK(c.read_line() == "{\"id\":1,\"status\":\"ok\"}");
	}
	TEST_CASE("an IPv6 host as text has brackets") {
		auto o = tcp_options();
		o.json = false;
		o.listen[0].host = "::1";
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		if (s.done) return;
		REQUIRE_FALSE(lines.empty());
		CHECK(has(lines[0], " listening on [::1]:"));
	}
	TEST_CASE("a host that is no IP address is an error that names it") {
		auto o = tcp_options();
		o.listen[0].host = "no.such.host";
		o.make_handler = [](const line_session&) {
			return std::unique_ptr<session_handler>{};
		};
		auto r = line_server_run(o);
		REQUIRE_FALSE(r.has_value());
		std::ostringstream os;
		r.report().print(os, term::colors(false));
		CHECK(has(os.str(), "Invalid listen address"));
		CHECK(has(os.str(), "no.such.host"));
	}
}

TEST_SUITE("line server: reply line end") {
	TEST_CASE("the default: a tcp reply ends with one newline") {
		CHECK(reply_line_ends{}.local_socket == "\n");
		CHECK(reply_line_ends{}.tcp == "\n");
		CHECK(reply_line_ends{}.websocket.empty());
		server_thread s(tcp_options());
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"id\":1}");
		const auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(10);
		while (c.in_buf.find('\n') == std::string::npos
			&& c.read_until(deadline)) {}
		CHECK(c.in_buf == "{\"id\":1,\"status\":\"ok\"}\n");
	}
	TEST_CASE("a tcp line end of CR LF ends a raw reply") {
		std::atomic<int> json_lines{ 0 };
		auto o = raw_options(tcp_options(), json_lines);
		o.reply_line_end.tcp = "\r\n";
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("a");
		const auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(10);
		while (c.in_buf.find('\n') == std::string::npos
			&& c.read_until(deadline)) {}
		CHECK(c.in_buf == "got:a\r\n");
	}
	TEST_CASE("a tcp line end of CR LF also ends a server error") {
		auto o = tcp_options();
		o.reply_line_end.tcp = "\r\n";
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("no json");
		const auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(10);
		while (c.in_buf.find('\n') == std::string::npos
			&& c.read_until(deadline)) {}
		CHECK(has(c.in_buf, "\"status\":\"error\""));
		CHECK(c.in_buf.ends_with("}\r\n"));
	}
	TEST_CASE("a websocket line end goes into each message") {
		auto o = ws_options();
		o.reply_line_end.websocket = "\n";
		server_thread s(o);
		auto t = ws_connect(s.tcp_port());
		std::string l;
		CHECK(t->send_line("{\"id\":1}"));
		REQUIRE(t->read_line(l) == read_status::ok);
		CHECK(l == "{\"id\":1,\"status\":\"ok\"}\n");
	}
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

TEST_SUITE("line server: max_sessions 0") {
	TEST_CASE("max_sessions 0 takes one session") {
		auto o = tcp_options();
		o.max_sessions = 0;
		server_thread s(o);
		tcp_client c1(loopback(s.tcp_port()));
		c1.send("{\"cmd\":\"open\"}");
		CHECK(has(c1.read_line(), "\"status\":\"ok\""));
		tcp_client c2(loopback(s.tcp_port()));
		CHECK(has(c2.read_line(), o.busy_message));
	}
}

TEST_SUITE("line server: write timeout") {
	TEST_CASE("a tcp client that never reads: the write timer closes it") {
		auto o = tcp_options();
		o.write_timeout_seconds = 1;
		std::atomic<int> closes{ 0 };
		o.make_handler = [&closes](const line_session&) {
			return std::make_unique<large_reply_handler>(closes);
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		// 256 answers of 64 KiB fill the socket buffers, so the writes stall.
		std::string lines;
		for (int i = 0; i != 256; ++i)
			lines += "{\"id\":" + std::to_string(i) + "}\n";
		c.send_raw(lines);
		CHECK(wait_for(closes, 1));
	}
}

TEST_SUITE("line server: idle timeout") {
	TEST_CASE("a quiet tcp session ends after idle_timeout_seconds") {
		auto o = tcp_options();
		o.idle_timeout_seconds = 1;
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		CHECK(c.at_eof(5000));
		CHECK(wait_for(s.closes, 1));
	}
	TEST_CASE("idle_timeout_seconds 0 keeps a quiet session") {
		auto o = tcp_options();
		o.idle_timeout_seconds = 0;
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		CHECK(has(c.read_line(), "\"status\":\"ok\""));
		CHECK_FALSE(c.at_eof(1500));
	}
}

TEST_SUITE("line server: logs") {
	TEST_CASE("log_lines false: no in or out line, the events stay") {
		const auto dir = unique_temp_dir("line_server_no_lines_");
		auto o = tcp_options();
		o.log_dir = dir.string();
		o.log_lines = false;
		o.debug_sessions = true;
		server_thread s(o);
		{
			tcp_client c(loopback(s.tcp_port()));
			c.send("{\"cmd\":\"open\"}");
			CHECK(has(c.read_line(), "\"status\":\"ok\""));
		}
		CHECK(wait_for(s.closes, 1));
		s.stop();
		std::string session_log;
		for (const auto& e : std::filesystem::directory_iterator(dir))
			if (e.path().filename() != "server.log") {
				std::ifstream is(e.path());
				session_log.assign(std::istreambuf_iterator<char>(is), {});
			}
		CHECK(has(session_log, "\"event\":\"new\""));
		CHECK_FALSE(has(session_log, "\"dir\":\"in\""));
		CHECK_FALSE(has(session_log, "\"dir\":\"out\""));
		CHECK_FALSE(has(s.out.text(), "\"dir\":\"in\""));
		std::filesystem::remove_all(dir);
	}
	TEST_CASE("line_server_debug gives a line to on_output") {
		struct debug_handler : session_handler {
			std::string id;
			explicit debug_handler(std::string i) : id(std::move(i)) {}
			void on_line(tref, reply_sink& out) override {
				line_server_debug(jt::build_object({
					jt::build_pair("t", jt::build_string("T")),
					jt::build_pair("dir", jt::build_string("in")),
					jt::build_pair("line", jt::build_string("hello")) }),
					id);
				out.write(jt::build_object({ jt::build_pair("status",
					jt::build_string("ok")) }));
			}
		};
		auto o = tcp_options();
		o.json = false;
		o.make_handler = [](const line_session& se) {
			return std::make_unique<debug_handler>(se.id);
		};
		server_thread s(o);
		const auto lines = s.wait_listening(1);
		REQUIRE_FALSE(lines.empty());
		const auto at = lines[0].rfind(':');
		REQUIRE(at != std::string::npos);
		const auto port = static_cast<uint16_t>(
			std::stoi(lines[0].substr(at + 1)));
		tcp_client c(loopback(port));
		c.send("{\"cmd\":\"open\"}");
		c.send("{\"id\":1}");
		CHECK(c.read_line() == "{\"status\":\"ok\"}");
		s.stop();
		CHECK(has(s.out.text(), "T session "));
		CHECK(has(s.out.text(), " in  hello"));
	}
}

TEST_SUITE("line server: line_server_log") {
	TEST_CASE("an event of the handler goes into server.log and to on_output") {
		struct logging_handler : session_handler {
			void on_line(tref, reply_sink& out) override {
				line_server_log(jt::build_object({ jt::build_pair(
					"event", jt::build_string("handler-event")) }));
				out.write(jt::build_object({ jt::build_pair("status",
					jt::build_string("ok")) }));
			}
		};
		const auto dir = unique_temp_dir("line_server_log_tref_");
		auto o = tcp_options();
		o.log_dir = dir.string();
		o.make_handler = [](const line_session&) {
			return std::make_unique<logging_handler>();
		};
		server_thread s(o);
		{
			tcp_client c(loopback(s.tcp_port()));
			c.send("{\"cmd\":\"open\"}");
			c.send("{\"id\":1}");
			CHECK(c.read_line() == "{\"status\":\"ok\"}");
		}
		s.stop();
		std::ifstream is(dir / "server.log");
		const std::string log((std::istreambuf_iterator<char>(is)), {});
		CHECK(has(log, "\"event\":\"new\""));
		CHECK(has(log, "\"event\":\"handler-event\""));
		CHECK(has(s.out.text(), "handler-event"));
		std::filesystem::remove_all(dir);
	}
}

#ifndef _WIN32
TEST_SUITE("line server: require_same_user") {
	TEST_CASE("require_same_user false: a peer of the same user passes") {
		const std::string path = (std::filesystem::temp_directory_path()
			/ ("line_server_any_user_" + std::to_string(::getpid())
				+ ".sock")).string();
		std::filesystem::remove(path);
		line_server_options o;
		o.listen = { { line_transport_kind::local_socket, path, 0 } };
		o.require_same_user = false;
		o.json = true;
		server_thread s(o);
		REQUIRE_FALSE(s.wait_listening(1).empty());
		line_client_test::local_client c{
			ba::local::stream_protocol::endpoint(path) };
		c.send("{\"id\":1,\"cmd\":\"open\"}");
		CHECK(c.read_line()
			== "{\"id\":1,\"cmd\":\"open\",\"status\":\"ok\"}");
	}
}
#endif

// Pauses the read in on_open. The test starts it again through the sink.
struct pausing_handler : session_handler {
	pausing_handler(std::atomic<int>& l,
		std::shared_ptr<std::atomic<reply_sink*>> s)
		: lines(l), sink(std::move(s)) {}
	std::atomic<int>& lines;
	std::shared_ptr<std::atomic<reply_sink*>> sink;
	void on_open(tref, reply_sink& out) override {
		out.pause_read(true);
		*sink = &out;
	}
	void on_line(tref, reply_sink&) override { ++lines; }
	void on_close() override { *sink = nullptr; }
};

TEST_SUITE("line server: pause_read") {
	TEST_CASE("pause_read true stops the read, false starts it again") {
		std::atomic<int> lines{ 0 };
		auto sink = std::make_shared<std::atomic<reply_sink*>>(nullptr);
		auto o = tcp_options();
		o.make_handler = [&lines, sink](const line_session&) {
			return std::make_unique<pausing_handler>(lines, sink);
		};
		server_thread s(o);
		tcp_client c(loopback(s.tcp_port()));
		c.send("{\"cmd\":\"open\"}");
		// The pause holds between two reads, so the lines come after it.
		const auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::seconds(10);
		while (!sink->load() && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		REQUIRE(sink->load());
		c.send("{\"id\":1}");
		c.send("{\"id\":2}");
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		CHECK(lines.load() == 0);
		CHECK(line_server_post([sink] {
			if (reply_sink* out = sink->load()) out->pause_read(false);
		}));
		CHECK(wait_for(lines, 2));
	}
}
