// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the TGF CLI surface - subcommand routing and option defaults.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/tgf/tgf_cli.h"

#include <sstream>

using namespace std;
using namespace idni;

// ---------------------------------------------------------------------------
// helpers (mirrored from tests/doctest_cli.cpp, plus cmd_options check)
// ---------------------------------------------------------------------------

struct test_options {
	string name = "";
	vector<string> args = {};
	cli::commands cmds = {};
	string dflt_cmd = "";
	cli::options opts = {};
	string desc = "";
	string help_header = "";
};

struct test_expected {
	int status = -1;
	string cmd_name = "";
	map<string, cli::option::value> options = {};
	map<string, cli::option::value> cmd_options = {};
};

static ostream cnull(0);

static string args_to_string(const vector<string>& args) {
	string s;
	for (auto& a : args) { s += a; s += ' '; }
	return s;
}

static string value_to_string(const cli::option::value& v) {
	if (holds_alternative<string>(v)) return "'" + get<string>(v) + "'";
	if (holds_alternative<bool>(v))   return get<bool>(v) ? "true" : "false";
	if (holds_alternative<int>(v))    return to_string(get<int>(v));
	return "<unknown>";
}

// Run TGF CLI with given options, assert expected status / cmd / options /
// cmd_options. On failure, FAIL_CHECK reports the args plus what mismatched.
static void expect_cli(const test_options& o, const test_expected& exp) {
	cli cl(o.name, o.args, o.cmds, o.dflt_cmd, o.opts);
	if (o.desc.size())        cl.set_description(o.desc);
	if (o.help_header.size()) cl.set_help_header(o.help_header);
	cl.set_output_stream(cnull);
	cl.set_error_stream(cnull);

	cl.process_args();
	auto opts = cl.get_processed_options();
	auto cmd  = cl.get_processed_command();
	string argstr = args_to_string(o.args);

	if (exp.status != -1 && cl.status() != exp.status) {
		FAIL_CHECK("args [" << argstr << "]: expected status="
			<< exp.status << ", got status=" << cl.status());
		return;
	}
	if (cl.status() == 1) return; // no further checks meaningful

	for (auto& [name, value] : exp.options) {
		if (opts.find(name) == opts.end()) {
			FAIL_CHECK("args [" << argstr
				<< "]: expected global option '" << name
				<< "' not found");
			continue;
		}
		if (opts[name].get() != value)
			FAIL_CHECK("args [" << argstr << "]: global option '"
				<< name << "' expected " << value_to_string(value)
				<< ", got " << value_to_string(opts[name].get()));
	}

	for (auto& [name, value] : exp.cmd_options) {
		if (!cmd.has(name)) {
			FAIL_CHECK("args [" << argstr
				<< "]: expected cmd option '" << name
				<< "' not found");
			continue;
		}
		if (cmd[name].get() != value)
			FAIL_CHECK("args [" << argstr << "]: cmd option '"
				<< name << "' expected " << value_to_string(value)
				<< ", got " << value_to_string(cmd[name].get()));
	}

	if (exp.cmd_name.size() && cmd.name() != exp.cmd_name)
		FAIL_CHECK("args [" << argstr << "]: expected cmd_name='"
			<< exp.cmd_name << "', got '" << cmd.name() << "'");
}

// ---------------------------------------------------------------------------
// TEST SUITE: subcommand routing
// ---------------------------------------------------------------------------

TEST_SUITE("tgf cli: subcommands") {

	test_options o { "tgf", {}, tgf_commands(), "repl", tgf_options() };

	TEST_CASE("default command is repl") {
		o.args = { "tgf" };
		expect_cli(o, { .status = 0, .cmd_name = "repl" });
	}

	TEST_CASE("invalid subcommand") {
		o.args = { "tgf", "invalid" };
		expect_cli(o, { .status = 1 });
	}

	TEST_CASE("grammar subcommand") {
		o.args = { "tgf", "grammar" };
		expect_cli(o, { .status = 0, .cmd_name = "grammar" });
	}

	TEST_CASE("gen subcommand") {
		o.args = { "tgf", "gen" };
		expect_cli(o, { .status = 0, .cmd_name = "gen" });
	}

	TEST_CASE("parse subcommand") {
		o.args = { "tgf", "parse" };
		expect_cli(o, { .status = 0, .cmd_name = "parse" });
	}

	TEST_CASE("repl subcommand") {
		o.args = { "tgf", "repl" };
		expect_cli(o, { .status = 0, .cmd_name = "repl" });
	}

	TEST_CASE("repl --legacy-repl defaults to false") {
		o.args = { "tgf", "repl" };
		expect_cli(o, { .status = 0, .cmd_name = "repl",
			.cmd_options = { { "legacy-repl", false } } });
	}

	TEST_CASE("repl -X enables legacy terminal REPL") {
		o.args = { "tgf", "repl", "-X" };
		expect_cli(o, { .status = 0, .cmd_name = "repl",
			.cmd_options = { { "legacy-repl", true } } });
	}

	TEST_CASE("repl --legacy-repl long form") {
		o.args = { "tgf", "repl", "--legacy-repl" };
		expect_cli(o, { .status = 0, .cmd_name = "repl",
			.cmd_options = { { "legacy-repl", true } } });
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: gen subcommand options
// ---------------------------------------------------------------------------

TEST_SUITE("tgf cli: gen options") {

	test_options o { "tgf", {}, tgf_commands(), "repl", tgf_options() };

	TEST_CASE("gen default options") {
		o.args = { "tgf", "gen" };
		expect_cli(o, { .status = 0, .cmd_name = "gen",
			.cmd_options = {
				{ "help",    false },
				{ "name",    "" },
				{ "decoder", "" },
				{ "encoder", "" },
				{ "output",  "" },
			} });
	}

	TEST_CASE("gen with --name and --help") {
		o.args = { "tgf", "gen", "--name", "my_other_parser",
			"--help" };
		expect_cli(o, { .status = 0, .cmd_name = "gen",
			.cmd_options = {
				{ "help",    true },
				{ "name",    "my_other_parser" },
				{ "decoder", "" },
				{ "encoder", "" },
				{ "output",  "" },
			} });
	}

	TEST_CASE("gen with invalid option") {
		o.args = { "tgf", "gen", "--invalid" };
		expect_cli(o, { .status = 1 });
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: global / default-command options
// ---------------------------------------------------------------------------

TEST_SUITE("tgf cli: global options") {

	test_options o { "tgf", {}, tgf_commands(), "repl", tgf_options() };

	TEST_CASE("grammar command accepts -N for nullable") {
		o.args = { "tgf", "grammar", "-N" };
		expect_cli(o, { .status = 0, .cmd_name = "grammar",
			.cmd_options = {
				{ "nullable", true },
			} });
	}

	TEST_CASE("invalid global option") {
		o.args = { "tgf", "--invalid" };
		expect_cli(o, { .status = 1 });
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
