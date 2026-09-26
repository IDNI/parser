// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "format/json/json.h"
#include "format/ast.json/ast_json.h"
#include "parser.h"

#include <sstream>
#include <string>

using namespace idni;
using namespace idni::format::json;

TEST_SUITE("json: value kinds") {
	TEST_CASE("null") {
		auto r = parse("null");
		REQUIRE(r.has_value());
		CHECK(r.value().is_null());
	}
	TEST_CASE("true") {
		auto r = parse("true");
		REQUIRE(r.has_value());
		CHECK(r.value().is_bool());
		CHECK(r.value().as_bool());
	}
	TEST_CASE("false") {
		auto r = parse("false");
		REQUIRE(r.has_value());
		CHECK(r.value().is_bool());
		CHECK_FALSE(r.value().as_bool());
	}
	TEST_CASE("integer number") {
		auto r = parse("42");
		REQUIRE(r.has_value());
		CHECK(r.value().is_number());
		CHECK(r.value().as_number() == 42);
	}
	TEST_CASE("negative fractional exponent number") {
		auto r = parse("-1.5e2");
		REQUIRE(r.has_value());
		CHECK(r.value().as_number() == -150);
	}
	TEST_CASE("string") {
		auto r = parse("\"hello\"");
		REQUIRE(r.has_value());
		CHECK(r.value().is_string());
		CHECK(r.value().as_string() == "hello");
	}
	TEST_CASE("array") {
		auto r = parse("[1,2,3]");
		REQUIRE(r.has_value());
		CHECK(r.value().is_array());
		CHECK(r.value().size() == 3);
	}
	TEST_CASE("object") {
		auto r = parse("{\"a\":1}");
		REQUIRE(r.has_value());
		CHECK(r.value().is_object());
		REQUIRE(r.value().find("a") != nullptr);
		CHECK(r.value().find("a")->as_number() == 1);
	}
}

TEST_SUITE("json: array indexing and iteration") {
	TEST_CASE("operator[] and range-for see the same elements") {
		auto r = parse("[10,20,30]");
		REQUIRE(r.has_value());
		auto& v = r.value();
		CHECK(v[0].as_number() == 10);
		CHECK(v[1].as_number() == 20);
		CHECK(v[2].as_number() == 30);
		double sum = 0;
		for (auto& e : v) sum += e.as_number();
		CHECK(sum == 60);
	}
}

TEST_SUITE("json: nesting") {
	TEST_CASE("object containing an array containing an object") {
		auto r = parse(R"({"a":[1,{"b":true},null],"c":{"d":[]}})");
		REQUIRE(r.has_value());
		auto& v = r.value();
		REQUIRE(v.find("a") != nullptr);
		CHECK(v.find("a")->size() == 3);
		CHECK((*v.find("a"))[1].find("b")->as_bool());
		REQUIRE(v.find("c") != nullptr);
		REQUIRE(v.find("c")->find("d") != nullptr);
		CHECK(v.find("c")->find("d")->is_array());
		CHECK(v.find("c")->find("d")->size() == 0);
	}
}

TEST_SUITE("json: escaped string round trip") {
	TEST_CASE("newline escape reads back as a real newline") {
		std::ostringstream os;
		escape(os, "a\nb");
		auto r = parse(os.str());
		REQUIRE(r.has_value());
		CHECK(r.value().as_string() == "a\nb");
	}
	TEST_CASE("quote and backslash round trip") {
		std::ostringstream os;
		escape(os, "a\"b\\c");
		auto r = parse(os.str());
		REQUIRE(r.has_value());
		CHECK(r.value().as_string() == "a\"b\\c");
	}
}

TEST_SUITE("json: CR whitespace") {
	TEST_CASE("CR is whitespace between every token") {
		auto r = parse("{\r\"a\"\r:\r[\r1\r,\r2\r]\r,\r\"b\"\r:\rtrue\r}\r");
		REQUIRE(r.has_value());
		CHECK(r.value().is_object());
		REQUIRE(r.value().find("a") != nullptr);
		CHECK(r.value().find("a")->is_array());
		CHECK(r.value().find("a")->size() == 2);
		REQUIRE(r.value().find("b") != nullptr);
		CHECK(r.value().find("b")->as_bool());
	}
	TEST_CASE("CR before the first token") {
		auto r = parse("\r\r1");
		REQUIRE(r.has_value());
		CHECK(r.value().as_number() == 1);
	}
}

TEST_SUITE("json: empty containers") {
	TEST_CASE("empty array") {
		auto r = parse("[]");
		REQUIRE(r.has_value());
		CHECK(r.value().is_array());
		CHECK(r.value().size() == 0);
	}
	TEST_CASE("empty object") {
		auto r = parse("{}");
		REQUIRE(r.has_value());
		CHECK(r.value().is_object());
		CHECK(r.value().size() == 0);
	}
}

TEST_SUITE("json: missing object key") {
	TEST_CASE("find returns nullptr, not a crash") {
		auto r = parse("{\"a\":1}");
		REQUIRE(r.has_value());
		CHECK(r.value().find("missing") == nullptr);
	}
}

