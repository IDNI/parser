// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.

#include "../tools/tgf/tgf_cli.h"

#include "testing.h"

using namespace std;
using namespace idni;

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

ostream cnull(0);

bool run_test(const test_options& o, const test_expected& exp) {

	if (!testing::check()) return testing::info(cout), true;
	std::stringstream os;
	if (!testing::info(os)) return true;

	auto value2str = [](const cli::option::value& v) {
		if (holds_alternative<string>(v)) return get<string>(v);
		if (holds_alternative<bool>(v)) return string(get<bool>(v)
							? "true" : "false");
		if (holds_alternative<int>(v)) return to_string(get<int>(v));
		return string("");
	};

	if (testing::verbosity > 0) {
		// print out test_options
		os << "\ntest_options:" << "\n";
		os << "  name: " << o.name << "\n";
		os << "  args: ";
		for (auto& arg : o.args) os << arg << " ";
		os << "\n";
		os << "  cmds: ";
		for (auto& [name, cmd] : o.cmds) os << name << " ";
		os << "\n";
		os << "  dflt_cmd: " << o.dflt_cmd << "\n";
		os << "  opts: ";
		for (auto& [name, opt] : o.opts) os << "--" << name << " ";
		os << "\n";
		os << "  desc: " << o.desc << "\n";
		os << "  help_header: " << o.help_header << "\n";
		// print out test_expected
		os << "test_expected:" << "\n";
		os << "  status: " << exp.status << "\n";
		os << "  cmd_name: " << exp.cmd_name << "\n";
		os << "  options: ";
		for (auto& [name, value] : exp.options)
			os << name << ": " << value2str(value) << " ";
		os << "\n\n";
	}

	cli cl(o.name, o.args, o.cmds, o.dflt_cmd, o.opts);
	if (o.desc.size())        cl.set_description(o.desc);
	if (o.help_header.size()) cl.set_help_header(o.help_header);
	cl.set_output_stream(cnull);
	cl.set_error_stream(cnull);

	cl.process_args();
	auto opts = cl.get_processed_options();
	auto cmd = cl.get_processed_command();

	bool failed = true;

	if (exp.status != -1 && cl.status() != exp.status) {
		os << "expected status " << exp.status << ", got "
			<< cl.status() << "\n";
		goto end;
	}

	// if processing failed, we can't check anything else
	if (cl.status() == 1) { failed = false; goto end; }

	for (auto& [name, value] : exp.options) {
		if (opts.find(name) == opts.end()) {
			os << "expected option " << name << " not found"
				<< "\n";
			goto end;
		}
		if (opts[name].get() != value) {
			os << "expected option " << name << " value "
				<< value2str(value) << ", got "
				<< value2str(opts[name].get()) << "\n";
			goto end;
		}
	}
	if (exp.cmd_name.size() && cmd.name() != exp.cmd_name) {
		os << "expected cmd_name " << exp.cmd_name << ", got "
			<< cmd.name() << "\n";
		goto end;
	}
	failed = false;

end:
	if (!testing::print_only_failed || failed) std::cout << os.str();

	return true;
}

int main(int argc, char** argv) {
	testing::process_args(argc, argv);

	test_options o2{	"test", {}, tgf_commands(), "show", tgf_options() };
	test_options o{ "test", {}, {}, "", {
		{ "bool",   { "bool",   'b', false } },
		{ "int",    { "int",    'i', -1 } },
		{ "string", { "string", 's', "" } },
	} };

	TEST("options", "invalid")
	o.args = { "cmd", "--invalid" };
	run_test(o, { .status = 1 });

	TEST("options", "bool")
	o.args = { "cmd" };
	run_test(o, { .status = 0, .options = { { "bool", false } }});
	o.args = { "cmd", "--bool" };
	run_test(o, { .status = 0, .options = { { "bool", true } }});
	for (auto& v : { "1", "t", "on", "yes", "true" }) {
		o.args = { "cmd", "--bool", v };
		run_test(o, { .status = 0, .options = { { "bool", true } }});
	}
	for (auto& v : { "0", "f", "off", "no", "false", "any", "2" }) {
		o.args = { "cmd", "--bool", v };
		run_test(o, { .status = 0, .options = { { "bool", false } }});
	}

	TEST("options", "int")
	o.args = { "cmd" }; // default value
	run_test(o, { .status = 0, .options = { { "int", -1 } }});
	o.args = { "cmd", "--int" }; // missing value
	run_test(o, { .status = 1 });
	o.args = { "cmd", "--int", "532" };
	run_test(o, { .status = 0, .options = { { "int", 532 } }});
	o.args = { "cmd", "--int", "-42" };
	run_test(o, { .status = 0, .options = { { "int", -42 } }});
	o.args = { "cmd", "--int", "invalid" };
	run_test(o, { .status = 1 });

	TEST("options", "string")
	o.args = { "cmd" }; // default value
	run_test(o, { .status = 0, .options = { { "string", "" } }});
	o.args = { "cmd", "--string" }; // missing value
	run_test(o, { .status = 1 });
	o.args = { "cmd", "--string", "ok" };
	run_test(o, { .status = 0, .options = { { "string", "ok" } }});
	o.args = { "cmd", "--string", "hello world" };
	run_test(o, { .status = 0, .options = { { "string", "hello world" } }});

	TEST("options", "multiple")
	o.args = { "cmd", "--bool", "--int", "-2", "--string", "ok" };
	run_test(o, { .status = 0, .options = {
		{ "bool", true },
		{ "int", -2 },
		{ "string", "ok" }
	}});

	TEST("commands", "default")
	o2.args = { "cmd" };
	run_test(o2, { .status = 0, .cmd_name = "show" });

	TEST("commands", "invalid")
	o2.args = { "cmd", "invalid" };
	run_test(o2, { .status = 1 });

	TEST("commands", "valid")
	o2.args = { "cmd", "show" };
	run_test(o2, { .status = 0, .cmd_name = "show" });
	o2.args = { "cmd", "gen" };
	run_test(o2, { .status = 0, .cmd_name = "gen" });
	o2.args = { "cmd", "parse" };
	run_test(o2, { .status = 0, .cmd_name = "parse" });

	TEST("command options", "gen with default options")
	o2.args = { "cmd", "gen" };
	run_test(o2, { .status = 0, .cmd_name = "gen", .options = {
		{ "help", false }
	}, .cmd_options = {
		{ "help", false },
		{ "name", "my_parser" },
		{ "decoder", "" },
		{ "encoder", "" },
		{ "output", "-" }
	}});

	TEST("command options", "gen command with invalid option")
	o2.args = { "cmd", "gen", "--invalid" };
	run_test(o2, { .status = 1, .cmd_name = "gen" });

	TEST("command options", "default command with valid option")
	o2.args = { "cmd", "--grammar" };
	run_test(o2, { .status = 0, .cmd_name = "show", .cmd_options = {
		{ "grammar", true }
	}});

	TEST("command options", "default command with invalid option")
	o2.args = { "cmd", "--invalid" };
	run_test(o2, { .status = 1, .cmd_name = "show" });

	TEST("command options", "gen command with name and help")
	o2.args = { "cmd", "gen", "--name", "my_other_parser", "--help" };
	run_test(o2, { .status = 0, .cmd_name = "gen", .options = {
		{ "help", false }
	}, .cmd_options = {
		{ "help", true },
		{ "name", "my_other_parser" },
		{ "decoder", "" },
		{ "encoder", "" },
		{ "output", "-" }
	}});

	cout << "\n";
	if (testing::failed) cout << "FAILED\n";
	return testing::failed ? 1 : 0;
}