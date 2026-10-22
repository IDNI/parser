// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for `tgf connect`: a remote evaluator against a server. The text
// output is compared with the local REPL renderer.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "serve/tgf_serve_test_fixture.h"
#include "../src/tgf/tgf_cli.h"
#include "../src/tgf/tgf_connect.h"
#include "../src/tgf/tgf_serve.h"
#include "format/json/json.h"

#include <chrono>
#include <filesystem>
#include <fstream>
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
using tgf_serve_test::grammar_path;
using tgf_serve_test::member;
using tgf_serve_test::parse_line;
using tgf_serve_test::repl_options;
using tgf_serve_test::serve_fixture;
using jt = idni::format::json::tree;

namespace ba = boost::asio;
using ba::ip::tcp;
using boost::system::error_code;

static std::string write_temp_grammar(const std::string& text) {
	std::string dir = tgf_serve_test::make_temp_dir("tgf_connect_");
	std::string path = dir + "/g.tgf";
	std::ofstream f(path, std::ios::binary);
	f << text;
	f.close();
	return path;
}

static void remove_temp_grammar(const std::string& path) {
	std::filesystem::remove_all(std::filesystem::path(path).parent_path());
}

// The text the local REPL prints for the last of @p srcs, with or
// without colors. The remote text must match it byte for byte. The caller
// passes the same commands in the same order as the session, so a command
// that adds productions to the grammar sees them too.
static std::string local_text(const std::string& grammar,
	const std::vector<std::string>& srcs, bool colors = false)
{
	tgf_repl_evaluator local(grammar, repl_options(false, colors));
	eval_result last;
	for (const auto& src : srcs) {
		last = local.run(src);
		REQUIRE_FALSE(last.results.empty());
	}
	std::ostringstream os;
	local.render_text(last.results.front(), os);
	return os.str();
}

// The client options for @p target, attached to @p session when it is
// not empty.
static connect_options client_of(connect_target target,
	const std::string& session = "")
{
	connect_options c;
	c.target = std::move(target);
	if (!session.empty()) c.open_fields = jt::geth(jt::build_object({
		jt::build_pair("session", jt::build_string(session)) }));
	return c;
}

// A client connected to the test server on 127.0.0.1.
static bool open_local(repl_client_evaluator& remote, uint16_t port,
	const std::string& session)
{
	return remote.open_session(client_of({ .kind = line_transport_kind::tcp,
		.host = "127.0.0.1", .port = port }, session));
}

struct line_capture_peer {
	ba::io_context io;
	tcp::acceptor acceptor;
	uint16_t port;
	std::thread worker;
	bool received = false;
	std::string request;

	line_capture_peer()
		: acceptor(io, tcp::endpoint(tcp::v4(), 0)),
		  port(acceptor.local_endpoint().port()),
		  worker([this] {
			  tcp::socket sock(io);
			  error_code ec;
			  acceptor.accept(sock, ec);
			  if (ec) return;
			  ba::streambuf buf;
			  auto read_line = [&sock, &buf](std::string& line) {
				  error_code read_ec;
				  ba::read_until(sock, buf, '\n', read_ec);
				  if (read_ec) return false;
				  std::istream in(&buf);
				  return static_cast<bool>(std::getline(in, line));
			  };
			  auto write_line = [&sock](const std::string& line) {
				  error_code write_ec;
				  ba::write(sock, ba::buffer(line), write_ec);
				  return !write_ec;
			  };
			  std::string first;
			  if (!read_line(first)) return;
			  std::ostringstream hello;
			  format::json::write_line(jt::build_object({
				  jt::build_pair("hello", jt::build_object({
					  jt::build_pair("session", jt::build_string("capture")) })),
				  jt::build_pair("state", jt::build_object({})) }), hello);
			  if (!write_line(hello.str())) return;
			  if (!read_line(request)) return;
			  received = true;
			  std::ostringstream response;
			  format::json::write_line(jt::build_object({
				  jt::build_pair("status", jt::build_string("error")) }),
				  response);
			  write_line(response.str());
		  })
	{
	}

