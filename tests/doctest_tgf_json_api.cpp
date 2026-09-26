// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the tgf JSON API front end (tgf_json_loop) and the one-shot
// --json forms of the parse, grammar and gen commands.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/tgf/tgf_cli.h"
#include "format/json/json.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace idni;
using namespace idni::format;

static std::string grammar_path() {
	return std::string(PROJECT_SOURCE_DIR) + "/tests/fixtures/tiny.tgf";
}

static std::string csv_grammar_path() {
	return std::string(PROJECT_SOURCE_DIR) + "/src/format/csv/csv.tgf";
}

static std::string json_grammar_path() {
	return std::string(PROJECT_SOURCE_DIR) + "/src/format/json/json.tgf";
}

static std::string ambig_bc_path() {
	return std::string(PROJECT_SOURCE_DIR)
		+ "/tests/fixtures/ambig_bc.tgf";
}

static tgf_repl_evaluator::options json_options() {
	tgf_repl_evaluator::options opt;
	opt.json_api = true;
	opt.print_json = true;
	return opt;
}

// Parse one response line.
static json::value parse_line(const std::string& line) {
	auto p = json::parse(line);
	REQUIRE(p.has_value());
	return p.value();
}

struct repl_run {
	std::vector<json::value> responses;   // hello is not included
	std::string raw;
};

// Drive one evaluator on @p grammar with @p lines and collect the
// response values.
static repl_run run_repl_on(const std::string& grammar,
	const std::vector<std::string>& lines)
{
	tgf_repl_evaluator re(grammar, json_options());
	std::string input;
	for (const auto& l : lines) input += l + "\n";
	std::istringstream in(input);
	std::ostringstream out;
	tgf_json_loop(re, in, out);
	repl_run r;
	r.raw = out.str();
	std::istringstream ls(out.str());
	std::string l;
	bool first = true;
	while (std::getline(ls, l)) {
		if (first) { first = false; continue; }   // hello
		r.responses.push_back(parse_line(l));
	}
	return r;
}

// Drive one evaluator with @p lines and collect the response values.
static repl_run run_repl(const std::vector<std::string>& lines) {
	return run_repl_on(grammar_path(), lines);
}

static const json::value* result_of(const json::value& resp) {
	return resp.find("result");
}

// Result data: the structured result, or an eval response's first result.
static const json::value* response_data(const json::value& resp) {
	if (auto d = resp.find("result"); d) return d;
	if (auto rs = resp.find("results"); rs && rs->size())
		return (*rs)[0].find("result");
	return nullptr;
}

// One JSON line built with the writer, so paths and quotes stay valid.
static std::string to_line(const json::value& v) {
	std::ostringstream os;
	json::print(v, os);
	return os.str();
}

// A quoted REPL string. The encoder turns a backslash into an escape
// sequence, so a native path survives the REPL string decoder.
static std::string repl_string(const std::string& s) {
	return "\"" + escapes::encode(s, escapes::tgf_string) + "\"";
}

// {"id":id,"cmd":"eval","src":src}
static std::string eval_request(int id, const std::string& src) {
	json::value v = json::value::object();
	v.set("id", json::value::number(static_cast<double>(id)))
	 .set("cmd", json::value::string("eval"))
	 .set("src", json::value::string(src));
	return to_line(v);
}

// {"id":id,"cmd":cmd,"file":path}
static std::string file_request(int id, const std::string& cmd,
	const std::string& path)
{
	json::value v = json::value::object();
	v.set("id", json::value::number(static_cast<double>(id)))
	 .set("cmd", json::value::string(cmd))
	 .set("file", json::value::string(path));
	return to_line(v);
}

static std::string read_file(const std::string& path) {
	std::ifstream f(path, std::ios::binary);
	std::ostringstream os;
	os << f.rdbuf();
	return os.str();
}

// A scratch file under the build tree, removed when the test ends.
struct scratch_file {
	std::string path;
	scratch_file(const std::string& name, const std::string& content) {
		path = (std::filesystem::path(PROJECT_BINARY_DIR) / name)
			.string();
		std::ofstream os(path, std::ios::binary);
		os << content;
	}
	~scratch_file() {
		std::error_code ec;
		std::filesystem::remove(path, ec);
	}
	scratch_file(const scratch_file&) = delete;
	scratch_file& operator=(const scratch_file&) = delete;
};

// Compare a JSON array of tree paths with @p want.
static bool treepaths_are(const json::value& v,
	const std::vector<std::vector<std::string>>& want)
{
	if (!v.is_array() || v.size() != want.size()) return false;
	for (size_t i = 0; i != want.size(); ++i) {
		const json::value& p = v[i];
		if (!p.is_array() || p.size() != want[i].size()) return false;
		for (size_t j = 0; j != want[i].size(); ++j)
			if (p[j].as_string() != want[i][j]) return false;
	}
	return true;
}

TEST_SUITE("tgf json api: hello") {
	TEST_CASE("the hello line carries the evaluator state") {
		auto r = run_repl({});
		std::istringstream ls(r.raw);
		std::string line;
		bool got = static_cast<bool>(std::getline(ls, line));
		REQUIRE(got);
		auto p = parse_line(line);
		auto h = p.find("hello");
		REQUIRE(h != nullptr);
		REQUIRE(h->find("protocol") != nullptr);
		CHECK(h->find("protocol")->as_number() == 1);
		REQUIRE(h->find("grammar") != nullptr);
		CHECK(h->find("grammar")->as_string() == grammar_path());
		REQUIRE(h->find("fixed_grammar") != nullptr);
		CHECK(h->find("fixed_grammar")->is_bool());
		REQUIRE(h->find("start") != nullptr);
		CHECK(h->find("start")->as_string() == "start");
		auto opts = h->find("options");
		REQUIRE(opts != nullptr);
		CHECK(opts->find("debug") != nullptr);
		CHECK(opts->find("trim") != nullptr);
		CHECK(opts->find("inline") != nullptr);
		CHECK(opts->find("derive-char-classes") != nullptr);
		CHECK(opts->find("error-verbosity") != nullptr);
		CHECK(h->find("report") != nullptr);
		const json::value* s = p.find("state");
		REQUIRE(s != nullptr);
		REQUIRE(s->find("grammar") != nullptr);
		CHECK(s->find("grammar")->as_string() == grammar_path());
		REQUIRE(s->find("start") != nullptr);
		CHECK(s->find("start")->as_string() == "start");
	}
}

