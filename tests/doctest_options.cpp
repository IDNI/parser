// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the options repository: declare, typed access, last write wins,
// bound fields, the current repository, text access, the environment and the
// command line.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <charconv>
#include <cstdlib>
#include <initializer_list>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
#	define OPTIONS_TEST_THREADS 1
#	include <thread>
#endif

#include "../src/utility/options.h"
#include "test_env.h"

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

// A writable argv that lives as long as the object.
struct argv_of {
	vector<string> store;
	vector<char*> ptrs;
	argv_of(initializer_list<const char*> args)
		: store(args.begin(), args.end())
	{
		for (auto& s : store) ptrs.push_back(s.data());
	}
	int argc() const { return static_cast<int>(ptrs.size()); }
	char** argv() { return ptrs.data(); }
};

option_set tool_set() {
	return option_set{
		.options = {
			{ "help", "cli", false, "show the help",
				{ { "", 'h' } } },
			{ "verbose", "cli", false, "print more",
				{ { "", 'v' } } },
			{ "count", "run", size_t{ 1 }, "how many times",
				{ { "run", 'n' } } },
			{ "name", "run", string("x"), "a name",
				{ { "run", 's', string("none") } } },
			{ "tags", "run", option_list{}, "the tags",
				{ { "run", 't' } } },
			{ "force", "run", false, "force it",
				{ { "run", 'f' }, { "show", 'f' } } },
			{ "quiet", "run", false, "print less",
				{ { "run", 'q' } } },
			{ "input", "show", string(""), "the input",
				{ { "show" } }, false },
		},
		.commands = {
			{ "run", "run the tool" },
			{ "show", "show the state" },
		},
	};
}

// A repository with the tool set.
struct fixture {
	options_repository repo;
	fixture() { REQUIRE(repo.declare(tool_set()).has_value()); }
};

// A set of numbers in a list option.
struct int_set_codec {
	option_value to_value(const set<int>& field) const {
		option_list out;
		for (int x : field) out.push_back(to_string(x));
		return out;
	}
	diagnostics::result<set<int>> from_value(const option_value& v) const {
		diagnostics::result<set<int>> r;
		set<int> out;
		if (const auto* list = get_if<option_list>(&v))
			for (const auto& s : *list) {
				int x = 0;
				const char* end = s.data() + s.size();
				auto [p, ec] = from_chars(s.data(), end, x);
				if (ec != errc{} || p != end) return r.with_error(
					diagnostics::code::invalid_argument,
					"not a number");
				out.insert(x);
			}
		return r.with_value(std::move(out));
	}
};

// An "ids" list and a "flag" on the global surface.
option_set ids_set() {
	return option_set{ .options = {
		{ "ids", "test", option_list{}, "", { { "" } } },
		{ "flag", "test", false, "", { { "" } } } } };
}

args_config tool_config() {
	args_config cfg;
	cfg.name = "tool";
	cfg.description = "a test tool";
	return cfg;
}

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

TEST_CASE("find gives a declared option by name") {
	fixture f;
	auto count = f.repo.find("count");
	REQUIRE(count != nullptr);
	CHECK(count->name == "count");
	CHECK(count->owner == "run");
	CHECK(count->fallback == option_value(size_t{ 1 }));
	CHECK(f.repo.find("no-such-option") == nullptr);
	// a later declare keeps the spec valid
	REQUIRE(f.repo.declare(option_set{ .options = {
		{ "extra", "run", false, "", {} } } }).has_value());
	CHECK(count->help == "how many times");
	CHECK(f.repo.find("extra") != nullptr);
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
		{ "a", "", true, "", {} } } });
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "Option already exists"));
	CHECK(repo.names().empty());
}

TEST_CASE("a command twice is an error") {
	options_repository repo;
	auto r = repo.declare(option_set{ .commands = {
		{ "run", "" }, { "run", "other" } } });
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "Command already exists"));
}

