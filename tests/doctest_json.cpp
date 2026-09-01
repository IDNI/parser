// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "format/json/json.h"

#include <sstream>

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