TEST_SUITE("tgf json api: eval form") {
	TEST_CASE("one command, id echoed, results array") {
		auto r = run_repl({R"({"id":7,"cmd":"eval","src":"version"})"});
		REQUIRE(r.responses.size() == 1);
		const auto& v = r.responses[0];
		CHECK(v.find("id")->as_number() == 7);
		CHECK(v.find("status")->as_string() == "ok");
		auto res = v.find("results");
		REQUIRE(res != nullptr);
		REQUIRE(res->size() == 1);
		CHECK((*res)[0].find("cmd")->as_string() == "version");
		REQUIRE((*res)[0].find("result") != nullptr);
		CHECK((*res)[0].find("result")->find("version") != nullptr);
	}

	TEST_CASE("multi statement input gives one entry per statement") {
		auto r = run_repl({
			R"({"id":1,"cmd":"eval","src":"version . license"})"});
		REQUIRE(r.responses.size() == 1);
		auto res = r.responses[0].find("results");
		REQUIRE(res != nullptr);
		REQUIRE(res->size() == 2);
		CHECK((*res)[0].find("cmd")->as_string() == "version");
		CHECK((*res)[1].find("cmd")->as_string() == "license");
	}

	TEST_CASE("incomplete input is reported and keeps no partial text") {
		auto r = run_repl({R"({"id":3,"cmd":"eval","src":"set"})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(r.responses[0].find("id")->as_number() == 3);
		CHECK(r.responses[0].find("status")->as_string() == "incomplete");
		CHECK(r.responses[0].find("state") != nullptr);
		CHECK(r.responses[0].find("report") != nullptr);
	}

	TEST_CASE("a missing src is an error") {
		auto r = run_repl({R"({"id":4,"cmd":"eval"})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(r.responses[0].find("status")->as_string() == "error");
		CHECK(r.responses[0].find("id")->as_number() == 4);
	}
}

TEST_SUITE("tgf json api: invalid and unknown requests") {
	TEST_CASE("invalid JSON gives id null and an error") {
		auto r = run_repl({"{ not json"});
		REQUIRE(r.responses.size() == 1);
		CHECK(r.responses[0].find("status")->as_string() == "error");
		CHECK(r.responses[0].find("state") != nullptr);
		CHECK(r.responses[0].find("id")->is_null());
		CHECK(r.responses[0].find("report") != nullptr);
	}

	TEST_CASE("an unknown cmd is an error") {
		auto r = run_repl({R"({"id":5,"cmd":"bogus"})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(r.responses[0].find("status")->as_string() == "error");
		CHECK(r.responses[0].find("id")->as_number() == 5);
	}
}

TEST_SUITE("tgf json api: options") {
	TEST_CASE("set and get a list option") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["a","b"]})",
			R"({"id":2,"cmd":"get","option":"trim"})"});
		REQUIRE(r.responses.size() == 2);
		auto v0 = result_of(r.responses[0])->find("value");
		REQUIRE(v0 != nullptr);
		REQUIRE(v0->size() == 2);
		CHECK((*v0)[0].as_string() == "a");
		auto v1 = result_of(r.responses[1])->find("value");
		REQUIRE(v1 != nullptr);
		CHECK(v1->size() == 2);
	}

	TEST_CASE("an empty value clears the list") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["a","b"]})",
			R"({"id":2,"cmd":"set","option":"trim","value":[]})",
			R"({"id":3,"cmd":"get","option":"trim"})"});
		REQUIRE(r.responses.size() == 3);
		REQUIRE(result_of(r.responses[1]) != nullptr);
		REQUIRE(result_of(r.responses[1])->find("value") != nullptr);
		CHECK(result_of(r.responses[1])->find("value")->size() == 0);
		REQUIRE(result_of(r.responses[2]) != nullptr);
		REQUIRE(result_of(r.responses[2])->find("value") != nullptr);
		CHECK(result_of(r.responses[2])->find("value")->size() == 0);
	}

	TEST_CASE("eval form set trim and set trim = clear the list") {
		auto r = run_repl({
			R"({"id":1,"cmd":"eval","src":"set trim a, b . get trim"})",
			R"({"id":2,"cmd":"eval","src":"set trim . get trim"})",
			R"({"id":3,"cmd":"eval","src":"set trim a . set trim = . get trim"})"});
		REQUIRE(r.responses.size() == 3);
		auto res0 = r.responses[0].find("results");
		REQUIRE(res0 != nullptr);
		REQUIRE(res0->size() == 2);
		REQUIRE((*res0)[1].find("result") != nullptr);
		REQUIRE((*res0)[1].find("result")->find("value") != nullptr);
		CHECK((*res0)[1].find("result")->find("value")->size() == 2);
		auto res1 = r.responses[1].find("results");
		REQUIRE(res1 != nullptr);
		REQUIRE((*res1)[1].find("result")->find("value") != nullptr);
		CHECK((*res1)[1].find("result")->find("value")->size() == 0);
		auto res2 = r.responses[2].find("results");
		REQUIRE(res2 != nullptr);
		REQUIRE((*res2)[2].find("result")->find("value") != nullptr);
		CHECK((*res2)[2].find("result")->find("value")->size() == 0);
	}

	TEST_CASE("an unknown option is an error") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"nope","value":true})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(r.responses[0].find("status")->as_string() == "error");
	}

	TEST_CASE("add and delete change a list") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["a"]})",
			R"({"id":2,"cmd":"add","option":"trim","value":["b"]})",
			R"({"id":3,"cmd":"delete","option":"trim","value":["a"]})"});
		REQUIRE(r.responses.size() == 3);
		REQUIRE(result_of(r.responses[1]) != nullptr);
		REQUIRE(result_of(r.responses[1])->find("value") != nullptr);
		CHECK(result_of(r.responses[1])->find("value")->size() == 2);
		auto v = result_of(r.responses[2])->find("value");
		REQUIRE(v != nullptr);
		REQUIRE(v->size() == 1);
		CHECK((*v)[0].as_string() == "b");
	}

	TEST_CASE("a second evaluator does not see the first one's options") {
		auto a = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["a"]})"});
		REQUIRE(a.responses.size() == 1);
		auto b = run_repl({
			R"({"id":1,"cmd":"get","option":"trim"})"});
		REQUIRE(b.responses.size() == 1);
		REQUIRE(result_of(b.responses[0]) != nullptr);
		REQUIRE(result_of(b.responses[0])->find("value") != nullptr);
		CHECK(result_of(b.responses[0])->find("value")->size() == 0);
	}

	TEST_CASE("nodisambig-list reaches the grammar") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"nodisambig-list","value":["num"]})",
			R"({"id":2,"cmd":"get","option":"nodisambig-list"})"});
		REQUIRE(r.responses.size() == 2);
		auto v = result_of(r.responses[1])->find("value");
		REQUIRE(v != nullptr);
		REQUIRE(v->size() == 1);
		CHECK((*v)[0].as_string() == "num");
	}
}

