// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "format/json/json.h"
#include "format/ast.json/ast_json.h"
#include "parser.h"

#include <sstream>
#include <string>

using namespace idni;
using namespace idni::format::json;

using jt = idni::format::json::tree;

// The tree of a JSON text that must parse.
static tref parsed(std::string_view text) {
	auto r = jt::get(text);
	REQUIRE(r.has_value());
	return r.value();
}

TEST_SUITE("json: value kinds") {
	TEST_CASE("null") {
		CHECK(jt::get(parsed("null")).is_null());
	}
	TEST_CASE("true") {
		const jt& t = jt::get(parsed("true"));
		CHECK(t.is_bool());
		CHECK(t.get_bool());
	}
	TEST_CASE("false") {
		const jt& t = jt::get(parsed("false"));
		CHECK(t.is_bool());
		CHECK_FALSE(t.get_bool());
	}
	TEST_CASE("integer number") {
		const jt& t = jt::get(parsed("42"));
		CHECK(t.is_number());
		CHECK(t.get_number() == 42);
	}
	TEST_CASE("negative fractional exponent number") {
		CHECK(jt::get(parsed("-1.5e2")).get_number() == -150);
	}
	TEST_CASE("string") {
		const jt& t = jt::get(parsed("\"hello\""));
		CHECK(t.is_string());
		CHECK(t.get_string() == "hello");
	}
	TEST_CASE("array") {
		const jt& t = jt::get(parsed("[1,2,3]"));
		CHECK(t.is_array());
		CHECK(t.children_size() == 3);
	}
	TEST_CASE("object") {
		const jt& t = jt::get(parsed("{\"a\":1}"));
		CHECK(t.is_object());
		REQUIRE(t.find("a") != nullptr);
		CHECK(jt::get(t.find("a")).get_number() == 1);
	}
}

TEST_SUITE("json: array indexing and iteration") {
	TEST_CASE("operator[] and children see the same elements") {
		const jt& t = jt::get(parsed("[10,20,30]"));
		CHECK(t[0].get_number() == 10);
		CHECK(t[1].get_number() == 20);
		CHECK(t[2].get_number() == 30);
		double sum = 0;
		for (const jt& e : t.children_trees()) sum += e.get_number();
		CHECK(sum == 60);
	}
}

TEST_SUITE("json: nesting") {
	TEST_CASE("object containing an array containing an object") {
		tref t = parsed(R"({"a":[1,{"b":true},null],"c":{"d":[]}})");
		auto a = tt(t) | tt::member("a");
		REQUIRE(a.has_value());
		CHECK(a.value_tree().children_size() == 3);
		CHECK((tt(a.value_tree()[1]) | tt::member("b") | tt::boolean));
		auto d = tt(t) | tt::member("c") | tt::member("d");
		REQUIRE(d.has_value());
		CHECK(d.value_tree().is_array());
		CHECK(d.value_tree().children_size() == 0);
	}
}

TEST_SUITE("json: escaped string round trip") {
	TEST_CASE("newline escape reads back as a real newline") {
		std::ostringstream os;
		escape(os, "a\nb");
		CHECK(jt::get(parsed(os.str())).get_string() == "a\nb");
	}
	TEST_CASE("quote and backslash round trip") {
		std::ostringstream os;
		escape(os, "a\"b\\c");
		CHECK(jt::get(parsed(os.str())).get_string() == "a\"b\\c");
	}
}

TEST_SUITE("json: CR whitespace") {
	TEST_CASE("CR is whitespace between every token") {
		tref t = parsed("{\r\"a\"\r:\r[\r1\r,\r2\r]\r,\r\"b\"\r:\rtrue\r}\r");
		CHECK(jt::get(t).is_object());
		auto a = tt(t) | tt::member("a");
		REQUIRE(a.has_value());
		CHECK(a.value_tree().is_array());
		CHECK(a.value_tree().children_size() == 2);
		CHECK((tt(t) | tt::member("b") | tt::boolean));
	}
	TEST_CASE("CR before the first token") {
		CHECK(jt::get(parsed("\r\r1")).get_number() == 1);
	}
}

