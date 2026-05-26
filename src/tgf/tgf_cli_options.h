// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_CLI_OPTIONS_H__
#define __IDNI__PARSER__TGF__TGF_CLI_OPTIONS_H__

// configure CLI options and descriptions
//

#include "../utility/cli.h"

namespace idni {

inline cli::options tgf_options() {
	cli::options os;
	os["help"] = cli::option("help", 'h', false)
		.set_description("detailed information about options");
	os["version"] = cli::option("version", 'v', false)
		.set_description("print version and exit");
	os["license"] = cli::option("license", 'L', false)
		.set_description("print license and exit");
	return os;
}

// configure CLI commands, their options and descriptions
//
inline cli::commands tgf_commands() {
	cli::commands cs;
	std::string lcmd = "", lopt = "";
	auto CMD  = [&](const struct cli::command& cmd) {
		lcmd = cmd.name(), cs[lcmd] = cmd; };
	auto OPT  = [&](const cli::option& opt) {
		lopt = opt.name(), cs[lcmd].add_option(opt);
		return cs[lcmd][lopt]; };
	auto DESC = [&](const std::string& d) {
		cs[lcmd][lopt].set_description(d); };

	// ---------------------------------------------------------------------

	CMD(cli::command("grammar",  "show information about grammar"));

	auto help = OPT(cli::option("help",   'h', false));
		DESC("detailed information about command options");
	auto start = OPT(cli::option("start", 's', ""));
		DESC("starting literal");
	auto print_grammar = OPT(cli::option("grammar", 'g', true));
		DESC("prints grammar");
	auto print_json = OPT(cli::option("json", 'J', false));
		DESC("respond with JSON");
	auto measure = OPT(cli::option("measure", 'm', false));
		DESC("benchmark: measure parse time and emit the diagnostics "
			"report");
	auto colors = OPT(cli::option("colors", 'c', true));
		DESC("enable terminal colors for output");
	OPT(cli::option("nullable", 'N', false));
		DESC("report nullable recursive productions");
	OPT(print_json);
	CMD(cli::command("parse", "parse an input string, file or stdin"));

	OPT(help);
	OPT(start);
	//OPT(char_type);
	//OPT(terminal_type);
	print_grammar.set(false);
	OPT(print_grammar);
	OPT(print_json);
	OPT(measure);
	OPT(colors);
	OPT(cli::option("input", 'i',
		"")),
		DESC("parse input from file or STDIN if -");
	OPT(cli::option("input-expression", 'e',
		"")),
		DESC("parse input from provided string");
	auto print_ambiguity = OPT(cli::option("print-ambiguity", 'a',
		true));
		DESC("prints ambiguity info, incl. ambig. nodes");
	OPT(cli::option("print-input", 'I',
		false)),
		DESC("prints input");
	OPT(cli::option("terminals", 't',
		true)),
		DESC("prints all parsed terminals serialized");
	auto error_verbosity = OPT(cli::option("error-verbosity", 'v',
		"basic"));
		DESC("error info verbosity: basic, detailed, root-cause");
	auto tree_path_opt = OPT(cli::option("tree-path", 'P',
		"bintree"));
		DESC("parse tree path: bintree or forest");
	auto auto_disam_opt = OPT(cli::option("auto-disambiguate", 'a',
		true));
		DESC("enables auto-disambiguation");
	auto print_graphs = OPT(cli::option("print-graphs", 'p',
		true));
		DESC("prints parsed graph");
	auto tml_rules = OPT(cli::option("tml-rules", 'r',
		false));
		DESC("prints parsed graph in tml rules");
	auto tml_facts = OPT(cli::option("tml-facts", 'f',
		false));
		DESC("prints parsed graph in tml facts");

	// ---------------------------------------------------------------------

	CMD(cli::command("gen", "generate a parser code"));

	OPT(help);
	OPT(start);
	OPT(print_json);
	OPT(cli::option("char-type", 'C', "char")),
		DESC("input character type (e.g. char, char32_t)");
	OPT(cli::option("terminal-type", 'T', "char")),
		DESC("terminal character type (e.g. char, char32_t)");
	OPT(cli::option("utf8", 'U', false)),
		DESC("shorthand: --char-type char --terminal-type char32_t "
			"--decoder idni::utf8_to_u32_conv "
			"--encoder idni::u32_to_utf8_conv");
	OPT(cli::option("name", 'n',
		"")),
		DESC("name of the generated parser struct");
	OPT(cli::option("output-dir", 'O',
		"")),
		DESC("output directory for output files");
	OPT(cli::option("output", 'o',
		"")),
		DESC("output file for parser");
	OPT(cli::option("auto-disambiguate", 'a',
		true)),
		DESC("enables auto-disambiguation");
	OPT(cli::option("nodisambig-list", 'A',
		"")),
		DESC("comma separated list of non-disambiguation nodes");
	OPT(cli::option("namespace", 'N',
		"")),
		DESC("namespace for the generated parser code");
	OPT(cli::option("decoder", 'd',
		"")),
		DESC("decoder function");
	OPT(cli::option("encoder", 'e',
		"")),
		DESC("encoder function");

	// ---------------------------------------------------------------------

	CMD(cli::command("repl", "run TGF repl"));

	OPT(help);
	OPT(start);
	OPT(print_grammar);
	OPT(print_json);
	OPT(print_ambiguity);
	OPT(print_graphs);
	OPT(tml_rules);
	OPT(tml_facts);
	OPT(error_verbosity);
	OPT(measure);
	OPT(colors);
	OPT(cli::option("evaluate", 'e', ""));
		DESC("run repl command with input to evaluate and quit");
	//OPT(char_type);
	//OPT(terminal_type);

	// ---------------------------------------------------------------------

	CMD(cli::command("test", "run tester (with provided tgf.test file)"));
	OPT(help);

	return cs;
}

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_CLI_OPTIONS_H__