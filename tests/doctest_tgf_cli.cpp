// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the TGF CLI surface - subcommand routing and option defaults.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/tgf/tgf_cli.h"

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
