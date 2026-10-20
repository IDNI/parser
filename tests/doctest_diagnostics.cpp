// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Unit tests for the diagnostics facility (namespace idni::diagnostics).
//
// Focused on the ergonomics surface: closing a scope_guard early, the
// measure() sugar, a null pointer as a legitimate value in result<T*>,
// forward_as<T>, a string value against a named result, take_or_error(),
// and the expected-style chain: and_then(), transform(), or_else(), value_or().

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "debug_skip.h"
#include "utility/diagnostics.h"
#include "utility/program_codes.h"

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <sstream>

using namespace idni::diagnostics;

TEST_SUITE("diagnostics: scope_guard::close()") {

	TEST_CASE("close() finalizes the scope before destruction") {
		report r;
		auto g = r.open("work");
		g.close();
		CHECK(r.nodes().size() == 1);
	}

	TEST_CASE("close() is idempotent") {
		report r;
		auto g = r.open("work");
		g.close();
		g.close();
		CHECK(r.nodes().size() == 1);
	}

	TEST_CASE("closing early lets a sibling scope open at the same depth") {
		report r;
		{
			auto g = r.open("first");
			g.close();
		}
		auto g2 = r.open("second");
		g2.close();
		CHECK(r.nodes().size() == 2);
		CHECK(r.nodes()[0].parent == -1);
		CHECK(r.nodes()[1].parent == -1);
	}
}

TEST_SUITE("diagnostics: measure()") {

	TEST_CASE("report::measure returns the callable's result") {
		report r;
		int v = r.measure("work", [] { return 42; });
		CHECK(v == 42);
	}

	TEST_CASE("report::measure opens and closes a timed scope") {
		report r;
		r.measure("work", [] {});
		CHECK(r.nodes().size() == 1);
		CHECK(r.nodes()[0].tag == code::info_micros);
	}

	TEST_CASE("result<T>::measure mirrors report::measure") {
		result<int> res;
		int v = res.measure("work", [] { return 7; });
		CHECK(v == 7);
		CHECK(res.report().nodes().size() == 1);
	}
}

TEST_SUITE("diagnostics: first error") {

	TEST_CASE("the first error, not a note before it or a later error") {
		report r;
		r.info("Opened the store");
		r.warning("The cache is cold");
		r.error(code::io_error, "Port is in use");
		r.error(code::parse_error, "Bad header");
		CHECK(r.first_error_message() == "Port is in use");
		CHECK(r.first_error_code() == code::io_error);
		const auto i = r.first_error_index();
		REQUIRE(i.has_value());
		CHECK(r.str(r.nodes()[*i].key) == "Port is in use");
		CHECK(r.nodes()[*i].tag == code::io_error);
	}

	TEST_CASE("format_message of the first error index holds the attributes") {
		using idni::parser_strings::label;
		report r;
		r.warning("Port is in use", { { label::port, size_t{ 1 } } });
		r.error(code::io_error, "Port is in use",
			{ { label::port, size_t{ 41137 } } });
		r.error(code::io_error, "Bad header");
		const auto i = r.first_error_index();
		REQUIRE(i.has_value());
		CHECK(r.nodes()[*i].tag == code::io_error);
		CHECK(r.format_message(*i) == "Port is in use (port=41137)");
	}

	TEST_CASE("a report without an error gives no message and unknown") {
		report r;
		CHECK_FALSE(r.first_error_index().has_value());
		CHECK(r.first_error_message().empty());
		CHECK(r.first_error_code() == code::unknown);
		r.info("Opened the store");
		r.warning("The cache is cold");
		CHECK(r.first_error_message() == "");
		CHECK(r.first_error_code() == code::unknown);
	}

	TEST_CASE("a demoted error is no first error") {
		report r;
		r.error(code::io_error, "Port is in use");
		r.demote_errors_to_warnings();
		CHECK_FALSE(r.first_error_index().has_value());
		CHECK(r.first_error_message().empty());
		CHECK(r.first_error_code() == code::unknown);
	}
}

TEST_SUITE("diagnostics: null pointer is a value") {

	TEST_CASE("operator= with a null pointer stores it as a value") {
		result<int*> r;
		r = static_cast<int*>(nullptr);
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
		CHECK(r.value() == nullptr);
	}

	TEST_CASE("emplace with a null pointer stores it as a value") {
		result<int*> r;
		r.emplace(static_cast<int*>(nullptr));
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
	}

	TEST_CASE("operator= with a non-null pointer still succeeds") {
		int x = 1;
		result<int*> r;
		r = &x;
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
	}

	TEST_CASE("has_value() is true for a held null pointer, unlike a value-less result") {
		result<int*> with_null;
		with_null = static_cast<int*>(nullptr);
		result<int*> without_value;
		CHECK(with_null.has_value());
		CHECK_FALSE(without_value.has_value());
	}
}

