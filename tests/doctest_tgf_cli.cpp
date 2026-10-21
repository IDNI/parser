// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the TGF CLI surface - subcommand routing and option defaults.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "../src/tgf/tgf_cli.h"
#include "../src/tgf/parser_gen.h"
#include "test_env.h"
#include "tgf_run_capture.h"

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <sstream>
#include <utility>

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
		for (const auto* set : { &tgf_option_set, &grammar_option_set(),
			&parser_option_set(), &tgf_repl_option_set })
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

static string csv_path() {
	return string(PROJECT_SOURCE_DIR) + "/src/format/csv/csv.tgf";
}

static string json_path() {
	return string(PROJECT_SOURCE_DIR) + "/src/format/json/json.tgf";
}

TEST_SUITE("tgf cli: help") {

	TEST_CASE("-h with no command prints the program help") {
		string out = capture_tgf_run({ "tgf", "-h" }).out;
		CHECK(out.find("Commands:") != string::npos);
	}

	TEST_CASE("parse -h prints the help of the command") {
		string out = capture_tgf_run({ "tgf", "parse", "-h" }).out;
		CHECK(out.find("Command options:") != string::npos);
		CHECK(out.find("Commands:") == string::npos);
	}
}

// The last line of @p out that starts with @p prefix.
static string last_line_with(const string& out, const string& prefix) {
	string last, line;
	istringstream is(out);
	while (getline(is, line))
		if (line.rfind(prefix, 0) == 0) last = line;
	return last;
}

TEST_SUITE("tgf cli: a load writes the command line again") {


	TEST_CASE("-G holds over the directives of a loaded grammar") {
		// csv.tgf enables comma, crlf and no_header itself
		string out = capture_tgf_run({ "tgf", csv_path(), "repl",
			"-c", "false", "-G", "tab", "-e",
			"load \"" + csv_path() + "\" . get enabled-productions" }).out;
		string ep = last_line_with(out, "enabled-productions:");
		CHECK(ep.find("tab") != string::npos);
		CHECK(ep.find("comma") == string::npos);
	}

	TEST_CASE("--variant wins over the default profile of a loaded grammar") {
		// rfc4180 is the first profile of csv.tgf, so it is the default
		string out = capture_tgf_run({ "tgf", csv_path(), "repl",
			"-c", "false", "--variant", "tab", "-e",
			"get variant . get enabled-productions . load \""
			+ csv_path() + "\" . get variant" }).out;
		string first = last_line_with(out, "enabled-productions:");
		CHECK(first.find("tab") != string::npos);
		CHECK(first.find("comma") == string::npos);
		string v = last_line_with(out, "variant:");
		CHECK(v.find("tab") != string::npos);
		CHECK(v.find("rfc4180") == string::npos);
	}

	TEST_CASE("set variant in the REPL selects a profile of csv.tgf") {
		string out = capture_tgf_run({ "tgf", csv_path(), "repl",
			"-c", "false", "-e",
			"set variant tab_lf . get enabled-productions" }).out;
		string ep = last_line_with(out, "enabled-productions:");
		CHECK(ep.find("tab") != string::npos);
		CHECK(ep.find("lf") != string::npos);
		CHECK(ep.find("crlf") == string::npos);
		CHECK(ep.find("comma") == string::npos);
	}

	TEST_CASE("a parse option survives a load and a grammar option not") {
		// -p false is not written again: print-graphs is no grammar option
		string out = capture_tgf_run({ "tgf", csv_path(), "repl",
			"-c", "false", "-p", "false", "-e",
			"set gc false . set trim record . set print-graphs true . "
			"load \"" + csv_path() + "\" . get gc . get trim . "
			"get print-graphs" }).out;
		CHECK(last_line_with(out, "gc:").find("off") != string::npos);
		// csv.tgf trims EOL, SEP and DQUOTE itself
		string trim = last_line_with(out, "trim:");
		CHECK(trim.find("record") == string::npos);
		CHECK(trim.find("EOL") != string::npos);
		CHECK(last_line_with(out, "print-graphs:").find("on")
			!= string::npos);
	}
}

