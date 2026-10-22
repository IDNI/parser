// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the tgf JSON API front end (tgf_json_loop) and the one-shot
// --json forms of the parse, grammar and gen commands.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "../src/tgf/tgf_cli.h"
#include "tgf_run_capture.h"
#include "serve/tgf_serve_test_fixture.h"
#include "format/json/json.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace idni;
using namespace idni::format;
using tgf_serve_test::grammar_path;
using tgf_serve_test::json_node_has_key;
using tgf_serve_test::member;
using tgf_serve_test::parse_line;
using tgf_serve_test::repl_options;
using tgf_serve_test::report_contains_key;

using jt = json::tree;
using jtt = json::tt;

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

TEST_CASE("tgf error response preserves the id without session state") {
	bintree<json::node>::gc_pause pause;
	diagnostics::report rep;
	rep.error(diagnostics::code::invalid_argument, "invalid request");
	const tref id = jt::build_string("request-42");
	const jt& response = jt::get(tgf_repl_evaluator_base::error_response(id, rep));
	CHECK(response.find("id") == id);
	REQUIRE(member(response, "status") != nullptr);
	CHECK(member(response, "status")->get_string() == "error");
	REQUIRE(member(response, "report") != nullptr);
	auto decoded = json::report_of(response.find("report"));
	CHECK(decoded.has_error());
	CHECK(decoded.first_error_code() == diagnostics::code::invalid_argument);
	CHECK(decoded.first_error_message() == "invalid request");
	CHECK(member(response, "state") == nullptr);
	CHECK(member(response, "cmd") == nullptr);
}

TEST_SUITE("tgf serve: report keys") {
	TEST_CASE("only keys in report children match") {
		using tgf_serve_test::json_node_has_key;
		auto node = tgf_serve_test::parse_line(R"({"key":"root","children":[
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

struct repl_run {
	std::vector<htref> responses;   // hello is not included
	std::string raw;
};

// Drive @p re with @p lines and collect the response values.
static repl_run run_repl_re(tgf_repl_evaluator& re,
	const std::vector<std::string>& lines)
{
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

// Drive one evaluator on @p grammar with @p lines and collect the
// response values.
static repl_run run_repl_on(const std::string& grammar,
	const std::vector<std::string>& lines)
{
	tgf_repl_evaluator re(grammar, repl_options());
	return run_repl_re(re, lines);
}

// Drive one evaluator with no grammar.
static repl_run run_repl_no_grammar(
	const std::vector<std::string>& lines)
{
	tgf_repl_evaluator re(repl_options());
	return run_repl_re(re, lines);
}

// Drive one evaluator with @p lines and collect the response values.
static repl_run run_repl(const std::vector<std::string>& lines) {
	return run_repl_on(grammar_path(), lines);
}

static const jt* result_of(const htref& resp) {
	return member(resp, "result");
}

// Result data: the structured result, or an eval response's first result.
static const jt* response_data(const htref& resp) {
	if (auto d = member(resp, "result"); d) return d;
	if (auto rs = member(resp, "results"); rs && rs->children_size())
		return member((*rs)[0], "result");
	return nullptr;
}

// A quoted REPL string. The encoder turns a backslash into an escape
// sequence, so a native path survives the REPL string decoder.
static std::string repl_string(const std::string& s) {
	return "\"" + escapes::encode(s, escapes::tgf_string) + "\"";
}

// The JSON text of the request {"id":id, then the string @p fields}.
static std::string request_text(int id,
	std::initializer_list<std::pair<const char*, std::string>> fields)
{
	bintree<json::node>::gc_pause pause;
	trefs v{ jt::build_pair("id",
		jt::build_number(static_cast<double>(id))) };
	for (const auto& [key, text] : fields)
		v.push_back(jt::build_pair(key, jt::build_string(text)));
	return jt::get(jt::build_object(v)).to_str();
}

// {"id":id,"cmd":"eval","src":src}
static std::string eval_request(int id, const std::string& src) {
	return request_text(id, { { "cmd", "eval" }, { "src", src } });
}

// {"id":id,"cmd":cmd,"file":path}
static std::string file_request(int id, const std::string& cmd,
	const std::string& path)
{
	return request_text(id, { { "cmd", cmd }, { "file", path } });
}

// {"id":id,"cmd":"load","name":name,"source":source}
static std::string load_source_request(int id, const std::string& name,
	const std::string& source)
{
	return request_text(id, { { "cmd", "load" }, { "name", name },
		{ "source", source } });
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
static bool strings_are(const jt* v,
	const std::vector<std::string>& want)
{
	if (!v || !v->is_array() || v->children_size() != want.size())
		return false;
	for (size_t i = 0; i != want.size(); ++i)
		if ((*v)[i].get_string() != want[i]) return false;
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
		auto h = member(p, "hello");
		REQUIRE(h != nullptr);
		REQUIRE(member(h, "protocol") != nullptr);
		CHECK(member(h, "protocol")->get_number() == 1);
		REQUIRE(member(h, "grammar") != nullptr);
		CHECK(member(h, "grammar")->get_string() == grammar_path());
		REQUIRE(member(h, "fixed_grammar") != nullptr);
		CHECK(member(h, "fixed_grammar")->is_bool());
		REQUIRE(member(h, "start") != nullptr);
		CHECK(member(h, "start")->get_string() == "start");
		auto opts = member(h, "options");
		REQUIRE(opts != nullptr);
		CHECK(member(opts, "debug") != nullptr);
		CHECK(member(opts, "trim") != nullptr);
		CHECK(member(opts, "inline") != nullptr);
		CHECK(member(opts, "derive-char-classes") != nullptr);
		CHECK(member(opts, "error-verbosity") != nullptr);
		CHECK(member(h, "report") != nullptr);
		const jt* s = member(p, "state");
		REQUIRE(s != nullptr);
		REQUIRE(member(s, "grammar") != nullptr);
		CHECK(member(s, "grammar")->get_string() == grammar_path());
		REQUIRE(member(s, "start") != nullptr);
		CHECK(member(s, "start")->get_string() == "start");
		// state names the character class nonterminals.
		REQUIRE(member(s, "char_classes") != nullptr);
		CHECK(member(s, "char_classes")->is_array());
		CHECK(member(s, "char_classes")->children_size() >= 1);
		// the options carry the directives of the grammar file
		auto cc = member(opts, "char-classes");
		REQUIRE(cc != nullptr);
		REQUIRE(cc->is_array());
		REQUIRE(cc->children_size() == 1);
		CHECK((*cc)[0].get_string() == "digit");
		REQUIRE(member(opts, "start") != nullptr);
		CHECK(member(opts, "start")->get_string() == "start");
		CHECK(member(h, "directives") == nullptr);
	}

	TEST_CASE("a session sees no option of the tool") {
		options_repository mine;
		options_scope scope(mine);
		// the tool declares port; the evaluator declares json
		REQUIRE(idni::options().declare(tgf_option_set).has_value());
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"port","value":80})" });
		std::istringstream ls(r.raw);
		std::string line;
		REQUIRE(static_cast<bool>(std::getline(ls, line)));
		auto p = parse_line(line);
		auto h = member(p, "hello");
		REQUIRE(h != nullptr);
		auto opts = member(h, "options");
		REQUIRE(opts != nullptr);
		CHECK(member(opts, "port") == nullptr);
		CHECK(member(opts, "json") == nullptr);
		CHECK(member(opts, "trim") != nullptr);
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "status")->get_string() == "error");
	}

	// The options after a load of src, read from the evaluator.
	static htref load_options(tgf_repl_evaluator& re,
		const std::string& src, bool reload = false)
	{
		std::vector<std::string> lines = {
			load_source_request(1, "g.tgf", src) };
		if (reload) lines.push_back(R"({"id":2,"cmd":"reload"})");
		auto r = run_repl_re(re, lines);
		REQUIRE(r.responses.size() == lines.size());
		for (const auto& resp : r.responses) {
			const jt* data = response_data(resp);
			REQUIRE(data != nullptr);
			REQUIRE(member(data, "loaded") != nullptr);
			CHECK(member(data, "loaded")->get_bool());
			CHECK(member(data, "directives") == nullptr);
		}
		bintree<json::node>::gc_pause pause;
		return jt::geth(re.option_values());
	}

	// The entries of the list option @p name in @p o.
	static std::vector<std::string> entries(const htref& o,
		const std::string& name)
	{
		auto l = jtt(o) | jtt::member(name);
		REQUIRE(l.is(json_parser::arr));
		std::vector<std::string> out;
		for (const auto& e : (l | jtt::children)())
			out.push_back(e | jtt::string);
		return out;
	}

	TEST_CASE("a load and a reload give the directives as options") {
		tgf_repl_evaluator re(repl_options());
		auto o = load_options(re, "@start num.\n"
			"@use char classes digit, alpha.\n"
			"@enable productions comma, crlf.\n"
			"num => digit+.\n", true);
		REQUIRE((jtt(o) | jtt::member("start")).has_value());
		CHECK((jtt(o) | jtt::member("start") | jtt::string) == "num");
		CHECK(entries(o, "char-classes")
			== std::vector<std::string>{ "digit", "alpha" });
		CHECK(entries(o, "enabled-productions")
			== std::vector<std::string>{ "comma", "crlf" });
	}

	TEST_CASE("the enabled productions hold only guard names") {
		tgf_repl_evaluator re(repl_options());
		auto o = load_options(re, "@enable productions guard1.\n"
			"@enable disambiguation.\n"
			"@disable disambiguation.\n"
			"start => 'a'.\n");
		CHECK(entries(o, "enabled-productions")
			== std::vector<std::string>{ "guard1" });
		REQUIRE((jtt(o) | jtt::member("auto-disambiguate")).has_value());
		CHECK_FALSE((jtt(o) | jtt::member("auto-disambiguate")
			| jtt::boolean));
	}

	TEST_CASE("a renamed class is the entry class as name") {
		tgf_repl_evaluator re(repl_options());
		auto o = load_options(re,
			"@use char classes alpha, digit as d.\n"
			"start => alpha d.\n");
		CHECK(entries(o, "char-classes")
			== std::vector<std::string>{ "alpha", "digit as d" });
	}

	TEST_CASE("the classes without a rename are plain entries") {
		tgf_repl_evaluator re(repl_options());
		auto o = load_options(re,
			"@use char classes alpha, digit.\n"
			"start => alpha digit.\n");
		CHECK(entries(o, "char-classes")
			== std::vector<std::string>{ "alpha", "digit" });
	}
}