TEST_SUITE("diagnostics: forward_as") {

	TEST_CASE("forward_as<std::string> succeeds when src has a value") {
		result<int> src;
		src = 1;
		auto r = forward_as<std::string>(std::move(src), std::string("ok"));
		CHECK(r.has_value());
		CHECK(r.value() == "ok");
	}

	TEST_CASE("forward_as<std::string> fails when src has an error") {
		result<int> src;
		src.error(code::internal_error, "boom");
		auto r = forward_as<std::string>(std::move(src), std::string("ok"));
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}
}

TEST_SUITE("diagnostics: a string value and a named result") {

	TEST_CASE("every text argument of a constructor is a value") {
		result<std::string> from_string(std::string("x"));
		REQUIRE(from_string.has_value());
		CHECK(from_string.value() == "x");
		result<std::string> from_literal("x");
		REQUIRE(from_literal.has_value());
		CHECK(from_literal.value() == "x");
		const std::string_view view = "x";
		result<std::string> from_view(view);
		REQUIRE(from_view.has_value());
		CHECK(from_view.value() == "x");
		CHECK(from_view.report().nodes().empty());
	}

	TEST_CASE("a named result keeps its name and holds no value") {
		auto r = result<std::string>::named("load");
		CHECK_FALSE(r.has_value());
		CHECK_FALSE(r.has_error());
		REQUIRE_FALSE(r.report().nodes().empty());
		CHECK(r.report().str(r.report().nodes()[0].key) == "load");
		r = std::string("x");
		CHECK(r.value() == "x");
		CHECK(r.report().str(r.report().nodes()[0].key) == "load");
	}

	TEST_CASE("no text type selects a name") {
		static_assert(!std::is_constructible_v<result<int>,
			std::string_view>);
		static_assert(!std::is_constructible_v<result<int>,
			const char*>);
		static_assert(std::is_constructible_v<result<std::string>,
			std::string_view>);
		static_assert(std::is_constructible_v<result<std::string>,
			const char*>);
		static_assert(!std::is_convertible_v<std::string,
			result<std::string>>);
		CHECK(true);
	}
}

TEST_SUITE("diagnostics: take_or_error") {

	TEST_CASE("well-formed child with a value is taken and merged") {
		result<int> outer;
		result<int> child;
		child = 7;
		auto v = outer.take_or_error(std::move(child),
			code::internal_error, "should not fire");
		CHECK(v.has_value());
		CHECK(*v == 7);
		CHECK_FALSE(outer.has_error());
	}

	TEST_CASE("well-formed child with an error propagates it, no synthesis") {
		result<int> outer;
		result<int> child;
		child.error(code::parse_error, "bad token");
		auto v = outer.take_or_error(std::move(child),
			code::internal_error, "should not fire");
		CHECK_FALSE(v.has_value());
		CHECK(outer.has_error());
		CHECK(outer.report().nodes().back().tag == code::parse_error);
	}

	TEST_CASE("malformed child synthesizes the given error") {
		result<int> outer;
		result<int> child; // unassigned: no value, no error
		auto v = outer.take_or_error(std::move(child),
			code::internal_error, "child failed silently");
		CHECK_FALSE(v.has_value());
		CHECK(outer.has_error());
		CHECK(outer.report().nodes().back().tag == code::internal_error);
	}
}

TEST_SUITE("diagnostics: and_then") {

	TEST_CASE("on value, calls f and keeps this result's report before the child's") {
		result<int> r;
		r.report().count("before", 1);
		r = 3;
		auto r2 = std::move(r).and_then([](int v) {
			result<int> out;
			out = v + 1;
			out.report().count("child", 1);
			return out;
		});
		CHECK(r2.has_value());
		CHECK(*r2 == 4);
		CHECK(r2.report().nodes().size() == 2);
		CHECK(r2.report().str(r2.report().nodes()[0].key) == "before");
		CHECK(r2.report().str(r2.report().nodes()[1].key) == "child");
	}

	TEST_CASE("on error, f is not called and the report carries the error") {
		result<int> r;
		r.error(code::internal_error, "boom");
		bool called = false;
		auto r2 = std::move(r).and_then([&](int v) {
			called = true;
			result<int> out;
			out = v + 1;
			return out;
		});
		CHECK_FALSE(called);
		CHECK_FALSE(r2.has_value());
		CHECK(r2.has_error());
		CHECK(r2.report().nodes().back().tag == code::internal_error);
	}
}

TEST_SUITE("diagnostics: transform") {

	TEST_CASE("on value, wraps f(value) with the moved report") {
		result<int> r;
		r.report().count("before", 1);
		r = 3;
		auto r2 = std::move(r).transform([](int v) { return v * 2; });
		CHECK(r2.has_value());
		CHECK(*r2 == 6);
		CHECK(r2.report().nodes().size() == 1);
		CHECK(r2.report().str(r2.report().nodes()[0].key) == "before");
	}

	TEST_CASE("on error, f is not called and the report carries the error") {
		result<int> r;
		r.error(code::internal_error, "boom");
		bool called = false;
		auto r2 = std::move(r).transform([&](int v) { called = true; return v; });
		CHECK_FALSE(called);
		CHECK_FALSE(r2.has_value());
		CHECK(r2.has_error());
	}

	TEST_CASE("a null pointer from f is stored as a value, not an error") {
		result<int> r;
		r = 5;
		auto r2 = std::move(r).transform(
			[](int) -> int* { return nullptr; });
		CHECK(r2.has_value());
		CHECK_FALSE(r2.has_error());
		CHECK(r2.value() == nullptr);
	}
}

