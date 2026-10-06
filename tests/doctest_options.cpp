// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the options repository: declare, typed access, sources and
// precedence, the thread source, views, text access, the environment and
// the command line.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <atomic>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
#	define OPTIONS_TEST_THREADS 1
#	include <thread>
#endif

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

// A repository with the tool set and three sources, lowest first.
struct fixture {
	options_repository repo;
	size_t env     = repo.source("env");
	size_t cli     = repo.source("command-line");
	size_t session = repo.source("session");
	fixture() { REQUIRE(repo.declare(tool_set()).has_value()); }
};

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

TEST_CASE("a command twice is an error") {
	options_repository repo;
	auto r = repo.declare(option_set{ .commands = {
		{ "run", "" }, { "run", "" } } });
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "Command already exists"));
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

#ifdef OPTIONS_TEST_THREADS
TEST_CASE("set_local is seen only by its own thread") {
	fixture f;
	f.repo.set("count", size_t{ 10 }, f.session);
	size_t in_a = 0, in_b = 0, in_c = 0;
	thread a([&] {
		f.repo.set_local("count", size_t{ 1 });
		in_a = f.repo.get<size_t>("count");
	});
	thread b([&] {
		f.repo.set_local("count", size_t{ 2 });
		in_b = f.repo.get<size_t>("count");
	});
	a.join();
	b.join();
	thread c([&] { in_c = f.repo.get<size_t>("count"); });
	c.join();
	CHECK(in_a == 1u);
	CHECK(in_b == 2u);
	CHECK(in_c == 10u);
	CHECK(f.repo.get<size_t>("count") == 10u);
}

TEST_CASE("a view keeps its values while another thread writes") {
	fixture f;
	f.repo.set("count", size_t{ 3 }, f.session);
	const options_view v = f.repo.view();
	thread w([&] { f.repo.set("count", size_t{ 4 }, f.session); });
	w.join();
	CHECK(v.get<size_t>("count") == 3u);
	CHECK(f.repo.get<size_t>("count") == 4u);
}
#endif

TEST_CASE("the thread source is above every other source") {
	fixture f;
	f.repo.set("count", size_t{ 3 }, f.session);
	f.repo.set_local("count", size_t{ 9 });
	CHECK(f.repo.get<size_t>("count") == 9u);
	const options_view v = f.repo.view();
	f.repo.clear_local("count");
	CHECK(f.repo.get<size_t>("count") == 3u);
	CHECK(v.get<size_t>("count") == 9u);
}

TEST_CASE("thread values belong to one repository") {
	fixture a;
	fixture b;
	a.repo.set_local("count", size_t{ 2 });
	CHECK(a.repo.get<size_t>("count") == 2u);
	CHECK(b.repo.get<size_t>("count") == 1u);
}

TEST_CASE("a new repository never sees the thread values of an old one") {
	for (size_t n = 0; n < 1000; ++n) {
		fixture f;
		CHECK(f.repo.get<size_t>("count") == 1u);
		f.repo.set_local("count", n + 2);
		CHECK(f.repo.get<size_t>("count") == n + 2);
	}
}

TEST_CASE("set_local changes the version of its own thread only") {
	fixture f;
	const size_t before = f.repo.version();
	f.repo.set_local("count", size_t{ 5 });
	const size_t after = f.repo.version();
	CHECK(after > before);
	f.repo.set_local("count", size_t{ 5 });
	CHECK(f.repo.version() == after);
	f.repo.clear_local("count");
	CHECK(f.repo.version() > after);
}

#ifdef OPTIONS_TEST_THREADS
TEST_CASE("repositories and threads leave no record behind") {
	vector<thread> workers;
	for (int t = 0; t < 4; ++t) workers.emplace_back([] {
		for (size_t n = 0; n < 200; ++n) {
			fixture f;
			f.repo.set_local("count", n + 2);
			CHECK(f.repo.get<size_t>("count") == n + 2);
		}
	});
	{
		fixture shared;
		vector<thread> readers;
		for (int t = 0; t < 4; ++t) readers.emplace_back([&shared] {
			shared.repo.set_local("count", size_t{ 9 });
			CHECK(shared.repo.get<size_t>("count") == 9u);
		});
		for (auto& t : readers) t.join();
		// the readers ended, so their records left the shared repository
		CHECK(detail::link_counts().records
			<= workers.size());
	}
	for (auto& t : workers) t.join();
	const auto c = detail::link_counts();
	CHECK(c.entries == 0);
	CHECK(c.records == 0);
}