TEST_SUITE("tgf json api: eval form") {
	TEST_CASE("one command, id echoed, results array") {
		auto r = run_repl({R"({"id":7,"cmd":"eval","src":"version"})"});
		REQUIRE(r.responses.size() == 1);
		const auto& v = r.responses[0];
		CHECK(member(v, "id")->get_number() == 7);
		CHECK(member(v, "status")->get_string() == "ok");
		auto res = member(v, "results");
		REQUIRE(res != nullptr);
		REQUIRE(res->children_size() == 1);
		CHECK(member((*res)[0], "cmd")->get_string() == "version");
		REQUIRE(member((*res)[0], "result") != nullptr);
		CHECK(member(member((*res)[0], "result"), "version") != nullptr);
	}

	TEST_CASE("multi statement input gives one entry per statement") {
		auto r = run_repl({
			R"({"id":1,"cmd":"eval","src":"version . license"})"});
		REQUIRE(r.responses.size() == 1);
		auto res = member(r.responses[0], "results");
		REQUIRE(res != nullptr);
		REQUIRE(res->children_size() == 2);
		CHECK(member((*res)[0], "cmd")->get_string() == "version");
		CHECK(member((*res)[1], "cmd")->get_string() == "license");
	}

	TEST_CASE("incomplete input is reported and keeps no partial text") {
		auto r = run_repl({R"({"id":3,"cmd":"eval","src":"set"})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "id")->get_number() == 3);
		CHECK(member(r.responses[0], "status")->get_string() == "incomplete");
		CHECK(member(r.responses[0], "state") != nullptr);
		CHECK(member(r.responses[0], "report") != nullptr);
	}

	TEST_CASE("a missing src is an error") {
		auto r = run_repl({R"({"id":4,"cmd":"eval"})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "status")->get_string() == "error");
		CHECK(member(r.responses[0], "id")->get_number() == 4);
	}
}

TEST_SUITE("tgf json api: invalid and unknown requests") {
	TEST_CASE("invalid JSON gives id null and an error") {
		auto r = run_repl({"{ not json"});
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "status")->get_string() == "error");
		CHECK(member(r.responses[0], "state") != nullptr);
		CHECK(member(r.responses[0], "id")->is_null());
		CHECK(member(r.responses[0], "report") != nullptr);
	}

	TEST_CASE("an unknown cmd is an error") {
		auto r = run_repl({R"({"id":5,"cmd":"bogus"})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "status")->get_string() == "error");
		CHECK(member(r.responses[0], "id")->get_number() == 5);
	}
}

TEST_SUITE("tgf json api: command names") {
	static const std::string unknown_command(
		parser_strings::messages::unknown_command);

	TEST_CASE("name_of drops the _cmd or _sym suffix") {
		using rp = tgf_repl_parser;
		using base = tgf_repl_evaluator_base;
		CHECK(base::name_of(rp::parse_file_cmd) == "parse_file");
		CHECK(base::name_of(rp::parse_file_sym) == "parse_file");
		CHECK(base::name_of(rp::help_arg).empty());
		CHECK(base::name_of(0).empty());
	}

	TEST_CASE("the old long names are unknown commands") {
		scratch_file in("tgf_json_api_names.txt", "123");
		auto r = run_repl({
			file_request(1, "parse file", in.path),
			R"({"id":2,"cmd":"internal-grammar"})",
			R"({"id":3,"cmd":"delete","option":"trim","value":["num"]})" });
		REQUIRE(r.responses.size() == 3);
		for (const auto& resp : r.responses) {
			CHECK(member(resp, "status")->get_string() == "error");
			CHECK(member(resp, "result") == nullptr);
			CHECK(report_contains_key(resp, unknown_command));
		}
	}

	TEST_CASE("a structured request with a new name runs") {
		scratch_file in("tgf_json_api_names.txt", "123");
		auto r = run_repl({
			file_request(1, "parse_file", in.path),
			R"({"id":2,"cmd":"igrammar"})",
			R"({"id":3,"cmd":"igrammar","symbol":"num"})",
			R"({"id":4,"cmd":"add","option":"trim","value":["num"]})",
			R"({"id":5,"cmd":"del","option":"trim","value":["num"]})" });
		REQUIRE(r.responses.size() == 5);
		const char* cmds[] = { "parse_file", "igrammar", "igrammar",
			"add", "del" };
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(member(r.responses[i], "status")->get_string()
				== "ok");
			CHECK(member(r.responses[i], "cmd")->get_string()
				== cmds[i]);
		}
	}

	TEST_CASE("help takes a new name and gives it back") {
		auto r = run_repl({
			R"({"id":1,"cmd":"help","command":"parse_file"})",
			R"({"id":2,"cmd":"help","command":"igrammar"})",
			R"({"id":3,"cmd":"help","command":"del"})",
			R"({"id":4,"cmd":"help","command":"parse file"})",
			R"({"id":5,"cmd":"help","command":"internal-grammar"})" });
		REQUIRE(r.responses.size() == 5);
		const char* names[] = { "parse_file", "igrammar", "del" };
		for (size_t i = 0; i != 3; ++i) {
			CHECK(member(r.responses[i], "status")->get_string()
				== "ok");
			const jt* data = result_of(r.responses[i]);
			REQUIRE(data != nullptr);
			CHECK(member(data, "command")->get_string() == names[i]);
		}
		CHECK(member(r.responses[3], "status")->get_string() == "error");
		CHECK(member(r.responses[4], "status")->get_string() == "error");
	}

	TEST_CASE("the REPL accepts the new names as words") {
		scratch_file in("tgf_json_api_names.txt", "123");
		auto r = run_repl({
			eval_request(1, "parse_file " + repl_string(in.path)),
			eval_request(2, "igrammar"),
			eval_request(3, "igrammar num"),
			eval_request(4, "help parse_file"),
			eval_request(5, "help igrammar"),
			eval_request(6, "add trim num"),
			eval_request(7, "delete trim num") });
		REQUIRE(r.responses.size() == 7);
		const char* cmds[] = { "parse_file", "igrammar", "igrammar",
			"help", "help", "add", "del" };
		for (size_t i = 0; i != r.responses.size(); ++i) {
			const auto& resp = r.responses[i];
			auto res = member(resp, "results");
			REQUIRE(res != nullptr);
			REQUIRE(res->children_size() == 1);
			CHECK(member((*res)[0], "cmd")->get_string() == cmds[i]);
			CHECK(member((*res)[0], "status")->get_string() == "ok");
		}
	}
}

TEST_SUITE("tgf json api: option values as JSON") {

	// The option value of the JSON text @p text.
	static std::optional<option_value> option_of_text(
		std::string_view text)
	{
		auto t = jt::get(text);
		REQUIRE(t.has_value());
		return option_of(t.value());
	}

	TEST_CASE("each kind gives its JSON form") {
		CHECK(jt::get(build_option(option_value(true))).to_str()
			== "true");
		CHECK(jt::get(build_option(option_value(false))).to_str()
			== "false");
		CHECK(jt::get(build_option(option_value(size_t{ 7 })))
			.to_str() == "7");
		CHECK(jt::get(build_option(option_value(std::string("x"))))
			.to_str() == R"("x")");
		CHECK(jt::get(build_option(option_value(
			option_list{ "a > b", "c" }))).to_str()
			== R"(["a > b","c"])");
		CHECK(jt::get(build_option(option_value(option_list{})))
			.to_str() == "[]");
	}

	TEST_CASE("each kind reads back from its JSON form") {
		for (const option_value& v : { option_value(true),
			option_value(size_t{ 0 }), option_value(size_t{ 42 }),
			option_value(std::string("")),
			option_value(std::string("x")),
			option_value(option_list{}),
			option_value(option_list{ "a", "b" }) })
		{
			auto back = option_of(build_option(v));
			REQUIRE(back.has_value());
			CHECK(*back == v);
		}
		auto big = option_of_text("9007199254740992");
		REQUIRE(big.has_value());
		CHECK(std::get<size_t>(*big) == size_t{ 9007199254740992u });
		auto zero = option_of_text("-0");
		REQUIRE(zero.has_value());
		CHECK(std::get<size_t>(*zero) == 0);
	}

	TEST_CASE("a wrong kind, a negative number, a fraction or a number "
		"over 2^53 gives nothing")
	{
		for (const char* text : { "null", "{}", R"({"a":1})", "[1]",
			R"(["a",true])", "[[]]", "-1", "1.5", "-0.5",
			"9007199254740994", "1e300" })
		{
			CAPTURE(text);
			CHECK_FALSE(option_of_text(text).has_value());
		}
		CHECK_FALSE(option_of(nullptr).has_value());
	}

	TEST_CASE("a number over the largest size_t gives nothing") {
		// 2^32 - 1 fits a 32-bit size_t, 2^32 fits only a 64-bit one
		CHECK(option_of_text("4294967295").has_value());
		CHECK(option_of_text("4294967296").has_value()
			== (std::numeric_limits<size_t>::max() >= 4294967296ull));
	}
}

TEST_SUITE("tgf json api: gc") {
	TEST_CASE("many requests keep the node cache small and the state") {
		tgf_repl_evaluator re(grammar_path(), repl_options());
		std::vector<std::string> lines{
			R"({"id":1,"cmd":"start","symbol":"num"})",
			R"({"id":2,"cmd":"set","option":"print-ambiguity",)"
			R"("value":false})" };
		// Each answer has new nodes: its id, its input and its terminals.
		const size_t n = 3000;
		for (size_t i = 0; i != n; ++i)
			lines.push_back(R"({"id":)" + std::to_string(10 + i)
				+ R"(,"cmd":"parse","input":")"
				+ std::to_string(100000 + i) + "\"}");
		lines.push_back(R"({"id":5000,"cmd":"get",)"
			R"("option":"print-ambiguity"})");
		lines.push_back(R"({"id":5001,"cmd":"parse","input":"42"})");
		std::string input;
		for (const auto& l : lines) input += l + "\n";
		std::istringstream in(input);
		std::ostringstream out;
		tgf_json_loop(re, in, out);
		CHECK(jt::m_size() < 16384);
		// Only the last two answers are read: the JSON parser is slow.
		std::vector<std::string> answers;
		std::istringstream ls(out.str());
		for (std::string l; std::getline(ls, l); ) answers.push_back(l);
		REQUIRE(answers.size() == lines.size() + 1);   // and hello
		const htref get = parse_line(answers[n + 3]);
		REQUIRE(result_of(get) != nullptr);
		REQUIRE(member(result_of(get), "value") != nullptr);
		CHECK_FALSE(member(result_of(get), "value")->get_bool());
		const htref parse = parse_line(answers[n + 4]);
		CHECK(member(parse, "status")->get_string() == "ok");
		REQUIRE(member(parse, "state") != nullptr);
		CHECK(member(member(parse, "state"), "start")->get_string() == "num");
		REQUIRE(result_of(parse) != nullptr);
		CHECK(member(result_of(parse), "ambiguous") == nullptr);
		REQUIRE(member(result_of(parse), "tree") != nullptr);
		CHECK(member(member(result_of(parse), "tree"), "symbol")
			->get_string() == "num");
	}
}

TEST_SUITE("tgf json api: options") {
	TEST_CASE("set and get a list option") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["num","start"]})",
			R"({"id":2,"cmd":"get","option":"trim"})"});
		REQUIRE(r.responses.size() == 2);
		auto v0 = member(result_of(r.responses[0]), "value");
		REQUIRE(v0 != nullptr);
		REQUIRE(v0->children_size() == 2);
		CHECK((*v0)[0].get_string() == "start");
		auto v1 = member(result_of(r.responses[1]), "value");
		REQUIRE(v1 != nullptr);
		CHECK(v1->children_size() == 2);
	}

	TEST_CASE("an empty value clears the list") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["num","start"]})",
			R"({"id":2,"cmd":"set","option":"trim","value":[]})",
			R"({"id":3,"cmd":"get","option":"trim"})"});
		REQUIRE(r.responses.size() == 3);
		REQUIRE(result_of(r.responses[1]) != nullptr);
		REQUIRE(member(result_of(r.responses[1]), "value") != nullptr);
		CHECK(member(result_of(r.responses[1]), "value")->children_size() == 0);
		REQUIRE(result_of(r.responses[2]) != nullptr);
		REQUIRE(member(result_of(r.responses[2]), "value") != nullptr);
		CHECK(member(result_of(r.responses[2]), "value")->children_size() == 0);
	}

	TEST_CASE("eval form set trim and set trim = clear the list") {
		auto r = run_repl({
			R"({"id":1,"cmd":"eval","src":"set trim num, start . get trim"})",
			R"({"id":2,"cmd":"eval","src":"set trim . get trim"})",
			R"({"id":3,"cmd":"eval","src":"set trim num . set trim = . get trim"})"});
		REQUIRE(r.responses.size() == 3);
		auto res0 = member(r.responses[0], "results");
		REQUIRE(res0 != nullptr);
		REQUIRE(res0->children_size() == 2);
		REQUIRE(member((*res0)[1], "result") != nullptr);
		REQUIRE(member(member((*res0)[1], "result"), "value") != nullptr);
		CHECK(member(member((*res0)[1], "result"), "value")->children_size() == 2);
		auto res1 = member(r.responses[1], "results");
		REQUIRE(res1 != nullptr);
		REQUIRE(member(member((*res1)[1], "result"), "value") != nullptr);
		CHECK(member(member((*res1)[1], "result"), "value")->children_size() == 0);
		auto res2 = member(r.responses[2], "results");
		REQUIRE(res2 != nullptr);
		REQUIRE(member(member((*res2)[2], "result"), "value") != nullptr);
		CHECK(member(member((*res2)[2], "result"), "value")->children_size() == 0);
	}

	TEST_CASE("an unknown option is an error") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"nope","value":true})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "status")->get_string() == "error");
	}

	TEST_CASE("add and delete change a list") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["num"]})",
			R"({"id":2,"cmd":"add","option":"trim","value":["start"]})",
			R"({"id":3,"cmd":"del","option":"trim","value":["num"]})"});
		REQUIRE(r.responses.size() == 3);
		REQUIRE(result_of(r.responses[1]) != nullptr);
		REQUIRE(member(result_of(r.responses[1]), "value") != nullptr);
		CHECK(member(result_of(r.responses[1]), "value")->children_size() == 2);
		auto v = member(result_of(r.responses[2]), "value");
		REQUIRE(v != nullptr);
		REQUIRE(v->children_size() == 1);
		CHECK((*v)[0].get_string() == "start");
	}

	TEST_CASE("a second evaluator does not see the first one's options") {
		auto a = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["num"]})"});
		REQUIRE(a.responses.size() == 1);
		auto b = run_repl({
			R"({"id":1,"cmd":"get","option":"trim"})"});
		REQUIRE(b.responses.size() == 1);
		REQUIRE(result_of(b.responses[0]) != nullptr);
		REQUIRE(member(result_of(b.responses[0]), "value") != nullptr);
		CHECK(member(result_of(b.responses[0]), "value")->children_size() == 0);
	}

	TEST_CASE("nodisambig-list reaches the grammar") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"nodisambig-list","value":["num"]})",
			R"({"id":2,"cmd":"get","option":"nodisambig-list"})"});
		REQUIRE(r.responses.size() == 2);
		auto v = member(result_of(r.responses[1]), "value");
		REQUIRE(v != nullptr);
		REQUIRE(v->children_size() == 1);
		CHECK((*v)[0].get_string() == "num");
	}
}