TEST_SUITE("diagnostics: or_else") {

	TEST_CASE("on value, returns this result unchanged and f is not called") {
		result<int> r;
		r = 9;
		bool called = false;
		auto r2 = std::move(r).or_else([&](const report&) {
			called = true;
			result<int> alt;
			alt = -1;
			return alt;
		});
		CHECK_FALSE(called);
		CHECK(r2.has_value());
		CHECK(*r2 == 9);
	}

	TEST_CASE("on successful recovery, the original error is demoted to a "
		"warning so the recovered result reads as ok")
	{
		result<int> r;
		r.error(code::internal_error, "boom");
		auto r2 = std::move(r).or_else([](const report&) {
			result<int> alt;
			alt = 7;
			alt.report().count("recovered", 1);
			return alt;
		});
		CHECK(r2.has_value());
		CHECK_FALSE(r2.has_error());
		CHECK(*r2 == 7);
		CHECK(r2.report().nodes().size() == 2);
		CHECK(r2.report().nodes()[0].tag == code::warning);
		CHECK(r2.report().str(r2.report().nodes()[0].key) == "boom");
		CHECK(r2.report().nodes()[1].tag == code::info_count);
	}

	TEST_CASE("on a failed alternative, the original error is left as is "
		"and the chain stays failed")
	{
		result<int> r;
		r.error(code::internal_error, "boom");
		auto r2 = std::move(r).or_else([](const report&) {
			result<int> alt;
			alt.error(code::parse_error, "still bad");
			return alt;
		});
		CHECK_FALSE(r2.has_value());
		CHECK(r2.has_error());
		CHECK(r2.report().nodes().size() == 2);
		CHECK(r2.report().nodes()[0].tag == code::internal_error);
		CHECK(r2.report().nodes()[1].tag == code::parse_error);
	}
}

TEST_SUITE("diagnostics: value_or") {

	TEST_CASE("on value, returns the value and drops the report") {
		result<int> r;
		r = 4;
		CHECK(std::move(r).value_or(-1) == 4);
	}

	TEST_CASE("on error, returns the fallback") {
		result<int> r;
		r.error(code::internal_error, "boom");
		CHECK(std::move(r).value_or(-1) == -1);
	}
}

// Mirrors the recovery a REPL evaluator does for an incomplete-input status
// code: a child report built from a failed parse still needs to reach the
// caller, but the status code (a value) must survive the append.
TEST_SUITE("diagnostics: append() after demote_errors_to_warnings()") {

	TEST_CASE("append of an error report drops the value") {
		result<int> res(2);
		report child;
		child.error(code::parse_error, "unexpected end of input");
		res.append(std::move(child));
		CHECK_FALSE(res.has_value());
		CHECK(res.has_error());
	}

	TEST_CASE("demoting the child report before append keeps the value") {
		result<int> res(2);
		report child;
		child.error(code::parse_error, "unexpected end of input");
		child.demote_errors_to_warnings();
		res.append(std::move(child));
		CHECK(res.has_value());
		CHECK_FALSE(res.has_error());
		CHECK(std::move(res).value_or(-1) == 2);
	}
}

// Proves with_error/with_value compile in the shape they exist for: a
// `return` on the very line that calls them, from a function returning
// result<T> by value (result's copy ctor is deleted).
static result<int> fails_with_error() {
	result<int> r;
	return r.with_error(code::internal_error, "boom", 7);
}

static result<int> fails_after_merge() {
	result<int> r;
	result<int> child;
	child.report().count("child-metric", 1);
	r.merge(std::move(child));
	return r.with_error(code::solver_error, "backend failed");
}

static result<int> succeeds_with_value() {
	result<int> r;
	return r.with_value(5);
}

static result<int> succeeds_with_value_after_merge() {
	result<int> r;
	result<int> child;
	child.report().count("child-metric", 1);
	r.merge(std::move(child));
	return r.with_value(5);
}

