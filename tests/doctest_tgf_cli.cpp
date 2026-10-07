// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the TGF CLI surface - subcommand routing and option defaults.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/tgf/tgf_cli.h"

#include <initializer_list>
#include <sstream>

using namespace std;
using namespace idni;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// A repository with the options of the tgf tool on their commands. A short
// letter used twice on one command fails the fixture.
struct tgf_options_fixture {
	options_repository repo;
	tgf_options_fixture() {
		for (const auto* set : { &tgf_option_set, &grammar_option_set,
			&parser_option_set, &tgf_repl_option_set })
				REQUIRE(repo.declare(*set).has_value());
		for (const auto& [name, surface] : tgf_surfaces)
			REQUIRE(repo.add_surface(name, surface).has_value());
	}
	diagnostics::result<parsed_args> parse(
		initializer_list<const char*> args)
	{
		vector<string> store(args.begin(), args.end());
		vector<char*> argv;
		for (auto& a : store) argv.push_back(a.data());
		return repo.parse_args(static_cast<int>(argv.size()),
			argv.data(), tgf_args_config);
	}
};

// ---------------------------------------------------------------------------
// TEST SUITE: subcommand routing
// ---------------------------------------------------------------------------

TEST_SUITE("tgf cli: subcommands") {

	TEST_CASE("default command is repl") {
		tgf_options_fixture f;
		auto r = f.parse({ "tgf" });
		REQUIRE(r.has_value());
		CHECK(r.value().command == "repl");
	}

	TEST_CASE("invalid subcommand") {
		tgf_options_fixture f;
		CHECK(f.parse({ "tgf", "invalid" }).has_error());
	}

	TEST_CASE("each command routes") {
		for (const char* c : { "grammar", "gen", "parse", "repl",
			"serve", "connect", "test" })
		{
			tgf_options_fixture f;
			auto r = f.parse({ "tgf", c });
			REQUIRE(r.has_value());
			CHECK(r.value().command == c);
		}
	}

	TEST_CASE("repl --legacy-repl defaults to false") {
		tgf_options_fixture f;
		REQUIRE(f.parse({ "tgf", "repl" }).has_value());
		CHECK(!f.repo.get<bool>("legacy-repl"));
	}

	TEST_CASE("repl -X enables legacy terminal REPL") {
		tgf_options_fixture f;
		REQUIRE(f.parse({ "tgf", "repl", "-X" }).has_value());
		CHECK(f.repo.get<bool>("legacy-repl"));
	}

	TEST_CASE("repl --legacy-repl long form") {
		tgf_options_fixture f;
		REQUIRE(f.parse({ "tgf", "repl", "--legacy-repl" }).has_value());
		CHECK(f.repo.get<bool>("legacy-repl"));
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: gen subcommand options
// ---------------------------------------------------------------------------

TEST_SUITE("tgf cli: gen options") {

	TEST_CASE("gen default options") {
		tgf_options_fixture f;
		auto r = f.parse({ "tgf", "gen" });
		REQUIRE(r.has_value());
		CHECK(r.value().command == "gen");
		CHECK(!f.repo.get<bool>("help"));
		CHECK(f.repo.get<string>("name").empty());
		CHECK(f.repo.get<string>("decoder").empty());
		CHECK(f.repo.get<string>("encoder").empty());
		CHECK(f.repo.get<string>("output").empty());
		CHECK(f.repo.get<bool>("header-only"));
	}

	TEST_CASE("gen with --name and --help") {
		tgf_options_fixture f;
		auto r = f.parse({ "tgf", "gen", "--name", "my_other_parser",
			"--help" });
		REQUIRE(r.has_value());
		CHECK(r.value().command == "gen");
		CHECK(f.repo.get<bool>("help"));
		CHECK(f.repo.get<string>("name") == "my_other_parser");
	}

	TEST_CASE("gen --dynamic takes name=value entries") {
		tgf_options_fixture f;
		REQUIRE(f.parse({ "tgf", "gen", "-D",
			"ba_type=sbf,ba_type=tau" }).has_value());
		CHECK(f.repo.get<option_list>("dynamic")
			== option_list{ "ba_type=sbf", "ba_type=tau" });
	}

	TEST_CASE("gen with invalid option") {
		tgf_options_fixture f;
		CHECK(f.parse({ "tgf", "gen", "--invalid" }).has_error());
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: global / default-command options
// ---------------------------------------------------------------------------

TEST_SUITE("tgf cli: global options") {

	TEST_CASE("grammar command accepts -N for nullable") {
		tgf_options_fixture f;
		auto r = f.parse({ "tgf", "grammar", "-N" });
		REQUIRE(r.has_value());
		CHECK(r.value().command == "grammar");
		CHECK(f.repo.get<bool>("nullable"));
	}

	TEST_CASE("the grammar flag has a default per command") {
		tgf_options_fixture f;
		REQUIRE(f.parse({ "tgf", "grammar" }).has_value());
		CHECK(f.repo.get<bool>("grammar"));
		REQUIRE(f.parse({ "tgf", "parse" }).has_value());
		CHECK(!f.repo.get<bool>("grammar"));
	}

	TEST_CASE("-G names the enabled productions on each command") {
		for (const char* c : { "grammar", "parse", "gen", "repl",
			"serve", "test" })
		{
			tgf_options_fixture f;
			auto r = f.parse({ "tgf", c, "-G", "a,b" });
			REQUIRE(r.has_value());
			CHECK(f.repo.get<option_list>("enabled-productions")
				== option_list{ "a", "b" });
		}
	}

	TEST_CASE("invalid global option") {
		tgf_options_fixture f;
		CHECK(f.parse({ "tgf", "--invalid" }).has_error());
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: derived character class report and switch
// ---------------------------------------------------------------------------

#if defined(TAU_TEST_HAS_TGF_CLI) && defined(PROJECT_SOURCE_DIR)

// Runs the TGF CLI with args and returns what it wrote to stdout and
// stderr. The parse error report writes errors to stderr, so a test that
// reads only stdout would see an empty string for a rejected input.
static string capture_tgf_run(const vector<string>& args) {
	vector<string> a = args;
	vector<char*> argv;
	for (auto& s : a) argv.push_back(s.data());
	ostringstream os;
	streambuf* old_out = cout.rdbuf(os.rdbuf());
	streambuf* old_err = cerr.rdbuf(os.rdbuf());
	tgf_run((int)argv.size(), argv.data());
	cout.rdbuf(old_out);
	cerr.rdbuf(old_err);
	return os.str();
}

TEST_SUITE("tgf cli: help") {

	TEST_CASE("-h with no command prints the program help") {
		string out = capture_tgf_run({ "tgf", "-h" });
		CHECK(out.find("Commands:") != string::npos);
	}

	TEST_CASE("parse -h prints the help of the command") {
		string out = capture_tgf_run({ "tgf", "parse", "-h" });
		CHECK(out.find("Command options:") != string::npos);
		CHECK(out.find("Commands:") == string::npos);
	}
}

TEST_SUITE("tgf cli: derived character classes") {

	TEST_CASE("grammar --char-class-report lists unescaped as derived") {
		string json = string(PROJECT_SOURCE_DIR)
			+ "/src/format/json/json.tgf";
		string out = capture_tgf_run({ "tgf", json, "grammar",
			"--char-class-report" });
		CHECK(out.find("derived:") != string::npos);
		CHECK(out.find("unescaped") != string::npos);
		CHECK(out.find("rejected:") != string::npos);
	}

	TEST_CASE("parse --derive-char-classes false matches the default") {
		string json = string(PROJECT_SOURCE_DIR)
			+ "/src/format/json/json.tgf";
		string on = capture_tgf_run({ "tgf", json, "parse",
			"-e", "\"a\"", "-c", "false" });
		string off = capture_tgf_run({ "tgf", json, "parse",
			"-e", "\"a\"", "-c", "false",
			"--derive-char-classes", "false" });
		CHECK(!on.empty());
		CHECK(on == off);
	}

	TEST_CASE("parse --derive-char-classes false turns the classes off") {
		string json = string(PROJECT_SOURCE_DIR)
			+ "/src/format/json/json.tgf";
		string on = capture_tgf_run({ "tgf", json, "parse",
			"-e", "\"a", "-v", "detailed" });
		string off = capture_tgf_run({ "tgf", json, "parse",
			"-e", "\"a", "-v", "detailed",
			"--derive-char-classes", "false" });
		CHECK(on.find("expecting unescaped") != string::npos);
		CHECK(off.find("expecting unescaped") == string::npos);
		CHECK(on != off);
	}
}

#endif // TAU_TEST_HAS_TGF_CLI && PROJECT_SOURCE_DIR