TEST_SUITE("tgf json api: parse data") {
	TEST_CASE("parse returns the tree and terminals") {
		auto r = run_repl({
			R"({"id":1,"cmd":"parse","input":"123"})"});
		REQUIRE(r.responses.size() == 1);
		const jt* res = result_of(r.responses[0]);
		REQUIRE(res != nullptr);
		const jt* tree = member(res, "tree");
		REQUIRE(tree != nullptr);
		CHECK(member(res, "ambiguous") != nullptr);
		REQUIRE(member(tree, "symbol") != nullptr);
		CHECK(member(tree, "symbol")->get_string() == "start");
		REQUIRE(member(tree, "id") != nullptr);
		CHECK(member(tree, "id")->is_number());
		auto num_kids = member(tree, "children");
		REQUIRE(num_kids != nullptr);
		REQUIRE(num_kids->children_size() == 1);
		const jt& num = (*num_kids)[0];
		REQUIRE(member(num, "id") != nullptr);
		CHECK(member(num, "id")->get_number()
			!= member(tree, "id")->get_number());
		auto digits = member(num, "children");
		REQUIRE(digits != nullptr);
		REQUIRE(digits->children_size() == 3);
		const jt& digit = (*digits)[0];
		REQUIRE(member(digit, "id") != nullptr);
		CHECK(member(digit, "id")->get_number()
			== member((*digits)[1], "id")->get_number());
		auto dk = member(digit, "children");
		REQUIRE(dk != nullptr);
		REQUIRE(dk->children_size() == 1);
		const jt& leaf = (*dk)[0];
		REQUIRE(member(leaf, "symbol") != nullptr);
		CHECK(member(leaf, "symbol")->get_string() == "");
		CHECK(member(leaf, "id") == nullptr);
		REQUIRE(member(leaf, "text") != nullptr);
		CHECK(member(leaf, "text")->get_string() == "1");
	}

	TEST_CASE("ambiguous is absent when print-ambiguity is off") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"print-ambiguity","value":false})",
			R"({"id":2,"cmd":"parse","input":"123"})"});
		REQUIRE(r.responses.size() == 2);
		CHECK(member(result_of(r.responses[1]), "ambiguous") == nullptr);
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
		REQUIRE(member(result_of(r.responses[0]), "text") != nullptr);
		auto text = member(result_of(r.responses[0]), "text")->get_string();
		CHECK(text.find("load") != std::string::npos);
		CHECK(text.find("short: l") != std::string::npos);
	}

	TEST_CASE("quit stops the loop") {
		auto r = run_repl({
			R"({"id":1,"cmd":"quit"})",
			R"({"id":2,"cmd":"version"})"});
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "status")->get_string() == "quit");
	}
}