	~line_capture_peer() {
		error_code ec;
		acceptor.close(ec);
		join();
	}

	void join() {
		if (worker.joinable()) worker.join();
	}
};

static std::string report_text(const diagnostics::report& rep) {
	std::ostringstream os;
	rep.print(os, term::colors(false));
	return os.str();
}

TEST_CASE("connect: grammar, load, parse, quit and attach again") {
	serve_fixture f;

	std::ostringstream sink;
	repl_client_evaluator remote(sink, repl_options(false, false));
	REQUIRE(open_local(remote, f.bound_port(), ""));
	CHECK(remote.session_id().size() == 32);

	// The text of grammar comes from the data and equals the local text.
	sink.str("");
	sink.clear();
	auto gr = remote.eval("grammar");
	REQUIRE(gr.has_value());
	CHECK(gr.value() == 0);
	CHECK(sink.str() == local_text(grammar_path(), { "grammar" }));

	// load reads the file on the client and sends the text.
	// The dead rule gives `unreachable` a production to render, and its
	// char class makes both commands color a nonterminal with TC_CC.
	std::string gpath = write_temp_grammar(
		"@use char classes digit, alpha.\n"
		"start => num.\n"
		"num   => digit+.\n"
		"dead  => alpha.\n");
	sink.str("");
	sink.clear();
	auto lr = remote.eval("load \"" + gpath + "\"");
	REQUIRE(lr.has_value());
	CHECK(lr.value() == 0);
	CHECK(sink.str().find("loaded:") != std::string::npos);

	// The text of parse comes from the data and equals the local text.
	sink.str("");
	sink.clear();
	auto pr = remote.eval("parse \"123\"");
	REQUIRE(pr.has_value());
	CHECK(pr.value() == 0);
	CHECK(sink.str() == local_text(gpath, { "parse \"123\"" }));

	// quit ends the client; the session stays.
	std::string id = remote.session_id();
	sink.str("");
	sink.clear();
	auto qr = remote.eval("quit");
	REQUIRE(qr.has_value());
	CHECK(qr.value() == 1);

	// Attach again with the id, with colors on the client only.
	std::ostringstream sink2;
	repl_client_evaluator remote2(sink2, repl_options(false, true));
	REQUIRE(open_local(remote2, f.bound_port(), id));
	CHECK(remote2.session_id() == id);

	// The production listings carry the nonterminal ids and colors, so the
	// remote text matches the live local REPL text. Colors are on, so a
	// character class nonterminal takes the character class color.
	sink2.str("");
	sink2.clear();
	auto ir = remote2.eval("internal-grammar");
	REQUIRE(ir.has_value());
	CHECK(ir.value() == 0);
	CHECK(sink2.str() == local_text(gpath,
		{ "parse \"123\"", "internal-grammar" }, true));

	sink2.str("");
	sink2.clear();
	auto ig = remote2.eval("igrammar");
	REQUIRE(ig.has_value());
	CHECK(ig.value() == 0);
	CHECK(sink2.str() == local_text(gpath,
		{ "parse \"123\"", "igrammar" }, true));

	sink2.str("");
	sink2.clear();
	auto ur = remote2.eval("unreachable");
	REQUIRE(ur.has_value());
	CHECK(ur.value() == 0);
	CHECK(sink2.str() == local_text(gpath,
		{ "parse \"123\"", "unreachable" }, true));

	sink2.str("");
	sink2.clear();
	auto vr = remote2.eval("version");
	REQUIRE(vr.has_value());
	CHECK(vr.value() == 0);
	CHECK_FALSE(sink2.str().empty());

	remove_temp_grammar(gpath);
}

TEST_CASE("connect: a bad escape is an error and sends nothing") {
	serve_fixture f;
	std::ostringstream sink;
	repl_client_evaluator remote(sink, repl_options(false, false));
	REQUIRE(open_local(remote, f.bound_port(), ""));
	sink.str("");
	sink.clear();
	std::ostringstream err;
	auto old = std::cerr.rdbuf(err.rdbuf());
	auto r = remote.eval("parse \"\\q\"");
	std::cerr.rdbuf(old);
	REQUIRE(r.has_value());
	CHECK(sink.str().empty());
	CHECK_FALSE(err.str().empty());
	// the session still answers in order
	auto v = remote.eval("version");
	REQUIRE(v.has_value());
	CHECK(sink.str().find("version") != std::string::npos);
}