TEST_SUITE("json: malformed input reports an error") {
	TEST_CASE("empty input") {
		auto r = parse("");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("trailing comma in array") {
		auto r = parse("[1,2,]");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("trailing comma in object") {
		auto r = parse("{\"a\":1,}");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("unterminated string") {
		auto r = parse("\"abc");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("lone high surrogate is a semantic error the grammar can't catch") {
		auto r = parse("\"\\uD800\"");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
	TEST_CASE("lone high surrogate as an object key") {
		auto r = parse("{\"\\uD800\":1}");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
}

TEST_SUITE("json: writer builds arbitrary objects and arrays") {
	TEST_CASE("build, print, and re-parse an object with a nested array") {
		value v = value::object();
		v.set("id", value::number(1));
		v.set("result", value::string("ok"));
		value nums = value::array();
		nums.push_back(value::number(1)).push_back(value::number(2));
		v.set("nums", nums);

		std::ostringstream os;
		print(v, os);
		auto r = parse(os.str());
		REQUIRE(r.has_value());
		REQUIRE(r.value().find("id") != nullptr);
		CHECK(r.value().find("id")->as_number() == 1);
		CHECK(r.value().find("result")->as_string() == "ok");
		REQUIRE(r.value().find("nums") != nullptr);
		CHECK(r.value().find("nums")->size() == 2);
	}
}

TEST_SUITE("json: report to nested value") {
	using idni::diagnostics::code;
	using idni::diagnostics::report;
	using idni::parser_strings::label;

	TEST_CASE("nested tree carries message names on request") {
		report r;
		{
			auto s = r.open("parse", code::info_count, size_t(120));
			r.info("child");
		}
		auto v = to_value(r, true);
		REQUIRE(v.is_object());
		auto nodes = v.find("nodes");
		REQUIRE(nodes != nullptr);
		REQUIRE(nodes->is_array());
		REQUIRE(nodes->size() == 1);
		const auto& n = (*nodes)[0];
		CHECK(n.find("tag")->as_number()
			== static_cast<double>(code::info_count));
		CHECK(n.find("message")->as_string()
			== std::string(idni::diagnostics::code_name(code::info_count)));
		CHECK(n.find("key")->as_string() == "parse");
		CHECK(n.find("value")->as_number() == 120);
		auto kids = n.find("children");
		REQUIRE(kids != nullptr);
		REQUIRE(kids->size() == 1);
		CHECK((*kids)[0].find("message")->as_string()
			== std::string(idni::diagnostics::code_name(code::info)));
		CHECK((*kids)[0].find("children") == nullptr);
	}

	TEST_CASE("names false omits the message field") {
		report r;
		r.error(code::parse_error, "bad");
		auto v = to_value(r, false);
		REQUIRE(v.find("nodes")->size() == 1);
		CHECK((*v.find("nodes"))[0].find("message") == nullptr);
	}

	TEST_CASE("a text label attr is a string, a numeric attr a number") {
		report r;
		r.error(code::io_error, "failed", 7,
			{{label::name, std::string_view("tok")},
			 {label::exit_code, idni::int_t(-1)}});
		auto v = to_value(r, true);
		REQUIRE(v.find("nodes")->size() == 1);
		const auto& n = (*v.find("nodes"))[0];
		CHECK(n.find("value")->as_number() == 7);
		auto attrs = n.find("attrs");
		REQUIRE(attrs != nullptr);
		REQUIRE(attrs->size() == 2);
		CHECK((*attrs)[0].find("key")->as_string() == "name");
		CHECK((*attrs)[0].find("value")->is_string());
		CHECK((*attrs)[0].find("value")->as_string() == "tok");
		CHECK((*attrs)[1].find("value")->is_number());
		CHECK((*attrs)[1].find("value")->as_number() == -1);
	}

	TEST_CASE("the report print is one line") {
		report r;
		r.error(code::parse_error, "bad");
		std::ostringstream os;
		print(r, os, true);
		CHECK(os.str().find('\n') == std::string::npos);
		CHECK(os.str().find("\"nodes\"") != std::string::npos);
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

		auto v = idni::format::ast_json::to_value<tree_t>(
			root, "start", "1");
		CHECK(v.find("format")->as_string() == "ast");
		CHECK(v.find("start")->as_string() == "start");
		CHECK(v.find("input")->as_string() == "1");
		auto ast = v.find("ast");
		REQUIRE(ast != nullptr);
		CHECK(ast->find("symbol")->as_string() == "start");
		REQUIRE(ast->find("id") != nullptr);
		CHECK(ast->find("id")->as_number()
			== static_cast<double>(nts("start").n()));
		CHECK(ast->find("text") == nullptr);
		auto kids = ast->find("children");
		REQUIRE(kids != nullptr);
		REQUIRE(kids->size() == 1);
		const auto& digit = (*kids)[0];
		CHECK(digit.find("symbol")->as_string() == "digit");
		REQUIRE(digit.find("id") != nullptr);
		CHECK(digit.find("id")->as_number()
			== static_cast<double>(nts("digit").n()));
		CHECK(digit.find("text") == nullptr);
		auto dk = digit.find("children");
		REQUIRE(dk != nullptr);
		REQUIRE(dk->size() == 1);
		const auto& leaf = (*dk)[0];
		CHECK(leaf.find("symbol")->as_string() == "");
		CHECK(leaf.find("id") == nullptr);
		CHECK(leaf.find("text")->as_string() == "1");
		CHECK(leaf.find("children") == nullptr);
	}

	TEST_CASE("range holds the code point offsets") {
		tref term = term_node(char32_t('x'), 2, 3);
		tref root = tree_t::get(pnode_t(nts("start"), {2, 3}),
			trefs{term});
		auto v = idni::format::ast_json::node_to_value<tree_t>(root);
		auto range = v.find("range");
		REQUIRE(range != nullptr);
		REQUIRE(range->size() == 2);
		CHECK((*range)[0].as_number() == 2);
		CHECK((*range)[1].as_number() == 3);
	}
}