TEST_SUITE("tgf json api: parse file") {
	TEST_CASE("eval form parses an input file") {
		scratch_file in("tgf_json_api_input.txt", "123");
		auto r = run_repl({
			eval_request(1, "parse file " + repl_string(in.path)) });
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "status")->get_string() == "ok");
		auto res = member(r.responses[0], "results");
		REQUIRE(res != nullptr);
		REQUIRE(res->children_size() == 1);
		CHECK(member((*res)[0], "cmd")->get_string() == "parse_file");
		CHECK(member((*res)[0], "status")->get_string() == "ok");
		const jt* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		CHECK(member(data, "tree") != nullptr);
		REQUIRE(member(data, "terminals") != nullptr);
		CHECK(member(data, "terminals")->get_string() == "123");
	}

	TEST_CASE("structured form parses an input file") {
		scratch_file in("tgf_json_api_input.txt", "123");
		auto r = run_repl({ file_request(1, "parse_file", in.path) });
		REQUIRE(r.responses.size() == 1);
		CHECK(member(r.responses[0], "status")->get_string() == "ok");
		const jt* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		CHECK(member(data, "tree") != nullptr);
		REQUIRE(member(data, "terminals") != nullptr);
		CHECK(member(data, "terminals")->get_string() == "123");
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
			CHECK(member(resp, "status")->get_string() == "ok");
			const jt* data = response_data(resp);
			REQUIRE(data != nullptr);
			REQUIRE(member(data, "file") != nullptr);
			CHECK(member(data, "file")->get_string() == grammar_path());
			REQUIRE(member(data, "source") != nullptr);
			CHECK(member(data, "source")->get_string() == text);
		}
	}

	TEST_CASE("internal-grammar gives productions with and without a symbol") {
		auto r = run_repl({
			R"({"id":1,"cmd":"igrammar"})",
			eval_request(2, "internal-grammar"),
			R"({"id":3,"cmd":"igrammar","symbol":"num"})",
			eval_request(4, "internal-grammar num") });
		REQUIRE(r.responses.size() == 4);
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(member(r.responses[i], "status")->get_string() == "ok");
			const jt* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			const jt* prods = member(data, "productions");
			REQUIRE(prods != nullptr);
			CHECK(prods->is_array());
			CHECK(prods->children_size() > 0);
			for (const jt& p : prods->children_trees()) CHECK(p.is_string());
			REQUIRE(member(data, "start") != nullptr);
			CHECK(member(data, "start")->get_string()
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
			CHECK(member(r.responses[i], "status")->get_string() == "ok");
			const jt* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			REQUIRE(member(data, "start") != nullptr);
			CHECK(member(data, "start")->get_string()
				== (i >= 2 ? "num" : "start"));
			REQUIRE(member(data, "changed") != nullptr);
			CHECK(member(data, "changed")->get_bool() == (i >= 2));
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
			CHECK(member(r.responses[i], "status")->get_string() == "ok");
			const jt* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			REQUIRE(member(data, "symbol") != nullptr);
			CHECK(member(data, "symbol")->get_string()
				== (i >= 2 ? "num" : "start"));
			REQUIRE(member(data, "productions") != nullptr);
			CHECK(member(data, "productions")->is_array());
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
			CHECK(member(r.responses[i], "status")->get_string() == "ok");
			const jt* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			if (i == 4) {
				REQUIRE(member(data, "file") != nullptr);
				CHECK(member(data, "file")->get_string() == g.path);
				continue;
			}
			REQUIRE(member(data, "loaded") != nullptr);
			CHECK(member(data, "loaded")->get_bool());
			REQUIRE(member(data, "grammar") != nullptr);
			CHECK(member(data, "grammar")->get_string() == g.path);
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
			CHECK(member(resp, "status")->get_string() == "ok");
		const jt* data = response_data(r.responses[2]);
		REQUIRE(data != nullptr);
		REQUIRE(member(data, "source") != nullptr);
		CHECK(member(data, "source")->get_string() == crlf_grammar());
	}

	TEST_CASE("parse file reads a CRLF input file") {
		scratch_file g("tgf_json_api_crlf2.tgf", crlf_grammar());
		scratch_file in("tgf_json_api_crlf_input.txt", "123\r\n");
		auto r = run_repl({
			file_request(1, "load", g.path),
			file_request(2, "parse_file", in.path) });
		REQUIRE(r.responses.size() == 2);
		CHECK(member(r.responses[1], "status")->get_string() == "ok");
		const jt* data = response_data(r.responses[1]);
		REQUIRE(data != nullptr);
		REQUIRE(member(data, "terminals") != nullptr);
		CHECK(member(data, "terminals")->get_string() == "123\r\n");
	}

	TEST_CASE("a request line that ends with CRLF is accepted") {
		auto r = run_repl({
			"{\"id\":1,\"cmd\":\"version\"}\r",
			"{\"id\":2,\"cmd\":\"parse\",\"input\":\"123\"\r}",
			"{\"id\":3,\r\"cmd\":\"version\"}" });
		REQUIRE(r.responses.size() == 3);
		for (const auto& resp : r.responses)
			CHECK(member(resp, "status")->get_string() == "ok");
	}

	TEST_CASE("the text grammar command drops the CR of a CRLF source") {
		scratch_file lf("tgf_json_api_text_lf.tgf", "start => 'a'.\n");
		scratch_file crlf("tgf_json_api_text_crlf.tgf",
			"start => 'a'.\r\n");
		std::ostringstream lfos, cos;
		std::streambuf* old = std::cout.rdbuf();
		// one evaluator at a time: a thread binds one grammar at a time
		{
			tgf_repl_evaluator lfe(lf.path);
			std::cout.rdbuf(lfos.rdbuf());
			lfe.eval("grammar");
			std::cout.rdbuf(old);
		}
		{
			tgf_repl_evaluator ce(crlf.path);
			std::cout.rdbuf(cos.rdbuf());
			ce.eval("grammar");
			std::cout.rdbuf(old);
		}
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
			CHECK(member(r.responses[i], "status")->get_string() == "ok");
			const jt* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			const char* key = i < 2 ? "version" : "license";
			REQUIRE(member(data, key) != nullptr);
			CHECK(member(data, key)->get_string().size() > 0);
		}
	}

	TEST_CASE("clear answers ok with empty data") {
		auto r = run_repl({
			R"({"id":1,"cmd":"clear"})",
			eval_request(2, "clear") });
		REQUIRE(r.responses.size() == 2);
		for (const auto& resp : r.responses) {
			CHECK(member(resp, "status")->get_string() == "ok");
			const jt* data = response_data(resp);
			REQUIRE(data != nullptr);
			CHECK(data->is_object());
			CHECK(data->children_size() == 0);
		}
	}
}