TEST_SUITE("tgf cli: environment") {

	TEST_CASE("TGF_MAX_LENGTH sets max-length") {
		const string csv = csv_path();
		test_setenv("TGF_MAX_LENGTH", "7");
		string out = capture_tgf_run({ "tgf", csv, "repl",
			"-c", "false", "-e", "get max-length" }).out;
		test_unsetenv("TGF_MAX_LENGTH");
		CHECK(last_line_with(out, "max-length:").find("7")
			!= string::npos);
	}
}

TEST_SUITE("tgf cli: grammar options") {

	TEST_CASE("--char-classes adds a class to the loaded grammar") {
		namespace fs = std::filesystem;
		const string path = (fs::temp_directory_path()
			/ "tgf_cli_char_classes.tgf").string();
		{
			ofstream f(path);
			f << "start => alpha+.\n";
		}
		string without = capture_tgf_run({ "tgf", path, "parse",
			"-e", "ab", "-c", "false" }).out;
		string with = capture_tgf_run({ "tgf", path, "parse",
			"-e", "ab", "-c", "false", "--char-classes", "alpha" }).out;
		CHECK(without.find("parsed terminals: ab") == string::npos);
		CHECK(with.find("parsed terminals: ab") != string::npos);
		fs::remove(path);
	}

	// The terminals block of a generated header.
	static string terminals_block(const string& header) {
		ifstream in(header);
		REQUIRE(in.is_open());
		ostringstream ss;
		ss << in.rdbuf();
		const string text = ss.str();
		const auto b = text.find("terminals() {");
		REQUIRE(b != string::npos);
		return text.substr(b, text.find("};", b) - b);
	}

	TEST_CASE("gen -D writes a dynamic value and keeps UTF-8 terminals") {
		namespace fs = std::filesystem;
		const string dir = fs::temp_directory_path().string() + "/";
		const string path = dir + "tgf_cli_gen.tgf";
		// U+00E9 is above 0x7F, so its UTF-8 bytes show a truncation
		const string src = "@dynamic tn.\nstart => tn | \"\\u00e9\".\n";
		{
			ofstream f(path);
			f << src;
		}
		capture_tgf_run({ "tgf", path, "gen", "-D", "tn=v", "-O", dir,
			"-o", "tgf_cli_gen.generated.h",
			"-n", "tgf_cli_gen_parser" });
		const string tool_header = dir + "tgf_cli_gen.generated.h";
		ifstream in(tool_header);
		REQUIRE(in.is_open());
		ostringstream text;
		text << in.rdbuf();
		CHECK(text.str().find("{ \"tn\", { \"v\" } }") != string::npos);

		// the direct load gives tn the value of -D as its default
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts, string(
			"@dynamic tn defaults v.\nstart => tn | \"\\u00e9\".\n"));
		REQUIRE(gr.has_value());
		parser_gen_options gopt;
		gopt.output_dir = dir;
		gopt.output = "tgf_cli_gen_direct.generated.h";
		gopt.name = "tgf_cli_gen_direct_parser";
		REQUIRE(generate_parser_cpp<char>(path, gr.value(), gopt)
			.has_value());
		const string direct_header = dir + gopt.output;
		CHECK(terminals_block(tool_header)
			== terminals_block(direct_header));
		in.close();
		fs::remove(path);
		fs::remove(tool_header);
		fs::remove(direct_header);
	}

	TEST_CASE("gen on a grammar with no production warns") {
		namespace fs = std::filesystem;
		const string dir = fs::temp_directory_path().string() + "/";
		const string path = dir + "tgf_cli_gen_empty.tgf";
		const string header = dir + "tgf_cli_gen_empty.generated.h";
		{
			ofstream f(path);
			f << "# a comment only\n";
		}
		fs::remove(header);
		string out = capture_tgf_run({ "tgf", path, "gen", "-O", dir,
			"-o", "tgf_cli_gen_empty.generated.h",
			"-n", "tgf_cli_gen_empty_parser" }).out;
		CHECK(out.find(parser_strings::messages::empty_grammar)
			!= string::npos);
		CHECK_FALSE(fs::exists(header));
		fs::remove(path);
	}
}