TEST_CASE("declare takes an equal spec again") {
	fixture f;
	f.repo.set("count", 4);
	REQUIRE(f.repo.declare(tool_set()).has_value());
	CHECK(f.repo.names().size() == 8);
	CHECK(f.repo.get<size_t>("count") == 4u);
	auto other = f.repo.declare(option_set{ .options = {
		{ "count", "run", size_t{ 1 }, "another help", {} } } });
	CHECK(other.has_error());
}

TEST_CASE("two options with one short letter on one command") {
	options_repository repo;
	auto r = repo.declare(option_set{ .options = {
		{ "alpha", "", false, "", { { "run", 'x' } } },
		{ "beta", "", false, "", { { "run", 'x' } } } } });
	CHECK(r.has_error());
	const string msg = messages_of(r.report());
	CHECK(contains(msg, "alpha"));
	CHECK(contains(msg, "beta"));
	CHECK(repo.names().empty());
	// the same letter on two commands is fine
	auto ok = repo.declare(option_set{ .options = {
		{ "alpha", "", false, "", { { "run", 'x' } } },
		{ "beta", "", false, "", { { "show", 'x' } } } } });
	CHECK(ok.has_value());
	// a later set is checked against the declared options too
	auto late = repo.declare(option_set{ .options = {
		{ "gamma", "", false, "", { { "run", 'x' } } } } });
	CHECK(late.has_error());
}

} // TEST_SUITE

TEST_SUITE("options kind") {

TEST_CASE("kind_of gives the kind of a value and of a spec") {
	CHECK(kind_of(option_value(true)) == option_kind::flag);
	CHECK(kind_of(option_value(size_t{ 1 })) == option_kind::numeric);
	CHECK(kind_of(option_value(string("x"))) == option_kind::text);
	CHECK(kind_of(option_value(option_list{})) == option_kind::list);
	for (const auto& spec : tool_set().options)
		CHECK(kind_of(spec) == kind_of(spec.fallback));
}

} // TEST_SUITE

TEST_SUITE("options values") {

TEST_CASE("set takes a literal and an int") {
	fixture f;
	f.repo.set("name", "abc");
	f.repo.set("count", 3);
	CHECK(f.repo.get<string>("name") == "abc");
	CHECK(f.repo.get<size_t>("count") == 3u);
}

TEST_CASE("the last write wins") {
	fixture f;
	f.repo.set("count", size_t{ 5 });
	CHECK(f.repo.get<size_t>("count") == 5u);
	REQUIRE(f.repo.set_text("count", "6").has_value());
	CHECK(f.repo.get<size_t>("count") == 6u);
	test_setenv("OPTTEST_COUNT", "7");
	auto env = f.repo.load_env("OPTTEST_");
	test_unsetenv("OPTTEST_COUNT");
	REQUIRE(env.has_value());
	CHECK(f.repo.get<size_t>("count") == 7u);
	argv_of a{ "tool", "run", "--count", "8" };
	REQUIRE(f.repo.parse_args(a.argc(), a.argv(), tool_config())
		.has_value());
	CHECK(f.repo.get<size_t>("count") == 8u);
	// an earlier kind of write after a later one wins too
	f.repo.set("count", size_t{ 9 });
	CHECK(f.repo.get<size_t>("count") == 9u);
	REQUIRE(f.repo.set_text("count", "4").has_value());
	CHECK(f.repo.get<size_t>("count") == 4u);
}

} // TEST_SUITE