TEST_SUITE("diagnostics: with_error") {

	TEST_CASE("with_error sets the same error as error") {
		result<int> a;
		a.error(code::internal_error, "boom", 7);
		result<int> b = fails_with_error();
		CHECK(a.report().nodes().size() == b.report().nodes().size());
		CHECK(a.report().nodes().back().tag == b.report().nodes().back().tag);
		CHECK(a.report().nodes().back().value == b.report().nodes().back().value);
		CHECK(a.report().str(a.report().nodes().back().key) ==
			b.report().str(b.report().nodes().back().key));
	}

	TEST_CASE("with_error returns from a by-value result<T> function") {
		result<int> r = fails_with_error();
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
		CHECK(r.report().nodes().back().tag == code::internal_error);
	}

	TEST_CASE("a report merged before with_error survives the call") {
		result<int> r = fails_after_merge();
		CHECK(r.report().nodes().size() == 2);
		CHECK(r.report().str(r.report().nodes()[0].key) == "child-metric");
		CHECK(r.report().nodes().back().tag == code::solver_error);
		CHECK(r.report().str(r.report().nodes().back().key) ==
			"backend failed");
	}
}

TEST_SUITE("diagnostics: with_value") {

	TEST_CASE("with_value sets the same value as operator=") {
		result<int> a;
		a = 5;
		result<int> b = succeeds_with_value();
		CHECK(a.has_value());
		CHECK(b.has_value());
		CHECK(*a == *b);
	}

	TEST_CASE("with_value returns from a by-value result<T> function") {
		result<int> r = succeeds_with_value();
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
		CHECK(*r == 5);
	}

	TEST_CASE("a report merged before with_value survives the call") {
		result<int> r = succeeds_with_value_after_merge();
		CHECK(r.has_value());
		CHECK(*r == 5);
		CHECK(r.report().nodes().size() == 1);
		CHECK(r.report().str(r.report().nodes()[0].key) == "child-metric");
	}
}

static result<int> fails_with_assert_check_error() {
	result<int> r;
	return r.with_assert_check_error(code::internal_error, "boom", 7);
}

static result<int> fails_with_assert_check_error_after_merge() {
	result<int> r;
	result<int> child;
	child.report().count("child-metric", 1);
	r.merge(std::move(child));
	return r.with_assert_check_error(code::solver_error, "backend failed");
}

static result<int> succeeds_with_assert_check_value() {
	result<int> r;
	return r.with_assert_check_value(5);
}

static result<int> succeeds_with_assert_check_value_after_merge() {
	result<int> r;
	result<int> child;
	child.report().count("child-metric", 1);
	r.merge(std::move(child));
	return r.with_assert_check_value(5);
}

TEST_SUITE("diagnostics: with_assert_check_error") {

	TEST_CASE("with_assert_check_error sets the same error as error") {
		result<int> a;
		a.error(code::internal_error, "boom", 7);
		result<int> b = fails_with_assert_check_error();
		CHECK(a.report().nodes().size() == b.report().nodes().size());
		CHECK(a.report().nodes().back().tag == b.report().nodes().back().tag);
		CHECK(a.report().nodes().back().value == b.report().nodes().back().value);
		CHECK(a.report().str(a.report().nodes().back().key) ==
			b.report().str(b.report().nodes().back().key));
	}

	TEST_CASE("with_assert_check_error returns from a by-value result<T> function") {
		result<int> r = fails_with_assert_check_error();
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
		CHECK(r.report().nodes().back().tag == code::internal_error);
	}

	TEST_CASE("a report merged before with_assert_check_error survives the call") {
		result<int> r = fails_with_assert_check_error_after_merge();
		CHECK(r.report().nodes().size() == 2);
		CHECK(r.report().str(r.report().nodes()[0].key) == "child-metric");
		CHECK(r.report().nodes().back().tag == code::solver_error);
		CHECK(r.report().str(r.report().nodes().back().key) ==
			"backend failed");
	}
}

TEST_SUITE("diagnostics: with_assert_check_value") {

	TEST_CASE("with_assert_check_value sets the same value as operator=") {
		result<int> a;
		a = 5;
		result<int> b = succeeds_with_assert_check_value();
		CHECK(a.has_value());
		CHECK(b.has_value());
		CHECK(*a == *b);
	}

	TEST_CASE("with_assert_check_value returns from a by-value result<T> function") {
		result<int> r = succeeds_with_assert_check_value();
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
		CHECK(*r == 5);
	}

	TEST_CASE("a report merged before with_assert_check_value survives the call") {
		result<int> r = succeeds_with_assert_check_value_after_merge();
		CHECK(r.has_value());
		CHECK(*r == 5);
		CHECK(r.report().nodes().size() == 1);
		CHECK(r.report().str(r.report().nodes()[0].key) == "child-metric");
	}
}

TEST_SUITE("diagnostics: chained and_then().transform()") {

	TEST_CASE("a two-step chain applies both steps and keeps report order") {
		result<int> r;
		r = 2;
		auto r2 = std::move(r)
			.and_then([](int v) {
				result<int> out;
				out = v * 2;
				out.report().count("doubled", 1);
				return out;
			})
			.transform([](int v) { return v + 1; });
		CHECK(r2.has_value());
		CHECK(*r2 == 5);
		CHECK(r2.report().nodes().size() == 1);
		CHECK(r2.report().str(r2.report().nodes()[0].key) == "doubled");
	}
}