TEST_SUITE("tgf json api: parse data") {
	TEST_CASE("parse returns the tree and terminals") {
		auto r = run_repl({
			R"({"id":1,"cmd":"parse","input":"123"})"});
		REQUIRE(r.responses.size() == 1);
		const json::value* res = result_of(r.responses[0]);
		REQUIRE(res != nullptr);
		const json::value* tree = res->find("tree");
		REQUIRE(tree != nullptr);
		CHECK(res->find("ambiguous") != nullptr);
		REQUIRE(tree->find("symbol") != nullptr);
		CHECK(tree->find("symbol")->as_string() == "start");
		REQUIRE(tree->find("id") != nullptr);
		CHECK(tree->find("id")->is_number());
		auto num_kids = tree->find("children");
		REQUIRE(num_kids != nullptr);
		REQUIRE(num_kids->size() == 1);
		const json::value& num = (*num_kids)[0];
		REQUIRE(num.find("id") != nullptr);
		CHECK(num.find("id")->as_number()
			!= tree->find("id")->as_number());
		auto digits = num.find("children");
		REQUIRE(digits != nullptr);
		REQUIRE(digits->size() == 3);
		const json::value& digit = (*digits)[0];
		REQUIRE(digit.find("id") != nullptr);
		CHECK(digit.find("id")->as_number()
			== (*digits)[1].find("id")->as_number());
		auto dk = digit.find("children");
		REQUIRE(dk != nullptr);
		REQUIRE(dk->size() == 1);
		const json::value& leaf = (*dk)[0];
		REQUIRE(leaf.find("symbol") != nullptr);
		CHECK(leaf.find("symbol")->as_string() == "");
		CHECK(leaf.find("id") == nullptr);
		REQUIRE(leaf.find("text") != nullptr);
		CHECK(leaf.find("text")->as_string() == "1");
	}

	TEST_CASE("ambiguous is absent when print-ambiguity is off") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"print-ambiguity","value":false})",
			R"({"id":2,"cmd":"parse","input":"123"})"});
		REQUIRE(r.responses.size() == 2);
		CHECK(result_of(r.responses[1])->find("ambiguous") == nullptr);
	}

	TEST_CASE("set colors true in JSON mode adds no escape byte") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"colors","value":true})",
			R"({"id":2,"cmd":"parse","input":"123"})"});
		REQUIRE(r.responses.size() == 2);
		CHECK(r.raw.find('\x1b') == std::string::npos);
	}
}

