// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "format/json/json.h"
#include "format/ast.json/ast_json.h"

#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace idni;
using namespace idni::format::json;

using jt = idni::format::json::tree;

// The tree of a JSON text that must parse.
static tref parsed(std::string_view text) {
	auto r = jt::get(text);
	REQUIRE(r.has_value());
	return r.value();
}

// The bytes of the hex text @p s.
static std::string unhex(std::string_view s) {
	std::string b;
	for (size_t i = 0; i + 1 < s.size(); i += 2)
		b.push_back(static_cast<char>(std::stoi(
			std::string(s.substr(i, 2)), nullptr, 16)));
	return b;
}

TEST_SUITE("json tree: round trip") {
	// The texts the tests of doctest_json read.
	const std::vector<std::string> texts{
		"null", "true", "false", "42", "-1.5e2", "\"hello\"",
		"[1,2,3]", "{\"a\":1}", "[10,20,30]",
		R"({"a":[1,{"b":true},null],"c":{"d":[]}})",
		"\"line\\nbreak\"", R"("q\"b\\")",
		"{\r\"a\"\r:\r[\r1\r,\r2\r]\r,\r\"b\"\r:\rtrue\r}\r",
		"\r\r1", "[]", "{}" };

	TEST_CASE("the tree prints the expected text") {
		const std::vector<std::string> printed{
			"null", "true", "false", "42", "-150", "\"hello\"",
			"[1,2,3]", "{\"a\":1}", "[10,20,30]",
			R"({"a":[1,{"b":true},null],"c":{"d":[]}})",
			"\"line\\nbreak\"", R"("q\"b\\")",
			R"({"a":[1,2],"b":true})", "1", "[]", "{}" };
		REQUIRE(printed.size() == texts.size());
		for (size_t i = 0; i != texts.size(); ++i) {
			CAPTURE(texts[i]);
			CHECK(jt::get(parsed(texts[i])).to_str() == printed[i]);
		}
	}

	TEST_CASE("the printed text parses back to the same node") {
		for (const auto& s : texts) {
			CAPTURE(s);
			tref t = parsed(s);
			CHECK(parsed(jt::get(t).to_str()) == t);
		}
	}

	TEST_CASE("malformed input reports an error and no value") {
		for (const char* s : { "", "[1,2,]", "{\"a\":1,}", "\"abc",
				"\"\\uD800\"", "{\"\\uD800\":1}" })
		{
			CAPTURE(s);
			auto r = jt::get(std::string_view(s));
			CHECK_FALSE(r.has_value());
			CHECK(r.has_error());
		}
	}
}

TEST_SUITE("json tree: values") {
	TEST_CASE("a repeated key keeps its first position and its last value") {
		tref t = parsed(R"({"a":1,"b":2,"a":3})");
		CHECK(jt::get(t).to_str() == R"({"a":3,"b":2})");
	}

	TEST_CASE("-0 is 0") {
		CHECK(jt::build_number(-0.0) == jt::build_number(0.0));
		CHECK(jt::get(parsed("-0")).to_str() == "0");
		CHECK(jt::get(parsed("[-0,0,-0.0]")).to_str() == "[0,0,0]");
		CHECK(parsed("-0") == parsed("0"));
	}

	TEST_CASE("escapes read back decoded and print escaped") {
		tref t = parsed(R"("a\nb\"c\\d\/e\t")");
		CHECK(jt::get(t).get_string() == "a\nb\"c\\d/e\t");
		CHECK(jt::get(t).to_str() == R"("a\nb\"c\\d/e\t")");
	}

	TEST_CASE("unicode escapes and raw UTF-8 decode to UTF-8") {
		tref t = parsed(R"("\u00e9\ud83d\ude00")");
		CHECK(jt::get(t).get_string() == "\xc3\xa9\xf0\x9f\x98\x80");
		CHECK(parsed("\"\xc3\xa9\xf0\x9f\x98\x80\"") == t);
		CHECK(parsed(jt::get(t).to_str()) == t);
	}

	TEST_CASE("an element printed alone has no trailing comma") {
		tref t = parsed("[1,[2,3],4]");
		CHECK(jt::get(jt::get(t).first()).to_str() == "1");
		CHECK(jt::get(jt::get(t).second()).to_str() == "[2,3]");
	}

	TEST_CASE("builders, queries and the traverser") {
		tref state = jt::build_object({
			jt::build_pair("grammar", jt::build_string("g.tgf")),
			jt::build_pair("start", jt::_null()) });
		tref t = jt::build_object({
			jt::build_pair("id", jt::build_number(7)),
			jt::build_pair("ok", jt::build_bool(true)),
			jt::build_pair("list", jt::build_array({
				jt::build_string("x"), jt::build_number(1.5) })),
			jt::build_pair("state", state) });
		CHECK(jt::get(t).to_str() == R"({"id":7,"ok":true,)"
			R"("list":["x",1.5],"state":{"grammar":"g.tgf",)"
			R"("start":null}})");
		CHECK((tt(t) | tt::member("id") | tt::number) == 7);
		CHECK((tt(t) | tt::member("ok") | tt::boolean) == true);
		CHECK((tt(t) | tt::member("state") | tt::member("grammar")
			| tt::string) == "g.tgf");
		CHECK_FALSE((tt(t) | tt::member("missing")).has_value());
		CHECK_FALSE((tt(t) | tt::member("missing") | tt::member("x"))
			.has_value());
		CHECK((tt(t) | tt::member("list") | tt::children).size() == 2);
		CHECK((tt(t) || json_parser::object_pair).size() == 4);
		CHECK((tt(t) | tt::member("list") | json_parser::number
			| tt::number) == 1.5);
		CHECK(*string_member(state, "grammar") == "g.tgf");
		CHECK(string_member(state, "start") == nullptr);
		CHECK(format::json::is(t, json_parser::object));
		CHECK(format::json::is(json_parser::object)(t));
		CHECK(format::json::is(t,
			{ json_parser::arr, json_parser::object }));
		CHECK(jt::get(t).find("state") == state);
		CHECK(jt::get(t).find("nope") == nullptr);
		tref u = jt::set_member(t, "id", jt::build_number(8));
		CHECK(jt::get(u).to_str().starts_with(R"({"id":8,"ok":true)"));
		tref w = jt::set_member(state, "start", jt::build_string("s"));
		CHECK(jt::get(w).to_str() == R"({"grammar":"g.tgf","start":"s"})");
	}
}