TEST_SUITE("tgf json api: option commands") {
	TEST_CASE("derive-char-classes is a get and set bool option") {
		auto r = run_repl({
			R"({"id":1,"cmd":"get","option":"derive-char-classes"})",
			R"({"id":2,"cmd":"set","option":"derive-char-classes","value":false})",
			R"({"id":3,"cmd":"get","option":"derive-char-classes"})",
			eval_request(4,
				"set derive-char-classes on . get derive-char-classes") });
		REQUIRE(r.responses.size() == 4);
		const jt* v0 = member(result_of(r.responses[0]), "value");
		REQUIRE(v0 != nullptr);
		CHECK(v0->get_bool());
		REQUIRE(result_of(r.responses[1]) != nullptr);
		REQUIRE(member(result_of(r.responses[1]), "option") != nullptr);
		CHECK(member(result_of(r.responses[1]), "option")->get_string()
			== "derive-char-classes");
		REQUIRE(member(result_of(r.responses[1]), "value") != nullptr);
		CHECK(!member(result_of(r.responses[1]), "value")->get_bool());
		REQUIRE(member(result_of(r.responses[2]), "value") != nullptr);
		CHECK(!member(result_of(r.responses[2]), "value")->get_bool());
		auto res = member(r.responses[3], "results");
		REQUIRE(res != nullptr);
		REQUIRE(res->children_size() == 2);
		REQUIRE(member(member((*res)[0], "result"), "value") != nullptr);
		CHECK(member(member((*res)[0], "result"), "value")->get_bool());
		REQUIRE(member(member((*res)[1], "result"), "value") != nullptr);
		CHECK(member(member((*res)[1], "result"), "value")->get_bool());
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
			CHECK(member(r.responses[i], "status")->get_string() == "ok");
			const jt* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			REQUIRE(member(data, "option") != nullptr);
			CHECK(member(data, "option")->get_string()
				== "print-ambiguity");
			REQUIRE(member(data, "value") != nullptr);
			CHECK(member(data, "value")->get_bool() == want[i]);
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
			CHECK(member(r.responses[i], "status")->get_string() == "ok");
			const jt* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			REQUIRE(member(data, "option") != nullptr);
			CHECK(member(data, "option")->get_string()
				== "error-verbosity");
			REQUIRE(member(data, "value") != nullptr);
			CHECK(member(data, "value")->get_string() == want[i]);
		}
		CHECK(member(r.responses[6], "status")->get_string() == "error");
		CHECK(member(r.responses[7], "status")->get_string() == "error");
	}

	TEST_CASE("inline set, add and delete tree paths") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"inline","value":["start > num"]})",
			R"({"id":2,"cmd":"add","option":"inline","value":["num > digit"]})",
			R"({"id":3,"cmd":"del","option":"inline","value":["start > num"]})",
			eval_request(4, "set inline start > num"),
			eval_request(5, "add inline num > digit"),
			eval_request(6, "delete inline start > num") });
		REQUIRE(r.responses.size() == 6);
		const std::vector<std::string> one = { "start > num" };
		const std::vector<std::string> two = {
			"start > num", "num > digit" };
		const std::vector<std::string> second = { "num > digit" };
		const std::vector<std::string> want[6] = {
			one, two, second, one, two, second };
		for (size_t i = 0; i != r.responses.size(); ++i) {
			CHECK(member(r.responses[i], "status")->get_string() == "ok");
			const jt* data = response_data(r.responses[i]);
			REQUIRE(data != nullptr);
			CHECK(strings_are(member(data, "value"), want[i]));
		}
	}

	TEST_CASE("set gc false reaches the parse options") {
		tgf_repl_evaluator re(grammar_path(), repl_options());
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
		std::vector<htref> vs;
		while (std::getline(ls, line)) vs.push_back(parse_line(line));
		REQUIRE(vs.size() == 5);
		const bool want[5] = { false, false, true, true, false };
		for (size_t i = 0; i != vs.size(); ++i) {
			CHECK(member(vs[i], "status")->get_string() == "ok");
			const jt* data = response_data(vs[i]);
			REQUIRE(data != nullptr);
			REQUIRE(member(data, "value") != nullptr);
			CHECK(member(data, "value")->get_bool() == want[i]);
		}
		CHECK(!idni::options().get<bool>("gc"));
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
		CHECK(member(r.responses[0], "status")->get_string() == "error");
		CHECK(member(r.responses[1], "status")->get_string() == "error");
		CHECK(member(r.responses[2], "status")->get_string() == "error");
		CHECK(member(r.responses[3], "status")->get_string() == "ok");
		const jt* data = response_data(r.responses[3]);
		REQUIRE(data != nullptr);
		REQUIRE(member(data, "version") != nullptr);
		CHECK(member(data, "version")->get_string().size() > 0);
	}

	TEST_CASE("a missing or wrongly typed field is an error") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim"})",
			R"({"id":2,"cmd":"set","option":"trim","value":true})",
			R"({"id":3,"cmd":"parse"})",
			R"({"id":4,"cmd":"parse","input":123})" });
		REQUIRE(r.responses.size() == 4);
		for (const auto& resp : r.responses)
			CHECK(member(resp, "status")->get_string() == "error");
	}

	TEST_CASE("a set with both value and text is an error") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["num"]})",
			R"({"id":2,"cmd":"set","option":"trim","value":["start"],"text":"start"})",
			R"({"id":3,"cmd":"get","option":"trim"})" });
		REQUIRE(r.responses.size() == 3);
		CHECK(member(r.responses[0], "status")->get_string() == "ok");
		CHECK(member(r.responses[1], "status")->get_string() == "error");
		const jt* data = response_data(r.responses[2]);
		REQUIRE(data != nullptr);
		CHECK(strings_are(member(data, "value"), { "num" }));
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
			const jt* s = member(resp, "state");
			REQUIRE(s != nullptr);
			REQUIRE(member(s, "grammar") != nullptr);
			CHECK(member(s, "grammar")->get_string() == grammar_path());
			REQUIRE(member(s, "start") != nullptr);
		}
		REQUIRE(member(r.responses[0], "state") != nullptr);
		REQUIRE(member(member(r.responses[0], "state"), "start") != nullptr);
		CHECK(member(member(r.responses[0], "state"), "start")
			->get_string() == "start");
		REQUIRE(member(r.responses[1], "state") != nullptr);
		REQUIRE(member(member(r.responses[1], "state"), "start") != nullptr);
		CHECK(member(member(r.responses[1], "state"), "start")
			->get_string() == "num");
		REQUIRE(member(r.responses[2], "state") != nullptr);
		REQUIRE(member(member(r.responses[2], "state"), "start") != nullptr);
		CHECK(member(member(r.responses[2], "state"), "start")
			->get_string() == "num");
		REQUIRE(member(r.responses[3], "state") != nullptr);
		REQUIRE(member(member(r.responses[3], "state"), "start") != nullptr);
		CHECK(member(member(r.responses[3], "state"), "start")
			->get_string() == "num");
		auto res = member(r.responses[3], "results");
		REQUIRE(res != nullptr);
		REQUIRE(res->children_size() == 1);
		CHECK(member((*res)[0], "state") == nullptr);
	}
}