TEST_SUITE("options bind") {

TEST_CASE("a bound option lives in its field") {
	fixture f;
	bool force = false;
	size_t count = 0;
	REQUIRE(f.repo.bind("force", force).has_value());
	REQUIRE(f.repo.bind("count", count).has_value());
	// with no stored value the field keeps what it holds
	CHECK(f.repo.get<size_t>("count") == 0u);
	f.repo.set("count", 5);
	CHECK(count == 5u);
	count = 6;
	CHECK(f.repo.get<size_t>("count") == 6u);
	REQUIRE(f.repo.set_text("force", "on").has_value());
	CHECK(force);
	argv_of a{ "tool", "run", "--count", "7", "--force", "false" };
	REQUIRE(f.repo.parse_args(a.argc(), a.argv(), tool_config())
		.has_value());
	CHECK(count == 7u);
	CHECK(!force);
	test_setenv("OPTTEST_COUNT", "8");
	auto env = f.repo.load_env("OPTTEST_");
	test_unsetenv("OPTTEST_COUNT");
	REQUIRE(env.has_value());
	CHECK(count == 8u);
	f.repo.unbind("count");
	f.repo.unbind("force");
}

TEST_CASE("bind puts a stored value into the field") {
	fixture f;
	f.repo.set("count", 4);
	size_t count = 0;
	REQUIRE(f.repo.bind("count", count).has_value());
	CHECK(count == 4u);
	f.repo.unbind("count");
}

TEST_CASE("unbind keeps the value for the next bind") {
	fixture f;
	size_t first = 0, second = 0;
	REQUIRE(f.repo.bind("count", first).has_value());
	f.repo.set("count", 3);
	f.repo.unbind("count");
	first = 9;
	CHECK(f.repo.get<size_t>("count") == 3u);
	REQUIRE(f.repo.bind("count", second).has_value());
	CHECK(second == 3u);
	f.repo.unbind("count");
}

TEST_CASE("reset gives the default to the options of one owner") {
	fixture f;
	bool force = true;
	REQUIRE(f.repo.bind("force", force).has_value());
	f.repo.set("count", 5);
	f.repo.set("verbose", true);
	f.repo.reset("run");
	CHECK(!force);
	CHECK(f.repo.get<size_t>("count") == 1u);
	CHECK(f.repo.get<bool>("verbose"));
	f.repo.unbind("force");
}

TEST_CASE("a codec binds a field of another type") {
	options_repository repo;
	REQUIRE(repo.declare(ids_set()).has_value());
	set<int> ids;
	REQUIRE(repo.bind("ids", ids, int_set_codec{}).has_value());
	REQUIRE(repo.set_text("ids", "1, 2").has_value());
	CHECK(ids == set<int>{ 1, 2 });
	auto text = repo.get_text("ids");
	REQUIRE(text.has_value());
	CHECK(text.value() == "1, 2");
	auto bad = repo.set_text("ids", "x");
	CHECK(bad.has_error());
	CHECK(contains(messages_of(bad.report()), "not a number"));
	CHECK(ids == set<int>{ 1, 2 });
	repo.unbind("ids");
	CHECK(repo.get<option_list>("ids") == option_list{ "1", "2" });
}

TEST_CASE("the hook runs once per write and not on a failed write") {
	options_repository repo;
	REQUIRE(repo.declare(ids_set()).has_value());
	set<int> ids;
	size_t runs = 0;
	REQUIRE(repo.bind("ids", ids, int_set_codec{}, [&] { ++runs; })
		.has_value());
	CHECK(runs == 0);
	repo.set("ids", option_list{ "3" });
	CHECK(runs == 1);
	REQUIRE(repo.set_text("ids", "4").has_value());
	CHECK(runs == 2);
	CHECK(repo.set_text("ids", "y").has_error());
	CHECK(runs == 2);
	argv_of a{ "tool", "--ids", "5", "--ids", "6" };
	REQUIRE(repo.parse_args(a.argc(), a.argv()).has_value());
	CHECK(runs == 3);
	CHECK(ids == set<int>{ 5, 6 });
	repo.reset("test");
	CHECK(runs == 4);
	CHECK(ids.empty());
	repo.unbind("ids");
}

TEST_CASE("parse_args writes no field when a value does not fit") {
	options_repository repo;
	REQUIRE(repo.declare(ids_set()).has_value());
	set<int> ids{ 1 };
	bool flag = false;
	REQUIRE(repo.bind("ids", ids, int_set_codec{}).has_value());
	REQUIRE(repo.bind("flag", flag).has_value());
	argv_of a{ "tool", "--flag", "--ids", "x" };
	auto r = repo.parse_args(a.argc(), a.argv());
	CHECK(r.has_error());
	CHECK(!flag);
	CHECK(ids == set<int>{ 1 });
	repo.unbind("flag");
	repo.unbind("ids");
}

} // TEST_SUITE