TEST_SUITE("json tree: integers") {
	TEST_CASE("an integer above 2^53 prints as it was read") {
		for (const char* s : { "9007199254740993",
				"9223372036854775807", "-9223372036854775808" })
		{
			CAPTURE(s);
			tref t = parsed(s);
			CHECK(jt::get(t).to_str() == s);
			REQUIRE(jt::get(t).get_integer().has_value());
			CHECK(std::to_string(*jt::get(t).get_integer()) == s);
		}
		CHECK(*jt::get(parsed("9007199254740993")).get_integer()
			== 9007199254740993);
	}

	TEST_CASE("-0 prints 0 and has the integer 0") {
		CHECK(jt::get(parsed("-0")).to_str() == "0");
		CHECK(jt::get(parsed("-0")).get_integer() == 0);
	}

	TEST_CASE("a fraction has no integer") {
		CHECK_FALSE(jt::get(parsed("1.5")).get_integer().has_value());
		CHECK_FALSE(jt::get(parsed("2.5e-1")).get_integer().has_value());
		CHECK_FALSE(jt::get(jt::build_number(1.5)).get_integer()
			.has_value());
	}

	TEST_CASE("an exponent form with an integer value is that integer") {
		CHECK(jt::get(parsed("1e3")).get_integer() == 1000);
		CHECK(jt::get(parsed("1e3")).to_str() == "1000");
		CHECK(parsed("1e3") == parsed("1000"));
		CHECK(parsed("-1.5e2") == jt::build_integer(-150));
		CHECK(parsed("2.0") == parsed("2"));
		CHECK(parsed("-0.0") == parsed("0"));
	}

	TEST_CASE("a fraction or exponent literal has an integer only when exact") {
		CHECK(jt::get(parsed("1e3")).get_integer() == 1000);
		CHECK(jt::get(parsed("100.0")).get_integer() == 100);
		CHECK(parsed("100.0") == parsed("100"));
		CHECK(parsed("1e3") == parsed("1000"));
		CHECK_FALSE(jt::get(parsed("1.5")).get_integer().has_value());
		// its double is 2^53, not the literal
		CHECK_FALSE(jt::get(parsed("9007199254740993.0")).get_integer()
			.has_value());
		CHECK(jt::get(parsed("9007199254740992.0")).get_integer()
			== 9007199254740992);
		CHECK(jt::get(parsed("-0.0")).get_integer() == 0);
		CHECK(jt::get(parsed("2500e-2")).get_integer() == 25);
		CHECK_FALSE(jt::get(parsed("1500e-3")).get_integer().has_value());
		// the leading zeros of an exponent do not count
		CHECK(jt::get(parsed("1e000003")).get_integer() == 1000);
		CHECK(jt::get(parsed("1000000e-000006")).get_integer() == 1);
	}

	TEST_CASE("a value below the smallest double reads as 0") {
		const std::string tiny = "0." + std::string(400, '0') + "1";
		for (const std::string& s : { std::string("1e-400"),
				std::string("-1e-400"), std::string("123.45E-500"),
				tiny, "1" + std::string(400, '0') + "e-800" })
		{
			CAPTURE(s);
			tref t = parsed(s);
			CHECK(jt::get(t).get_number() == 0);
			CHECK(jt::get(t).to_str() == "0");
		}
	}

	TEST_CASE("a value above the largest double is an error") {
		for (const std::string& s : { std::string("1e400"),
				std::string("-1e400"), "1" + std::string(400, '0') + ".5",
				std::string("0.0001e500") })
		{
			CAPTURE(s);
			CHECK_FALSE(jt::get(s).has_value());
		}
	}

	TEST_CASE("number_below_one reads the digits") {
		using idni::format::json::detail::number_below_one;
		CHECK(number_below_one("0.5"));
		CHECK(number_below_one("-5e-1"));
		CHECK(number_below_one("10e-2"));
		CHECK_FALSE(number_below_one("1"));
		CHECK_FALSE(number_below_one("10e-1"));
		CHECK_FALSE(number_below_one("0"));
		CHECK_FALSE(number_below_one("0.000"));
		CHECK_FALSE(number_below_one("1e" + std::string(30, '9')));
		CHECK(number_below_one("1e-" + std::string(30, '9')));
		CHECK_FALSE(number_below_one("1e"));
		CHECK_FALSE(number_below_one("abc"));
	}

	TEST_CASE("split_decimal gives the sign, the digits and the exponent") {
		using idni::format::json::detail::split_decimal;
		const auto p = split_decimal("-12.50e-3");
		REQUIRE(p.has_value());
		CHECK(p->neg);
		CHECK(p->integral == "12");
		CHECK(p->fraction == "50");
		CHECK(p->exp10 == -3);
		const auto q = split_decimal("7E+2");
		REQUIRE(q.has_value());
		CHECK_FALSE(q->neg);
		CHECK(q->integral == "7");
		CHECK(q->fraction.empty());
		CHECK(q->exp10 == 2);
		CHECK(split_decimal("1e" + std::string(30, '9'))->exp10 == 1000000);
		CHECK(split_decimal("1e-" + std::string(30, '9'))->exp10 == -1000000);
		for (const char* s : { "1e", "-", "1e+", "abc", "", "1x" }) {
			CAPTURE(s);
			CHECK_FALSE(split_decimal(s).has_value());
		}
	}

	TEST_CASE("an infinity or a NaN builds null") {
		CHECK(jt::build_number(INFINITY) == jt::_null());
		CHECK(jt::build_number(-INFINITY) == jt::_null());
		CHECK(jt::build_number(NAN) == jt::_null());
		CHECK(jt::get(jt::build_array({ jt::build_number(NAN) })).to_str()
			== "[null]");
	}

	TEST_CASE("an integer outside int64 prints its text and has no integer") {
		tref t = parsed("18446744073709551616");
		CHECK(jt::get(t).to_str() == "18446744073709551616");
		CHECK_FALSE(jt::get(t).get_integer().has_value());
		CHECK(jt::get(t).get_number() == 0x1p64);
	}

	TEST_CASE("get_integer_text gives the text of an integer of any size") {
		for (const char* s : { "0", "-7", "9223372036854775807",
				"18446744073709551615", "-18446744073709551616" })
		{
			CAPTURE(s);
			CHECK(jt::get(parsed(s)).get_integer_text() == s);
		}
		const std::string big = "1" + std::string(40, '0');
		CHECK(jt::get(jt::build_integer(std::string_view(big)).value())
			.get_integer_text() == big);
		CHECK(jt::get(jt::build_integer(-150)).get_integer_text() == "-150");
		CHECK(jt::get(parsed("-0")).get_integer_text() == "0");
		CHECK(jt::get(parsed("1e3")).get_integer_text() == "1000");
		CHECK_FALSE(jt::get(parsed("1.5")).get_integer_text().has_value());
		CHECK_FALSE(jt::get(jt::build_float(2.0)).get_integer_text()
			.has_value());
		CHECK_FALSE(jt::get(jt::build_string("12")).get_integer_text()
			.has_value());
		CHECK_FALSE(jt::get(jt::_null()).get_integer_text().has_value());
	}

	TEST_CASE("the builders and the parse of an integer give one node") {
		CHECK(parsed("1") == jt::build_number(1.0));
		CHECK(parsed("1") == jt::build_integer(1));
		CHECK(parsed("9007199254740993")
			== jt::build_integer(9007199254740993));
		CHECK(jt::get(jt::build_integer(-7)).to_str() == "-7");
		CHECK(jt::get(jt::build_integer(-7)).get_integer() == -7);
		CHECK(jt::get(jt::build_number(0x1p63)).to_str()
			== "9223372036854775808");
		CHECK_FALSE(jt::get(jt::build_number(0x1p63)).get_integer()
			.has_value());
		CHECK(jt::get(jt::build_string("12")).get_integer()
			== std::nullopt);
	}
}