TEST_SUITE("json: empty containers") {
	TEST_CASE("empty array") {
		const jt& t = jt::get(parsed("[]"));
		CHECK(t.is_array());
		CHECK(t.children_size() == 0);
	}
	TEST_CASE("empty object") {
		const jt& t = jt::get(parsed("{}"));
		CHECK(t.is_object());
		CHECK(t.children_size() == 0);
	}
}

TEST_SUITE("json: missing object key") {
	TEST_CASE("find returns nullptr, not a crash") {
		CHECK(jt::get(parsed("{\"a\":1}")).find("missing") == nullptr);
	}
}

TEST_SUITE("json: malformed input reports an error") {
	TEST_CASE("empty input") {
		auto r = jt::get("");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("trailing comma in array") {
		auto r = jt::get("[1,2,]");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("trailing comma in object") {
		auto r = jt::get("{\"a\":1,}");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("unterminated string") {
		auto r = jt::get("\"abc");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("lone high surrogate is a semantic error the grammar can't catch") {
		auto r = jt::get("\"\\uD800\"");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("lone high surrogate as an object key") {
		auto r = jt::get("{\"\\uD800\":1}");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
}

TEST_SUITE("json: writer builds arbitrary objects and arrays") {
	TEST_CASE("build, print, and re-parse an object with a nested array") {
		tref v = jt::build_object({
			jt::build_pair("id", jt::build_number(1)),
			jt::build_pair("result", jt::build_string("ok")),
			jt::build_pair("nums", jt::build_array({
				jt::build_number(1), jt::build_number(2) })) });
		const std::string text = jt::get(v).to_str();
		CHECK(text == R"({"id":1,"result":"ok","nums":[1,2]})");
		tref r = parsed(text);
		CHECK(r == v);
		CHECK((tt(r) | tt::member("id") | tt::number) == 1);
		CHECK((tt(r) | tt::member("result") | tt::string) == "ok");
		CHECK((tt(r) | tt::member("nums")).value_tree().children_size()
			== 2);
	}
}

TEST_SUITE("json: report to nested JSON") {
	using idni::diagnostics::code;
	using idni::diagnostics::report;
	using idni::parser_strings::label;

	TEST_CASE("nested tree carries message names on request") {
		report r;
		{
			auto s = r.open("parse", code::info_count, size_t(120));
			r.info("child");
		}
		tref v = jt::build_report(r, true);
		REQUIRE(jt::get(v).is_object());
		auto nodes = tt(v) | tt::member("nodes");
		REQUIRE(nodes.is(json_parser::arr));
		REQUIRE(nodes.value_tree().children_size() == 1);
		tt n(nodes.value_tree()[0]);
		CHECK((n | tt::member("tag") | tt::number)
			== static_cast<double>(code::info_count));
		CHECK((n | tt::member("message") | tt::string)
			== std::string(idni::diagnostics::code_name(
				code::info_count)));
		CHECK((n | tt::member("key") | tt::string) == "parse");
		CHECK((n | tt::member("value") | tt::number) == 120);
		auto kids = n | tt::member("children");
		REQUIRE(kids.has_value());
		REQUIRE(kids.value_tree().children_size() == 1);
		tt kid(kids.value_tree()[0]);
		CHECK((kid | tt::member("message") | tt::string)
			== std::string(idni::diagnostics::code_name(code::info)));
		CHECK_FALSE((kid | tt::member("children")).has_value());
	}

	TEST_CASE("names false omits the message field") {
		report r;
		r.error(code::parse_error, "bad");
		auto nodes = tt(jt::build_report(r, false)) | tt::member("nodes");
		REQUIRE(nodes.value_tree().children_size() == 1);
		CHECK_FALSE((tt(nodes.value_tree()[0]) | tt::member("message"))
			.has_value());
	}

	TEST_CASE("a text label attr is a string, a numeric attr a number") {
		report r;
		r.error(code::io_error, "failed", 7,
			{{label::name, std::string_view("tok")},
			 {label::exit_code, idni::int_t(-1)}});
		auto nodes = tt(jt::build_report(r, true)) | tt::member("nodes");
		REQUIRE(nodes.value_tree().children_size() == 1);
		tt n(nodes.value_tree()[0]);
		CHECK((n | tt::member("value") | tt::number) == 7);
		auto attrs = n | tt::member("attrs");
		REQUIRE(attrs.has_value());
		REQUIRE(attrs.value_tree().children_size() == 2);
		tt a0(attrs.value_tree()[0]), a1(attrs.value_tree()[1]);
		CHECK((a0 | tt::member("key") | tt::string) == "name");
		CHECK((a0 | tt::member("value")).is(json_parser::str));
		CHECK((a0 | tt::member("value") | tt::string) == "tok");
		CHECK((a1 | tt::member("value")).is(json_parser::number));
		CHECK((a1 | tt::member("value") | tt::number) == -1);
	}

	TEST_CASE("the report print is one line") {
		report r;
		r.error(code::parse_error, "bad");
		const std::string text = jt::get(jt::build_report(r, true))
			.to_str();
		CHECK(text.find('\n') == std::string::npos);
		CHECK(text.find("\"nodes\"") != std::string::npos);
	}

	TEST_CASE("report to JSON and back prints the same text") {
		report r;
		{
			auto s = r.open("parse", code::info_count, size_t(120));
			r.info("child");
		}
		r.error(code::io_error, "failed", 7,
			{{label::name, std::string_view("tok")},
			 {label::exit_code, idni::int_t(-1)}});
		r.warning("careful");
		auto back = report_of(jt::build_report(r, true));
		std::ostringstream a, b;
		r.print(a);
		back.print(b);
		CHECK(a.str() == b.str());
	}
}

TEST_SUITE("ast_json: parser tree to AST JSON") {
	using p_t = idni::parser<char, char32_t>;
	using lit_t = idni::lit<char, char32_t>;
	using tree_t = p_t::tree;
	using pnode_t = p_t::pnode;

	static idni::nonterminals<char, char32_t> nts({"", "start", "digit"});

	static tref term_node(char32_t c, size_t b, size_t e) {
		return tree_t::get(pnode_t(lit_t(c), {b, e}));
	}

	TEST_CASE("symbol, range, terminal text, null skipped") {
		tref term = term_node(char32_t('1'), 0, 1);
		tref nul = tree_t::get(pnode_t(lit_t(), {1, 1}));
		tref dig = tree_t::get(pnode_t(nts("digit"), {0, 1}),
			trefs{term, nul});
		tref root = tree_t::get(pnode_t(nts("start"), {0, 1}),
			trefs{dig});

		tref v = idni::format::ast_json::to_tree<tree_t>(
			root, "start", "1");
		CHECK((tt(v) | tt::member("format") | tt::string) == "ast");
		CHECK((tt(v) | tt::member("start") | tt::string) == "start");
		CHECK((tt(v) | tt::member("input") | tt::string) == "1");
		auto ast = tt(v) | tt::member("ast");
		REQUIRE(ast.has_value());
		CHECK((ast | tt::member("symbol") | tt::string) == "start");
		REQUIRE((ast | tt::member("id")).has_value());
		CHECK((ast | tt::member("id") | tt::number)
			== static_cast<double>(nts("start").n()));
		CHECK_FALSE((ast | tt::member("text")).has_value());
		auto kids = ast | tt::member("children");
		REQUIRE(kids.has_value());
		REQUIRE(kids.value_tree().children_size() == 1);
		tt digit(kids.value_tree()[0]);
		CHECK((digit | tt::member("symbol") | tt::string) == "digit");
		REQUIRE((digit | tt::member("id")).has_value());
		CHECK((digit | tt::member("id") | tt::number)
			== static_cast<double>(nts("digit").n()));
		CHECK_FALSE((digit | tt::member("text")).has_value());
		auto dk = digit | tt::member("children");
		REQUIRE(dk.has_value());
		REQUIRE(dk.value_tree().children_size() == 1);
		tt leaf(dk.value_tree()[0]);
		CHECK((leaf | tt::member("symbol") | tt::string) == "");
		CHECK_FALSE((leaf | tt::member("id")).has_value());
		CHECK((leaf | tt::member("text") | tt::string) == "1");
		CHECK_FALSE((leaf | tt::member("children")).has_value());
	}

	TEST_CASE("range holds the code point offsets") {
		tref term = term_node(char32_t('x'), 2, 3);
		tref root = tree_t::get(pnode_t(nts("start"), {2, 3}),
			trefs{term});
		tref v = idni::format::ast_json::node_to_tree<tree_t>(root);
		auto range = tt(v) | tt::member("range");
		REQUIRE(range.has_value());
		REQUIRE(range.value_tree().children_size() == 2);
		CHECK((range | tt::first | tt::number) == 2);
		CHECK((range | tt::second | tt::number) == 3);
	}
}

TEST_SUITE("json: report print and report nodes") {
	using idni::diagnostics::code;
	using idni::diagnostics::report;
	using idni::parser_strings::label;

	// A root with a child, a root with two attrs and a warning.
	static report sample() {
		report r;
		{
			auto s = r.open("parse", code::info_count, size_t(120));
			r.info("child");
		}
		r.error(code::io_error, "failed", 7,
			{{label::name, std::string_view("tok")},
			 {label::exit_code, idni::int_t(-1)}});
		r.warning("careful");
		return r;
	}

	static std::string printed(const report& r, bool names) {
		std::ostringstream os;
		print(r, os, names);
		return os.str();
	}

	const std::string named = "{\"nodes\":["
		"{\"tag\":32771,\"message\":\"Count.\",\"key\":\"parse\","
		"\"value\":120,\"attrs\":[],\"children\":["
		"{\"tag\":32768,\"message\":\"Info.\",\"key\":\"child\","
		"\"attrs\":[]}]},"
		"{\"tag\":3,\"message\":\"I/O error.\",\"key\":\"failed\","
		"\"value\":7,\"attrs\":[{\"key\":\"name\",\"value\":\"tok\"},"
		"{\"key\":\"exit code\",\"value\":-1}]},"
		"{\"tag\":16384,\"message\":\"Warning.\",\"key\":\"careful\","
		"\"value\":0,\"attrs\":[]}]}";
	const std::string plain = "{\"nodes\":["
		"{\"tag\":32771,\"key\":\"parse\",\"value\":120,\"attrs\":[],"
		"\"children\":[{\"tag\":32768,\"key\":\"child\","
		"\"attrs\":[]}]},"
		"{\"tag\":3,\"key\":\"failed\",\"value\":7,\"attrs\":["
		"{\"key\":\"name\",\"value\":\"tok\"},"
		"{\"key\":\"exit code\",\"value\":-1}]},"
		"{\"tag\":16384,\"key\":\"careful\",\"value\":0,\"attrs\":[]}]}";

	TEST_CASE("print writes the report with names") {
		CHECK(printed(sample(), true) == named);
	}

	TEST_CASE("print writes the report without names") {
		CHECK(printed(sample(), false) == plain);
	}

	TEST_CASE("print writes no names by default") {
		std::ostringstream os;
		print(sample(), os);
		CHECK(os.str() == plain);
	}

	TEST_CASE("print of an empty report") {
		CHECK(printed(report{}, true) == "{\"nodes\":[]}");
	}

	TEST_CASE("build_report_node gives one root of build_report") {
		report r = sample();
		std::vector<std::vector<size_t>> kids(r.nodes().size());
		for (size_t i = 0; i < r.nodes().size(); ++i)
			if (r.nodes()[i].parent >= 0) kids[static_cast<size_t>(
				r.nodes()[i].parent)].push_back(i);
		auto nodes = tt(jt::build_report(r, true)) | tt::member("nodes");
		REQUIRE(nodes.value_tree().children_size() == 3);
		// an element tref also holds its siblings, so compare the text
		CHECK(jt::get(jt::build_report_node(r, 0, kids, true)).to_str()
			== nodes.value_tree()[0].to_str());
		CHECK(jt::get(jt::build_report_node(r, 2, kids, false)).to_str()
			== (tt(jt::build_report(r, false)) | tt::member("nodes"))
				.value_tree()[1].to_str());
	}

	TEST_CASE("report_node_of appends one node under the open parent") {
		report r = sample();
		auto nodes = tt(jt::build_report(r, true)) | tt::member("nodes");
		report back;
		{
			auto s = back.open("outer", code::info, size_t(0));
			report_node_of(back, nodes.value_tree()[0].get());
		}
		REQUIRE(back.nodes().size() == 3);
		CHECK(back.nodes()[1].parent == 0);
		CHECK(back.nodes()[2].parent == 1);
		CHECK(back.str(back.nodes()[1].key) == "parse");
		CHECK(back.nodes()[1].value == 120);
		CHECK(back.str(back.nodes()[2].key) == "child");
	}

	TEST_CASE("report_of reads back what print wrote") {
		report back = report_of(parsed(printed(sample(), true)));
		CHECK(printed(back, true) == named);
	}

	TEST_CASE("report_of reads no number that does not fit") {
		report back = report_of(parsed(R"({"nodes":[)"
			R"({"tag":70000,"key":"a","value":1e300,"attrs":[)"
			R"({"key":"port","value":-1e300}]},)"
			R"({"tag":-1,"key":"b","value":1.5,"attrs":[]},)"
			R"({"tag":3.5,"key":"c","attrs":[]}]})"));
		REQUIRE(back.nodes().size() == 3);
		for (const auto& n : back.nodes()) {
			CHECK(n.tag == code::unknown);
			CHECK(n.value == 0);
		}
		CHECK(back.nodes()[0].attr_cnt == 0);
	}

	TEST_CASE("a report value above 2^53 keeps its digits") {
		constexpr int64_t big = 9007199254740993;
		report r;
		// size_t is 32 bits on wasm32, so the number goes in an attr
		diagnostics::attr port;
		port.key = label::port;
		port.value = big;
		r.error(code::io_error, "big", 0, { port });
		r.push_node(code::info_count, r.intern("n"), big, {});
		const std::string text = printed(r, false);
		CHECK(text.find(R"("value":9007199254740993)") != std::string::npos);
		report back = report_of(parsed(text));
		REQUIRE(back.nodes().size() == 2);
		CHECK(back.attrs()[back.nodes()[0].attr_off].value == big);
		CHECK(back.nodes()[1].value == big);
		CHECK(printed(back, false) == text);
	}
}

TEST_SUITE("json: parse a text and a parse tree node") {
	TEST_CASE("result holds the tree of a parsed text") {
		result r = jt::get("[1,\"a\"]");
		REQUIRE(r.has_value());
		CHECK(jt::get(r.value()).to_str() == "[1,\"a\"]");
	}

	TEST_CASE("the tree of one str node of a parse tree") {
		const std::string text = "\"a\\nb\"";
		json_parser::parse_options po{ .start = json_parser::start };
		auto pr = json_parser::instance().parse(text.data(), text.size(),
			po);
		REQUIRE(pr.found);
		auto str = trv(pr.get_shaped_tree2()) | json_parser::value
			| json_parser::str;
		REQUIRE(str.has_value());
		auto r = jt::get(parse_tree::get(str.value()));
		REQUIRE(r.has_value());
		CHECK(jt::get(r.value()).get_string() == "a\nb");
	}
}