// Colour is a parameter of rendering, not a property of the message: the
// bands are wrapped at print() time, so a caller wanting data (a language
// binding filling structured fields, a log file) asks for it plain without
// disturbing anyone else's output.
TEST_SUITE("diagnostics: print() colouring") {

	// Collect the three bands from one print() call.
	struct bands {
		std::vector<std::string> errors, warnings, infos;
		sinks as_sinks() {
			return {
				.error   = [this](std::string_view l) {
					errors.emplace_back(l); },
				.warning = [this](std::string_view l) {
					warnings.emplace_back(l); },
				.info    = [this](std::string_view l) {
					infos.emplace_back(l); },
			};
		}
	};

	static bool has_ansi(const std::string& s) {
		return s.find("\x1b[") != std::string::npos;
	}

	TEST_CASE("an explicit disabled colour set renders plain") {
		report r;
		r.error(code::parse_error, "boom");
		bands b;
		r.print(b.as_sinks(), idni::term::colors(false));
		REQUIRE(b.errors.size() == 1);
		CHECK(!has_ansi(b.errors[0]));
		CHECK(b.errors[0].find("boom") != std::string::npos);
	}

	TEST_CASE("an explicit enabled colour set wraps the message") {
		report r;
		r.error(code::parse_error, "boom");
		bands b;
		r.print(b.as_sinks(), idni::term::colors(true));
		REQUIRE(b.errors.size() == 1);
		CHECK(has_ansi(b.errors[0]));
		CHECK(b.errors[0].find("boom") != std::string::npos);
	}

	TEST_CASE("warnings and info scopes honour the colour set too") {
		report r;
		r.warning("careful");
		{ auto g = r.open("work"); }
		bands plain, coloured;
		r.print(plain.as_sinks(), idni::term::colors(false));
		r.reprint(coloured.as_sinks(), idni::term::colors(true));
		REQUIRE(plain.warnings.size() == 1);
		REQUIRE(plain.infos.size() == 1);
		CHECK(!has_ansi(plain.warnings[0]));
		CHECK(!has_ansi(plain.infos[0]));
		CHECK(has_ansi(coloured.warnings[0]));
		CHECK(has_ansi(coloured.infos[0]));
	}

	TEST_CASE("the global switch drives print() when no colour set is given") {
		report r;
		r.error(code::parse_error, "boom");
		const bool saved = idni::TC.enabled;

		idni::TC.disable();
		bands off;
		r.print(off.as_sinks());
		REQUIRE(off.errors.size() == 1);
		CHECK(!has_ansi(off.errors[0]));

		idni::TC.enable();
		bands on;
		r.reprint(on.as_sinks());
		REQUIRE(on.errors.size() == 1);
		CHECK(has_ansi(on.errors[0]));

		idni::TC.set(saved);
	}

	TEST_CASE("operator<< follows the global switch") {
		report r;
		r.error(code::parse_error, "boom");
		const bool saved = idni::TC.enabled;

		idni::TC.disable();
		std::ostringstream off; off << r;
		CHECK(!has_ansi(off.str()));

		idni::TC.enable();
		std::ostringstream on; on << r;
		CHECK(has_ansi(on.str()));

		idni::TC.set(saved);
	}

	TEST_CASE("an explicit colour set does not disturb the global one") {
		report r;
		r.error(code::parse_error, "boom");
		const bool saved = idni::TC.enabled;
		idni::TC.enable();

		bands b;
		r.print(b.as_sinks(), idni::term::colors(false));
		CHECK(idni::TC.enabled);          // still on for everyone else
		CHECK(!has_ansi(b.errors[0]));    // but this render stayed plain

		idni::TC.set(saved);
	}

	TEST_CASE("print_pending writes the errors and warnings once") {
		report r;
		r.error(code::parse_error, "boom");
		r.warning("careful");
		r.info("note");
		const idni::term::colors plain(false);

		bands first;
		r.print_pending(first.as_sinks(), plain);
		REQUIRE(first.errors.size() == 1);
		REQUIRE(first.warnings.size() == 1);
		CHECK(first.errors[0].find("boom") != std::string::npos);
		CHECK(first.warnings[0].find("careful") != std::string::npos);
		CHECK(first.infos.empty());

		bands second;
		r.print_pending(second.as_sinks(), plain);
		CHECK(second.errors.empty());
		CHECK(second.warnings.empty());
		CHECK(second.infos.empty());

		bands third;
		r.print(third.as_sinks(), plain);
		CHECK(third.errors.empty());
		CHECK(third.warnings.empty());
		CHECK(third.infos.size() == 1);

		bands all;
		r.reprint(all.as_sinks(), plain);
		CHECK(all.errors.size() == 1);
		CHECK(all.warnings.size() == 1);
		CHECK(all.infos.size() == 1);
	}

	TEST_CASE("print_pending skips a printed warning of an appended child") {
		const idni::term::colors plain(false);
		report child;
		child.warning("from child");
		bands c;
		child.print_pending(c.as_sinks(), plain);
		REQUIRE(c.warnings.size() == 1);

		report parent;
		parent.append(child);
		parent.warning("from parent");
		bands p;
		parent.print_pending(p.as_sinks(), plain);
		REQUIRE(p.warnings.size() == 1);
		CHECK(p.warnings[0].find("from parent") != std::string::npos);
	}

	TEST_CASE("print_pending runs while a scope is open") {
		report r;
		auto g = r.open("work");
		r.warning("inside");
		bands b;
		r.print_pending(b.as_sinks(), idni::term::colors(false));
		CHECK(b.warnings.size() == 1);
	}

	TEST_CASE("demote_errors_to_warnings keeps the printed mark") {
		report r;
		r.error(code::parse_error, "boom");
		bands first;
		r.print_pending(first.as_sinks(), idni::term::colors(false));
		REQUIRE(first.errors.size() == 1);
		r.demote_errors_to_warnings();
		bands second;
		r.print_pending(second.as_sinks(), idni::term::colors(false));
		CHECK(second.errors.empty());
		CHECK(second.warnings.empty());
	}

	TEST_CASE("result<T> forwards print_pending and reprint") {
		result<int> res;
		res.warning("careful");
		bands first;
		res.print_pending(first.as_sinks());
		CHECK(first.warnings.size() == 1);
		bands second;
		res.print_pending(second.as_sinks());
		CHECK(second.warnings.empty());
		bands again;
		res.reprint(again.as_sinks());
		CHECK(again.warnings.size() == 1);
	}
}