TEST_SUITE("tgf json api: production ids") {
	TEST_CASE("production_ids matches productions on tiny.tgf") {
		auto r = run_repl({
			R"({"id":1,"cmd":"igrammar"})"});
		REQUIRE(r.responses.size() == 1);
		const jt* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		const jt* prods = member(data, "productions");
		const jt* pids = member(data, "production_ids");
		REQUIRE(prods != nullptr);
		REQUIRE(pids != nullptr);
		REQUIRE(prods->children_size() == 4);
		CHECK(pids->children_size() == prods->children_size());
		for (size_t i = 0; i != pids->children_size(); ++i) {
			const jt& e = (*pids)[i];
			REQUIRE(member(e, "index") != nullptr);
			CHECK(member(e, "index")->get_number()
				== static_cast<double>(i));
			REQUIRE(member(e, "head") != nullptr);
			CHECK(member(e, "head")->is_number());
			REQUIRE(member(e, "body") != nullptr);
			CHECK(member(e, "body")->is_array());
			REQUIRE(member(e, "guard") != nullptr);
			CHECK(member(e, "guard")->is_null());
			REQUIRE(member(e, "conjunctive") != nullptr);
			CHECK(!member(e, "conjunctive")->get_bool());
		}
		CHECK((*prods)[0].get_string() == "start => num.");
		const jt* body = member((*pids)[0], "body");
		REQUIRE(body != nullptr);
		REQUIRE(body->children_size() == 1);
		REQUIRE((*body)[0].children_size() == 1);
		CHECK((*body)[0][0].is_number());
	}

	TEST_CASE("a guarded production carries its guard") {
		auto r = run_repl_on(csv_grammar_path(), {
			R"({"id":1,"cmd":"igrammar"})"});
		REQUIRE(r.responses.size() == 1);
		const jt* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		const jt* pids = member(data, "production_ids");
		REQUIRE(pids != nullptr);
		bool found = false;
		for (const jt& e : pids->children_trees()) {
			const jt* g = member(e, "guard");
			if (g && !g->is_null()) {
				CHECK(g->is_string());
				CHECK(g->get_string().size() > 0);
				found = true;
			}
		}
		CHECK(found);
	}

	TEST_CASE("a conjunctive production has two conjuncts") {
		auto r = run_repl_on(json_grammar_path(), {
			R"({"id":1,"cmd":"igrammar"})"});
		REQUIRE(r.responses.size() == 1);
		const jt* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		const jt* pids = member(data, "production_ids");
		REQUIRE(pids != nullptr);
		bool found = false;
		for (const jt& e : pids->children_trees()) {
			REQUIRE(member(e, "conjunctive") != nullptr);
			if (member(e, "conjunctive")->get_bool()) {
				REQUIRE(member(e, "body") != nullptr);
				CHECK(member(e, "body")->children_size() >= 2);
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
		const jt* data = response_data(r.responses[0]);
		REQUIRE(data != nullptr);
		const jt* amb = member(data, "ambiguous");
		REQUIRE(amb != nullptr);
		REQUIRE(member(amb, "trees") != nullptr);
		CHECK(member(amb, "trees")->get_number() == 2);
		const jt* nodes = member(amb, "nodes");
		REQUIRE(nodes != nullptr);
		REQUIRE(nodes->children_size() == 1);
		const jt& n = (*nodes)[0];
		REQUIRE(member(n, "symbol") != nullptr);
		CHECK(member(n, "symbol")->get_string() == "A");
		CHECK(member(n, "id")->is_number());
		REQUIRE(member(n, "range") != nullptr);
		CHECK(member(n, "range")->children_size() == 2);
		const jt* alts = member(n, "alternatives");
		REQUIRE(alts != nullptr);
		REQUIRE(alts->children_size() == 2);
		std::set<std::string> got;
		for (size_t i = 0; i != 2; ++i) {
			const jt* ch = member((*alts)[i], "children");
			REQUIRE(ch != nullptr);
			REQUIRE(ch->children_size() == 1);
			REQUIRE(member((*ch)[0], "symbol") != nullptr);
			got.insert(member((*ch)[0], "symbol")->get_string());
			CHECK(member((*ch)[0], "id")->is_number());
			CHECK(member((*ch)[0], "children") != nullptr);
		}
		CHECK(got == std::set<std::string>({ "B", "C" }));
	}
}

TEST_SUITE("tgf json api: cmd echo") {
	TEST_CASE("a structured response carries the canonical cmd") {
		auto r = run_repl({
			R"({"id":1,"cmd":"set","option":"trim","value":["a"]})",
			R"({"id":2,"cmd":"igrammar"})",
			eval_request(3, "p 12") });
		REQUIRE(r.responses.size() == 3);
		CHECK(member(r.responses[0], "cmd")->get_string() == "set");
		CHECK(member(r.responses[1], "cmd")->get_string()
			== "igrammar");
		std::vector<std::string> keys;
		for (const jt& kv : jt::get(r.responses[0]).children_trees())
			keys.push_back(kv.get_string());
		REQUIRE(keys.size() >= 2);
		CHECK(keys[0] == "id");
		CHECK(keys[1] == "cmd");
		auto res = member(r.responses[2], "results");
		REQUIRE(res != nullptr);
		REQUIRE(res->children_size() == 1);
		CHECK(member((*res)[0], "cmd")->get_string() == "parse");
	}
}

TEST_SUITE("tgf json api: no grammar") {
	static htref hello_value(const repl_run& r) {
		std::istringstream ls(r.raw);
		std::string line;
		REQUIRE(static_cast<bool>(std::getline(ls, line)));
		return parse_line(line);
	}

	static bool node_has_key(const jt& node, const std::string& key) {
		const jt* k = member(node, "key");
		if (k && k->get_string() == key) return true;
		const jt* ch = member(node, "children");
		if (!ch) return false;
		for (const jt& c : ch->children_trees())
			if (node_has_key(c, key)) return true;
		return false;
	}

	static bool report_has_key(const htref& resp, const std::string& key) {
		const jt* rep = member(resp, "report");
		if (!rep) return false;
		const jt* nodes = member(rep, "nodes");
		if (!nodes) return false;
		for (const jt& n : nodes->children_trees())
			if (node_has_key(n, key)) return true;
		return false;
	}

	TEST_CASE("hello and state carry a null grammar") {
		auto r = run_repl_no_grammar({});
		htref p = hello_value(r);
		auto h = member(p, "hello");
		REQUIRE(h != nullptr);
		REQUIRE(member(h, "grammar") != nullptr);
		CHECK(member(h, "grammar")->is_null());
		REQUIRE(member(h, "fixed_grammar") != nullptr);
		CHECK(!member(h, "fixed_grammar")->get_bool());
		REQUIRE(member(h, "start") != nullptr);
		CHECK(member(h, "start")->is_null());
		CHECK(member(h, "directives") == nullptr);
		auto s = member(p, "state");
		REQUIRE(s != nullptr);
		REQUIRE(member(s, "grammar") != nullptr);
		CHECK(member(s, "grammar")->is_null());
		REQUIRE(member(s, "start") != nullptr);
		CHECK(member(s, "start")->is_null());
		REQUIRE(member(s, "char_classes") != nullptr);
		CHECK(member(s, "char_classes")->is_array());
		CHECK(member(s, "char_classes")->children_size() == 0);
	}

	TEST_CASE("set start with no grammar is an error") {
		auto r = run_repl_no_grammar({
			eval_request(1, "set start foo"),
			R"({"id":2,"cmd":"get","option":"start"})",
			R"({"id":3,"cmd":"set","option":"start","value":"bar"})",
			eval_request(4, "get start") });
		REQUIRE(r.responses.size() == 4);
		auto res1 = member(r.responses[0], "results");
		REQUIRE(res1 != nullptr);
		REQUIRE(res1->children_size() == 1);
		CHECK(member((*res1)[0], "status")->get_string() == "error");
		CHECK(member(member(r.responses[0], "state"), "start")->is_null());
		REQUIRE(result_of(r.responses[1]) != nullptr);
		REQUIRE(member(result_of(r.responses[1]), "value") != nullptr);
		CHECK(member(result_of(r.responses[1]), "value")->get_string()
			== "start");
		CHECK(member(r.responses[2], "status")->get_string() == "error");
		const jt* data = response_data(r.responses[3]);
		REQUIRE(data != nullptr);
		REQUIRE(member(data, "value") != nullptr);
		CHECK(member(data, "value")->get_string() == "start");
	}

	TEST_CASE("the start command gives null with no start symbol") {
		auto r = run_repl_no_grammar({
			R"({"id":1,"cmd":"start"})",
			eval_request(2, "start") });
		REQUIRE(r.responses.size() == 2);
		CHECK(member(result_of(r.responses[0]), "start")->is_null());
		CHECK(!member(result_of(r.responses[0]), "changed")->get_bool());
		auto res2 = member(r.responses[1], "results");
		REQUIRE(res2 != nullptr);
		REQUIRE(res2->children_size() == 1);
		CHECK(member(member((*res2)[0], "result"), "start")->is_null());
	}

	TEST_CASE("commands that need a grammar report the error") {
		auto r = run_repl_no_grammar({
			R"({"id":1,"cmd":"parse","input":"1"})",
			R"({"id":2,"cmd":"igrammar"})",
			R"({"id":3,"cmd":"unreachable"})",
			R"({"id":4,"cmd":"grammar"})",
			R"({"id":5,"cmd":"reload"})" });
		REQUIRE(r.responses.size() == 5);
		for (const auto& resp : r.responses) {
			CHECK(member(resp, "status")->get_string() == "error");
			CHECK(report_has_key(resp,
				"no grammar loaded, use load"));
		}
	}

	TEST_CASE("options and simple commands work with no grammar") {
		auto r = run_repl_no_grammar({
			R"({"id":1,"cmd":"get","option":"status"})",
			R"({"id":2,"cmd":"set","option":"trim","value":[]})",
			R"({"id":3,"cmd":"toggle","option":"print-ambiguity"})",
			R"({"id":4,"cmd":"help","command":"load"})",
			R"({"id":5,"cmd":"version"})",
			R"({"id":6,"cmd":"license"})",
			R"({"id":7,"cmd":"clear"})" });
		REQUIRE(r.responses.size() == 7);
		for (const auto& resp : r.responses)
			CHECK(member(resp, "status")->get_string() == "ok");
	}

	TEST_CASE("start before load is an error and load then parses") {
		auto r = run_repl_no_grammar({
			R"({"id":1,"cmd":"start","symbol":"num"})",
			file_request(2, "load", grammar_path()),
			R"({"id":3,"cmd":"start"})",
			R"({"id":4,"cmd":"parse","input":"123"})" });
		REQUIRE(r.responses.size() == 4);
		CHECK(member(r.responses[0], "status")->get_string() == "error");
		REQUIRE(member(member(r.responses[0], "result"), "changed")
			!= nullptr);
		CHECK(!member(member(r.responses[0], "result"), "changed")
			->get_bool());
		CHECK(member(member(r.responses[1], "result"), "loaded")
			->get_bool());
		CHECK(member(member(r.responses[2], "result"), "start")
			->get_string() == "start");
		CHECK(member(r.responses[3], "status")->get_string() == "ok");
		CHECK(member(member(r.responses[3], "result"), "tree")
			!= nullptr);
	}
}

TEST_SUITE("tgf json api: one-shot CLI") {
	static std::vector<htref> lines_of(const std::string& s) {
		std::vector<htref> out;
		std::istringstream ls(s);
		std::string l;
		while (std::getline(ls, l)) {
			if (l.empty()) continue;
			out.push_back(parse_line(l));
		}
		return out;
	}

	TEST_CASE("parse --json writes one response") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(), "parse",
				"--json", "-e", "123" }, "", false);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(member(vs[0], "status")->get_string() == "ok");
		CHECK(member(vs[0], "cmd")->get_string() == "parse");
		REQUIRE(member(vs[0], "result") != nullptr);
		CHECK(member(member(vs[0], "result"), "tree") != nullptr);
		const jt* s = member(vs[0], "state");
		REQUIRE(s != nullptr);
		REQUIRE(member(s, "grammar") != nullptr);
		CHECK(member(s, "grammar")->get_string() == grammar_path());
		REQUIRE(member(s, "start") != nullptr);
		CHECK(member(s, "start")->get_string() == "start");
		CHECK(code == 0);
	}

	TEST_CASE("grammar --json writes one response") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(), "grammar",
				"--json" }, "", false);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(member(vs[0], "cmd")->get_string() == "grammar");
		auto res = member(vs[0], "result");
		REQUIRE(res != nullptr);
		REQUIRE(member(res, "start") != nullptr);
		CHECK(member(res, "start")->get_string() == "start");
		CHECK(member(res, "productions") != nullptr);
		REQUIRE(member(res, "production_ids") != nullptr);
		REQUIRE(member(res, "productions") != nullptr);
		CHECK(member(res, "production_ids")->children_size()
			== member(res, "productions")->children_size());
		CHECK(member(vs[0], "state") != nullptr);
		CHECK(code == 0);
	}

	TEST_CASE("gen --json writes one response with the file list") {
		auto dir = std::filesystem::temp_directory_path()
			/ "tgf_json_api_gen";
		std::filesystem::create_directories(dir);
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(), "gen",
				"--json", "--name", "tiny", "--header-only", "false",
				"--output-dir", dir.string() }, "", false);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(member(vs[0], "status")->get_string() == "ok");
		CHECK(member(vs[0], "cmd")->get_string() == "gen");
		REQUIRE(member(vs[0], "result") != nullptr);
		auto files = member(member(vs[0], "result"), "files");
		REQUIRE(files != nullptr);
		CHECK(files->children_size() >= 2);
		CHECK(member(vs[0], "state") != nullptr);
		CHECK(code == 0);
		std::filesystem::remove_all(dir);
	}

	TEST_CASE("repl --json --evaluate writes one response and no hello") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(), "repl",
				"--json", "--evaluate", "version" }, "", false);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(member(vs[0], "hello") == nullptr);
		CHECK(member(vs[0], "status")->get_string() == "ok");
		auto res = member(vs[0], "results");
		REQUIRE(res != nullptr);
		REQUIRE(res->children_size() == 1);
		CHECK(member((*res)[0], "cmd")->get_string() == "version");
		REQUIRE(member(vs[0], "state") != nullptr);
		REQUIRE(member(member(vs[0], "state"), "start") != nullptr);
		CHECK(member(member(vs[0], "state"), "start")->get_string()
			== "start");
		CHECK(code == 0);
	}

	TEST_CASE("parse --json --grammar adds internal_grammar") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(), "parse",
				"--json", "--grammar", "-e", "123" }, "", false);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(member(vs[0], "status")->get_string() == "ok");
		CHECK(member(vs[0], "cmd")->get_string() == "parse");
		REQUIRE(member(vs[0], "result") != nullptr);
		auto ig = member(member(vs[0], "result"), "internal_grammar");
		REQUIRE(ig != nullptr);
		REQUIRE(member(ig, "start") != nullptr);
		CHECK(member(ig, "start")->get_string() == "start");
		auto prods = member(ig, "productions");
		REQUIRE(prods != nullptr);
		CHECK(prods->children_size() > 0);
		REQUIRE(member(ig, "production_ids") != nullptr);
		CHECK(member(ig, "production_ids")->children_size() == prods->children_size());
		CHECK(code == 0);
	}

	TEST_CASE("parse --json --measure adds bintree_totals and no text") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(), "parse",
				"--json", "--measure", "-e", "123" }, "", false);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(member(vs[0], "status")->get_string() == "ok");
		CHECK(member(vs[0], "cmd")->get_string() == "parse");
		REQUIRE(member(vs[0], "result") != nullptr);
		auto bt = member(member(vs[0], "result"), "bintree_totals");
		REQUIRE(bt != nullptr);
		CHECK(member(bt, "get_hits") != nullptr);
		const jt* hist = member(bt, "chain_length_histogram");
		REQUIRE(hist != nullptr);
		CHECK(member(hist, "0") != nullptr);
		CHECK(member(hist, "64+") != nullptr);
		const jt* groups = member(bt, "top_hash_groups");
		REQUIRE(groups != nullptr);
		CHECK(groups->children_size() == 5);
		for (const jt& g : groups->children_trees()) {
			REQUIRE(member(g, "size") != nullptr);
			CHECK(member(g, "size")->is_number());
			REQUIRE(member(g, "triples") != nullptr);
			CHECK(member(g, "triples")->is_number());
		}
		REQUIRE(member(bt, "largest_group_samples") != nullptr);
		CHECK(text.find("bintree lookups") == std::string::npos);
		CHECK(code == 0);
	}

	TEST_CASE("grammar --json --nullable adds symbol id and index") {
		scratch_file g("tgf_json_api_nullable.tgf",
			"start => a.\na => a b | 'x'.\nb => [ 'y' ].\n");
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", g.path, "grammar",
				"--json", "--nullable" }, "", false);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(member(vs[0], "cmd")->get_string() == "grammar");
		REQUIRE(member(vs[0], "result") != nullptr);
		auto nl = member(member(vs[0], "result"), "nullable");
		REQUIRE(nl != nullptr);
		REQUIRE(nl->children_size() == 1);
		const jt& e = (*nl)[0];
		REQUIRE(member(e, "symbol") != nullptr);
		CHECK(member(e, "symbol")->get_string() == "a");
		REQUIRE(member(e, "id") != nullptr);
		CHECK(member(e, "id")->is_number());
		REQUIRE(member(e, "index") != nullptr);
		CHECK(member(e, "index")->is_number());
		REQUIRE(member(e, "production") != nullptr);
		CHECK(member(e, "production")->is_string());
		CHECK(code == 0);
	}

	TEST_CASE("JSON mode writes only JSON objects, one per line") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(), "parse",
				"--json", "--print-input", "--print-tml-rules",
				"--print-tml-facts", "--measure", "-e", "123" },
				"", false);
		std::istringstream ls(text);
		std::string l;
		size_t lines = 0;
		while (std::getline(ls, l)) {
			if (l.empty()) continue;
			++lines;
			CHECK(l.front() == '{');
			auto p = jt::get(std::string_view(l));
			CHECK(p.has_value());
		}
		CHECK(lines == 1);
		CHECK(code == 0);
	}

	TEST_CASE("one-shot JSON commands with no grammar report the error") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", "parse", "--json", "-e",
				"1" }, "", false);
		auto vs = lines_of(text);
		REQUIRE(vs.size() == 1);
		CHECK(member(vs[0], "status")->get_string() == "error");
		CHECK(member(vs[0], "cmd")->get_string() == "parse");
		REQUIRE(member(vs[0], "report") != nullptr);
		const jt* nodes = member(member(vs[0], "report"), "nodes");
		REQUIRE(nodes != nullptr);
		REQUIRE(nodes->children_size() >= 1);
		REQUIRE(member((*nodes)[0], "key") != nullptr);
		CHECK(member((*nodes)[0], "key")->get_string()
			== "no grammar loaded, use load");
		CHECK(code == 1);

		auto [gcode, gtext, gtext_err] =
			capture_tgf_run({ "tgf", "grammar", "--json" }, "", false);
		auto gvs = lines_of(gtext);
		REQUIRE(gvs.size() == 1);
		CHECK(member(gvs[0], "status")->get_string() == "error");
		CHECK(member(gvs[0], "cmd")->get_string() == "grammar");
		CHECK(gcode == 1);

		auto dir = std::filesystem::temp_directory_path()
			/ "tgf_json_api_nogen";
		std::filesystem::create_directories(dir);
		auto [ncode, ntext, ntext_err] =
			capture_tgf_run({ "tgf", "gen", "--json", "--name",
				"nogen", "--header-only", "false", "--output-dir",
				dir.string() }, "", false);
		auto nvs = lines_of(ntext);
		REQUIRE(nvs.size() == 1);
		CHECK(member(nvs[0], "status")->get_string() == "error");
		CHECK(member(nvs[0], "cmd")->get_string() == "gen");
		CHECK(ncode == 1);
		std::filesystem::remove_all(dir);
	}

	TEST_CASE("one-shot text commands with no grammar exit 1") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", "parse", "-e", "1" }, "", false);
		CHECK(code == 1);
		CHECK(text.empty());   // the error goes to stderr, no help text
		auto [gcode, gtext, gtext_err] =
			capture_tgf_run({ "tgf", "grammar" }, "", false);
		CHECK(gcode == 1);
		CHECK(gtext.empty());
	}

	TEST_CASE("text repl set start and get start") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(), "repl",
				"--evaluate", "set start num . get start" }, "", false);
		CHECK(code == 0);
		CHECK(text.find("start:") != std::string::npos);
		CHECK(text.find("num") != std::string::npos);
	}
}