TEST_SUITE("tgf json api: help and quit") {
	TEST_CASE("help load names load and its alias") {
		auto r = run_repl({
			R"({"id":1,"cmd":"help","command":"load"})"});
		REQUIRE(r.responses.size() == 1);
		REQUIRE(result_of(r.responses[0]) != nullptr);
		REQUIRE(result_of(r.responses[0])->find("text") != nullptr);
		auto text = result_of(r.responses[0])->find("text")->as_string();
		CHECK(text.find("load") != std::string::npos);
		CHECK(text.find("short: l") != std::string::npos);
	}

	TEST_CASE("quit stops the loop") {
		auto r = run_repl({
			R"({"id":1,"cmd":"quit"})",
			R"({"id":2,"cmd":"version"})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(r.responses[0].find("status")->as_string() == "quit");
	}
}

TEST_SUITE("tgf json api: parse file") {
	TEST_CASE("eval form parses an input file") {
		scratch_file in("tgf_json_api_input.txt", "123");
		auto r = run_repl({
			eval_request(1, "parse file " + repl_string(in.path)) });
		REQUIRE(r.responses.size() == 1);
		CHECK(r.responses[0].find("status")->as_string() == "ok");
		auto res = r.responses[0].find("results");
		REQUIRE(res != nullptr);
		REQUIRE(res->size() == 1);
		CHECK((*res)[0].find("cmd")->as_string() == "parse file");
		CHECK((*res)[0].find("status")->as_string() == "ok");
		const json::value* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		CHECK(data->find("tree") != nullptr);
		REQUIRE(data->find("terminals") != nullptr);
		CHECK(data->find("terminals")->as_string() == "123");
	}

	TEST_CASE("structured form parses an input file") {
		scratch_file in("tgf_json_api_input.txt", "123");
		auto r = run_repl({ file_request(1, "parse file", in.path) });
		REQUIRE(r.responses.size() == 1);
		CHECK(r.responses[0].find("status")->as_string() == "ok");
		const json::value* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		CHECK(data->find("tree") != nullptr);
		REQUIRE(data->find("terminals") != nullptr);
		CHECK(data->find("terminals")->as_string() == "123");
	}
}

TEST_SUITE("tgf json api: grammar commands") {
	TEST_CASE("grammar returns the grammar file text") {
		const std::string text = read_file(grammar_path());
		auto r = run_repl({
			R"({"id":1,"cmd":"grammar"})",
			eval_request(2, "grammar") });
		REQUIRE(r.responses.size() == 2);
		for (const auto& resp : r.responses) {
			CHECK(resp.find("status")->as_string() == "ok");
			const json::value* data = response_data(resp);
			REQUIRE(data != nullptr);
			REQUIRE(data->find("file") != nullptr);
			CHECK(data->find("file")->as_string() == grammar_path());
			REQUIRE(data->find("source") != nullptr);
			CHECK(data->find("source")->as_string() == text);
		}
	}

	TEST_CASE("internal-grammar gives productions with and without a symbol") {
		auto r = run_repl({
			R"({"id":1,"cmd":"internal-grammar"})",
			eval_request(2, "internal-grammar"),
			R"({"id":3,"cmd":"internal-grammar","symbol":"num"})",
			eval_request(4, "internal-grammar num") });
		REQUIRE(r.responses.size() == 4);
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(r.responses[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			const json::value* prods = data->find("productions");
			REQUIRE(prods != nullptr);
			CHECK(prods->is_array());
			CHECK(prods->size() > 0);
			for (const auto& p : *prods) CHECK(p.is_string());
			REQUIRE(data->find("start") != nullptr);
			CHECK(data->find("start")->as_string()
				== (i >= 2 ? "num" : "start"));
		}
	}

	TEST_CASE("start shows and then sets the start symbol") {
		auto r = run_repl({
			R"({"id":1,"cmd":"start"})",
			eval_request(2, "start"),
			R"({"id":3,"cmd":"start","symbol":"num"})",
			eval_request(4, "start num") });
		REQUIRE(r.responses.size() == 4);
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(r.responses[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			REQUIRE(data->find("start") != nullptr);
			CHECK(data->find("start")->as_string()
				== (i >= 2 ? "num" : "start"));
			REQUIRE(data->find("changed") != nullptr);
			CHECK(data->find("changed")->as_bool() == (i >= 2));
		}
	}

	TEST_CASE("unreachable reports the symbol with and without a symbol") {
		auto r = run_repl({
			R"({"id":1,"cmd":"unreachable"})",
			eval_request(2, "unreachable"),
			R"({"id":3,"cmd":"unreachable","symbol":"num"})",
			eval_request(4, "unreachable num") });
		REQUIRE(r.responses.size() == 4);
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(r.responses[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			REQUIRE(data->find("symbol") != nullptr);
			CHECK(data->find("symbol")->as_string()
				== (i >= 2 ? "num" : "start"));
			REQUIRE(data->find("productions") != nullptr);
			CHECK(data->find("productions")->is_array());
		}
	}
}

TEST_SUITE("tgf json api: load and reload") {
	TEST_CASE("load adopts a second grammar and reload reloads it") {
		scratch_file g("tgf_json_api_second.tgf",
			read_file(grammar_path()));
		auto r = run_repl({
			file_request(1, "load", g.path),
			eval_request(2, "load " + repl_string(g.path)),
			R"({"id":3,"cmd":"reload"})",
			eval_request(4, "reload"),
			R"({"id":5,"cmd":"grammar"})" });
		REQUIRE(r.responses.size() == 5);
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(r.responses[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			if (i == 4) {
				REQUIRE(data->find("file") != nullptr);
				CHECK(data->find("file")->as_string() == g.path);
				continue;
			}
			REQUIRE(data->find("loaded") != nullptr);
			CHECK(data->find("loaded")->as_bool());
			REQUIRE(data->find("grammar") != nullptr);
			CHECK(data->find("grammar")->as_string() == g.path);
		}
	}
}

// A grammar and an input, both with CRLF line ends, written in binary so
// the test does not depend on the checkout line ends.
static std::string crlf_grammar() {
	return "@use char class digit, space.\r\n"
		"start => num __.\r\n"
		"num => digit+.\r\n"
		"__ => space+.\r\n";
}

TEST_SUITE("tgf json api: CRLF text") {
	TEST_CASE("load, reload and grammar keep the CRLF source") {
		scratch_file g("tgf_json_api_crlf.tgf", crlf_grammar());
		auto r = run_repl({
			file_request(1, "load", g.path),
			eval_request(2, "reload"),
			eval_request(3, "grammar") });
		REQUIRE(r.responses.size() == 3);
		for (const auto& resp : r.responses)
			CHECK(resp.find("status")->as_string() == "ok");
		const json::value* data = response_data(r.responses[2]);
		REQUIRE(data != nullptr);
		REQUIRE(data->find("source") != nullptr);
		CHECK(data->find("source")->as_string() == crlf_grammar());
	}

	TEST_CASE("parse file reads a CRLF input file") {
		scratch_file g("tgf_json_api_crlf2.tgf", crlf_grammar());
		scratch_file in("tgf_json_api_crlf_input.txt", "123\r\n");
		auto r = run_repl({
			file_request(1, "load", g.path),
			file_request(2, "parse file", in.path) });
		REQUIRE(r.responses.size() == 2);
		CHECK(r.responses[1].find("status")->as_string() == "ok");
		const json::value* data = response_data(r.responses[1]);
		REQUIRE(data != nullptr);
		REQUIRE(data->find("terminals") != nullptr);
		CHECK(data->find("terminals")->as_string() == "123\r\n");
	}

	TEST_CASE("a request line that ends with CRLF is accepted") {
		auto r = run_repl({
			"{\"id\":1,\"cmd\":\"version\"}\r",
			"{\"id\":2,\"cmd\":\"parse\",\"input\":\"123\"\r}",
			"{\"id\":3,\r\"cmd\":\"version\"}" });
		REQUIRE(r.responses.size() == 3);
		for (const auto& resp : r.responses)
			CHECK(resp.find("status")->as_string() == "ok");
	}

	TEST_CASE("the text grammar command drops the CR of a CRLF source") {
		scratch_file lf("tgf_json_api_text_lf.tgf", "start => 'a'.\n");
		scratch_file crlf("tgf_json_api_text_crlf.tgf",
			"start => 'a'.\r\n");
		tgf_repl_evaluator lfe(lf.path), ce(crlf.path);
		std::ostringstream lfos, cos;
		std::streambuf* old = std::cout.rdbuf();
		std::cout.rdbuf(lfos.rdbuf());
		lfe.eval("grammar");
		std::cout.rdbuf(cos.rdbuf());
		ce.eval("grammar");
		std::cout.rdbuf(old);
		CHECK(cos.str().find('\r') == std::string::npos);
		CHECK(cos.str() == lfos.str());
	}
}

TEST_SUITE("tgf json api: version, license and clear") {
	TEST_CASE("version and license give non-empty strings") {
		auto r = run_repl({
			R"({"id":1,"cmd":"version"})",
			eval_request(2, "version"),
			R"({"id":3,"cmd":"license"})",
			eval_request(4, "license") });
		REQUIRE(r.responses.size() == 4);
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(r.responses[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			const char* key = i < 2 ? "version" : "license";
			REQUIRE(data->find(key) != nullptr);
			CHECK(data->find(key)->as_string().size() > 0);
		}
	}

	TEST_CASE("clear answers ok with empty data") {
		auto r = run_repl({
			R"({"id":1,"cmd":"clear"})",
			eval_request(2, "clear") });
		REQUIRE(r.responses.size() == 2);
		for (const auto& resp : r.responses) {
			CHECK(resp.find("status")->as_string() == "ok");
			const json::value* data = response_data(resp);
			REQUIRE(data != nullptr);
			CHECK(data->is_object());
			CHECK(data->size() == 0);
		}
	}
}

TEST_SUITE("tgf json api: option commands") {
	TEST_CASE("derive-char-classes is a get and set bool option") {
		auto r = run_repl({
			R"({"id":1,"cmd":"get","option":"derive-char-classes"})",
			R"({"id":2,"cmd":"set","option":"derive-char-classes","value":false})",
			R"({"id":3,"cmd":"get","option":"derive-char-classes"})",
			eval_request(4, "set dcc on . get dcc") });
		REQUIRE(r.responses.size() == 4);
		const json::value* v0 = result_of(r.responses[0])->find("value");
		REQUIRE(v0 != nullptr);
		CHECK(v0->as_bool());
		REQUIRE(result_of(r.responses[1]) != nullptr);
		REQUIRE(result_of(r.responses[1])->find("option") != nullptr);
		CHECK(result_of(r.responses[1])->find("option")->as_string()
			== "derive-char-classes");
		REQUIRE(result_of(r.responses[1])->find("value") != nullptr);
		CHECK(!result_of(r.responses[1])->find("value")->as_bool());
		REQUIRE(result_of(r.responses[2])->find("value") != nullptr);
		CHECK(!result_of(r.responses[2])->find("value")->as_bool());
		auto res = r.responses[3].find("results");
		REQUIRE(res != nullptr);
		REQUIRE(res->size() == 2);
		REQUIRE((*res)[0].find("result")->find("value") != nullptr);
		CHECK((*res)[0].find("result")->find("value")->as_bool());
		REQUIRE((*res)[1].find("result")->find("value") != nullptr);
		CHECK((*res)[1].find("result")->find("value")->as_bool());
	}
	TEST_CASE("toggle, enable and disable flip a bool option") {
		auto r = run_repl({
			R"({"id":1,"cmd":"toggle","option":"print-ambiguity"})",
			R"({"id":2,"cmd":"enable","option":"print-ambiguity"})",
			R"({"id":3,"cmd":"disable","option":"print-ambiguity"})",
			eval_request(4, "toggle print-ambiguity"),
			eval_request(5, "enable print-ambiguity"),
			eval_request(6, "disable print-ambiguity") });
		REQUIRE(r.responses.size() == 6);
		const bool want[6] = { false, true, false, true, true, false };
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(r.responses[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			REQUIRE(data->find("option") != nullptr);
			CHECK(data->find("option")->as_string()
				== "print-ambiguity");
			REQUIRE(data->find("value") != nullptr);
			CHECK(data->find("value")->as_bool() == want[i]);
		}
	}

	TEST_CASE("error-verbosity accepts three values and rejects others") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"error-verbosity","value":"basic"})",
			R"({"id":2,"cmd":"set","option":"error-verbosity","value":"detailed"})",
			R"({"id":3,"cmd":"set","option":"error-verbosity","value":"root-cause"})",
			eval_request(4, "set error-verbosity basic"),
			eval_request(5, "set error-verbosity detailed"),
			eval_request(6, "set error-verbosity root-cause"),
			R"({"id":7,"cmd":"set","option":"error-verbosity","value":"nope"})",
			eval_request(8, "set error-verbosity nope") });
		REQUIRE(r.responses.size() == 8);
		const char* want[6] = { "basic", "detailed", "root-cause",
			"basic", "detailed", "root-cause" };
		for (size_t i = 0; i != 6; ++i) {
			CHECK(r.responses[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			REQUIRE(data->find("option") != nullptr);
			CHECK(data->find("option")->as_string()
				== "error-verbosity");
			REQUIRE(data->find("value") != nullptr);
			CHECK(data->find("value")->as_string() == want[i]);
		}
		CHECK(r.responses[6].find("status")->as_string() == "error");
		CHECK(r.responses[7].find("status")->as_string() == "error");
	}

	TEST_CASE("inline set, add and delete tree paths") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"inline","value":[["a","b"]]})",
			R"({"id":2,"cmd":"add","option":"inline","value":[["c","d"]]})",
			R"({"id":3,"cmd":"delete","option":"inline","value":[["a","b"]]})",
			eval_request(4, "set inline a > b"),
			eval_request(5, "add inline c > d"),
			eval_request(6, "delete inline a > b") });
		REQUIRE(r.responses.size() == 6);
		const std::vector<std::vector<std::string>> one = { { "a", "b" } };
		const std::vector<std::vector<std::string>> two = {
			{ "a", "b" }, { "c", "d" } };
		const std::vector<std::vector<std::string>> c_only = {
			{ "c", "d" } };
		const std::vector<std::vector<std::string>> want[6] = {
			one, two, c_only, one, two, c_only };
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(r.responses[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			CHECK(treepaths_are(*data->find("value"), want[i]));
		}
	}

	TEST_CASE("set gc false reaches the parse options") {
		tgf_repl_evaluator re(grammar_path(), json_options());
		std::string src = R"({"id":1,"cmd":"set","option":"gc","value":false})";
		src += "\n" + eval_request(2, "get gc");
		src += "\n" + eval_request(3, "set gc true");
		src += "\n" + eval_request(4, "get gc");
		src += "\n";
		src += R"({"id":5,"cmd":"set","option":"gc","value":false})";
		src += "\n";
		std::istringstream in(src);
		std::ostringstream out;
		tgf_json_loop(re, in, out);
		std::istringstream ls(out.str());
		std::string line;
		REQUIRE(static_cast<bool>(std::getline(ls, line)));   // hello
		std::vector<json::value> vs;
		while (std::getline(ls, line)) vs.push_back(parse_line(line));
		REQUIRE(vs.size() == 5);
		const bool want[5] = { false, false, true, true, false };
		for (size_t i = 0; i != vs.size(); ++i) {
			CHECK(vs[i].find("status")->as_string() == "ok");
			const json::value* data = response_data(vs[i]);
			REQUIRE(data != nullptr);
			REQUIRE(data->find("value") != nullptr);
			CHECK(data->find("value")->as_bool() == want[i]);
		}
		CHECK(!re.get_parse_options().enable_gc);
	}
}

TEST_SUITE("tgf json api: request safety") {
	TEST_CASE("a structured set cannot inject REPL text") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["a . quit"]})",
			R"({"id":2,"cmd":"set","option":"nope","value":true})",
			R"({"id":3,"cmd":"help","command":"bogus"})",
			R"({"id":4,"cmd":"version"})" });
		REQUIRE(r.responses.size() == 4);
		CHECK(r.responses[0].find("status")->as_string() == "error");
		CHECK(r.responses[1].find("status")->as_string() == "error");
		CHECK(r.responses[2].find("status")->as_string() == "error");
		CHECK(r.responses[3].find("status")->as_string() == "ok");
		const json::value* data = response_data(r.responses[3]);
		REQUIRE(data != nullptr);
		REQUIRE(data->find("version") != nullptr);
		CHECK(data->find("version")->as_string().size() > 0);
	}

	TEST_CASE("a missing or wrongly typed field is an error") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim"})",
			R"({"id":2,"cmd":"set","option":"trim","value":true})",
			R"({"id":3,"cmd":"parse"})",
			R"({"id":4,"cmd":"parse","input":123})" });
		REQUIRE(r.responses.size() == 4);
		for (const auto& resp : r.responses)
			CHECK(resp.find("status")->as_string() == "error");
	}
}