TEST_SUITE("json tree: print options") {
	// The tree of one vector line: { } [ ] open and close, k: a key,
	// s: a string, i: an int64, u: a uint64, each value in hex or decimal.
	static tref vector_tree(const std::string& calls) {
		struct level {
			bool object;
			trefs items{};
			std::string key{};
		};
		std::vector<level> open;
		tref root = nullptr;
		auto add = [&open, &root](tref v) {
			if (open.empty()) { root = v; return; }
			level& l = open.back();
			l.items.push_back(l.object ? jt::build_pair(l.key, v) : v);
		};
		std::istringstream ss(calls);
		std::string c;
		while (ss >> c) {
			if (c == "{" || c == "[") open.push_back({ c == "{" });
			else if (c == "}" || c == "]") {
				level l = std::move(open.back());
				open.pop_back();
				add(l.object ? jt::build_object(l.items)
					: jt::build_array(l.items));
			}
			else if (c.starts_with("k:")) open.back().key = unhex(c.substr(2));
			else if (c.starts_with("s:")) add(jt::build_string(unhex(c.substr(2))));
			else if (c.starts_with("i:"))
				add(jt::build_integer(std::stoll(c.substr(2))));
			// no builder takes a uint64, so the parse of the literal does
			else if (c.starts_with("u:")) add(parsed(c.substr(2)));
			else FAIL("unknown call " << c);
		}
		return root;
	}

#ifndef __EMSCRIPTEN__
	// a wasm test under node has no host file system
	TEST_CASE("sorted keys and ascii_only give the bytes of Python json.dumps") {
		const auto file = std::filesystem::path(__FILE__).parent_path()
			/ "data" / "json_vectors.txt";
		std::ifstream in(file);
		REQUIRE(in.good());
		std::string line;
		size_t count = 0;
		while (std::getline(in, line)) {
			const size_t tab = line.find('\t');
			REQUIRE(tab != std::string::npos);
			const std::string calls = line.substr(0, tab);
			CAPTURE(calls);
			tref t = vector_tree(calls);
			REQUIRE(t != nullptr);
			CHECK(jt::get(t).to_str({ .sorted_keys = true,
				.ascii_only = true }) == unhex(line.substr(tab + 1)));
			++count;
		}
		CHECK(count == 67);
	}
#endif

	TEST_CASE("sorted keys order every object and leave the tree as it is") {
		tref t = parsed(R"({"b":{"z":1,"a":[{"y":2,"x":3}]},"a":0,)"
			"\"\xc3\xa9\":1,\"z\":2}");
		const std::string before = jt::get(t).to_str();
		CHECK(jt::get(t).to_str({ .sorted_keys = true })
			== R"({"a":0,"b":{"a":[{"x":3,"y":2}],"z":1},"z":2,)"
			"\"\xc3\xa9\":1}");
		CHECK(jt::get(t).to_str() == before);
		CHECK(jt::get(t).to_str(print_options{}) == before);
	}

	TEST_CASE("ascii_only escapes every non-ASCII character") {
		tref t = jt::build_string("\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80");
		CHECK(jt::get(t).to_str({ .ascii_only = true })
			== R"("\u00e9\u20ac\ud83d\ude00")");
		CHECK(jt::get(t).to_str()
			== "\"\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80\"");
	}

	// The expected bytes come from Python json.dumps.
	TEST_CASE("the profiles print the layout of Python json.dumps") {
		tref t = parsed(R"({"a": [1, 2], "b": {}})");
		CHECK(jt::get(t).to_str(print_options::minified())
			== R"({"a":[1,2],"b":{}})");
		CHECK(jt::get(t).to_str(print_options::normal())
			== R"({"a": [1, 2], "b": {}})");
		CHECK(jt::get(t).to_str(print_options::pretty(2))
			== "{\n  \"a\": [\n    1,\n    2\n  ],\n  \"b\": {}\n}");
		CHECK(jt::get(t).to_str(print_options::pretty(4))
			== "{\n    \"a\": [\n        1,\n        2\n    ],\n"
			"    \"b\": {}\n}");
	}

	TEST_CASE("pretty keeps an empty array or object on one line") {
		tref t = parsed(R"({"x":[[],{}],"y":[{}],"z":{"e":[]}})");
		CHECK(jt::get(t).to_str(print_options::normal())
			== R"({"x": [[], {}], "y": [{}], "z": {"e": []}})");
		CHECK(jt::get(t).to_str(print_options::pretty(2))
			== "{\n  \"x\": [\n    [],\n    {}\n  ],\n  \"y\": [\n"
			"    {}\n  ],\n  \"z\": {\n    \"e\": []\n  }\n}");
		CHECK(jt::get(parsed("[]")).to_str(print_options::pretty(2))
			== "[]");
		CHECK(jt::get(parsed("{}")).to_str(print_options::pretty(2))
			== "{}");
		CHECK(jt::get(parsed("1")).to_str(print_options::pretty(2))
			== "1");
	}

	TEST_CASE("normal and pretty escape every non-ASCII character") {
		tref t = parsed("{\"k\":\"caf\xc3\xa9 \xf0\x9f\x98\x80\"}");
		CHECK(jt::get(t).to_str(print_options::normal())
			== R"({"k": "caf\u00e9 \ud83d\ude00"})");
		CHECK(jt::get(t).to_str(print_options::pretty(2))
			== "{\n  \"k\": \"caf\\u00e9 \\ud83d\\ude00\"\n}");
	}

	TEST_CASE("a profile takes a change of a single field") {
		auto o = print_options::normal();
		o.sorted_keys = true;
		CHECK(jt::get(parsed(R"({"b":1,"a":[2,3]})")).to_str(o)
			== R"({"a": [2, 3], "b": 1})");
	}

	TEST_CASE("the default options equal minified") {
		tref t = parsed(R"({"b":{"z":1,"a":[{"y":2},[]]},"a":"\u00e9"})");
		const std::string m = jt::get(t).to_str(print_options::minified());
		CHECK(jt::get(t).to_str() == m);
		CHECK(jt::get(t).to_str(print_options{}) == m);
		CHECK(m == "{\"b\":{\"z\":1,\"a\":[{\"y\":2},[]]},"
			"\"a\":\"\xc3\xa9\"}");
	}
}

TEST_SUITE("json tree: the grammar variant nonfinite") {
	// The tree of a text that the nonfinite grammar must parse.
	static tref nonfinite_parsed(std::string_view text) {
		json_parser::parse_options po{ .start = json_parser::start };
		auto r = idni::format::json::detail::nonfinite_parser().parse(text.data(), text.size(),
			po);
		auto t = jt::get(r);
		REQUIRE(t.has_value());
		return t.value();
	}

	TEST_CASE("NaN, Infinity and -Infinity are number nodes") {
		const tref nan = nonfinite_parsed("NaN");
		REQUIRE(jt::get(nan).is_number());
		CHECK(std::isnan(jt::get(nan).get_number()));
		CHECK(jt::get(nan).value.str.empty());
		CHECK(nonfinite_parsed("NaN") == nan);
		const tref inf = nonfinite_parsed("Infinity");
		CHECK(jt::get(inf).get_number()
			== std::numeric_limits<double>::infinity());
		const tref minf = nonfinite_parsed("-Infinity");
		CHECK(jt::get(minf).get_number()
			== -std::numeric_limits<double>::infinity());
		CHECK(inf != minf);
	}

	TEST_CASE("a constant in an array and in an object") {
		const tref t = nonfinite_parsed(R"({"a":[NaN,1,-Infinity]})");
		const tree& a = jt::get(jt::get(t).find("a"));
		REQUIRE(a.is_array());
		CHECK(std::isnan(a[0].get_number()));
		CHECK(a[1].get_integer() == 1);
		CHECK(a[2].get_number()
			== -std::numeric_limits<double>::infinity());
	}

	TEST_CASE("the shared parser stays on the variant standard") {
		(void)nonfinite_parsed("NaN");
		CHECK(idni::format::json::detail::nonfinite_grammar().opt().variant == "nonfinite");
		CHECK(json_parser_data::grammar().opt().variant == "standard");
		for (const char* text : { "NaN", "Infinity", "-Infinity",
				"[NaN]", R"({"a":Infinity})" })
		{
			CAPTURE(text);
			CHECK_FALSE(jt::get(std::string_view(text)).has_value());
		}
		CHECK(parsed("[1,-2.5]") != nullptr);
	}
}