TEST_SUITE("tgf json api: hello request") {
	TEST_CASE("a hello request answers a fresh hello object") {
		auto r = run_repl({ R"({"id":1,"cmd":"hello"})" });
		REQUIRE(r.responses.size() == 1);
		const auto& v = r.responses[0];
		REQUIRE(member(v, "id") != nullptr);
		CHECK(member(v, "id")->get_number() == 1);
		REQUIRE(member(v, "cmd") != nullptr);
		CHECK(member(v, "cmd")->get_string() == "hello");
		REQUIRE(member(v, "status") != nullptr);
		CHECK(member(v, "status")->get_string() == "ok");
		auto h = member(v, "hello");
		REQUIRE(h != nullptr);
		REQUIRE(member(h, "protocol") != nullptr);
		CHECK(member(h, "protocol")->get_number() == 1);
		REQUIRE(member(h, "grammar") != nullptr);
		CHECK(member(h, "grammar")->get_string() == grammar_path());
		CHECK(member(h, "session") == nullptr);
		CHECK(member(v, "state") != nullptr);
	}

	TEST_CASE("the session id appears in the hello line and the response") {
		tgf_repl_evaluator re(grammar_path(), repl_options());
		re.session_id = "0123456789abcdef";
		auto r = run_repl_re(re, { R"({"id":1,"cmd":"hello"})" });
		std::istringstream ls(r.raw);
		std::string line;
		REQUIRE(static_cast<bool>(std::getline(ls, line)));
		auto first = parse_line(line);
		REQUIRE(member(first, "hello") != nullptr);
		REQUIRE(member(member(first, "hello"), "session") != nullptr);
		CHECK(member(member(first, "hello"), "session")->get_string()
			== "0123456789abcdef");
		REQUIRE(r.responses.size() == 1);
		auto h = member(r.responses[0], "hello");
		REQUIRE(h != nullptr);
		REQUIRE(member(h, "session") != nullptr);
		CHECK(member(h, "session")->get_string()
			== "0123456789abcdef");
	}

	TEST_CASE("--session sets the hello session id") {
		auto [code, text, text_err] =
			capture_tgf_run({ "tgf", grammar_path(),
				"repl", "--json", "--session", "deadbeef" },
				"{\"id\":1,\"cmd\":\"hello\"}\n", false);
		REQUIRE(code == 0);
		std::istringstream ls(text);
		std::string line;
		REQUIRE(static_cast<bool>(std::getline(ls, line)));
		auto first = parse_line(line);
		REQUIRE(member(first, "hello") != nullptr);
		REQUIRE(member(member(first, "hello"), "session") != nullptr);
		CHECK(member(member(first, "hello"), "session")->get_string()
			== "deadbeef");
	}
}