TEST_SUITE("tgf json api: state") {
	TEST_CASE("every response carries state and start num updates it") {
		auto r = run_repl({
			R"({"id":1,"cmd":"version"})",
			R"({"id":2,"cmd":"start","symbol":"num"})",
			R"({"id":3,"cmd":"bogus"})",
			eval_request(4, "start") });
		REQUIRE(r.responses.size() == 4);
		for (const auto& resp : r.responses) {
			const json::value* s = resp.find("state");
			REQUIRE(s != nullptr);
			REQUIRE(s->find("grammar") != nullptr);
			CHECK(s->find("grammar")->as_string() == grammar_path());
			REQUIRE(s->find("start") != nullptr);
		}
		REQUIRE(r.responses[0].find("state") != nullptr);
		REQUIRE(r.responses[0].find("state")->find("start") != nullptr);
		CHECK(r.responses[0].find("state")->find("start")
			->as_string() == "start");
		REQUIRE(r.responses[1].find("state") != nullptr);
		REQUIRE(r.responses[1].find("state")->find("start") != nullptr);
		CHECK(r.responses[1].find("state")->find("start")
			->as_string() == "num");
		REQUIRE(r.responses[2].find("state") != nullptr);
		REQUIRE(r.responses[2].find("state")->find("start") != nullptr);
		CHECK(r.responses[2].find("state")->find("start")
			->as_string() == "num");
		REQUIRE(r.responses[3].find("state") != nullptr);
		REQUIRE(r.responses[3].find("state")->find("start") != nullptr);
		CHECK(r.responses[3].find("state")->find("start")
			->as_string() == "num");
		auto res = r.responses[3].find("results");
		REQUIRE(res != nullptr);
		REQUIRE(res->size() == 1);
		CHECK((*res)[0].find("state") == nullptr);
	}
}