TEST_SUITE("diagnostics: append() remaps a text attribute's value") {

	TEST_CASE("a merged attribute keeps its own text, not the destination's") {
		using label = idni::parser_strings::label;
		report dst;
		dst.error(code::internal_error, "boom");

		report src;
		src.error(code::io_error, "failed",
			{{label::path, std::string_view("src-path-value")}});

		dst.append(std::move(src));

		REQUIRE(dst.nodes().size() == 2);
		CHECK(dst.format_message(1).find("src-path-value")
			!= std::string::npos);
	}

	TEST_CASE("a merged numeric attribute keeps its negative number") {
		using label = idni::parser_strings::label;
		report dst;
		dst.error(code::internal_error, "boom");

		report src;
		src.error(code::io_error, "failed", {{label::exit_code, -1}});

		dst.append(std::move(src));

		REQUIRE(dst.nodes().size() == 2);
		CHECK(dst.format_message(1).find("exit code=-1")
			!= std::string::npos);
	}
}

TEST_SUITE("diagnostics: an empty text under a text label") {

	TEST_CASE("an empty value is interned and rendered") {
		using label = idni::parser_strings::label;
		report r;
		r.error(code::invalid_argument, "bad value",
			{{label::name, std::string_view("count")},
			 {label::value, std::string_view("")}});
		REQUIRE(r.nodes().size() == 1);
		const std::string msg = r.format_message(0);
		CHECK(msg.find("name=count") != std::string::npos);
		CHECK(msg.find("value=)") != std::string::npos);
		std::ostringstream os;
		r.print(os, idni::term::colors(false));
		CHECK(os.str().find("value=)") != std::string::npos);
	}

	TEST_CASE("an empty value survives append()") {
		using label = idni::parser_strings::label;
		report dst;
		dst.error(code::internal_error, "boom");
		report src;
		src.error(code::invalid_argument, "bad value",
			{{label::value, std::string_view("")}});
		dst.append(std::move(src));
		REQUIRE(dst.nodes().size() == 2);
		CHECK(dst.format_message(1).find("value=)") != std::string::npos);
	}
}

TEST_SUITE("diagnostics: resolve() lets the label decide, not text.data()") {

	TEST_CASE("a number under a text label renders the missing-text marker, "
		"not a mismatched label name" * doctest::skip(
			idni::testing::skip_under_debug_assert)) {
		using label = idni::parser_strings::label;
		report r;
		r.error(code::io_error, "failed", {{label::path, label::root}});

		CHECK(r.format_message(0).find("root") == std::string::npos);
		CHECK(r.format_message(0).find("<missing text>")
			!= std::string::npos);
	}

	TEST_CASE("a string under a numeric label renders as 0, "
		"not an interned key" * doctest::skip(
			idni::testing::skip_under_debug_assert)) {
		using label = idni::parser_strings::label;
		report r;
		r.error(code::io_error, "failed",
			{{label::exit_code, std::string_view("stray-text")}});

		CHECK(r.format_message(0).find("exit code=0") != std::string::npos);
	}

	TEST_CASE("a pre-interned key under label::name renders the "
		"missing-text marker, in a release build with no DBG assert" *
		doctest::skip(idni::testing::skip_under_debug_assert)) {
		using label = idni::parser_strings::label;
		report r;
		r.error(code::io_error, "failed", {{label::name, label::root}});

		CHECK(r.format_message(0).find("<missing text>")
			!= std::string::npos);
	}
}

