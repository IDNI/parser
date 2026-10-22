// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the TGF CLI surface - subcommand routing and option defaults.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "../src/tgf/tgf_cli.h"
#include "../src/tgf/parser_gen.h"
#include "../src/parser_strings.h"
#include "test_env.h"
#include "tgf_run_capture.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <sstream>
#include <streambuf>
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

TEST_SUITE("tgf cli: evaluator option defaults") {
	TEST_CASE("the REPL option defaults match the evaluator fields") {
		const tgf_repl_options defaults{};
		const std::pair<std::string_view, bool> values[] = {
			{ "print-terminals", defaults.print_terminals },
			{ "status", defaults.status },
			{ "print-ambiguity", defaults.print_ambiguity },
			{ "print-graphs", defaults.print_graphs },
			{ "print-tml-rules", defaults.tml_rules },
			{ "print-tml-facts", defaults.tml_facts },
			{ "print-input", defaults.print_input },
			{ "colors", defaults.colors },
			{ "json", defaults.json },
		};
		REQUIRE(tgf_repl_option_set.options.size() == std::size(values));
		for (const auto& [name, expected] : values) {
			auto it = std::find_if(tgf_repl_option_set.options.begin(),
				tgf_repl_option_set.options.end(),
				[name](const option_spec& option) {
					return option.name == name;
				});
			REQUIRE(it != tgf_repl_option_set.options.end());
			REQUIRE(std::holds_alternative<bool>(it->fallback));
			CHECK(std::get<bool>(it->fallback) == expected);
		}
	}

	TEST_CASE("command line errors write text and return one") {
		std::ostringstream err;
		std::streambuf* old = std::cerr.rdbuf(err.rdbuf());
		const int code = command_line_error("bad command");
		std::cerr.rdbuf(old);
		CHECK(code == 1);
		CHECK(err.str() == "bad command\n");
	}
}

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

TEST_SUITE("tgf cli: reload data") {
	TEST_CASE("reload data restores a source-backed grammar") {
		options_repository mine;
		options_scope scope(mine);
		tgf_repl_evaluator re(tgf_repl_evaluator::options{});
		REQUIRE(re.load_source("memory", "start => 'a'.\n"));
		auto data = re.reload_data("unused.tgf");
		auto loaded = format::json::tree::get(data).find("loaded");
		REQUIRE(loaded);
		CHECK(format::json::tree::get(loaded).get_bool());
		const auto* name = format::json::string_member(data, "grammar");
		REQUIRE(name);
		CHECK(*name == "memory");
	}
}