TEST_SUITE("tgf json api: production ids") {
	TEST_CASE("production_ids matches productions on tiny.tgf") {
		auto r = run_repl({
			R"({"id":1,"cmd":"internal-grammar"})"});
		REQUIRE(r.responses.size() == 1);
		const json::value* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		const json::value* prods = data->find("productions");
		const json::value* pids = data->find("production_ids");
		REQUIRE(prods != nullptr);
		REQUIRE(pids != nullptr);
		REQUIRE(prods->size() == 4);
		CHECK(pids->size() == prods->size());
		for (size_t i = 0; i != pids->size(); ++i) {
			const json::value& e = (*pids)[i];
			REQUIRE(e.find("index") != nullptr);
			CHECK(e.find("index")->as_number()
				== static_cast<double>(i));
			REQUIRE(e.find("head") != nullptr);
			CHECK(e.find("head")->is_number());
			REQUIRE(e.find("body") != nullptr);
			CHECK(e.find("body")->is_array());
			REQUIRE(e.find("guard") != nullptr);
			CHECK(e.find("guard")->is_null());
			REQUIRE(e.find("conjunctive") != nullptr);
			CHECK(!e.find("conjunctive")->as_bool());
		}
		CHECK((*prods)[0].as_string() == "start => num.");
		const json::value* body = (*pids)[0].find("body");
		REQUIRE(body != nullptr);
		REQUIRE(body->size() == 1);
		REQUIRE((*body)[0].size() == 1);
		CHECK((*body)[0][0].is_number());
	}

	TEST_CASE("a guarded production carries its guard") {
		auto r = run_repl_on(csv_grammar_path(), {
			R"({"id":1,"cmd":"internal-grammar"})"});
		REQUIRE(r.responses.size() == 1);
		const json::value* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		const json::value* pids = data->find("production_ids");
		REQUIRE(pids != nullptr);
		bool found = false;
		for (const auto& e : *pids) {
			const json::value* g = e.find("guard");
			if (g && !g->is_null()) {
				CHECK(g->is_string());
				CHECK(g->as_string().size() > 0);
				found = true;
			}
		}
		CHECK(found);
	}

	TEST_CASE("a conjunctive production has two conjuncts") {
		auto r = run_repl_on(json_grammar_path(), {
			R"({"id":1,"cmd":"internal-grammar"})"});
		REQUIRE(r.responses.size() == 1);
		const json::value* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		const json::value* pids = data->find("production_ids");
		REQUIRE(pids != nullptr);
		bool found = false;
		for (const auto& e : *pids) {
			REQUIRE(e.find("conjunctive") != nullptr);
			if (e.find("conjunctive")->as_bool()) {
				REQUIRE(e.find("body") != nullptr);
				CHECK(e.find("body")->size() >= 2);
				found = true;
			}
		}
		CHECK(found);
	}
}