TEST_SUITE("options current repository") {

TEST_CASE("options gives the same repository twice on one thread") {
	options_repository& first = options();
	CHECK(&options() == &first);
}

TEST_CASE("a value set through options is read through options") {
	options_repository mine;
	options_scope scope(mine);
	REQUIRE(options().declare(option_set{ .options = {
		{ "current-test", "test", size_t{ 0 }, "", {} } } }).has_value());
	options().set("current-test", 5);
	CHECK(options().get<size_t>("current-test") == 5u);
	CHECK(mine.get<size_t>("current-test") == 5u);
}

TEST_CASE("a scope makes another repository current") {
	options_repository& outer = options();
	options_repository mine;
	{
		options_scope scope(mine);
		CHECK(&options() == &mine);
	}
	CHECK(&options() == &outer);
}

TEST_CASE("scopes nest") {
	options_repository& outer = options();
	options_repository a, b;
	{
		options_scope sa(a);
		{
			options_scope sb(b);
			CHECK(&options() == &b);
		}
		CHECK(&options() == &a);
	}
	CHECK(&options() == &outer);
}

#ifdef OPTIONS_TEST_THREADS
TEST_CASE("another thread has its own repository") {
	options_repository* here = &options();
	options_repository* there = nullptr;
	options_repository mine;
	options_scope scope(mine);
	thread t([&] { there = &options(); });
	t.join();
	CHECK(there != nullptr);
	CHECK(there != here);
	CHECK(there != &mine);
}
#endif

} // TEST_SUITE

TEST_SUITE("options command defaults") {

// An option with a default for the command "run" and none for "show".
option_set echo_set() {
	return option_set{
		.options = {
			{ "echo", "", false, "echo the input",
				{ { "run", 'e', true }, { "show", 'e' } } },
		},
		.commands = { { "run", "" }, { "show", "" } },
	};
}

TEST_CASE("a command default applies when no value is stored") {
	options_repository repo;
	REQUIRE(repo.declare(echo_set()).has_value());
	CHECK(!repo.get<bool>("echo"));
	argv_of run{ "tool", "run" };
	REQUIRE(repo.parse_args(run.argc(), run.argv()).has_value());
	CHECK(repo.get<bool>("echo"));
	auto text = repo.get_text("echo");
	REQUIRE(text.has_value());
	CHECK(text.value() == "true");
	argv_of show{ "tool", "show" };
	REQUIRE(repo.parse_args(show.argc(), show.argv()).has_value());
	CHECK(!repo.get<bool>("echo"));
}

TEST_CASE("a stored value wins over the command default") {
	options_repository repo;
	REQUIRE(repo.declare(echo_set()).has_value());
	repo.set("echo", false);
	argv_of run{ "tool", "run" };
	REQUIRE(repo.parse_args(run.argc(), run.argv()).has_value());
	CHECK(!repo.get<bool>("echo"));
}

TEST_CASE("the help of a command shows its default") {
	options_repository repo;
	REQUIRE(repo.declare(echo_set()).has_value());
	ostringstream run, show;
	repo.help(run, "run");
	repo.help(show, "show");
	CHECK(contains(run.str(), "echo the input (default: true)"));
	CHECK(contains(show.str(), "echo the input (default: false)"));
}

TEST_CASE("a command default of another kind is an error") {
	options_repository repo;
	auto r = repo.declare(option_set{ .options = {
		{ "echo", "", false, "", { { "run", 'e', size_t{ 1 } } } } } });
	CHECK(r.has_error());
	const string msg = messages_of(r.report());
	CHECK(contains(msg, "echo"));
	CHECK(contains(msg, "run"));
	CHECK(repo.names().empty());
}

} // TEST_SUITE