TEST_SUITE("tgf cli: derived character classes") {

	TEST_CASE("grammar --char-class-report lists unescaped as derived") {
		string json = json_path();
		string out = capture_tgf_run({ "tgf", json, "grammar",
			"--char-class-report" }).out;
		CHECK(out.find("derived:") != string::npos);
		CHECK(out.find("unescaped") != string::npos);
		CHECK(out.find("rejected:") != string::npos);
	}

	TEST_CASE("parse --derive-char-classes false matches the default") {
		string json = json_path();
		string on = capture_tgf_run({ "tgf", json, "parse",
			"-e", "\"a\"", "-c", "false" }).out;
		string off = capture_tgf_run({ "tgf", json, "parse",
			"-e", "\"a\"", "-c", "false",
			"--derive-char-classes", "false" }).out;
		CHECK(!on.empty());
		CHECK(on == off);
	}

	TEST_CASE("parse --derive-char-classes false turns the classes off") {
		string json = json_path();
		string on = capture_tgf_run({ "tgf", json, "parse",
			"-e", "\"a", "-v", "detailed" }).out;
		string off = capture_tgf_run({ "tgf", json, "parse",
			"-e", "\"a", "-v", "detailed",
			"--derive-char-classes", "false" }).out;
		CHECK(on.find("expecting unescaped") != string::npos);
		CHECK(off.find("expecting unescaped") == string::npos);
		CHECK(on != off);
	}
}

TEST_SUITE("tgf cli: trim keeps the root") {

	TEST_CASE("parse --trim start prints the tree with the root") {
		namespace fs = std::filesystem;
		const string path = (fs::temp_directory_path()
			/ "tgf_cli_trim_root.tgf").string();
		{
			ofstream f(path);
			f << "start => 'a'.\n";
		}
		string out = capture_tgf_run({ "tgf", path, "parse", "--trim",
			"start", "-e", "a", "-c", "false" }).out;
		CHECK(out.find("parsed graph:\n\tstart(") != string::npos);
		CHECK(out.find("\t\t'a'[0, 1]") != string::npos);
		fs::remove(path);
	}

	TEST_CASE("set trim start in the REPL, then a parse") {
		const string tiny = string(PROJECT_SOURCE_DIR)
			+ "/tests/fixtures/tiny.tgf";
		string out = capture_tgf_run({ "tgf", tiny, "repl", "-c", "false",
			"-e", "set trim start . parse 123" }).out;
		CHECK(out.find("parsed graph:\n\tstart(") != string::npos);
		CHECK(out.find("\t\tnum(") != string::npos);
	}
}

// On Windows tgf_main reads the command line of the test process instead.
#ifndef _WIN32
TEST_SUITE("tgf cli: tgf_main") {

	TEST_CASE("tgf_main gives its arguments to tgf_run") {
		string a0 = "tgf", a1 = "-h";
		char* argv[] = { a0.data(), a1.data(), nullptr };
		ostringstream os;
		streambuf* old_out = cout.rdbuf(os.rdbuf());
		int rc = tgf_main(2, argv);
		cout.rdbuf(old_out);
		CHECK(rc == 0);
		CHECK(os.str().find("Commands:") != string::npos);
	}
}
#endif // _WIN32

TEST_SUITE("tgf cli: tgf_command") {

	TEST_CASE("tgf_command_spec gives the entry of each command") {
		const std::pair<tgf_command, string> names[] = {
			{ tgf_command::grammar, "grammar" },
			{ tgf_command::parse, "parse" },
			{ tgf_command::gen, "gen" },
			{ tgf_command::repl, "repl" },
			{ tgf_command::serve, "serve" },
			{ tgf_command::connect, "connect" },
			{ tgf_command::test, "test" },
		};
		CHECK(tgf_option_set.commands.size() == std::size(names));
		for (const auto& [c, name] : names) {
			CAPTURE(name);
			CHECK(tgf_command_spec(c).name == name);
		}
		CHECK(tgf_args_config.default_command == "repl");
		CHECK(tgf_args_config.default_command_when_inputs == "repl");
	}
}

#endif // TAU_TEST_HAS_TGF_CLI && PROJECT_SOURCE_DIR