TEST_SUITE("tgf json api: ambiguous data") {
	TEST_CASE("trees and alternative children on ambig_bc.tgf") {
		auto r = run_repl_on(ambig_bc_path(), {
			R"({"id":1,"cmd":"parse","input":"x"})"});
		REQUIRE(r.responses.size() == 1);
		const json::value* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		const json::value* amb = data->find("ambiguous");
		REQUIRE(amb != nullptr);
		REQUIRE(amb->find("trees") != nullptr);
		CHECK(amb->find("trees")->as_number() == 2);
		const json::value* nodes = amb->find("nodes");
		REQUIRE(nodes != nullptr);
		REQUIRE(nodes->size() == 1);
		const json::value& n = (*nodes)[0];
		REQUIRE(n.find("symbol") != nullptr);
		CHECK(n.find("symbol")->as_string() == "A");
		CHECK(n.find("id")->is_number());
		REQUIRE(n.find("range") != nullptr);
		CHECK(n.find("range")->size() == 2);
		const json::value* alts = n.find("alternatives");
		REQUIRE(alts != nullptr);
		REQUIRE(alts->size() == 2);
		std::set<std::string> got;
		for (size_t i = 0; i != 2; ++i) {
			const json::value* ch = (*alts)[i].find("children");
			REQUIRE(ch != nullptr);
			REQUIRE(ch->size() == 1);
			REQUIRE((*ch)[0].find("symbol") != nullptr);
			got.insert((*ch)[0].find("symbol")->as_string());
			CHECK((*ch)[0].find("id")->is_number());
			CHECK((*ch)[0].find("children") != nullptr);
		}
		CHECK(got == std::set<std::string>({ "B", "C" }));
	}
}

TEST_SUITE("tgf json api: cmd echo") {
	TEST_CASE("a structured response carries the canonical cmd") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["a"]})",
			R"({"id":2,"cmd":"internal-grammar"})",
			eval_request(3, "p 12") });
		REQUIRE(r.responses.size() == 3);
		CHECK(r.responses[0].find("cmd")->as_string() == "set");
		CHECK(r.responses[1].find("cmd")->as_string()
			== "internal-grammar");
		const auto& m = r.responses[0].members();
		REQUIRE(m.size() >= 2);
		CHECK(m[0].first == "id");
		CHECK(m[1].first == "cmd");
		auto res = r.responses[2].find("results");
		REQUIRE(res != nullptr);
		REQUIRE(res->size() == 1);
		CHECK((*res)[0].find("cmd")->as_string() == "parse");
	}
}

