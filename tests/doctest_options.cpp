// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the options repository: declare, typed access, sources and
// precedence, text access and the environment.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdlib>
#include <string>
#include <vector>

#include "../src/utility/options.h"

using namespace std;
using namespace idni;

namespace {

// Every error and warning of a report, one per line.
string messages_of(const diagnostics::report& rep) {
	string out;
	for (size_t i = 0; i < rep.nodes().size(); ++i)
		if (auto m = rep.format_message(i); !m.empty()) out += m + "\n";
	return out;
}

bool contains(const string& text, const string& part) {
	return text.find(part) != string::npos;
}

option_set tool_set() {
	return option_set{
		.options = {
			{ "help", "cli", false, "show the help" },
			{ "verbose", "cli", false, "print more" },
			{ "count", "run", size_t{ 1 }, "how many times" },
			{ "name", "run", string("x"), "a name" },
			{ "tags", "run", option_list{}, "the tags" },
			{ "force", "run", false, "force it" },
			{ "quiet", "run", false, "print less" },
			{ "input", "show", string(""), "the input", false },
		},
	};
}

// A repository with the tool set and three sources, lowest first.
struct fixture {
	options_repository repo;
	size_t env     = repo.source("env");
	size_t cli     = repo.source("command-line");
	size_t session = repo.source("session");
	fixture() { REQUIRE(repo.declare(tool_set()).has_value()); }
};

} // namespace

TEST_SUITE("options declare") {

TEST_CASE("declare gives the fallback") {
	fixture f;
	CHECK(f.repo.get<bool>("help") == false);
	CHECK(f.repo.get<size_t>("count") == 1u);
	CHECK(f.repo.get<string>("name") == "x");
	CHECK(f.repo.get<option_list>("tags").empty());
	CHECK(f.repo.names() == vector<string>{ "help", "verbose", "count",
		"name", "tags", "force", "quiet", "input" });
}

TEST_CASE("a second declare of a name is an error") {
	fixture f;
	auto same = f.repo.declare(option_set{ .options = {
		{ "count", "run", size_t{ 2 }, "", {} } } });
	CHECK(same.has_error());
	CHECK(contains(messages_of(same.report()), "Option already exists"));
	auto other = f.repo.declare(option_set{ .options = {
		{ "count", "run", false, "", {} } } });
	CHECK(other.has_error());
	CHECK(f.repo.get<size_t>("count") == 1u);
	CHECK(f.repo.names().size() == 8);
}

TEST_CASE("a name twice in one set declares nothing") {
	options_repository repo;
	auto r = repo.declare(option_set{ .options = {
		{ "a", "", false, "", {} },
		{ "b", "", false, "", {} },
		{ "a", "", false, "", {} } } });
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "Option already exists"));
	CHECK(repo.names().empty());
}

} // TEST_SUITE

TEST_SUITE("options values") {

TEST_CASE("a source registered later wins") {
	fixture f;
	f.repo.set("count", size_t{ 5 }, f.env);
	f.repo.set("count", size_t{ 7 }, f.session);
	CHECK(f.repo.get<size_t>("count") == 7u);
	CHECK(f.repo.is_set("count", f.env));
	CHECK(!f.repo.is_set("count", f.cli));
}

TEST_CASE("clear falls back to the next lower source") {
	fixture f;
	f.repo.set("count", size_t{ 5 }, f.env);
	f.repo.set("count", size_t{ 6 }, f.cli);
	f.repo.set("count", size_t{ 7 }, f.session);
	f.repo.clear("count", f.session);
	CHECK(f.repo.get<size_t>("count") == 6u);
	f.repo.clear("count", f.cli);
	CHECK(f.repo.get<size_t>("count") == 5u);
	f.repo.clear("count", f.env);
	CHECK(f.repo.get<size_t>("count") == 1u);
}

TEST_CASE("set takes a literal and an int") {
	fixture f;
	f.repo.set("name", "abc", f.session);
	f.repo.set("count", 3, f.session);
	CHECK(f.repo.get<string>("name") == "abc");
	CHECK(f.repo.get<size_t>("count") == 3u);
}

TEST_CASE("source is idempotent and numbered in order") {
	options_repository repo;
	CHECK(repo.source("a") == 0);
	CHECK(repo.source("b") == 1);
	CHECK(repo.source("a") == 0);
}

TEST_CASE("version grows only when a value changes") {
	fixture f;
	const size_t v0 = f.repo.version();
	f.repo.set("count", size_t{ 4 }, f.session);
	const size_t v1 = f.repo.version();
	CHECK(v1 > v0);
	f.repo.set("count", size_t{ 4 }, f.session);
	CHECK(f.repo.version() == v1);
	REQUIRE(f.repo.set_text("count", "4", f.session).has_value());
	CHECK(f.repo.version() == v1);
	f.repo.clear("count", f.cli);
	CHECK(f.repo.version() == v1);
	f.repo.clear("count", f.session);
	CHECK(f.repo.version() > v1);
}

} // TEST_SUITE

TEST_SUITE("options text") {

TEST_CASE("set_text and get_text use the kind") {
	fixture f;
	REQUIRE(f.repo.set_text("force", "On", f.session).has_value());
	CHECK(f.repo.get<bool>("force"));
	REQUIRE(f.repo.set_text("tags", "a, b ,, c > d", f.session)
		.has_value());
	CHECK(f.repo.get<option_list>("tags")
		== option_list{ "a", "b", "c > d" });
	auto text = f.repo.get_text("tags");
	REQUIRE(text.has_value());
	CHECK(text.value() == "a, b, c > d");
}

TEST_CASE("set_text with a bad value is an error") {
	fixture f;
	auto r = f.repo.set_text("count", "4x", f.session);
	CHECK(r.has_error());
	const string msg = messages_of(r.report());
	CHECK(contains(msg, "count"));
	CHECK(contains(msg, "4x"));
	CHECK(f.repo.get<size_t>("count") == 1u);
	CHECK(f.repo.set_text("force", "maybe", f.session).has_error());
	CHECK(f.repo.set_text("count", "", f.session).has_error());
}

TEST_CASE("an unknown name is an error") {
	fixture f;
	CHECK(f.repo.set_text("nope", "1", f.session).has_error());
	CHECK(f.repo.get_text("nope").has_error());
}

TEST_CASE("load_env reads the variables and warns on a bad value") {
	fixture f;
	setenv("OPTTEST_FORCE", "OFF", 1);
	setenv("OPTTEST_COUNT", "x", 1);
	setenv("OPTTEST_INPUT", "file", 1);
	f.repo.set("force", true, f.cli);
	auto r = f.repo.load_env("OPTTEST_", f.env);
	unsetenv("OPTTEST_FORCE");
	unsetenv("OPTTEST_COUNT");
	unsetenv("OPTTEST_INPUT");
	CHECK(r.has_value());
	CHECK(!r.has_error());
	const string msg = messages_of(r.report());
	CHECK(contains(msg, "OPTTEST_COUNT"));
	CHECK(contains(msg, "x"));
	CHECK(f.repo.is_set("force", f.env));
	CHECK(f.repo.get<bool>("force"));
	f.repo.clear("force", f.cli);
	CHECK(!f.repo.get<bool>("force"));
	CHECK(f.repo.get<size_t>("count") == 1u);
	CHECK(!f.repo.is_set("input", f.env));
}

} // TEST_SUITE
