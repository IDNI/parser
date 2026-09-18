// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Unit tests for the diagnostics facility (namespace idni::diagnostics).
//
// Focused on the ergonomics surface: closing a scope_guard early, the
// measure() sugar, a null pointer as a legitimate value in result<T*>,
// forward_as<T> for a type result<T>(T) cannot construct, take_or_error(),
// and the expected-style chain: and_then(), transform(), or_else(), value_or().

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "utility/diagnostics.h"

#include <string>
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

TEST_SUITE("diagnostics: null pointer is a value") {

	TEST_CASE("operator= with a null pointer stores it as a value") {
		result<int*> r;
		r = static_cast<int*>(nullptr);
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
		CHECK(static_cast<int*>(r) == nullptr);
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

	TEST_CASE("a null pointer result is rejected like emplace rejects one") {
		result<int> r;
		r = 5;
		auto r2 = std::move(r).transform(
			[](int) -> int* { return nullptr; });
		CHECK_FALSE(r2.has_value());
		CHECK(r2.has_error());
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
		r.print(coloured.as_sinks(), idni::term::colors(true));
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
		r.print(on.as_sinks());
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
}