TEST_SUITE("tgf json api: one-shot CLI") {
	static std::string capture_run(const std::vector<std::string>& args,
		int& code)
	{
		std::vector<char*> argv;
		argv.reserve(args.size());
		for (const auto& a : args)
			argv.push_back(const_cast<char*>(a.c_str()));
		std::ostringstream cap;
		auto* old = std::cout.rdbuf(cap.rdbuf());
		code = tgf_run(static_cast<int>(argv.size()), argv.data());
		std::cout.rdbuf(old);
		return cap.str();
	}

	static std::vector<json::value> lines_of(const std::string& s) {
		std::vector<json::value> out;
		std::istringstream ls(s);
		std::string l;
		while (std::getline(ls, l)) {
			if (l.empty()) continue;
			out.push_back(parse_line(l));
		}
		return out;
	}

	TEST_CASE("parse --json writes one response") {
		int code = -1;
		auto text = capture_run({ "tgf", grammar_path(), "parse",
			"--json", "-e", "123" }, code);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(vs[0].find("status")->as_string() == "ok");
		CHECK(vs[0].find("cmd")->as_string() == "parse");
		REQUIRE(vs[0].find("result") != nullptr);
		CHECK(vs[0].find("result")->find("tree") != nullptr);
		const json::value* s = vs[0].find("state");
		REQUIRE(s != nullptr);
		REQUIRE(s->find("grammar") != nullptr);
		CHECK(s->find("grammar")->as_string() == grammar_path());
		REQUIRE(s->find("start") != nullptr);
		CHECK(s->find("start")->as_string() == "start");
		CHECK(code == 0);
	}

	TEST_CASE("grammar --json writes one response") {
		int code = -1;
		auto text = capture_run({ "tgf", grammar_path(), "grammar",
			"--json" }, code);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(vs[0].find("cmd")->as_string() == "grammar");
		auto res = vs[0].find("result");
		REQUIRE(res != nullptr);
		REQUIRE(res->find("start") != nullptr);
		CHECK(res->find("start")->as_string() == "start");
		CHECK(res->find("productions") != nullptr);
		REQUIRE(res->find("production_ids") != nullptr);
		REQUIRE(res->find("productions") != nullptr);
		CHECK(res->find("production_ids")->size()
			== res->find("productions")->size());
		CHECK(vs[0].find("state") != nullptr);
		CHECK(code == 0);
	}

	TEST_CASE("gen --json writes one response with the file list") {
		auto dir = std::filesystem::temp_directory_path()
			/ "tgf_json_api_gen";
		std::filesystem::create_directories(dir);
		int code = -1;
		auto text = capture_run({ "tgf", grammar_path(), "gen",
			"--json", "--name", "tiny", "--header-only", "false",
			"--output-dir", dir.string() }, code);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(vs[0].find("status")->as_string() == "ok");
		CHECK(vs[0].find("cmd")->as_string() == "gen");
		REQUIRE(vs[0].find("result") != nullptr);
		auto files = vs[0].find("result")->find("files");
		REQUIRE(files != nullptr);
		CHECK(files->size() >= 2);
		CHECK(vs[0].find("state") != nullptr);
		CHECK(code == 0);
		std::filesystem::remove_all(dir);
	}

	TEST_CASE("repl --json --evaluate writes one response and no hello") {
		int code = -1;
		auto text = capture_run({ "tgf", grammar_path(), "repl",
			"--json", "--evaluate", "version" }, code);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(vs[0].find("hello") == nullptr);
		CHECK(vs[0].find("status")->as_string() == "ok");
		auto res = vs[0].find("results");
		REQUIRE(res != nullptr);
		REQUIRE(res->size() == 1);
		CHECK((*res)[0].find("cmd")->as_string() == "version");
		REQUIRE(vs[0].find("state") != nullptr);
		REQUIRE(vs[0].find("state")->find("start") != nullptr);
		CHECK(vs[0].find("state")->find("start")->as_string()
			== "start");
		CHECK(code == 0);
	}

	TEST_CASE("parse --json --grammar adds internal_grammar") {
		int code = -1;
		auto text = capture_run({ "tgf", grammar_path(), "parse",
			"--json", "--grammar", "-e", "123" }, code);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(vs[0].find("status")->as_string() == "ok");
		CHECK(vs[0].find("cmd")->as_string() == "parse");
		REQUIRE(vs[0].find("result") != nullptr);
		auto ig = vs[0].find("result")->find("internal_grammar");
		REQUIRE(ig != nullptr);
		REQUIRE(ig->find("start") != nullptr);
		CHECK(ig->find("start")->as_string() == "start");
		auto prods = ig->find("productions");
		REQUIRE(prods != nullptr);
		CHECK(prods->size() > 0);
		REQUIRE(ig->find("production_ids") != nullptr);
		CHECK(ig->find("production_ids")->size() == prods->size());
		CHECK(code == 0);
	}

	TEST_CASE("parse --json --measure adds bintree_totals and no text") {
		int code = -1;
		auto text = capture_run({ "tgf", grammar_path(), "parse",
			"--json", "--measure", "-e", "123" }, code);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(vs[0].find("status")->as_string() == "ok");
		CHECK(vs[0].find("cmd")->as_string() == "parse");
		REQUIRE(vs[0].find("result") != nullptr);
		auto bt = vs[0].find("result")->find("bintree_totals");
		REQUIRE(bt != nullptr);
		CHECK(bt->find("get_hits") != nullptr);
		const json::value* hist = bt->find("chain_length_histogram");
		REQUIRE(hist != nullptr);
		CHECK(hist->find("0") != nullptr);
		CHECK(hist->find("64+") != nullptr);
		const json::value* groups = bt->find("top_hash_groups");
		REQUIRE(groups != nullptr);
		CHECK(groups->size() == 5);
		for (const auto& g : *groups) {
			REQUIRE(g.find("size") != nullptr);
			CHECK(g.find("size")->is_number());
			REQUIRE(g.find("triples") != nullptr);
			CHECK(g.find("triples")->is_number());
		}
		REQUIRE(bt->find("largest_group_samples") != nullptr);
		CHECK(text.find("bintree lookups") == std::string::npos);
		CHECK(code == 0);
	}

	TEST_CASE("grammar --json --nullable adds symbol id and index") {
		scratch_file g("tgf_json_api_nullable.tgf",
			"start => a.\na => a b | 'x'.\nb => [ 'y' ].\n");
		int code = -1;
		auto text = capture_run({ "tgf", g.path, "grammar",
			"--json", "--nullable" }, code);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(vs[0].find("cmd")->as_string() == "grammar");
		REQUIRE(vs[0].find("result") != nullptr);
		auto nl = vs[0].find("result")->find("nullable");
		REQUIRE(nl != nullptr);
		REQUIRE(nl->size() == 1);
		const json::value& e = (*nl)[0];
		REQUIRE(e.find("symbol") != nullptr);
		CHECK(e.find("symbol")->as_string() == "a");
		REQUIRE(e.find("id") != nullptr);
		CHECK(e.find("id")->is_number());
		REQUIRE(e.find("index") != nullptr);
		CHECK(e.find("index")->is_number());
		REQUIRE(e.find("production") != nullptr);
		CHECK(e.find("production")->is_string());
		CHECK(code == 0);
	}

	TEST_CASE("JSON mode writes only JSON objects, one per line") {
		int code = -1;
		auto text = capture_run({ "tgf", grammar_path(), "parse",
			"--json", "--print-input", "--tml-rules",
			"--tml-facts", "--measure", "-e", "123" }, code);
		std::istringstream ls(text);
		std::string l;
		size_t lines = 0;
		while (std::getline(ls, l)) {
			if (l.empty()) continue;
			++lines;
			CHECK(l.front() == '{');
			auto p = json::parse(l);
			CHECK(p.has_value());
		}
		CHECK(lines == 1);
		CHECK(code == 0);
	}
}