TEST_SUITE("json tree: keep_float_form") {
	const read_options floats{ .nonfinite = true,
		.keep_float_form = true };

	static tref read(std::string_view text, const read_options& o) {
		auto r = jt::get(text, o);
		REQUIRE(r.has_value());
		return r.value();
	}
	// A float node: a number with no decimal text.
	static bool is_float(tref t) {
		return jt::get(t).is_number() && jt::get(t).value.str.empty();
	}

	TEST_CASE("the default reader refuses the nonfinite and float forms") {
		for (const char* text : { "NaN", "Infinity", "-Infinity",
				"1e400", "-1e400" })
		{
			CAPTURE(text);
			CHECK_FALSE(jt::get(std::string_view(text)).has_value());
			CHECK_FALSE(jt::get(text, read_options{}).has_value());
		}
	}

	TEST_CASE("the default options read as the default reader") {
		for (const char* text : { "1.0", "-0", "-0.0", "1e2", "0.5",
				"123456789012345678901234567890", "[1,2.5,{}]" })
		{
			CAPTURE(text);
			CHECK(read(text, read_options{}) == parsed(text));
		}
		CHECK(jt::get(read("1.0", read_options{})).to_str() == "1");
	}

	TEST_CASE("nonfinite alone reads NaN and keeps the number rules") {
		const read_options o{ .nonfinite = true };
		CHECK(std::isnan(jt::get(read("NaN", o)).get_number()));
		CHECK(read("1.0", o) == parsed("1.0"));
		CHECK_FALSE(jt::get("1e400", o).has_value());
	}

	TEST_CASE("a fraction or an exponent gives a float node") {
		for (const char* text : { "1.0", "1e15", "1e16", "1E22", "0e0",
				"1.0e-5", "0.0001", "0.1", "0.30000000000000004",
				"123456789012345678.0", "1.7976931348623157e308",
				"5e-324" })
		{
			CAPTURE(text);
			const tref t = read(text, floats);
			CHECK(is_float(t));
			double d = 0;
			REQUIRE(parse_number(text, d));
			CHECK(jt::get(t).get_number() == d);
		}
		CHECK(read("1.0", floats) != read("1", floats));
		CHECK(read("1.0", floats) == read("1e0", floats));
		CHECK(jt::get(read("5e-324", floats)).get_number()
			== std::numeric_limits<double>::denorm_min());
		CHECK(jt::get(read("123456789012345678.0", floats)).get_number()
			== 1.2345678901234568e17);
	}

	TEST_CASE("-0.0 stays and the integer -0 is 0") {
		const tref nz = read("-0.0", floats);
		CHECK(is_float(nz));
		CHECK(std::signbit(jt::get(nz).get_number()));
		CHECK(nz != read("0.0", floats));
		const tref iz = read("-0", floats);
		CHECK(iz == read("0", floats));
		CHECK(jt::get(iz).value.str == "0");
		CHECK_FALSE(std::signbit(jt::get(iz).get_number()));
	}

	TEST_CASE("a float text out of range is an infinity or a signed zero") {
		const double inf = std::numeric_limits<double>::infinity();
		CHECK(jt::get(read("1e400", floats)).get_number() == inf);
		CHECK(jt::get(read("-1e400", floats)).get_number() == -inf);
		const tref under = read("1e-400", floats);
		CHECK(is_float(under));
		CHECK(jt::get(under).get_number() == 0);
		CHECK_FALSE(std::signbit(jt::get(under).get_number()));
		CHECK(std::signbit(jt::get(read("-1e-400", floats)).get_number()));
		CHECK(read("1e-400", floats) == read("0.0", floats));
	}

	// The stream of libc++ fails on a subnormal and keeps its value.
	TEST_CASE("a subnormal reads as its value") {
		const double sub_min = std::numeric_limits<double>::denorm_min();
		const struct { const char* text; double value; } cases[] = {
			{ "4.9e-324", sub_min },
			{ "5e-324", sub_min },
			{ "-4.9e-324", -sub_min },
			{ "1e-310", 1e-310 },
			{ "2.2250738585072009e-308", 0x0.fffffffffffffp-1022 },
			{ "2.2250738585072014e-308", std::numeric_limits<double>::min() },
			// just above and just below half of the smallest subnormal
			{ "2.4703282292062328e-324", sub_min },
			{ "2.4703282292062327e-324", 0 },
			{ "-2.4703282292062327e-324", 0 },
		};
		for (const auto& c : cases) {
			const std::string text = c.text;
			CAPTURE(text);
			double d = 1;
			REQUIRE(parse_number(c.text, d));
			CHECK(d == c.value);
			CHECK(jt::get(read(c.text, floats)).get_number() == c.value);
			CHECK(jt::get(parsed(c.text)).get_number() == c.value);
		}
		CHECK(std::signbit(parse_number_saturating("-2.4703282292062327e-324")));
		CHECK(std::signbit(jt::get(read("-2.4703282292062327e-324", floats))
			.get_number()));
	}

	TEST_CASE("an integer text keeps its digits, also out of range") {
		for (const char* text : { "0", "9223372036854775807",
				"-9223372036854775808", "18446744073709551616",
				"-1208925819614629174706176" })
		{
			CAPTURE(text);
			const tref t = read(text, floats);
			CHECK(jt::get(t).value.str == text);
			CHECK(t == parsed(text));
		}
		const std::string big(400, '9');
		CHECK_FALSE(jt::get(std::string_view(big)).has_value());
		const tref t = read(big, floats);
		CHECK(jt::get(t).value.str == big);
		CHECK(jt::get(t).get_number()
			== std::numeric_limits<double>::infinity());
		CHECK(jt::get(read("-" + big, floats)).get_number()
			== -std::numeric_limits<double>::infinity());
	}

	TEST_CASE("the floats of a tx") {
		const tref t = read(R"({"fee_limit":5.0,"amount":1e3})", floats);
		CHECK(is_float(jt::get(t).find("fee_limit")));
		CHECK(jt::get(jt::get(t).find("amount")).get_number() == 1000);
		CHECK(is_float(jt::get(t).find("amount")));
	}

}

TEST_SUITE("json tree: lone surrogates") {
	const read_options lone{ .lone_surrogates = true };

	static std::string str_of(std::string_view text, const read_options& o) {
		auto r = jt::get(text, o);
		REQUIRE(r.has_value());
		REQUIRE(jt::get(r.value()).is_string());
		return jt::get(r.value()).get_string();
	}

	TEST_CASE("the default reader refuses a lone surrogate escape") {
		for (const char* text : { R"("\ud800")", R"("\udfff")",
				R"("\ud83dA")", R"("\ude00\ud83d")" })
		{
			CAPTURE(text);
			CHECK_FALSE(jt::get(std::string_view(text)).has_value());
			CHECK_FALSE(jt::get(text, read_options{}).has_value());
		}
	}

	TEST_CASE("a lone surrogate gives its WTF-8 bytes") {
		CHECK(str_of(R"("\ud800")", lone) == "\xED\xA0\x80");
		CHECK(str_of(R"("\udfff")", lone) == "\xED\xBF\xBF");
		CHECK(str_of(R"("\ude00\ud83d")", lone)
			== "\xED\xB8\x80\xED\xA0\xBD");
		CHECK(str_of(R"("\ud83dA")", lone) == "\xED\xA0\xBD" "A");
		CHECK(str_of(R"("\ud83d\ude00")", lone) == "\xF0\x9F\x98\x80");
	}

	TEST_CASE("a lone surrogate in a key") {
		auto r = jt::get(R"({"\ud800":1,"\ue000":2,"a":3})", lone);
		REQUIRE(r.has_value());
		const tree& t = jt::get(r.value());
		CHECK(jt::get(t.find("\xED\xA0\x80")).get_integer() == 1);
		CHECK(jt::get(t.find("\xEE\x80\x80")).get_integer() == 2);
		CHECK(jt::get(t.find("a")).get_integer() == 3);
	}

	TEST_CASE("raw surrogate bytes stay invalid") {
		CHECK_FALSE(jt::get("\"\xED\xA0\x80\"", lone).has_value());
	}

}