TEST_CASE("internal-grammar text keeps a space terminal") {
	std::string gpath = write_temp_grammar("start => 'a' ' ' word.\n"
		"word => 'b'.\n");
	std::string text = local_text(gpath, { "i" });
	CHECK(text.find("=> 'a' ' ' word(") != std::string::npos);
	remove_temp_grammar(gpath);
}

TEST_CASE("parse tree text quotes a terminal") {
	std::string gpath = write_temp_grammar("start => 'a' ' ' q.\n"
		"q => '\\''.\n");
	std::string text = local_text(gpath, { "parse \"a '\"" });
	CHECK(text.find("'a'[0, 1]") != std::string::npos);
	CHECK(text.find("' '[1, 2]") != std::string::npos);
	CHECK(text.find("'\\''[2, 3]") != std::string::npos);
	remove_temp_grammar(gpath);
}

TEST_CASE("connect: many answers keep the client cache small") {
	serve_fixture f;
	std::string gpath = write_temp_grammar(
		"@use char classes digit, alpha.\n"
		"start => num.\n"
		"num   => digit+.\n"
		"dead  => alpha.\n");
	std::ostringstream sink;
	repl_client_evaluator remote(sink, repl_options(false, true));
	REQUIRE(open_local(remote, f.bound_port(), ""));
	auto lr = remote.eval("load \"" + gpath + "\"");
	REQUIRE(lr.has_value());
	CHECK(lr.value() == 0);
	// Each answer has new nodes, so the cache grows past the gc limit.
	for (size_t i = 0; i != 400; ++i) {
		auto pr = remote.eval("parse \"" + std::to_string(1000 + i)
			+ "\"");
		REQUIRE(pr.has_value());
		CHECK(pr.value() == 0);
	}
	CHECK(format::json::tree::m_size() < 16384);
	// The character class color comes from the state of the client.
	sink.str("");
	sink.clear();
	auto ur = remote.eval("unreachable");
	REQUIRE(ur.has_value());
	CHECK(ur.value() == 0);
	CHECK(sink.str() == local_text(gpath, { "unreachable" }, true));
	remove_temp_grammar(gpath);
}

TEST_CASE("connect --json: hello, load and answers as JSON lines") {
	serve_fixture f;

	std::ostringstream sink;
	repl_client_evaluator remote(sink, repl_options(true, false));
	REQUIRE(open_local(remote, f.bound_port(), ""));
	CHECK(remote.load_grammar(grammar_path()) == cmd_status::ok);
	// The empty line gets no answer, and nothing after quit is sent.
	std::istringstream in(
		"{\"id\":1,\"cmd\":\"version\"}\n"
		"\n"
		"{\"id\":2,\"cmd\":\"quit\"}\n"
		"{\"id\":3,\"cmd\":\"version\"}\n");
	CHECK(remote.run_json(in) == 0);

	std::vector<htref> lines;
	std::istringstream out(sink.str());
	for (std::string l; std::getline(out, l); ) {
		lines.push_back(parse_line(l));
	}
	REQUIRE(lines.size() == 4);
	CHECK(member(lines[0], "hello") != nullptr);
	CHECK(member(lines[1], "status")->get_string() == "ok");
	CHECK(member(lines[2], "id")->get_number() == 1);
	CHECK(member(lines[2], "status")->get_string() == "ok");
	CHECK(member(lines[3], "id")->get_number() == 2);
	CHECK(member(lines[3], "status")->get_string() == "quit");
}