TEST_SUITE("tgf cli: file read reports") {
	TEST_CASE("a missing grammar file keeps its path") {
		const auto path = std::filesystem::temp_directory_path()
			/ "tgf_cli_missing_grammar.tgf";
		std::filesystem::remove(path);
		options_repository mine;
		options_scope scope(mine);
		tgf_repl_evaluator re(tgf_repl_evaluator::options{});
		CHECK_FALSE(re.load_file_source(path.string()));
		auto report = re.take_report();
		auto index = report.first_error_index();
		REQUIRE(index);
		CHECK(report.format_message(*index).find(
			path.filename().string()) != string::npos);
	}

	TEST_CASE("a failed reload keeps its path") {
		const auto path = std::filesystem::temp_directory_path()
			/ "tgf_cli_failed_reload.tgf";
		{
			std::ofstream f(path);
			f << "start => 'a'.\n";
		}
		options_repository mine;
		options_scope scope(mine);
		tgf_repl_evaluator re(path.string());
		REQUIRE(re.good());
		re.take_report();
		std::filesystem::remove(path);
		CHECK_FALSE(re.reload(path.string()));
		auto report = re.take_report();
		auto index = report.first_error_index();
		REQUIRE(index);
		CHECK(report.format_message(*index).find(
			path.filename().string()) != string::npos);
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

TEST_SUITE("tgf cli: report colors") {

	TEST_CASE("a failed parse with -c false prints no escape sequence") {
		string off = capture_tgf_run({ "tgf", json_path(),
			"parse", "-c", "false", "-e", "{" }).out;
		CHECK_FALSE(off.empty());
		CHECK(off.find('\x1b') == string::npos);
		// The same failure with colors on proves the check sees colors.
		string on = capture_tgf_run({ "tgf", json_path(),
			"parse", "-c", "true", "-e", "{" }).out;
		CHECK(on.find('\x1b') != string::npos);
	}

	TEST_CASE("a command error with -c false prints no escape sequence") {
		string off = capture_tgf_run({ "tgf", "parse", "-c", "false",
			"-e", "1" }).out;
		CHECK_FALSE(off.empty());
		CHECK(off.find('\x1b') == string::npos);
	}

	TEST_CASE("a command report follows its output terminal by default") {
		options_repository mine;
		options_scope scope(mine);
		auto r = capture_tgf_run({ "tgf", "parse", "-e", "1" }, "", false);
		CHECK(r.code == 1);
		REQUIRE_FALSE(r.err.empty());
		CHECK((r.err.find('\x1b') != string::npos) == term::is_tty(stderr));
	}

	TEST_CASE("TGF_COLORS controls command reports in a pipe") {
		const char* current = std::getenv("TGF_COLORS");
		const bool had_value = current != nullptr;
		const string saved = current ? current : "";
		for (const char* value : { "false", "true" }) {
			options_repository mine;
			options_scope scope(mine);
			test_setenv("TGF_COLORS", value);
			auto r = capture_tgf_run({ "tgf", "parse", "-e", "1" }, "", false);
			CHECK(r.code == 1);
			CHECK_FALSE(r.err.empty());
			CHECK((r.err.find('\x1b') != string::npos) == (string(value) == "true"));
		}
		if (had_value) test_setenv("TGF_COLORS", saved.c_str());
		else test_unsetenv("TGF_COLORS");
	}
}

TEST_SUITE("tgf cli: parse exit code") {

	TEST_CASE("-e: a valid input gives 0, a syntax error gives 1") {
		CHECK(capture_tgf_run({ "tgf", json_path(), "parse", "-e",
			"[1,2]" }).code == 0);
		CHECK(capture_tgf_run({ "tgf", json_path(), "parse", "-e",
			"[1," }).code == 1);
	}

	TEST_CASE("-i <file>: a valid input gives 0, a syntax error gives 1") {
		namespace fs = std::filesystem;
		const auto dir = fs::temp_directory_path();
		const string good = (dir / "tgf_cli_exit_good.json").string();
		const string bad = (dir / "tgf_cli_exit_bad.json").string();
		ofstream(good) << "{\"a\":[1]}";
		ofstream(bad) << "{\"a\":";
		CHECK(capture_tgf_run({ "tgf", json_path(), "parse", "-i",
			good }).code == 0);
		CHECK(capture_tgf_run({ "tgf", json_path(), "parse", "-i",
			bad }).code == 1);
		fs::remove(good);
		fs::remove(bad);
	}

	TEST_CASE("-i -: stdin gives the same exit codes") {
		CHECK(capture_tgf_run({ "tgf", json_path(), "parse", "-i", "-" },
			"true").code == 0);
		CHECK(capture_tgf_run({ "tgf", json_path(), "parse", "-i", "-" },
			"tru").code == 1);
	}

	TEST_CASE("--json gives the same exit codes") {
		CHECK(capture_tgf_run({ "tgf", json_path(), "parse", "--json",
			"-e", "null" }).code == 0);
		CHECK(capture_tgf_run({ "tgf", json_path(), "parse", "--json",
			"-e", "nul" }).code == 1);
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
		auto r = capture_tgf_run({ "tgf", path, "parse", "--trim",
			"start", "-e", "a", "-c", "false" });
		CHECK(r.code == 0);
		CHECK(r.out.find("parsed graph:\n\tstart(") != string::npos);
		CHECK(r.out.find("\t\t'a'[0, 1]") != string::npos);
		fs::remove(path);
	}

	TEST_CASE("set trim start in the REPL, then a parse") {
		const string tiny = string(PROJECT_SOURCE_DIR)
			+ "/tests/fixtures/tiny.tgf";
		auto r = capture_tgf_run({ "tgf", tiny, "repl", "-c", "false",
			"-e", "set trim start . parse 123" });
		CHECK(r.code == 0);
		CHECK(r.out.find("parsed graph:\n\tstart(") != string::npos);
		CHECK(r.out.find("\t\tnum(") != string::npos);
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

// Checks the lines of a rendered parse tree: after the header line, line k
// has k + 1 tabs and the id depth - 1 - k. It keeps no copy of the text.
struct tree_text_check_buf : std::streambuf {
	explicit tree_text_check_buf(size_t depth) : depth(depth) {
		setp(buf, buf + sizeof buf);
	}
	size_t depth, lines = 0, bad = 0, tabs = 0;
	uint64_t id = 0;
	bool in_id = false;
	char buf[4096];
	void scan() {
		for (char* c = pbase(); c != pptr(); ++c) {
			if (*c == '\t') ++tabs;
			else if (*c == '(') in_id = true;
			else if (*c == ')') in_id = false;
			else if (in_id) id = id * 10 + uint64_t(*c - '0');
			else if (*c == '\n') {
				// line 0 is the header "parsed graph:"
				if (lines != 0 && (tabs != lines
						|| id != depth - lines))
					++bad;
				++lines, tabs = 0, id = 0;
			}
		}
		setp(buf, buf + sizeof buf);
	}
	int overflow(int c) override {
		scan();
		if (c != traits_type::eof()) *pptr() = char(c), pbump(1);
		return 0;
	}
	int sync() override { return scan(), 0; }
};

TEST_SUITE("tgf cli: render_command_text") {
	TEST_CASE("the parse tree of a chain 30000 levels deep renders each "
		"level on its own line with one more tab")
	{
		using jt = format::json::tree;
		// the text grows with the square of the depth
		constexpr size_t depth = 30000;
		auto node = [](size_t id, tref child) {
			trefs v{ jt::build_pair("symbol", jt::build_string("a")),
				jt::build_pair("id", jt::build_number(
					static_cast<double>(id))),
				jt::build_pair("range", jt::build_array({
					jt::build_number(0), jt::build_number(0) })) };
			if (child) v.push_back(jt::build_pair("children",
				jt::build_array({ child })));
			return jt::build_object(v);
		};
		tref t = node(0, nullptr);
		for (size_t i = 1; i != depth; ++i) t = node(i, t);
		cmd_result r;
		r.cmd = "parse";
		r.data = jt::geth(jt::build_object({
			jt::build_pair("tree", t) }));
		tree_text_check_buf buf(depth);
		std::ostream os(&buf);
		render_command_text(r, os, term::colors(false));
		os.flush();
		CHECK(buf.lines == depth + 1);
		CHECK(buf.bad == 0);
	}
}

TEST_SUITE("tgf cli: the REPL on the evaluator base") {

	static string tiny_grammar() {
		return string(PROJECT_SOURCE_DIR) + "/tests/fixtures/tiny.tgf";
	}

	TEST_CASE("a cut text gives the info incomplete input") {
		options_repository mine;
		options_scope scope(mine);
		tgf_repl_evaluator::options opt;
		opt.colors = false;
		tgf_repl_evaluator re(tiny_grammar(), opt);
		auto r = re.eval("p");
		REQUIRE(r.has_value());
		CHECK(r.value() == 2);
		CHECK(r.report().nodes().size() == 1);
		std::ostringstream os;
		r.report().print(os, term::colors(false));
		CHECK(os.str().find(parser_strings::messages::incomplete_input)
			!= string::npos);
	}

	TEST_CASE("an answer and a report have colors only on a terminal") {
		options_repository mine;
		options_scope scope(mine);
		auto ok = capture_tgf_run({ "tgf", tiny_grammar(), "repl",
			"-e", "parse 1" }, "", false);
		REQUIRE(ok.out.find("parsed graph:") != string::npos);
		CHECK((ok.out.find('\x1b') != string::npos)
			== term::is_tty(stdout));
		auto bad = capture_tgf_run({ "tgf", tiny_grammar(), "repl",
			"-e", "parse 1x" }, "", false);
		REQUIRE_FALSE(bad.err.empty());
		CHECK((bad.err.find('\x1b') != string::npos)
			== term::is_tty(stderr));
	}

	TEST_CASE("a colors option of the user gives colors in a pipe") {
		options_repository mine;
		options_scope scope(mine);
		auto r = capture_tgf_run({ "tgf", tiny_grammar(), "repl",
			"-c", "true", "-e", "parse 1x" }, "", false);
		CHECK(r.err.find('\x1b') != string::npos);
	}

	TEST_CASE("the debug graph prints with the tree renderer") {
		options_repository mine;
		options_scope scope(mine);
		tgf_repl_evaluator::options opt;
		opt.colors = false;
		tgf_repl_evaluator re(tiny_grammar(), opt);
		std::ostringstream out;
		std::streambuf* old = cout.rdbuf(out.rdbuf());
		re.eval("set debug true");
		out.str("");
		re.eval("parse 1");
		cout.rdbuf(old);
		CHECK(out.str().find("input command graph:\nparsed graph:\n\tstart(")
			!= string::npos);
	}
}

TEST_SUITE("tgf cli: the REPL words parse_file and igrammar") {

	static string tiny_path() {
		return string(PROJECT_SOURCE_DIR) + "/tests/fixtures/tiny.tgf";
	}

	TEST_CASE("igrammar prints what internal-grammar prints") {
		options_repository mine;
		options_scope scope(mine);
		auto old_word = capture_tgf_run({ "tgf", tiny_path(), "repl",
			"-c", "false", "-e", "internal-grammar" });
		auto new_word = capture_tgf_run({ "tgf", tiny_path(), "repl",
			"-c", "false", "-e", "igrammar" });
		REQUIRE(old_word.out.find("start") != string::npos);
		CHECK(new_word.code == 0);
		CHECK(new_word.out == old_word.out);
	}

	TEST_CASE("parse_file parses what parse file parses") {
		options_repository mine;
		options_scope scope(mine);
		const auto path = std::filesystem::temp_directory_path()
			/ "tgf_cli_parse_file_input.txt";
		{
			std::ofstream f(path, std::ios::binary);
			f << "123";
		}
		const string quoted = "\"" + path.generic_string() + "\"";
		auto old_word = capture_tgf_run({ "tgf", tiny_path(), "repl",
			"-c", "false", "-e", "parse file " + quoted });
		auto new_word = capture_tgf_run({ "tgf", tiny_path(), "repl",
			"-c", "false", "-e", "parse_file " + quoted });
		std::filesystem::remove(path);
		REQUIRE(old_word.out.find("parsed graph:") != string::npos);
		CHECK(new_word.code == 0);
		CHECK(new_word.out == old_word.out);
	}
}

#endif // TAU_TEST_HAS_TGF_CLI && PROJECT_SOURCE_DIR