// result<void> is the shape for a function that only succeeds or fails:
// success is the absence of an error, so there is no value to set and no
// third "neither value nor error" state.
static result<void> void_succeeds() {
	result<void> r;
	return r.with_value();
}

static result<void> void_fails() {
	result<void> r;
	return r.with_error(code::internal_error, "boom", 7);
}

static result<void> void_fails_with_assert_check() {
	result<void> r;
	return r.with_assert_check_error(code::solver_error, "backend failed");
}

TEST_SUITE("diagnostics: result<void>") {

	TEST_CASE("a fresh result<void> is a success") {
		result<void> r;
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
		CHECK(r.is_well_formed());
	}

	TEST_CASE("error() makes it fail") {
		result<void> r;
		r.error(code::parse_error, "bad token");
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
		CHECK(r.report().nodes().back().tag == code::parse_error);
	}

	TEST_CASE("with_error returns from a by-value result<void> function") {
		result<void> r = void_fails();
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
		CHECK(r.report().nodes().back().tag == code::internal_error);
		CHECK(r.report().nodes().back().value == 7);
	}

	TEST_CASE("with_assert_check_error returns from a by-value function") {
		result<void> r = void_fails_with_assert_check();
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
		CHECK(r.report().nodes().back().tag == code::solver_error);
	}

	TEST_CASE("with_value returns from a by-value result<void> function") {
		result<void> r = void_succeeds();
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
	}

	TEST_CASE("with_value does not clear an error already held") {
		result<void> r;
		r.error(code::internal_error, "boom");
		result<void> out = r.with_value();
		CHECK_FALSE(out.has_value());
		CHECK(out.has_error());
	}

	TEST_CASE("merge of a failed void child fails a void result") {
		result<void> outer;
		result<void> child;
		child.error(code::parse_error, "child failed");
		outer.merge(std::move(child));
		CHECK_FALSE(outer.has_value());
		CHECK(outer.has_error());
		CHECK(outer.report().nodes().back().tag == code::parse_error);
	}

	TEST_CASE("operator<< chains merges into a result<void>") {
		result<void> outer;
		result<void> a, b;
		a.report().count("a", 1);
		b.report().count("b", 1);
		outer << std::move(a) << std::move(b);
		CHECK(outer.has_value());
		CHECK(outer.report().nodes().size() == 2);
	}

	TEST_CASE("a result<void> merges a result<int> child") {
		result<void> outer;
		result<int> child;
		child = 7;
		child.report().count("child-metric", 1);
		outer.merge(std::move(child));
		CHECK(outer.has_value());
		CHECK(outer.report().nodes().size() == 1);
		CHECK(outer.report().str(outer.report().nodes()[0].key)
			== "child-metric");
	}

	TEST_CASE("a result<int> merges a void child and keeps its value") {
		result<int> outer;
		outer = 5;
		result<void> child;
		child.report().count("child-metric", 1);
		outer.merge(std::move(child));
		CHECK(outer.has_value());
		CHECK_FALSE(outer.has_error());
		CHECK(*outer == 5);
		CHECK(outer.report().nodes().size() == 1);
	}

	TEST_CASE("a failed void child drops a result<int>'s value") {
		result<int> outer;
		outer = 5;
		result<void> child;
		child.error(code::solver_error, "child failed");
		outer.merge(std::move(child));
		CHECK_FALSE(outer.has_value());
		CHECK(outer.has_error());
		CHECK(outer.report().nodes().back().tag == code::solver_error);
	}
}

TEST_SUITE("diagnostics: merge_ok") {

	TEST_CASE("a void child that succeeded reports true and merges") {
		result<void> outer;
		result<void> child;
		child.report().count("child-metric", 1);
		CHECK(outer.merge_ok(std::move(child)));
		CHECK(outer.has_value());
		CHECK(outer.report().nodes().size() == 1);
	}

	TEST_CASE("a failed void child reports false and still merges") {
		result<void> outer;
		result<void> child;
		child.error(code::internal_error, "child failed");
		CHECK_FALSE(outer.merge_ok(std::move(child)));
		CHECK(outer.has_error());
		CHECK(outer.report().nodes().back().tag == code::internal_error);
	}

	TEST_CASE("a result<int> merge_ok a void child and keeps its value") {
		result<int> outer;
		outer = 5;
		result<void> child;
		child.report().count("child-metric", 1);
		CHECK(outer.merge_ok(std::move(child)));
		CHECK(outer.has_value());
		CHECK(*outer == 5);
		CHECK(outer.report().nodes().size() == 1);
	}

	TEST_CASE("a result<int> merge_ok failure drops its value") {
		result<int> outer;
		outer = 5;
		result<void> child;
		child.error(code::solver_error, "child failed");
		CHECK_FALSE(outer.merge_ok(std::move(child)));
		CHECK_FALSE(outer.has_value());
		CHECK(outer.has_error());
	}
}