TEST_CASE("connect --json forwards non-JSON input unchanged") {
	line_capture_peer peer;
	std::ostringstream sink;
	{
		repl_client_evaluator remote(sink, repl_options(true, false));
		REQUIRE(remote.open_session(client_of({
			.kind = line_transport_kind::tcp,
			.host = "127.0.0.1", .port = peer.port })));
		std::istringstream in("not-json\n");
		CHECK(remote.run_json(in) == 0);
	}
	peer.join();
	CHECK(peer.received);
	CHECK(peer.request == "not-json");
}

TEST_CASE("connect --json drops CR at the end of an input line") {
	line_capture_peer peer;
	std::ostringstream sink;
	{
		repl_client_evaluator remote(sink, repl_options(true, false));
		REQUIRE(remote.open_session(client_of({
			.kind = line_transport_kind::tcp,
			.host = "127.0.0.1", .port = peer.port })));
		std::istringstream in("\r\n");
		CHECK(remote.run_json(in) == 0);
	}
	peer.join();
	CHECK_FALSE(peer.received);
}

TEST_CASE("connect: the address argument forms") {
	auto ok = [](const std::string& iri, const std::string& host,
		uint16_t port)
	{
		auto t = parse_connect_iri(iri);
		REQUIRE(t.has_value());
		CHECK(t.value().host == host);
		CHECK(t.value().port == port);
	};
	ok("tgf://localhost:4101", "localhost", 4101);
	ok("tgf://localhost:4101/", "localhost", 4101);
	ok("tgf://localhost", "localhost", 4000);
	ok("localhost:4101", "localhost", 4101);
	ok("localhost", "localhost", 4000);
	ok("127.0.0.1:4101", "127.0.0.1", 4101);
	ok("[::1]:4101", "::1", 4101);
	ok("[::1]", "::1", 4000);
	// a non-ASCII host name: "buecher" with u-umlaut, in UTF-8
	ok("b\xc3\xbc" "cher.example:5000", "b\xc3\xbc" "cher.example", 5000);
	for (const char* bad : { "http://localhost:4000",
		"tgf://localhost:4000/x", "localhost:99999", "localhost:0",
		"localhost:", "tgf://user@localhost", "localhost?a",
		"localhost#a", "::1:4000", "[::1", "[]:4000", "[::1]x",
		"tgf://", "" })
	{
		auto t = parse_connect_iri(bad);
		CHECK_FALSE(t.has_value());
		// The error names the argument.
		if (*bad) CHECK(report_text(t.report()).find(bad)
			!= std::string::npos);
	}
}

TEST_CASE("connect: a host name or an address reaches the server") {
	serve_fixture f;
	for (const char* host : { "localhost", "127.0.0.1" }) {
		std::ostringstream sink;
		repl_client_evaluator remote(sink, repl_options(false, false));
		CHECK(remote.open_session(
			client_of({ .kind = line_transport_kind::tcp,
				.host = host, .port = f.bound_port() })));
		CHECK(remote.session_id().size() == 32);
	}
	for (std::string iri : { "tgf://localhost:", "localhost:",
		"127.0.0.1:" })
	{
		auto t = parse_connect_iri(iri + std::to_string(f.bound_port()));
		REQUIRE(t.has_value());
		std::ostringstream sink;
		repl_client_evaluator remote(sink, repl_options(false, false));
		CHECK(remote.open_session(client_of(t.value())));
	}
}

TEST_CASE("connect: an unknown host gives the resolve error") {
	std::ostringstream sink;
	repl_client_evaluator remote(sink, repl_options(false, false));
	CHECK_FALSE(remote.open_session(
		client_of({ .kind = line_transport_kind::tcp,
			.host = "no-such-host.invalid", .port = 4101 })));
	std::string text = report_text(remote.take_report());
	CHECK(text.find("Could not resolve the server host")
		!= std::string::npos);
	CHECK(text.find("no-such-host.invalid") != std::string::npos);
}

TEST_CASE("connect: an address argument with --port is an error") {
	tgf_serve_test::child_process proc;
	REQUIRE(proc.start(tgf_serve_test::tgf_binary_path(), {
		"connect", "localhost:4101", "-p", "4101" }));
	std::string all, line;
	while (proc.read_stderr_line(line, 10000)) all += line + "\n";
	CHECK(all.find("either as an address argument") != std::string::npos);
}