TEST_CASE("a thread that ends leaves a live repository") {
	fixture f;
	const auto before = detail::link_counts();
	thread t([&] { f.repo.set_local("count", size_t{ 3 }); });
	t.join();
	const auto after = detail::link_counts();
	CHECK(after.records == before.records);
	CHECK(after.entries == before.entries);
	CHECK(after.threads == before.threads);
	CHECK(f.repo.get<size_t>("count") == 1u);
}

TEST_CASE("a destroyed repository leaves a live thread") {
	auto f = make_unique<fixture>();
	atomic<int> step{ 0 };
	size_t in_new = 0;
	thread t([&] {
		f->repo.set_local("count", size_t{ 3 });
		step = 1;
		while (step != 2) this_thread::yield();
		fixture g;
		in_new = g.repo.get<size_t>("count");
		step = 3;
		while (step != 4) this_thread::yield();
	});
	while (step != 1) this_thread::yield();
	const auto with = detail::link_counts();
	REQUIRE(with.records >= 1);
	REQUIRE(with.entries >= 1);
	f.reset();
	const auto without = detail::link_counts();
	CHECK(without.records == with.records - 1);
	CHECK(without.entries == with.entries - 1);
	step = 2;
	while (step != 3) this_thread::yield();
	CHECK(in_new == 1u);
	step = 4;
	t.join();
	CHECK(detail::link_counts().records == without.records);
}

