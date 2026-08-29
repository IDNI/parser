// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Unit tests for the diagnostics facility (namespace idni::diagnostics).
//
// Focused on the ergonomics surface: closing a scope_guard early, the
// measure() sugar, null-pointer rejection on result<T*>, forward_as<T> for
// a type result<T>(T) cannot construct, and take_or_error().

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "utility/diagnostics.h"

#include <string>

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

TEST_SUITE("diagnostics: null pointer rejection") {

	TEST_CASE("operator= with a null pointer records an error, not a value") {
		result<int*> r;
		r = static_cast<int*>(nullptr);
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}

	TEST_CASE("emplace with a null pointer records an error, not a value") {
		result<int*> r;
		r.emplace(static_cast<int*>(nullptr));
		CHECK_FALSE(r.has_value());
		CHECK(r.has_error());
	}

	TEST_CASE("operator= with a non-null pointer still succeeds") {
		int x = 1;
		result<int*> r;
		r = &x;
		CHECK(r.has_value());
		CHECK_FALSE(r.has_error());
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