TEST_CASE("connect -c false prints a server error without colors") {
	serve_fixture f;
	for (bool colors : { false, true }) {
		tgf_serve_test::child_process proc;
		REQUIRE(proc.start(tgf_serve_test::tgf_binary_path(), {
			"connect", "-p", std::to_string(f.bound_port()),
			"-c", colors ? "true" : "false",
			"--session", std::string(32, '0'), "--legacy-repl" }));
		std::string all, line;
		while (proc.read_stderr_line(line, 10000)) all += line + "\n";
		CHECK(all.find("Unknown session id") != std::string::npos);
		// The colored run proves the check sees an escape sequence.
		CHECK((all.find('\x1b') != std::string::npos) == colors);
	}
}

TEST_CASE("connect: prints the answers buffered before an attach") {
	std::string dir = tgf_serve_test::make_temp_dir(
		"tgf_connect_buffered_");
	serve_fixture f(16, 100000, dir, false);
	std::string id;
	{
		// A raw client starts a session, asks for a parse of a few
		// hundred digits and drops the socket right after. The answer
		// arrives with no client and waits in the session buffer.
		ba::io_context io;
		tcp::socket sock(io);
		error_code ec;
		sock.connect(tcp::endpoint(ba::ip::make_address(
			"127.0.0.1"), f.bound_port()), ec);
		REQUIRE_FALSE(ec);
		auto send = [&](const std::string& line) {
			std::string s = line + "\n";
			ba::write(sock, ba::buffer(s), ec);
			REQUIRE_FALSE(ec);
		};
		send(R"({"cmd":"new"})");
		ba::streambuf buf;
		ba::read_until(sock, buf, '\n', ec);
		REQUIRE_FALSE(ec);
		std::istream is(&buf);
		std::string hello_line;
		std::getline(is, hello_line);
		auto hello = parse_line(hello_line);
		auto h = member(hello, "hello");
		REQUIRE(h != nullptr);
		auto sid = member(h, "session");
		REQUIRE(sid != nullptr);
		id = sid->get_string();
		std::string digits(200, '1');
		send("{\"id\":1,\"cmd\":\"parse\",\"input\":\""
			+ digits + "\"}");
		sock.close(ec);
	}
	// Poll the session log until the parse answer is logged as an `out`
	// line after the `detach` event. The session detaches first, so that
	// order means the answer is held for the next attach.
	bool buffered = false;
	auto deadline = std::chrono::steady_clock::now()
		+ std::chrono::seconds(10);
	while (!buffered && std::chrono::steady_clock::now() < deadline) {
		for (const auto& e : std::filesystem::directory_iterator(dir)) {
			std::string name = e.path().filename().string();
			if (name.find(id) == std::string::npos) continue;
			if (name.size() < 4
					|| name.substr(name.size() - 4) != ".log")
				continue;
			std::ifstream in(e.path());
			std::string log_line;
			long long line_no = 0, detach_at = -1, answer_at = -1;
			while (std::getline(in, log_line)) {
				if (log_line.find("\"event\":\"detach\"")
						!= std::string::npos)
					detach_at = line_no;
				else if (log_line.find("\"dir\":\"out\"")
						!= std::string::npos
						&& log_line.find("parse")
						!= std::string::npos)
					answer_at = line_no;
				++line_no;
			}
			if (detach_at >= 0 && answer_at > detach_at)
				buffered = true;
			if (buffered) break;
		}
		if (!buffered) std::this_thread::yield();
	}
	CHECK(buffered);

	std::ostringstream sink;
	repl_client_evaluator remote(sink, repl_options(false, false));
	REQUIRE(open_local(remote, f.bound_port(), id));
	CHECK(remote.session_id() == id);
	std::string text = sink.str();
	CHECK(text.find("earlier answers:\n") != std::string::npos);
	CHECK(text.find("parsed terminals:") != std::string::npos);
	// The server holds the session log open, so stop it before the remove.
	f.proc.stop();
	std::filesystem::remove_all(dir);
}