TEST_CASE("the version of a thread and a shared write") {
	fixture f;
	atomic<int> step{ 0 };
	size_t a_before = 0, a_local = 0, a_shared = 0;
	thread a([&] {
		a_before = f.repo.version();
		f.repo.set_local("count", size_t{ 5 });
		a_local = f.repo.version();
		step = 1;
		while (step != 2) this_thread::yield();
		a_shared = f.repo.version();
	});
	while (step != 1) this_thread::yield();
	const size_t main_before = f.repo.version();
	f.repo.set("count", size_t{ 6 }, f.session);
	const size_t main_after = f.repo.version();
	step = 2;
	a.join();
	CHECK(a_local > a_before);
	CHECK(a_shared > a_local);
	CHECK(main_after > main_before);
	// the set_local of thread a is not in the version of this thread
	CHECK(main_before == a_before);
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

TEST_CASE("a command default applies when its command runs") {
	options_repository repo;
	const size_t cli = repo.source("command-line");
	REQUIRE(repo.declare(echo_set()).has_value());
	CHECK(!repo.get<bool>("echo"));
	argv_of run{ "tool", "run" };
	REQUIRE(repo.parse_args(run.argc(), run.argv(), cli).has_value());
	CHECK(repo.get<bool>("echo"));
	CHECK(repo.view().get<bool>("echo"));
	auto text = repo.get_text("echo");
	REQUIRE(text.has_value());
	CHECK(text.value() == "true");
	argv_of show{ "tool", "show" };
	REQUIRE(repo.parse_args(show.argc(), show.argv(), cli).has_value());
	CHECK(!repo.get<bool>("echo"));
}

TEST_CASE("a source value wins over the command default") {
	options_repository repo;
	const size_t cli = repo.source("command-line");
	const size_t session = repo.source("session");
	REQUIRE(repo.declare(echo_set()).has_value());
	repo.set("echo", false, session);
	argv_of run{ "tool", "run" };
	REQUIRE(repo.parse_args(run.argc(), run.argv(), cli).has_value());
	CHECK(!repo.get<bool>("echo"));
	repo.clear("echo", session);
	CHECK(repo.get<bool>("echo"));
}

TEST_CASE("the help of a command shows its default") {
	options_repository repo;
	repo.source("command-line");
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

TEST_SUITE("options command line") {

TEST_CASE("--name=value and --name value") {
	fixture f;
	argv_of a{ "tool", "run", "--count=3", "--name", "abc" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, tool_config());
	REQUIRE(r.has_value());
	CHECK(r.value().command == "run");
	CHECK(f.repo.get<size_t>("count") == 3u);
	CHECK(f.repo.get<string>("name") == "abc");
	CHECK(f.repo.is_set("count", f.cli));
}

TEST_CASE("-abc sets several short flags and -x takes a value") {
	fixture f;
	argv_of a{ "tool", "run", "-fq", "-n", "5" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, tool_config());
	REQUIRE(r.has_value());
	CHECK(f.repo.get<bool>("force"));
	CHECK(f.repo.get<bool>("quiet"));
	CHECK(f.repo.get<size_t>("count") == 5u);
}

TEST_CASE("a flag with no value, and a flag with a value") {
	fixture f;
	argv_of a{ "tool", "-v", "run", "--force", "--quiet", "off" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, tool_config());
	REQUIRE(r.has_value());
	CHECK(f.repo.get<bool>("verbose"));
	CHECK(f.repo.get<bool>("force"));
	CHECK(!f.repo.get<bool>("quiet"));
}

TEST_CASE("a repeated list appends and the last value wins otherwise") {
	fixture f;
	argv_of a{ "tool", "run", "-t", "a,b", "--tags", "c",
		"--count", "1", "--count", "2" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, tool_config());
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
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, cfg);
	REQUIRE(r.has_value());
	CHECK(r.value().command == "run");
	CHECK(f.repo.get<bool>("verbose"));
	CHECK(f.repo.get<size_t>("count") == 2u);
}

TEST_CASE("the file list and the default command when there are files") {
	fixture f;
	args_config cfg = tool_config();
	cfg.default_command = "run";
	cfg.default_command_when_files = "show";
	argv_of a{ "tool", "-" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, cfg);
	REQUIRE(r.has_value());
	CHECK(r.value().files == vector<string>{ "-" });
	CHECK(r.value().command == "show");
}

TEST_CASE("--help sets the help field") {
	fixture f;
	argv_of a{ "tool", "-h" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, tool_config());
	REQUIRE(r.has_value());
	CHECK(r.value().help);
	CHECK(r.value().command.empty());
}

TEST_CASE("an unknown option is an error with the help text") {
	fixture f;
	argv_of a{ "tool", "run", "--count", "7", "--nope" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, tool_config());
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
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, tool_config());
	CHECK(r.has_error());
}

TEST_CASE("an unknown command is an error") {
	fixture f;
	args_config cfg = tool_config();
	cfg.default_command = "missing";
	argv_of a{ "tool" };
	auto r = f.repo.parse_args(a.argc(), a.argv(), f.cli, cfg);
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "Unknown command"));
}

TEST_CASE("a missing argument and a bad value are errors") {
	fixture f;
	argv_of missing{ "tool", "run", "--count" };
	CHECK(f.repo.parse_args(missing.argc(), missing.argv(), f.cli,
		tool_config()).has_error());
	argv_of inner{ "tool", "run", "-nf", "3" };
	CHECK(f.repo.parse_args(inner.argc(), inner.argv(), f.cli,
		tool_config()).has_error());
	argv_of bad{ "tool", "run", "--count", "x" };
	auto r = f.repo.parse_args(bad.argc(), bad.argv(), f.cli,
		tool_config());
	CHECK(r.has_error());
	CHECK(contains(messages_of(r.report()), "count"));
	argv_of second{ "tool", "run", "show" };
	CHECK(f.repo.parse_args(second.argc(), second.argv(), f.cli,
		tool_config()).has_error());
	argv_of nofile{ "tool", "run", "no-such-file-here" };
	CHECK(f.repo.parse_args(nofile.argc(), nofile.argv(), f.cli,
		tool_config()).has_error());
}

TEST_CASE("the help text of the program and of a command") {
	fixture f;
	args_config cfg = tool_config();
	cfg.help_header = "tool <file>";
	argv_of a{ "tool" };
	REQUIRE(f.repo.parse_args(a.argc(), a.argv(), f.cli, cfg).has_value());
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