TEST_SUITE("options text") {

TEST_CASE("set_text and get_text use the kind") {
	fixture f;
	REQUIRE(f.repo.set_text("force", "On").has_value());
	CHECK(f.repo.get<bool>("force"));
	REQUIRE(f.repo.set_text("tags", "a, b ,, c > d")
		.has_value());
	CHECK(f.repo.get<option_list>("tags")
		== option_list{ "a", "b", "c > d" });
	auto text = f.repo.get_text("tags");
	REQUIRE(text.has_value());
	CHECK(text.value() == "a, b, c > d");
}

TEST_CASE("set_text with a bad value is an error") {
	fixture f;
	auto r = f.repo.set_text("count", "4x");
	CHECK(r.has_error());
	const string msg = messages_of(r.report());
	CHECK(contains(msg, "count"));
	CHECK(contains(msg, "4x"));
	CHECK(f.repo.get<size_t>("count") == 1u);
	CHECK(f.repo.set_text("force", "maybe").has_error());
	CHECK(f.repo.set_text("count", "").has_error());
}

TEST_CASE("an unknown name is an error") {
	fixture f;
	CHECK(f.repo.set_text("nope", "1").has_error());
	CHECK(f.repo.get_text("nope").has_error());
}

TEST_CASE("load_env reads the variables and warns on a bad value") {
	fixture f;
	test_setenv("OPTTEST_FORCE", "OFF");
	test_setenv("OPTTEST_COUNT", "x");
	test_setenv("OPTTEST_INPUT", "file");
	f.repo.set("force", true);
	auto r = f.repo.load_env("OPTTEST_");
	test_unsetenv("OPTTEST_FORCE");
	test_unsetenv("OPTTEST_COUNT");
	test_unsetenv("OPTTEST_INPUT");
	CHECK(r.has_value());
	CHECK(!r.has_error());
	const string msg = messages_of(r.report());
	CHECK(contains(msg, "OPTTEST_COUNT"));
	CHECK(contains(msg, "x"));
	CHECK(!f.repo.get<bool>("force"));
	CHECK(f.repo.get<size_t>("count") == 1u);
	// input does not read the environment
	CHECK(f.repo.get<string>("input").empty());
}

} // TEST_SUITE

