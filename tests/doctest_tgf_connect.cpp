// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for `tgf connect`: a remote evaluator against a server. The text
// output is compared with the local REPL renderer.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
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
using tgf_serve_test::json_options;
using tgf_serve_test::serve_fixture;

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

// The text a local evaluator renders for one command from its data. The
// remote text must match it for grammar and parse.
static std::string local_text(const std::string& grammar,
	const std::string& src)
{
	tgf_repl_evaluator local(grammar, json_options());
	auto er = local.run(src);
	REQUIRE_FALSE(er.results.empty());
	std::ostringstream os;
	local.render_text(er.results.front(), os);
	return os.str();
}

// The text the local REPL prints for the last of @p srcs, with or
// without colors. The production listings must match it byte for byte.
// The caller passes the same commands in the same order as the session,
// so a command that adds productions to the grammar sees them too.
static std::string local_repl_text(const std::string& grammar,
	const std::vector<std::string>& srcs, bool colors)
{
	tgf_repl_evaluator::options opt;
	opt.json_api = false;
	opt.colors = colors;
	tgf_repl_evaluator local(grammar, opt);
	eval_result last;
	for (const auto& src : srcs) {
		last = local.run(src);
		REQUIRE_FALSE(last.results.empty());
	}
	std::ostringstream os;
	local.render_text(last.results.front(), os);
	return os.str();
}

TEST_CASE("connect: grammar, load, parse, quit and attach again") {
	serve_fixture f;

	std::ostringstream sink;
	tgf_remote_evaluator remote(sink);
	REQUIRE(remote.connect("127.0.0.1", f.bound_port(), ""));
	CHECK(remote.session_id().size() == 32);

	// The text of grammar comes from the data and equals the local text.
	sink.str("");
	sink.clear();
	auto gr = remote.eval("grammar");
	REQUIRE(gr.has_value());
	CHECK(gr.value() == 0);
	CHECK(sink.str() == local_text(grammar_path(), "grammar"));

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
	CHECK(sink.str() == local_text(gpath, "parse \"123\""));

	// The production listings carry the nonterminal ids and colors, so the
	// remote text matches the live local REPL text. Colors are on, so a
	// character class nonterminal takes the character class color.
	remote.set_colors(true);
	sink.str("");
	sink.clear();
	auto ir = remote.eval("internal-grammar");
	REQUIRE(ir.has_value());
	CHECK(ir.value() == 0);
	CHECK(sink.str() == local_repl_text(gpath,
		{ "parse \"123\"", "internal-grammar" }, true));

	sink.str("");
	sink.clear();
	auto ur = remote.eval("unreachable");
	REQUIRE(ur.has_value());
	CHECK(ur.value() == 0);
	CHECK(sink.str() == local_repl_text(gpath,
		{ "parse \"123\"", "unreachable" }, true));
	remote.set_colors(false);

	// quit ends the client; the session stays.
	std::string id = remote.session_id();
	sink.str("");
	sink.clear();
	auto qr = remote.eval("quit");
	REQUIRE(qr.has_value());
	CHECK(qr.value() == 1);

	// Attach again with the id.
	std::ostringstream sink2;
	tgf_remote_evaluator remote2(sink2);
	REQUIRE(remote2.connect("127.0.0.1", f.bound_port(), id));
	CHECK(remote2.session_id() == id);
	auto vr = remote2.eval("version");
	REQUIRE(vr.has_value());
	CHECK(vr.value() == 0);
	CHECK_FALSE(sink2.str().empty());

	std::filesystem::remove_all(
		std::filesystem::path(gpath).parent_path());
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
		auto hello = format::json::parse(hello_line);
		REQUIRE(hello.has_value());
		auto h = hello.value().find("hello");
		REQUIRE(h != nullptr);
		auto sid = h->find("session");
		REQUIRE(sid != nullptr);
		id = sid->as_string();
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
	tgf_remote_evaluator remote(sink);
	REQUIRE(remote.connect("127.0.0.1", f.bound_port(), id));
	CHECK(remote.session_id() == id);
	std::string text = sink.str();
	CHECK(text.find("earlier answers:\n") != std::string::npos);
	CHECK(text.find("parsed terminals:") != std::string::npos);
	// The server holds the session log open, so stop it before the remove.
	f.proc.stop();
	std::filesystem::remove_all(dir);
}