// merge_take's constraint is on the child's value type, so a result<void>
// parent can still take a value out of a result<int> child.
TEST_SUITE("diagnostics: a result<void> takes a child's value") {

	TEST_CASE("merge_take takes the child's value or its error") {
		result<void> outer;
		result<int> ok;
		ok = 7;
		auto v = outer.merge_take(std::move(ok));
		CHECK(v == 7);
		CHECK(outer.has_value());

		result<void> failed;
		result<int> bad;
		bad.error(code::solver_error, "child failed");
		auto none = failed.merge_take(std::move(bad));
		CHECK(none == std::nullopt);
		CHECK_FALSE(failed.has_value());
		CHECK(failed.report().nodes().back().tag == code::solver_error);
	}
}

TEST_SUITE("diagnostics: an info message prints as a message line") {

	using label = idni::parser_strings::label;

	// The lines of one print() call on the info sink, with no colour.
	static std::vector<std::string> info_lines(const report& r) {
		std::vector<std::string> lines;
		r.print({ .error = {}, .warning = {},
			.info = [&lines](std::string_view l) {
				lines.emplace_back(l); } }, idni::term::colors(false));
		return lines;
	}

	TEST_CASE("an info node with an attribute prints its text and the attribute") {
		report r;
		r.info("Grammar reloaded", {{label::path, std::string_view("g.tgf")}});
		CHECK(info_lines(r) == std::vector<std::string>{
			"Grammar reloaded (path=g.tgf)" });
		CHECK(r.format_message(0) == "Grammar reloaded (path=g.tgf)");
	}

	TEST_CASE("a program info node prints as a message line") {
		report r;
		r.info(program_info(1), "fee paid",
			{{label::name, std::string_view("alice")}});
		r.info(program_info(2), "fee", size_t{ 12 });
		CHECK(info_lines(r) == std::vector<std::string>{
			"fee paid (name=alice)", "fee (value=12)" });
	}

	TEST_CASE("a counter prints as a counter row beside a message") {
		report r;
		r.count("parse", size_t{ 120 });
		r.info("Grammar reloaded", {{label::path, std::string_view("g.tgf")}});
		CHECK(info_lines(r) == std::vector<std::string>{
			"parse:    120", "Grammar reloaded (path=g.tgf)" });
		CHECK(r.format_message(0).empty());
	}

	TEST_CASE("is_info_message tells a message from a metric") {
		CHECK(is_info_message(code::info));
		CHECK(is_info_message(program_info(0)));
		CHECK(is_info_message(static_cast<code>(0x8010)));
		CHECK_FALSE(is_info_message(code::info_count));
		CHECK_FALSE(is_info_message(code::info_micros));
		CHECK_FALSE(is_info_message(code::info_scope));
		CHECK_FALSE(is_info_message(code::warning));
	}
}

TEST_SUITE("diagnostics: a deep report prints") {

	TEST_CASE("a chain of 1000000 nested errors prints each line in order") {
		constexpr size_t deep = 1000000;
		report r;
		const auto k = r.intern("e");
		for (size_t i = 0; i != deep; ++i)
			r.push_node(code::io_error, k, static_cast<int64_t>(i));
		for (size_t i = 0; i != deep; ++i) r.pop_node();
		size_t lines = 0;
		bool ordered = true;
		r.print({ .error = [&](std::string_view l) {
				ordered = ordered && l.starts_with("e")
					&& r.nodes()[lines].value
						== static_cast<int64_t>(lines);
				++lines; },
			.warning = {}, .info = {} }, idni::term::colors(false));
		CHECK(lines == deep);
		CHECK(ordered);
	}

	TEST_CASE("children print after their parent, in order") {
		report r;
		{
			auto a = r.open("a", code::info_count, size_t(1));
			{ auto b = r.open("b", code::info_count, size_t(2)); }
			{ auto c = r.open("c", code::info_count, size_t(3)); }
		}
		{ auto d = r.open("d", code::info_count, size_t(4)); }
		std::vector<std::string> lines;
		r.print({ .error = {}, .warning = {},
			.info = [&lines](std::string_view l) {
				lines.emplace_back(l); } }, idni::term::colors(false));
		REQUIRE(lines.size() == 4);
		CHECK(lines[0].starts_with("a:"));
		CHECK(lines[1].starts_with("\tb:"));
		CHECK(lines[2].starts_with("\tc:"));
		CHECK(lines[3].starts_with("d:"));
	}
}

TEST_SUITE("diagnostics: a warning and an info take a code") {

	TEST_CASE("the node carries the given code, the message and the value") {
		report r;
		r.warning(code::warning, "x");
		r.info(code::info_count, "rules", size_t{ 3 });
		REQUIRE(r.nodes().size() == 2);
		CHECK(r.nodes()[0].tag == code::warning);
		CHECK(r.str(r.nodes()[0].key) == "x");
		CHECK(r.nodes()[1].tag == code::info_count);
		CHECK(r.str(r.nodes()[1].key) == "rules");
		CHECK(r.nodes()[1].value == 3);
		CHECK_FALSE(r.has_error());
	}
}