TEST_SUITE("options command line") {

TEST_CASE("--name=value and --name value") {
	fixture f;
	argv_of a{ "tool", "run", "--count=3", "--name", "abc" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	REQUIRE(r.has_value());
	CHECK(r.value().command == "run");
	CHECK(f.repo.get<size_t>("count") == 3u);
	CHECK(f.repo.get<string>("name") == "abc");
}

TEST_CASE("values lists the options of argv in argv order") {
	fixture f;
	argv_of a{ "tool", "-v", "run", "--name", "abc", "-t", "a",
		"--count", "2", "--tags", "b", "--count", "3", "-f" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	REQUIRE(r.has_value());
	// one entry per option, at its first place, with its final value
	const vector<pair<string, option_value>> want{
		{ "verbose", true },
		{ "name", string("abc") },
		{ "tags", option_list{ "a", "b" } },
		{ "count", size_t{ 3 } },
		{ "force", true },
	};
	CHECK(r.value().values == want);
	argv_of none{ "tool", "run" };
	auto empty = f.repo.parse_args(none.argc(), none.argv(),
		tool_config());
	REQUIRE(empty.has_value());
	CHECK(empty.value().values.empty());
}

TEST_CASE("-abc sets several short flags and -x takes a value") {
	fixture f;
	argv_of a{ "tool", "run", "-fq", "-n", "5" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	REQUIRE(r.has_value());
	CHECK(f.repo.get<bool>("force"));
	CHECK(f.repo.get<bool>("quiet"));
	CHECK(f.repo.get<size_t>("count") == 5u);
}

TEST_CASE("a flag with no value, and a flag with a value") {
	fixture f;
	argv_of a{ "tool", "-v", "run", "--force", "--quiet", "off" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	REQUIRE(r.has_value());
	CHECK(f.repo.get<bool>("verbose"));
	CHECK(f.repo.get<bool>("force"));
	CHECK(!f.repo.get<bool>("quiet"));
}

TEST_CASE("a repeated list appends and the last value wins otherwise") {
	fixture f;
	argv_of a{ "tool", "run", "-t", "a,b", "--tags", "c",
		"--count", "1", "--count", "2" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	REQUIRE(r.has_value());
	CHECK(f.repo.get<option_list>("tags")
		== option_list{ "a", "b", "c" });
	CHECK(f.repo.get<size_t>("count") == 2u);
}

TEST_CASE("the default command takes the options after the global ones") {
	fixture f;
	args_config cfg = tool_config();
	cfg.default_command = "run";
	argv_of a{ "tool", "-v", "--count", "2" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), cfg);
	REQUIRE(r.has_value());
	CHECK(r.value().command == "run");
	CHECK(f.repo.get<bool>("verbose"));
	CHECK(f.repo.get<size_t>("count") == 2u);
}

TEST_CASE("the input list and the default command when there are inputs") {
	fixture f;
	args_config cfg = tool_config();
	cfg.default_command = "run";
	cfg.default_command_when_inputs = "show";
	argv_of a{ "tool", "-" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), cfg);
	REQUIRE(r.has_value());
	CHECK(r.value().inputs == vector<string>{ "-" });
	CHECK(r.value().command == "show");
}

TEST_CASE("a URL is an input") {
	for (const char* url : { "http://example.com/a?b=c", "tcp://127.0.0.1:5000",
		"git+ssh://host/repo", "localhost:8080", "127.0.0.1:1" })
	{
		fixture f;
		args_config cfg = tool_config();
		cfg.default_command = "run";
		cfg.default_command_when_inputs = "show";
		argv_of a{ "tool", url };
		auto r = f.repo.parse_args(a.argc(), a.argv(), cfg);
		REQUIRE(r.has_value());
		CHECK(r.value().inputs == vector<string>{ url });
		CHECK(r.value().command == "show");
	}
}

TEST_CASE("a word that is no file and no URL is an error") {
	for (const char* word : { "no-such-input", "host:port", "host:",
		":80", "://x", "1http://x", "a b:80" })
	{
		fixture f;
		argv_of a{ "tool", "run", word };
		auto r = f.repo.parse_args(a.argc(), a.argv(),
			tool_config());
		CHECK(r.has_error());
		CHECK(contains(messages_of(r.report()),
			"Invalid command, or no such file or URL"));
	}
}

TEST_CASE("--help sets the help option") {
	fixture f;
	argv_of a{ "tool", "-h" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	REQUIRE(r.has_value());
	CHECK(f.repo.get<bool>("help"));
	CHECK(r.value().command.empty());
}

TEST_CASE("an unknown option is an error with the help text") {
	fixture f;
	argv_of a{ "tool", "run", "--count", "7", "--nope" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "Unknown option"));
	ostringstream printed;
	r.report().print(printed);
	CHECK(contains(printed.str(), "Commands:"));
	// nothing is stored on an error
	CHECK(f.repo.get<size_t>("count") == 1u);
}

TEST_CASE("an unknown option before the command with no default") {
	fixture f;
	argv_of a{ "tool", "--count", "2" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	CHECK(r.has_error());
}

TEST_CASE("an unknown command is an error") {
	fixture f;
	args_config cfg = tool_config();
	cfg.default_command = "missing";
	argv_of a{ "tool" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), cfg);
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "Unknown command"));
}

TEST_CASE("a missing argument and a bad value are errors") {
	fixture f;
	argv_of missing{ "tool", "run", "--count" };
	CHECK(f.repo.parse_args(missing.argc(), missing.argv(),
		tool_config()).has_error());
	argv_of inner{ "tool", "run", "-nf", "3" };
	CHECK(f.repo.parse_args(inner.argc(), inner.argv(),
		tool_config()).has_error());
	argv_of bad{ "tool", "run", "--count", "x" };
	auto r = f.repo.parse_args(bad.argc(), bad.argv(), tool_config());
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "count"));
	argv_of second{ "tool", "run", "show" };
	CHECK(f.repo.parse_args(second.argc(), second.argv(),
		tool_config()).has_error());
	argv_of nofile{ "tool", "run", "no-such-file-here" };
	CHECK(f.repo.parse_args(nofile.argc(), nofile.argv(),
		tool_config()).has_error());
}

