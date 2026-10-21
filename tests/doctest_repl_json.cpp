// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "utility/repl_json.h"
#include "testing.h"

#include <sstream>
#include <string>

using namespace idni;

using jt = idni::format::json::tree;

static std::string text(tref t) { return jt::get(t).to_str(); }

static const std::string& empty_report() {
	static const std::string s =
		text(jt::build_report(diagnostics::report{}, true));
	return s;
}

static diagnostics::report one_error() {
	diagnostics::report r;
	r.error(diagnostics::code::invalid_argument, "bad");
	return r;
}

TEST_SUITE("repl json: responses") {
	TEST_CASE("cmd_status_name gives the wire names") {
		CHECK(std::string(cmd_status_name(cmd_status::ok)) == "ok");
		CHECK(std::string(cmd_status_name(cmd_status::error)) == "error");
		CHECK(std::string(cmd_status_name(cmd_status::incomplete))
			== "incomplete");
		CHECK(std::string(cmd_status_name(cmd_status::quit)) == "quit");
	}

	TEST_CASE("json_cmd_value") {
		cmd_result r;
		r.cmd = "parse";
		CHECK(text(json_cmd_value(r)) == "{\"cmd\":\"parse\","
			"\"status\":\"ok\",\"result\":{},\"report\":"
			+ empty_report() + "}");
	}

	TEST_CASE("json_eval_response") {
		eval_result er;
		er.status = cmd_status::error;
		cmd_result r;
		r.cmd = "get";
		r.status = cmd_status::error;
		er.results.push_back(r);
		const tref state = jt::build_object({});
		CHECK(text(json_eval_response(jt::build_integer(1), er, state))
			== "{\"id\":1,\"status\":\"error\",\"results\":["
			"{\"cmd\":\"get\",\"status\":\"error\",\"result\":{},"
			"\"report\":" + empty_report() + "}],\"state\":{},"
			"\"report\":" + empty_report() + "}");
	}

	TEST_CASE("json_result_response with and without id") {
		const tref state = jt::build_object({});
		const std::string tail = "\"cmd\":\"load\",\"status\":\"ok\","
			"\"result\":true,\"state\":{},\"report\":"
			+ empty_report() + "}";
		CHECK(text(json_result_response(cmd_status::ok, "load",
			jt::_true(), state, diagnostics::report{}))
			== "{" + tail);
		CHECK(text(json_result_response(cmd_status::ok, "load",
			jt::_true(), state, diagnostics::report{},
			jt::build_string("a"))) == "{\"id\":\"a\"," + tail);
	}

	TEST_CASE("json_status_response and json_error_response") {
		const tref state = jt::build_object({});
		const auto rep = one_error();
		const std::string report = text(jt::build_report(rep, true));
		CHECK(text(json_status_response(jt::_null(),
			cmd_status::incomplete, state, rep))
			== "{\"id\":null,\"status\":\"incomplete\",\"state\":{},"
			"\"report\":" + report + "}");
		CHECK(text(json_error_response(jt::_null(), state, rep))
			== "{\"id\":null,\"status\":\"error\",\"state\":{},"
			"\"report\":" + report + "}");
	}

	TEST_CASE("json_hello_line and json_hello_response") {
		const tref hello = jt::build_object({
			jt::build_pair(repl_json_keys::protocol,
				jt::build_integer(1)) });
		const tref state = jt::build_object({});
		CHECK(text(json_hello_line(hello, state))
			== "{\"hello\":{\"protocol\":1},\"state\":{}}");
		CHECK(text(json_hello_response(jt::build_integer(7), hello,
			state)) == "{\"id\":7,\"cmd\":\"hello\",\"status\":\"ok\","
			"\"hello\":{\"protocol\":1},\"state\":{}}");
	}
}

TEST_SUITE("repl json: line reader") {
	TEST_CASE("lines, a kept CR and the end of input") {
		std::istringstream in("a\r\n\nlast");
		std::string line;
		bool too_long = true;
		REQUIRE(json_read_line(in, line, 16, too_long));
		CHECK(line == "a\r");
		CHECK_FALSE(too_long);
		REQUIRE(json_read_line(in, line, 16, too_long));
		CHECK(line.empty());
		REQUIRE(json_read_line(in, line, 16, too_long));
		CHECK(line == "last");
		CHECK_FALSE(json_read_line(in, line, 16, too_long));
	}

	TEST_CASE("a long line keeps the limit and drops its tail") {
		std::istringstream in("abcdefgh\nok\n");
		std::string line;
		bool too_long = false;
		REQUIRE(json_read_line(in, line, 3, too_long));
		CHECK(too_long);
		CHECK(line == "abc");
		REQUIRE(json_read_line(in, line, 3, too_long));
		CHECK_FALSE(too_long);
		CHECK(line == "ok");
		CHECK_FALSE(json_read_line(in, line, 3, too_long));
	}

	TEST_CASE("a long last line without a newline") {
		std::istringstream in("abcdef");
		std::string line;
		bool too_long = false;
		REQUIRE(json_read_line(in, line, 2, too_long));
		CHECK(too_long);
		CHECK_FALSE(json_read_line(in, line, 2, too_long));
	}
}

TEST_SUITE("repl json: wire_line") {
	TEST_CASE("a line of only whitespace has no value") {
		for (std::string_view s : { "", "\r", "  \t" }) {
			auto r = json_parse_line(s);
			REQUIRE(r.has_value());
			CHECK_FALSE(r.report().has_error());
			CHECK(r.value() == nullptr);
		}
	}

	TEST_CASE("a line of only whitespace has one parse") {
		// The auto disambiguation hides a second parse, so it is off.
		auto& g = json_parser_data::grammar();
		const bool saved = g.opt().auto_disambiguate;
		set_grammar_option(g, "auto-disambiguate", false);
		json_parser::parse_options po{ .start = json_parser::wire_line };
		for (auto path : { parse_tree_path::bintree_path,
				parse_tree_path::forest_path }) {
			po.tree_path = path;
			for (std::string_view s : { " ", "  \t", " \r\n " }) {
				CAPTURE(s);
				json_parser p;
				auto r = p.parse(s.data(), s.size(), po);
				CHECK(r.found);
				CHECK_FALSE(r.is_ambiguous());
			}
		}
		set_grammar_option(g, "auto-disambiguate", saved);
	}

	TEST_CASE("a value with a CR at the end") {
		auto r = json_parse_line("{\"cmd\":\"x\"}\r");
		REQUIRE(r.has_value());
		REQUIRE(r.value() != nullptr);
		CHECK(text(r.value()) == "{\"cmd\":\"x\"}");
	}

	TEST_CASE("a syntax error gives no value") {
		auto r = json_parse_line("{\"cmd\":");
		CHECK_FALSE(r.has_value());
		CHECK(r.report().has_error());
	}

	TEST_CASE("the general start stays strict") {
		CHECK_FALSE(jt::get(std::string_view("")).has_value());
		CHECK_FALSE(jt::get(std::string_view("  ")).has_value());
	}
}