TEST_SUITE("tgf json api: load from source") {
	TEST_CASE("structured load source adopts and reload reloads the text") {
		const std::string text = read_file(grammar_path());
		auto r = run_repl({
			load_source_request(1, "label.tgf", text),
			R"({"id":2,"cmd":"reload"})",
			R"({"id":3,"cmd":"grammar"})",
			R"({"id":4,"cmd":"parse","input":"123"})" });
		REQUIRE(r.responses.size() == 4);
		for (size_t i = 0; i != 3; ++i)
			CHECK(member(r.responses[i], "status")->get_string()
				== "ok");
		REQUIRE(result_of(r.responses[0]) != nullptr);
		CHECK(member(result_of(r.responses[0]), "loaded")->get_bool());
		CHECK(member(result_of(r.responses[0]), "grammar")
			->get_string() == "label.tgf");
		REQUIRE(result_of(r.responses[1]) != nullptr);
		CHECK(member(result_of(r.responses[1]), "loaded")->get_bool());
		CHECK(member(result_of(r.responses[1]), "grammar")
			->get_string() == "label.tgf");
		REQUIRE(result_of(r.responses[2]) != nullptr);
		CHECK(member(result_of(r.responses[2]), "file")->get_string()
			== "label.tgf");
		CHECK(member(result_of(r.responses[2]), "source")->get_string()
			== text);
		CHECK(member(r.responses[3], "status")->get_string() == "ok");
		CHECK(member(result_of(r.responses[3]), "tree") != nullptr);
		CHECK(member(member(r.responses[3], "state"), "grammar")
			->get_string() == "label.tgf");
	}

	TEST_CASE("load source with no grammar installs one") {
		auto r = run_repl_no_grammar({
			load_source_request(1, "g.tgf", "start => 'a'.\n"),
			R"({"id":2,"cmd":"parse","input":"a"})" });
		REQUIRE(r.responses.size() == 2);
		CHECK(member(r.responses[0], "status")->get_string() == "ok");
		CHECK(member(result_of(r.responses[0]), "loaded")->get_bool());
		CHECK(member(r.responses[1], "status")->get_string() == "ok");
	}

	TEST_CASE("a bad load source keeps the previous grammar") {
		const std::string text = read_file(grammar_path());
		auto r = run_repl({
			load_source_request(1, "g.tgf", text),
			load_source_request(2, "bad.tgf", ".\n"),
			R"({"id":3,"cmd":"grammar"})" });
		REQUIRE(r.responses.size() == 3);
		CHECK(member(r.responses[0], "status")->get_string() == "ok");
		CHECK(member(r.responses[1], "status")->get_string() == "error");
		CHECK(!member(result_of(r.responses[1]), "loaded")->get_bool());
		CHECK(member(result_of(r.responses[2]), "file")->get_string()
			== "g.tgf");
	}
}

TEST_SUITE("tgf json api: server paths") {
	TEST_CASE("file paths are refused when no_server_paths is on") {
		scratch_file in("tgf_json_api_server_input.txt", "123");
		tgf_repl_evaluator re(grammar_path(), repl_options(true, true, true));
		auto r = run_repl_re(re, {
			file_request(1, "parse_file", in.path),
			file_request(2, "load", grammar_path()),
			R"({"id":3,"cmd":"reload"})",
			eval_request(4, "parse file " + repl_string(in.path)),
			eval_request(5, "load " + repl_string(grammar_path())),
			eval_request(6, "reload") });
		REQUIRE(r.responses.size() == 6);
		for (const auto& resp : r.responses) {
			CHECK(member(resp, "status")->get_string() == "error");
			CHECK(report_contains_key(resp,
				"File paths are not allowed in a server session"));
		}
	}

	TEST_CASE("load source and its reload are allowed") {
		const std::string text = read_file(grammar_path());
		tgf_repl_evaluator re(grammar_path(), repl_options(true, true, true));
		auto r = run_repl_re(re, {
			load_source_request(1, "g.tgf", text),
			R"({"id":2,"cmd":"reload"})",
			R"({"id":3,"cmd":"parse","input":"123"})" });
		REQUIRE(r.responses.size() == 3);
		for (const auto& resp : r.responses)
			CHECK(member(resp, "status")->get_string() == "ok");
		CHECK(member(result_of(r.responses[2]), "tree") != nullptr);
	}
}

TEST_SUITE("tgf json api: line limit") {
	TEST_CASE("a line over the limit is an error and the loop goes on") {
		tgf_repl_evaluator re(grammar_path(), repl_options());
		std::istringstream in(
			"{\"id\":1,\"cmd\":\"version\"}\n"
			"{\"cmd\":\"version\"}\n"
			"{\"id\":3,\"cmd\":\"version\"}\n"
			"{\"cmd\":\"version\"}\n");
		std::ostringstream out;
		tgf_json_loop(re, in, out, 20);
		std::istringstream ls(out.str());
		std::string line;
		REQUIRE(static_cast<bool>(std::getline(ls, line)));   // hello
		// The first line is over the limit.
		REQUIRE(static_cast<bool>(std::getline(ls, line)));
		auto v1 = parse_line(line);
		CHECK(member(v1, "status")->get_string() == "error");
		CHECK(member(v1, "id")->is_null());
		CHECK(report_contains_key(v1,
			"Line is longer than the limit"));
		// The next line works, so the loop went on.
		REQUIRE(static_cast<bool>(std::getline(ls, line)));
		auto v2 = parse_line(line);
		CHECK(member(v2, "status")->get_string() == "ok");
		CHECK(member(v2, "cmd")->get_string() == "version");
		// A second over-limit line and a second working line.
		REQUIRE(static_cast<bool>(std::getline(ls, line)));
		CHECK(member(parse_line(line), "status")->get_string()
			== "error");
		REQUIRE(static_cast<bool>(std::getline(ls, line)));
		CHECK(member(parse_line(line), "cmd")->get_string()
			== "version");
		CHECK(!std::getline(ls, line));   // no more output
	}
}

TEST_SUITE("tgf json api: a line of only whitespace") {
	TEST_CASE("a request line of only whitespace gets no answer") {
		tgf_repl_evaluator re(grammar_path(), repl_options());
		std::istringstream in(
			"{\"id\":1,\"cmd\":\"version\"}\n"
			"  \t\n"
			" \r\n"
			"{\"id\":2,\"cmd\":\"version\"}\n");
		std::ostringstream out;
		tgf_json_loop(re, in, out);
		std::istringstream ls(out.str());
		std::string line;
		REQUIRE(static_cast<bool>(std::getline(ls, line)));   // hello
		for (double id : { 1.0, 2.0 }) {
			REQUIRE(static_cast<bool>(std::getline(ls, line)));
			auto v = parse_line(line);
			CHECK(member(v, "status")->get_string() == "ok");
			const jt* got = member(v, "id");
			REQUIRE(got != nullptr);
			REQUIRE(got->is_number());
			CHECK(got->get_number() == id);
		}
		CHECK(!std::getline(ls, line));   // no more output
	}

	TEST_CASE("an init line of only whitespace is a missing field") {
		tgf_repl_evaluator re(grammar_path(), repl_options());
		std::istringstream in(" \t\n{\"id\":1,\"cmd\":\"version\"}\n");
		std::ostringstream out;
		tgf_json_loop(re, in, out, tgf_json_max_line, true);
		std::istringstream ls(out.str());
		std::string line;
		REQUIRE(static_cast<bool>(std::getline(ls, line)));   // hello
		auto h = parse_line(line);
		const jt* rep = member(member(h, "hello"), "report");
		REQUIRE(rep != nullptr);
		const jt* nodes = member(rep, "nodes");
		REQUIRE(nodes != nullptr);
		bool missing = false;
		for (const jt& n : nodes->children_trees())
			if (json_node_has_key(n, "Missing field")) missing = true;
		CHECK(missing);
	}
}