TEST_CASE("a flag takes each true and false word") {
	for (const char* w : { "1", "t", "on", "yes", "true", "TRUE" }) {
		fixture f;
		argv_of a{ "tool", "run", "--force", w };
		REQUIRE(f.repo.parse_args(a.argc(), a.argv(),
			tool_config()).has_value());
		CHECK(f.repo.get<bool>("force"));
	}
	for (const char* w : { "0", "f", "off", "no", "false", "False" }) {
		fixture f;
		f.repo.set("force", true);
		argv_of a{ "tool", "run", "--force", w };
		REQUIRE(f.repo.parse_args(a.argc(), a.argv(),
			tool_config()).has_value());
		CHECK(!f.repo.get<bool>("force"));
	}
	fixture f;
	argv_of a{ "tool", "run", "--force=true", "--quiet=false" };
	REQUIRE(f.repo.parse_args(a.argc(), a.argv(),
		tool_config()).has_value());
	CHECK(f.repo.get<bool>("force"));
	CHECK(!f.repo.get<bool>("quiet"));
}

TEST_CASE("a flag does not take a file") {
	fixture f;
	argv_of a{ "tool", "run", "-f", "-", "--quiet", "-" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), tool_config());
	REQUIRE(r.has_value());
	CHECK(f.repo.get<bool>("force"));
	CHECK(f.repo.get<bool>("quiet"));
	CHECK(r.value().inputs == vector<string>{ "-", "-" });
}

TEST_CASE("false on the command line wins over a true default") {
	options_repository repo;
	REQUIRE(repo.declare(option_set{ .options = {
		{ "color", "", true, "", { { "", 'c' } } } } }).has_value());
	argv_of a{ "tool", "--color", "false" };
	REQUIRE(repo.parse_args(a.argc(), a.argv()).has_value());
	CHECK(!repo.get<bool>("color"));
}

TEST_CASE("a text value") {
	fixture f;
	argv_of space{ "tool", "run", "--name", "hello world" };
	REQUIRE(f.repo.parse_args(space.argc(), space.argv(),
		tool_config()).has_value());
	CHECK(f.repo.get<string>("name") == "hello world");
	argv_of missing{ "tool", "run", "--name" };
	CHECK(f.repo.parse_args(missing.argc(), missing.argv(),
		tool_config()).has_error());
}

TEST_CASE("several options of several kinds") {
	fixture f;
	argv_of a{ "tool", "run", "--force", "--count", "2", "--name", "ok" };
	REQUIRE(f.repo.parse_args(a.argc(), a.argv(),
		tool_config()).has_value());
	CHECK(f.repo.get<bool>("force"));
	CHECK(f.repo.get<size_t>("count") == 2u);
	CHECK(f.repo.get<string>("name") == "ok");
}

TEST_CASE("the help text of the program and of a command") {
	fixture f;
	args_config cfg = tool_config();
	cfg.help_header = "tool <file>";
	argv_of a{ "tool" };
	REQUIRE(f.repo.parse_args(a.argc(), a.argv(), cfg).has_value());
	ostringstream program;
	f.repo.help(program);
	const string p = program.str();
	CHECK(contains(p, "tool <file> [ options ] <command>"));
	CHECK(contains(p, "a test tool"));
	CHECK(contains(p, "Options:"));
	CHECK(contains(p, "--verbose\t-v\tprint more"));
	CHECK(contains(p, "Commands:"));
	CHECK(contains(p, "\trun\t\t\trun the tool"));
	ostringstream run;
	f.repo.help(run, "run");
	const string c = run.str();
	CHECK(contains(c, "tool <file> run [ options ]"));
	CHECK(contains(c, "Command options:"));
	CHECK(contains(c, "--count\t\t-n\thow many times (default: 1)"));
	// the surface default text replaces the value in the help
	CHECK(contains(c, "(default: none)"));
}

} // TEST_SUITE