TEST_SUITE("json tree: the read limits") {
	// The code of the first error of a read that must fail.
	static code failed(std::string_view text, const read_options& o) {
		auto r = jt::get(text, o);
		CHECK_FALSE(r.has_value());
		return r.report().first_error_code();
	}

	TEST_CASE("max_int_digits: at the limit and over it") {
		const auto lax = read_options::lenient(4300, 1000);
		const std::string at(4300, '9'), over(4301, '9');
		auto r = jt::get(at, lax);
		REQUIRE(r.has_value());
		CHECK(jt::get(r.value()).value.str == at);
		CHECK(jt::get("-" + at, lax).has_value());
		CHECK(failed(over, lax) == code::out_of_range);
		CHECK(failed("[1,-" + over + "]", lax) == code::out_of_range);
	}

	TEST_CASE("max_int_digits counts integer texts only") {
		const read_options o{ .max_int_digits = 3 };
		CHECK(jt::get("123", o).has_value());
		CHECK(jt::get("-123", o).has_value());
		CHECK(jt::get("1234.5", o).has_value());
		CHECK(jt::get("1234e0", o).has_value());
		CHECK(failed("1234", o) == code::out_of_range);
		CHECK(failed(R"({"a":-1234})", o) == code::out_of_range);
		const read_options none{ .keep_float_form = true };
		CHECK(jt::get(std::string(5000, '1'), none).has_value());
	}

	TEST_CASE("max_depth: at the limit and over it") {
		const read_options o{ .max_depth = 2 };
		CHECK(jt::get("1", o).has_value());
		CHECK(jt::get("[[1],[]]", o).has_value());
		CHECK(jt::get(R"({"a":{"b":1},"c":[]})", o).has_value());
		CHECK(jt::get(R"([{"a":"[[["}])", o).has_value());
		CHECK(failed("[[[1]]]", o) == code::out_of_range);
		CHECK(failed("[[[]]]", o) == code::out_of_range);
		CHECK(failed(R"({"a":{"b":[1]}})", o) == code::out_of_range);
		CHECK(failed(R"([1,{"a":[{}]}])", o) == code::out_of_range);
	}

	TEST_CASE("max_depth of the lenient options") {
		const auto lax = read_options::lenient(4300, 1000);
		const auto nest = [](size_t n) {
			return std::string(n, '[') + std::string(n, ']');
		};
		CHECK(jt::get(nest(1000), lax).has_value());
		CHECK(failed(nest(1001), lax) == code::out_of_range);
		CHECK(jt::get(nest(1001), read_options{}).has_value());
	}

	TEST_CASE("lenient sets every rule") {
		const auto lax = read_options::lenient(7, 9);
		CHECK(lax.nonfinite);
		CHECK(lax.keep_float_form);
		CHECK(lax.lone_surrogates);
		CHECK(lax.max_int_digits == 7);
		CHECK(lax.max_depth == 9);
		const read_options none{};
		CHECK_FALSE(none.nonfinite);
		CHECK_FALSE(none.keep_float_form);
		CHECK_FALSE(none.lone_surrogates);
		CHECK(none.max_int_digits == 0);
		CHECK(none.max_depth == 0);
	}
}

TEST_SUITE("json tree: the float format repr") {
	// The expected texts come from Python repr and json.dumps.
	TEST_CASE("float_repr gives the text of Python repr") {
		const std::pair<double, const char*> cases[] = {
			{ 1.0, "1.0" }, { 0.0001, "0.0001" },
			{ 100000.0, "100000.0" }, { 1e15, "1000000000000000.0" },
			{ 0.30000000000000004, "0.30000000000000004" },
			{ 1e16, "1e+16" }, { 1e22, "1e+22" }, { 1e-05, "1e-05" },
			{ 1.2345678901234568e+17, "1.2345678901234568e+17" },
			{ 5e-324, "5e-324" },
			{ 1.7976931348623157e+308, "1.7976931348623157e+308" },
			{ -0.0, "-0.0" }, { 0.0, "0.0" }, { 0.1, "0.1" },
			{ 123.456, "123.456" }, { -2.5, "-2.5" }, { 1e-07, "1e-07" },
			{ 1.5e-323, "1.5e-323" },
			{ 12345678901234567.0, "1.2345678901234568e+16" },
			{ 0.00012345, "0.00012345" }, { -1e100, "-1e+100" },
		};
		for (const auto& [d, text] : cases) {
			CAPTURE(text);
			CHECK(float_repr(d) == text);
		}
		CHECK(float_repr(std::numeric_limits<double>::quiet_NaN())
			== "nan");
		CHECK(float_repr(std::numeric_limits<double>::infinity())
			== "inf");
		CHECK(float_repr(-std::numeric_limits<double>::infinity())
			== "-inf");
	}

	TEST_CASE("the profiles print a float as repr and an integer as text") {
		const read_options o{ .keep_float_form = true };
		auto r = jt::get("[1.0,1e16,-0.0,1e3,0.5,7]", o);
		REQUIRE(r.has_value());
		const tree& t = jt::get(r.value());
		CHECK(t.to_str(print_options::normal())
			== "[1.0, 1e+16, -0.0, 1000.0, 0.5, 7]");
		auto p = jt::get(R"({"a":[1.5,2],"b":1e-7})", o);
		REQUIRE(p.has_value());
		CHECK(jt::get(p.value()).to_str(print_options::pretty(2))
			== "{\n  \"a\": [\n    1.5,\n    2\n  ],\n  \"b\": 1e-07\n}");
	}

	TEST_CASE("the default format stays the shortest text") {
		const read_options o{ .keep_float_form = true };
		auto r = jt::get("[1.0,1e16,-0.0,1e3,0.5]", o);
		REQUIRE(r.has_value());
		CHECK(jt::get(r.value()).to_str() == "[1,1e+16,-0,1000,0.5]");
		CHECK(jt::get(r.value()).to_str(print_options::minified())
			== "[1,1e+16,-0,1000,0.5]");
		CHECK(jt::get(parsed("[1.0,-0,0.5,1e2]")).to_str()
			== "[1,0,0.5,100]");
		CHECK(jt::get(parsed("[1.0,-0,0.5]")).to_str(
			print_options::normal()) == "[1, 0, 0.5]");
	}
}

TEST_SUITE("json tree: escape_delete") {
	// The expected bytes come from Python json.dumps.
	TEST_CASE("U+007F is raw with escape_delete off and ascii_only off") {
		const tref t = jt::build_string("a\x7f" "b");
		CHECK(jt::get(t).to_str() == R"("a\u007fb")");
		print_options o;
		o.escape_delete = false;
		CHECK(jt::get(t).to_str(o) == "\"a\x7f" "b\"");
		o.ascii_only = true;
		CHECK(jt::get(t).to_str(o) == R"("a\u007fb")");
		CHECK(jt::get(t).to_str(print_options::normal())
			== R"("a\u007fb")");
	}

	TEST_CASE("a control character stays an escape") {
		const tref t = parsed("{\"k\":\"\\u0000\\u001f\\u007f\\u0080\"}");
		print_options o;
		o.escape_delete = false;
		CHECK(jt::get(t).to_str(o)
			== "{\"k\":\"\\u0000\\u001f\x7f\xc2\x80\"}");
	}
}

TEST_SUITE("json tree: the print field nonfinite") {
	TEST_CASE("the default read makes no such node") {
		CHECK(jt::get(jt::build_number(
			std::numeric_limits<double>::quiet_NaN())).to_str() == "null");
		CHECK_FALSE(jt::get("NaN").has_value());
		CHECK_FALSE(jt::get("1e400").has_value());
	}
}

TEST_SUITE("json tree: the float and integer builders") {
	TEST_CASE("build_float gives float nodes apart from the integers") {
		CHECK(jt::build_float(-0.0) != jt::build_integer(int64_t{ 0 }));
		CHECK(jt::build_float(1.0) != jt::build_integer(int64_t{ 1 }));
		CHECK(jt::build_float(-0.0) != jt::build_float(0.0));
		CHECK(jt::build_float(1.0) == jt::build_float(1.0));
		CHECK(std::signbit(jt::get(jt::build_float(-0.0)).get_number()));
		CHECK(jt::get(jt::build_float(1.0)).value.str.empty());
		CHECK(jt::build_number(1.0) == jt::build_integer(int64_t{ 1 }));
		CHECK(jt::build_number(-0.0) == jt::build_integer(int64_t{ 0 }));
	}

	TEST_CASE("build_float is the node of a float read") {
		const read_options o{ .nonfinite = true, .keep_float_form = true };
		for (const char* text : { "1.0", "-0.0", "1e400", "0.5", "NaN" }) {
			CAPTURE(text);
			auto r = jt::get(text, o);
			REQUIRE(r.has_value());
			CHECK(jt::build_float(jt::get(r.value()).get_number())
				== r.value());
		}
		CHECK(jt::build_float(-std::numeric_limits<double>::quiet_NaN())
			== jt::build_float(std::numeric_limits<double>::quiet_NaN()));
	}

	TEST_CASE("the profiles print a built float as repr") {
		const tref t = jt::build_array({ jt::build_float(1.0),
			jt::build_float(-0.0), jt::build_integer(int64_t{ 1 }) });
		CHECK(jt::get(t).to_str(print_options::normal())
			== "[1.0, -0.0, 1]");
		CHECK(jt::get(t).to_str() == "[1,-0,1]");
	}

	TEST_CASE("build_integer of a text of any size") {
		const std::string big = "-" + std::string(400, '9');
		for (const std::string& text : { std::string("0"),
				std::string("-1"), std::string("9223372036854775807"),
				std::string("18446744073709551616"), big })
		{
			CAPTURE(text);
			auto r = jt::build_integer(std::string_view(text));
			REQUIRE(r.has_value());
			CHECK(jt::get(r.value()).value.str == text);
			CHECK(jt::get(r.value()).to_str() == text);
			auto read = jt::get(text, read_options::lenient(0, 0));
			REQUIRE(read.has_value());
			CHECK(r.value() == read.value());
		}
		CHECK(jt::build_integer(std::string_view("5")).value()
			== jt::build_integer(int64_t{ 5 }));
		CHECK(jt::get(jt::build_integer(std::string_view(big)).value())
			.get_number() == -std::numeric_limits<double>::infinity());
	}

	TEST_CASE("build_integer refuses a text that is no canonical integer") {
		for (const char* text : { "", "-", "+1", "01", "00", "-0", "-01",
				"1.0", "1e3", " 1", "1 ", "0x1", "1a" })
		{
			CAPTURE(text);
			auto r = jt::build_integer(std::string_view(text));
			CHECK_FALSE(r.has_value());
			CHECK(r.report().first_error_code() == code::invalid_argument);
		}
	}
}

TEST_SUITE("json tree: report") {
	using idni::diagnostics::code;
	using idni::diagnostics::report;
	using idni::parser_strings::label;

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

	TEST_CASE("the report prints the expected text") {
		report r = sample();
		CHECK(jt::get(jt::build_report(r, true)).to_str()
			== R"({"nodes":[{"tag":32771,"message":"Count.",)"
			R"("key":"parse","value":120,"attrs":[],"children":[)"
			R"({"tag":32768,"message":"Info.","key":"child",)"
			R"("attrs":[]}]},{"tag":3,)"
			R"("message":"I/O error.","key":"failed","value":7,)"
			R"("attrs":[{"key":"name","value":"tok"},)"
			R"({"key":"exit code","value":-1}]},{"tag":16384,)"
			R"("message":"Warning.","key":"careful","value":0,)"
			R"("attrs":[]}]})");
		CHECK(jt::get(jt::build_report(r, false)).to_str()
			== R"({"nodes":[{"tag":32771,"key":"parse","value":120,)"
			R"("attrs":[],"children":[{"tag":32768,"key":"child",)"
			R"("attrs":[]}]},{"tag":3,"key":"failed",)"
			R"("value":7,"attrs":[{"key":"name","value":"tok"},)"
			R"({"key":"exit code","value":-1}]},{"tag":16384,)"
			R"("key":"careful","value":0,"attrs":[]}]})");
	}

	TEST_CASE("report to JSON and back prints the same text") {
		report r = sample();
		report back = report_of(jt::build_report(r, true));
		std::ostringstream a, b;
		r.print(a);
		back.print(b);
		CHECK(a.str() == b.str());
	}
}

TEST_SUITE("json tree: AST JSON") {
	using p_t = idni::parser<char, char32_t>;
	using lit_t = idni::lit<char, char32_t>;
	using tree_t = p_t::tree;
	using pnode_t = p_t::pnode;

	static idni::nonterminals<char, char32_t> nts({"", "start", "digit"});

	static tref term_node(char32_t c, size_t b, size_t e) {
		return tree_t::get(pnode_t(lit_t(c), {b, e}));
	}

	// The shaped parse tree of a JSON text that must parse.
	static tref parse_tree_of(const std::string& text) {
		json_parser::parse_options po{ .start = json_parser::start };
		auto r = json_parser::instance().parse(text.data(),
			text.size(), po);
		REQUIRE(r.found);
		return r.get_shaped_tree2();
	}

	TEST_CASE("the tree form prints the expected text") {
		tref term = term_node(char32_t('1'), 0, 1);
		tref nul = tree_t::get(pnode_t(lit_t(), {1, 1}));
		tref dig = tree_t::get(pnode_t(nts("digit"), {0, 1}),
			trefs{term, nul});
		tref root = tree_t::get(pnode_t(nts("start"), {0, 1}),
			trefs{dig});
		const std::string start_id = std::to_string(nts("start").n());
		const std::string digit_id = std::to_string(nts("digit").n());
		const std::string text = R"({"format":"ast","version":"1",)"
			R"("start":"start","input":"1","ast":{"symbol":"start",)"
			R"("id":)" + start_id + R"(,"range":[0,1],"children":[)"
			R"({"symbol":"digit","id":)" + digit_id
			+ R"(,"range":[0,1],"children":[{"symbol":"",)"
			R"("range":[0,1],"text":"1"}]}]}})";
		namespace aj = idni::format::ast_json;
		CHECK(jt::get(aj::to_tree<tree_t>(root, "start", "1")).to_str()
			== text);

		tref x = term_node(char32_t('x'), 2, 3);
		tref r = tree_t::get(pnode_t(nts("start"), {2, 3}), trefs{x});
		CHECK(jt::get(aj::node_to_tree<tree_t>(r)).to_str()
			== R"({"symbol":"start","id":)" + start_id
			+ R"(,"range":[2,3],"children":[{"symbol":"",)"
			R"("range":[2,3],"text":"x"}]})");
	}

	TEST_CASE("the AST JSON of a JSON parse tree") {
		namespace aj = idni::format::ast_json;
		for (const char* s : { "1", "[1,2,3]", "[[],[]]",
			R"({"a":[1,{"b":true},null],"c":{"d":[]}})" })
		{
			CAPTURE(s);
			tref p = parse_tree_of(s);
			tref t = aj::node_to_tree<parse_tree>(p);
			CHECK(parsed(jt::get(t).to_str()) == t);
			CHECK((tt(t) | tt::member("symbol") | tt::string)
				== "start");
		}
	}
}

TEST_SUITE("json tree: deep documents and gc") {
	constexpr size_t deep = 1000000;
	// The deepest parse that stays under 10 s in a release build.
	constexpr size_t deep_parse = 150000;

	TEST_CASE("a document 1000000 levels deep builds, prints and is freed") {
		jt::gc();
		const size_t before = jt::m_size();
		{
			tref t = jt::_null();
			for (size_t i = 0; i != deep; ++i)
				t = i % 2 ? jt::build_array({ t })
					: jt::build_object({ jt::build_pair("k", t) });
			htref h = jt::geth(t);
			std::string open, close;
			for (size_t i = deep; i-- != 0; ) {
				open += i % 2 ? "[" : "{\"k\":";
				close += (deep - 1 - i) % 2 ? ']' : '}';
			}
			const std::string text = open + "null" + close;
			CHECK(jt::get(h).to_str() == text);
			jt::gc();
			CHECK(jt::get(h).to_str() == text);
			CHECK(jt::m_size() > deep);
		}
		jt::gc();
		CHECK(jt::m_size() == before);
	}

	TEST_CASE("parse and print deep arrays") {
		const std::string text = std::string(deep_parse, '[')
			+ std::string(deep_parse, ']');
		json_parser::parse_options po{ .start = json_parser::start };
		auto r = json_parser::instance().parse(text.data(),
			text.size(), po);
		REQUIRE(r.found);
		tref p = r.get_shaped_tree2();
		auto t = jt::get(parse_tree::get(p));
		REQUIRE(t.has_value());
		CHECK(jt::get(t.value()).to_str() == text);
		// the AST JSON of the same parse tree
		const std::string ast = jt::get(format::ast_json::
			node_to_tree<parse_tree>(p)).to_str();
		const std::string arr = R"("symbol":"arr")";
		size_t arrays = 0;
		for (size_t at = ast.find(arr); at != std::string::npos;
				at = ast.find(arr, at + arr.size()))
			++arrays;
		CHECK(arrays == deep_parse);
	}

	TEST_CASE("gc_if_grown runs above twice the kept nodes plus 4096") {
		htref held = jt::geth(jt::build_string("held"));
		int64_t next = 1000000000;
		auto grow_to = [&next](size_t n) {
			while (jt::m_size() < n) jt::build_integer(next++);
		};
		// the first run sets the count of the kept nodes
		bool ran = false;
		for (size_t i = 0; i != 100 && !ran; ++i) {
			grow_to(jt::m_size() + 1000);
			ran = jt::gc_if_grown();
		}
		REQUIRE(ran);
		const size_t kept = jt::m_size();
		grow_to(2 * kept + 4096);
		CHECK_FALSE(jt::gc_if_grown());
		CHECK(jt::m_size() == 2 * kept + 4096);
		grow_to(2 * kept + 4097);
		{
			bintree<node>::gc_pause pause;
			CHECK_FALSE(jt::gc_if_grown());
			CHECK(jt::m_size() == 2 * kept + 4097);
		}
		CHECK(jt::gc_if_grown());
		CHECK(jt::m_size() == kept);
		CHECK(jt::get(held).get_string() == "held");
	}

	TEST_CASE("gc after a response keeps the held document") {
		htref state = jt::geth(jt::build_object({
			jt::build_pair("grammar", jt::build_string("g.tgf")),
			jt::build_pair("start", jt::build_string("start")) }));
		const std::string state_text = jt::get(state).to_str();
		jt::gc();
		const size_t held = jt::m_size();
		{
			idni::diagnostics::report r;
			r.error(idni::diagnostics::code::parse_error, "bad");
			tref resp = jt::build_object({
				jt::build_pair("id", jt::build_number(1)),
				jt::build_pair("status", jt::build_string("error")),
				jt::build_pair("state", state->get()),
				jt::build_pair("report", jt::build_report(r)) });
			std::ostringstream os;
			write_line(resp, os);
			CHECK(os.str().ends_with("\n"));
			CHECK(jt::m_size() > held);
		}
		jt::gc();
		CHECK(jt::m_size() == held);
		CHECK(jt::get(state).to_str() == state_text);
	}
}

TEST_SUITE("json tree: nodes") {
	TEST_CASE("equal nodes give one tree, and each field tells nodes apart") {
		tref a = jt::get(node(json_parser::str, 0, "x"));
		CHECK(jt::get(node(json_parser::str, 0, "x")) == a);
		CHECK(jt::get(node(json_parser::str, 0, "y")) != a);
		CHECK(jt::get(node(json_parser::str, 1, "x")) != a);
		CHECK(jt::get(node(json_parser::object_pair, 0, "x")) != a);
		CHECK(jt::get(json_parser::null_sym)
			== jt::get(node(json_parser::null_sym)));
		CHECK(jt::m_size() > 0);
		CHECK(node::name(json_parser::str) == "str");
		CHECK(jt::get(a).value.name() == "str");
	}

	TEST_CASE("the children and the fast access of each kind") {
		tref one = jt::get(node(json_parser::number,
			std::bit_cast<uint64_t>(1.5)));
		tref yes = jt::get(json_parser::true_sym);
		tref arr = jt::get(json_parser::arr, { one, yes });
		const jt& a = jt::get(arr);
		CHECK(a.is_array());
		CHECK(a.get_type() == json_parser::arr);
		size_t n = 0;
		for (const jt& c : a.children_trees())
			n += c.is_number() || c.is_bool();
		CHECK(n == 2);
		// a child with a right sibling is another tree than its value alone
		CHECK(a.first_tree().value == jt::get(one).value);
		CHECK(a.second_tree().get() == yes);
		CHECK(a[1].get() == yes);
		CHECK(a.child_tree(0).is_number());
		CHECK(a.child_tree(0).get_number() == 1.5);
		CHECK(a.second_tree().is_bool());
		CHECK(a.second_tree().get_bool());
		CHECK(jt::get(jt::get(json_parser::false_sym)).is_bool());
		CHECK_FALSE(jt::get(jt::get(json_parser::false_sym)).get_bool());
		CHECK(jt::get(jt::get(json_parser::null_sym)).is_null());

		tref text = jt::get(node(json_parser::str, 0, "hello"));
		tref pair = jt::get(node(json_parser::object_pair, 0, "a"), text);
		tref obj = jt::get(json_parser::object, { pair });
		const jt& o = jt::get(obj);
		CHECK(o.is_object());
		CHECK(o.only_child_tree().get() == pair);
		CHECK(o.only_child_tree().get_string() == "a");
		CHECK(o.only_child_tree().value_tree().get() == text);
		CHECK(jt::get(text).is_string());
		CHECK(jt::get(text).get_string() == "hello");
		CHECK(o.find("a") == text);
		CHECK(o.find("b") == nullptr);
		CHECK(a.find("a") == nullptr);
		CHECK(jt::get(jt::geth(obj)).get() == obj);
	}
}

TEST_SUITE("json tree: traverser") {
	// {"a":[1.5,"x"],"b":true,"s":"t"} from nodes only
	static tref sample() {
		tref num = jt::get(node(json_parser::number,
			std::bit_cast<uint64_t>(1.5)));
		tref x = jt::get(node(json_parser::str, 0, "x"));
		tref arr = jt::get(json_parser::arr, { num, x });
		return jt::get(json_parser::object, {
			jt::get(node(json_parser::object_pair, 0, "a"), arr),
			jt::get(node(json_parser::object_pair, 0, "b"),
				jt::get(json_parser::true_sym)),
			jt::get(node(json_parser::object_pair, 0, "s"),
				jt::get(node(json_parser::str, 0, "t"))) });
	}

	TEST_CASE("a traverser holds values and drops a nullptr") {
		tref obj = sample();
		CHECK_FALSE(tt().has_value());
		CHECK(tt().empty());
		CHECK_FALSE(tt(nullptr));
		CHECK(tt(trefs{ obj, nullptr, obj }).size() == 2);
		CHECK(tt(jt::geth(obj)).value() == obj);
		CHECK(tt(jt::get(obj)).value() == obj);
		CHECK(jt::get(obj)().value() == obj);
		CHECK(tt(obj).is(json_parser::object));
		CHECK(tt(obj).value_tree().is_object());
		CHECK(tt(obj)[2].get_string() == "s");
		CHECK(tt(trefs{ obj, obj }).traversers().size() == 2);
		CHECK(tt(trefs{ obj, obj })().size() == 2);
	}

	TEST_CASE("| and || select children, and the extractors read them") {
		tref obj = sample();
		CHECK((tt(obj) | json_parser::object_pair).value_tree()
			.get_string() == "a");
		CHECK((tt(obj) || json_parser::object_pair).size() == 3);
		CHECK_FALSE((tt(obj) | json_parser::arr).has_value());
		CHECK_FALSE((tt() | json_parser::arr).has_value());
		CHECK((tt(obj) | tt::ref) == obj);
		CHECK((tt() | tt::ref) == nullptr);
		CHECK((tt(obj) | tt::refs).size() == 1);
		CHECK((tt(obj) | tt::handle)->get() == obj);
		CHECK((tt(obj) | tt::Tree).is_object());
		CHECK((tt(obj) | tt::nt) == json_parser::object);
		CHECK((tt(obj) | tt::member("s") | tt::string) == "t");
		CHECK((tt(obj) | tt::member("b") | tt::string).empty());
		CHECK((tt(obj) | tt::member("a") | json_parser::number
			| tt::number) == 1.5);
		CHECK((tt(obj) | tt::member("s") | tt::number) == 0);
		CHECK((tt(obj) | tt::member("b") | tt::boolean));
		CHECK_FALSE((tt(obj) | tt::member("missing")).has_value());
		CHECK_FALSE((tt(obj) | tt::member("missing")
			| tt::member("x")).has_value());
		CHECK((tt(obj) | tt::member("a") | tt::children).size() == 2);
		CHECK((tt(obj) | tt::member("a") | tt::first | tt::number) == 1.5);
		CHECK((tt(obj) | tt::member("a") | tt::second | tt::string)
			== "x");
		CHECK((tt(obj) | json_parser::object_pair | tt::only_child)
			.is(json_parser::arr));
		size_t n = 0;
		for (tref c : tt(obj) | tt::children_range) n += c != nullptr;
		CHECK(n == 3);
		n = 0;
		for (const jt& c : tt(obj) | tt::children_trees_range)
			n += c.is(json_parser::object_pair);
		CHECK(n == 3);
		auto strings = tt::f([](tref p) {
			tref v = jt::get(p).first();
			return jt::get(v).is_string() ? v : nullptr;
		});
		CHECK((tt(obj) | tt::children | strings).size() == 1);
	}
}

TEST_SUITE("json tree: free queries") {
	TEST_CASE("is tests the kind of a node, and nullptr is no kind") {
		tref obj = jt::get(json_parser::object, trefs{});
		CHECK(format::json::is(obj, json_parser::object));
		CHECK_FALSE(format::json::is(obj, json_parser::arr));
		CHECK_FALSE(format::json::is(nullptr, json_parser::object));
		CHECK(format::json::is(json_parser::object)(obj));
		CHECK_FALSE(format::json::is(json_parser::arr)(obj));
		CHECK(format::json::is(obj,
			{ json_parser::arr, json_parser::object }));
		CHECK_FALSE(format::json::is(obj,
			{ json_parser::arr, json_parser::str }));
		auto either = format::json::is(
			{ json_parser::arr, json_parser::object });
		CHECK(either(obj));
		CHECK_FALSE(either(nullptr));
	}

	TEST_CASE("string_member gives the text of a string member only") {
		tref obj = jt::get(json_parser::object, {
			jt::get(node(json_parser::object_pair, 0, "g"),
				jt::get(node(json_parser::str, 0, "g.tgf"))),
			jt::get(node(json_parser::object_pair, 0, "n"),
				jt::get(json_parser::null_sym)) });
		REQUIRE(string_member(obj, "g") != nullptr);
		CHECK(*string_member(obj, "g") == "g.tgf");
		CHECK(string_member(obj, "n") == nullptr);
		CHECK(string_member(obj, "missing") == nullptr);
		CHECK(string_member(nullptr, "g") == nullptr);
	}
}

TEST_SUITE("json tree: builders") {
	TEST_CASE("each builder gives the node of its kind") {
		CHECK(jt::_null() == jt::get(json_parser::null_sym));
		CHECK(jt::build_bool(true) == jt::_true());
		CHECK(jt::build_bool(false) == jt::_false());
		CHECK(jt::get(jt::build_number(1.5)).get_number() == 1.5);
		CHECK(jt::build_number(1.5) == jt::build_number(1.5));
		CHECK(jt::build_number(-0.0) == jt::build_number(0.0));
		CHECK(jt::get(jt::build_string("x")).get_string() == "x");
		tref arr = jt::build_array({ jt::build_string("x"),
			jt::build_number(1.5) });
		CHECK((tt(arr) | tt::first | tt::string) == "x");
		CHECK((tt(arr) | tt::second | tt::number) == 1.5);
		tref pair = jt::build_pair("k", jt::_true());
		CHECK(jt::get(pair).is(json_parser::object_pair));
		CHECK(jt::get(pair).get_string() == "k");
		CHECK(jt::get(pair).value_tree().get() == jt::_true());
	}

	TEST_CASE("a repeated key keeps its first position and its last value") {
		tref obj = jt::build_object({
			jt::build_pair("a", jt::build_number(1)),
			jt::build_pair("b", jt::build_number(2)),
			jt::build_pair("a", jt::build_number(3)) });
		CHECK((tt(obj) || json_parser::object_pair).size() == 2);
		CHECK(jt::get(obj).first_tree().get_string() == "a");
		CHECK((tt(obj) | tt::member("a") | tt::number) == 3);
		CHECK((tt(obj) | tt::member("b") | tt::number) == 2);
	}

	TEST_CASE("set_member changes a member or adds it at the end") {
		tref obj = jt::build_object({
			jt::build_pair("id", jt::build_number(7)),
			jt::build_pair("ok", jt::_true()) });
		tref changed = jt::set_member(obj, "id", jt::build_number(8));
		CHECK((tt(changed) | tt::member("id") | tt::number) == 8);
		CHECK(jt::get(changed).first_tree().get_string() == "id");
		CHECK((tt(obj) | tt::member("id") | tt::number) == 7);
		tref added = jt::set_member(obj, "new", jt::_null());
		CHECK((tt(added) || json_parser::object_pair).size() == 3);
		CHECK(jt::get(added)[2].get_string() == "new");
		CHECK(jt::set_member(obj, "ok", jt::_true()) == obj);
	}
}

TEST_SUITE("json tree: parse") {
	TEST_CASE("a parse gives the tree of each kind") {
		tref t = parsed(R"({"a":[1.5,{"b":true},null],"c":"x","d":false})");
		CHECK(jt::get(t).is_object());
		CHECK((tt(t) | tt::member("a") | tt::first | tt::number) == 1.5);
		CHECK((tt(t) | tt::member("a") | tt::second | tt::member("b")
			| tt::boolean));
		CHECK((tt(t) | tt::member("a") | tt::children).size() == 3);
		CHECK((tt(t) | tt::member("a") | json_parser::null_sym)
			.has_value());
		CHECK(*string_member(t, "c") == "x");
		CHECK_FALSE((tt(t) | tt::member("d") | tt::boolean));
		CHECK(parsed(" \r\n[ 1 , 2 ]\t") == parsed("[1,2]"));
		CHECK(parsed(R"({"a":[1.5,{"b":true},null],"c":"x","d":false})")
			== t);
	}

	TEST_CASE("a parse and the builders give one node") {
		CHECK(parsed("-0") == parsed("0"));
		CHECK(parsed("-0") == jt::build_number(0));
		CHECK(parsed("-1.5e2") == jt::build_number(-150));
		CHECK(parsed(R"({"a":1,"b":2,"a":3})") == jt::build_object({
			jt::build_pair("a", jt::build_number(3)),
			jt::build_pair("b", jt::build_number(2)) }));
		CHECK(parsed(R"("a\nb\"c\\d\/e\t")")
			== jt::build_string("a\nb\"c\\d/e\t"));
		CHECK(parsed(R"("\u00e9\ud83d\ude00")")
			== jt::build_string("\xc3\xa9\xf0\x9f\x98\x80"));
	}

	TEST_CASE("the tree of a parse result and of a parse tree") {
		json_parser::parse_options po{ .start = json_parser::start };
		const std::string text = R"([true,"s"])";
		auto r = json_parser::instance().parse(text.data(), text.size(),
			po);
		REQUIRE(r.found);
		auto from_tree = jt::get(parse_tree::get(r.get_shaped_tree2()));
		REQUIRE(from_tree.has_value());
		CHECK(from_tree.value() == parsed(text));
		auto from_result = jt::get(r);
		REQUIRE(from_result.has_value());
		CHECK(from_result.value() == parsed(text));
		const std::string bad = "[true,";
		auto f = json_parser::instance().parse(bad.data(), bad.size(), po);
		auto failed = jt::get(f);
		CHECK_FALSE(failed.has_value());
		CHECK(failed.report().first_error_code()
			== idni::diagnostics::code::parse_error);
	}
}

TEST_SUITE("json tree: printers") {
	TEST_CASE("the stream operators, write_line and the node dump") {
		tref t = parsed(R"({"a":[1.5,"x"]})");
		std::ostringstream os;
		os << jt::get(t);
		CHECK(os.str() == jt::get(t).to_str());
		std::ostringstream line;
		write_line(t, line);
		CHECK(line.str() == R"({"a":[1.5,"x"]})" "\n");
		std::ostringstream nodes;
		nodes << node(json_parser::number, std::bit_cast<uint64_t>(1.5))
			<< '|' << node(json_parser::str, 0, "x\"y")
			<< '|' << node(json_parser::null_sym);
		CHECK(nodes.str() == R"(number 1.5|str "x\"y"|null_sym)");
		std::ostringstream two;
		two << (tt(t) | tt::member("a") | tt::children);
		CHECK(two.str() == "1.5\n\"x\"");
		std::ostringstream none;
		none << tt();
		CHECK(none.str() == "(no value)");
		CHECK_FALSE(jt::get(t).dump_to_str().empty());
		std::ostringstream tree_text;
		jt::get(t).print_tree(tree_text);
		CHECK_FALSE(tree_text.str().empty());
		CHECK((tt() | tt::dump).empty());
	}
}

TEST_SUITE("json tree: print layout") {
	// The expected bytes are the bytes of Python json.dumps.
	TEST_CASE("the separators and the indent set the layout") {
		tref t = parsed(R"({"a": [1, 2], "b": {}})");
		CHECK(jt::get(t).to_str({ .item_separator = ", ",
			.key_separator = ": " }) == R"({"a": [1, 2], "b": {}})");
		CHECK(jt::get(t).to_str({ .key_separator = ": ", .indent = 2 })
			== "{\n  \"a\": [\n    1,\n    2\n  ],\n  \"b\": {}\n}");
		CHECK(jt::get(t).to_str({ .indent = 4 })
			== "{\n    \"a\":[\n        1,\n        2\n    ],\n"
			"    \"b\":{}\n}");
		CHECK(jt::get(t).to_str(print_options{}) == R"({"a":[1,2],"b":{}})");
	}

	TEST_CASE("an indent keeps an empty array or object on one line") {
		const print_options o{ .key_separator = ": ", .indent = 2 };
		CHECK(jt::get(parsed(R"({"x":[[],{}],"z":{"e":[]}})")).to_str(o)
			== "{\n  \"x\": [\n    [],\n    {}\n  ],\n  \"z\": {\n"
			"    \"e\": []\n  }\n}");
		CHECK(jt::get(parsed("[]")).to_str(o) == "[]");
		CHECK(jt::get(parsed("{}")).to_str(o) == "{}");
		CHECK(jt::get(parsed("1")).to_str(o) == "1");
	}
}

TEST_SUITE("json tree: the float format of the profiles") {
	// The expected texts come from Python json.dumps.
	TEST_CASE("normal and pretty print a float as repr") {
		tref t = jt::build_array({ jt::build_number(0.0001),
			jt::build_number(1.5), jt::build_integer(2) });
		CHECK(jt::get(t).to_str(print_options::normal())
			== "[0.0001, 1.5, 2]");
		CHECK(jt::get(t).to_str(print_options::pretty(2))
			== "[\n  0.0001,\n  1.5,\n  2\n]");
		CHECK(jt::get(t).to_str({ .floats =
			print_options::float_format::repr }) == "[0.0001,1.5,2]");
		CHECK(jt::get(t).to_str() == "[1e-04,1.5,2]");
	}
}

TEST_SUITE("json tree: the print field nonfinite of a parse") {
	// The tree of a text that the nonfinite grammar parses.
	static tref nonfinite_tree(std::string_view text) {
		json_parser::parse_options po{ .start = json_parser::start };
		auto r = idni::format::json::detail::nonfinite_parser().parse(
			text.data(), text.size(), po);
		auto t = jt::get(r);
		REQUIRE(t.has_value());
		return t.value();
	}

	// The expected bytes come from Python json.dumps.
	TEST_CASE("on: NaN, Infinity and -Infinity") {
		const tref t = nonfinite_tree("[NaN,Infinity,-Infinity,1.5]");
		auto o = print_options::normal();
		o.nonfinite = true;
		CHECK(jt::get(t).to_str(o) == "[NaN, Infinity, -Infinity, 1.5]");
		print_options m;
		m.nonfinite = true;
		CHECK(jt::get(t).to_str(m) == "[NaN,Infinity,-Infinity,1.5]");
	}

	TEST_CASE("off: the print writes null") {
		const tref t = nonfinite_tree(R"({"a":NaN,"b":[-Infinity],"c":1})");
		CHECK(jt::get(t).to_str() == R"({"a":null,"b":[null],"c":1})");
		CHECK(jt::get(t).to_str(print_options::normal())
			== R"({"a": null, "b": [null], "c": 1})");
		for (const auto& o : { print_options{}, print_options::minified(),
				print_options::normal(), print_options::pretty(2) })
			CHECK_FALSE(o.nonfinite);
	}
}
