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
#include <cassert>
#include <string.h>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include "tgf_cli.h"
#include "parser_gen.h"

#define PBOOL(bval) (( bval ) ? "true" : "false")

using namespace std;

namespace idni {

// configure CLI options and descriptions
//
cli::options tgf_options() {
	cli::options os;
	os["help"] = cli::option("help", 'h', false)
		.desc("detailed information about options");
	return os;
}

// configure CLI commands, their options and descriptions
//
cli::commands tgf_commands() {
	cli::commands cs;
	string lcmd = "", lopt = "";
	auto CMD  = [&](const struct cli::command& cmd) {
		lcmd = cmd.name(), cs[lcmd] = cmd; };
	auto OPT  = [&](const cli::option& opt) {
		lopt = opt.name(), cs[lcmd].add_option(opt);
		return cs[lcmd][lopt]; };
	auto DESC = [&](const string& d) {
		cs[lcmd][lopt].desc(d); };

	// ---------------------------------------------------------------------

	CMD(cli::command("show",  "show information about grammar / parser"));

	auto help = OPT(cli::option("help",   'h', false));
		DESC("detailed information about command options");
	auto start = OPT(cli::option("start", 's', "start"));
		DESC("starting literal");
	auto print_grammar = OPT(cli::option("grammar", 'g', true));
		DESC("prints grammar");
	OPT(cli::option("nullable-ambiguity", 'N', false));
		DESC("report possible nullable ambiguity");
	auto char_type = OPT(cli::option("char-type", 'C', "char"));
		DESC("type of input character");
	auto terminal_type = OPT(cli::option("terminal-type", 'T', "char"));
		DESC("type of terminal character");

	// ---------------------------------------------------------------------

	CMD(cli::command("parse", "parse an input string, file or stdin"));

	OPT(help);
	OPT(start);
	OPT(char_type);
	OPT(terminal_type);
	print_grammar.set(false);
	OPT(print_grammar);
	OPT(cli::option("input", 'i',
		"")),
		DESC("parse input from file or STDIN if -");
	OPT(cli::option("input-expression", 'e',
		"")),
		DESC("parse input from provided string");
	OPT(cli::option("print-ambiguity", 'a',
		true)),
		DESC("prints ambiguity info, incl. ambig. nodes");
	OPT(cli::option("print-input", 'I',
		false)),
		DESC("prints input");
	OPT(cli::option("terminals", 't',
		true)),
		DESC("prints all parsed terminals serialized");
	OPT(cli::option("detailed-error", 'd',
		false)),
		DESC("parse error is more verbose");
	OPT(cli::option("root-cause-error", 'D',
		false)),
		DESC("parse error is more verbose");
	OPT(cli::option("print-graphs", 'p',
		false)),
		DESC("prints parsed graph");
	OPT(cli::option("tml-rules", 'r',
		false)),
		DESC("prints parsed graph in tml rules");
	OPT(cli::option("tml-facts", 'f',
		false)),
		DESC("prints parsed graph in tml facts");

	// ---------------------------------------------------------------------

	CMD(cli::command("gen", "generate a parser code"));

	OPT(help);
	OPT(start);
	OPT(char_type);
	OPT(terminal_type);
	OPT(cli::option("name", 'n',
		"my_parser")),
		DESC("name of the generated parser struct");
	OPT(cli::option("decoder", 'd',
		"")),
		DESC("decoder function");
	OPT(cli::option("encoder", 'e',
		"")),
		DESC("encoder function");
	OPT(cli::option("output", 'o',
		"-")),
		DESC("output file");

	return cs;
}

// commands
//

int show(const string& tgf_file, const string start = "start",
	bool print_grammar = true, bool print_nullable_ambiguity = false)
{
	DBG(cout << tgf_file << " show" <<
		" --start " << start <<
		" --grammar " << PBOOL(print_grammar) <<
		" --nullable-ambiguity " << PBOOL(print_nullable_ambiguity) <<
		"\n";)
	nonterminals<char> nts;
	auto g = tgf<char>::from_file(nts, tgf_file, start);
	if (print_grammar) g.print_internal_grammar(cout);
	if (print_nullable_ambiguity) g.check_nullable_ambiguity(cout);
	return 0;
}

int gen(ostream& os, const string& tgf_file, const string name = "my_parser",
	const string start = "start", const string char_type = "char",
	const string terminal_type = "char", const string decoder = "",
	const string encoder = "")
{
	DBG(cout << tgf_file << " gen" <<
		" --name " << name <<
		" --char-type " << char_type <<
		" --terminal-type " << terminal_type <<
		" --start " << start <<
		" --decoder \"" << decoder << "\"" <<
		" --encoder \"" << encoder << "\"\n";)
	generate_parser_cpp_from_file<char>(os, name, tgf_file, start,
		char_type, terminal_type, decoder, encoder);
	return 0;
}

int parsed(std::unique_ptr<parser<char>::pforest> f, const cli& cl,
	cli::command& cmd)
{
	auto c = f->count_trees();
	stringstream ss;
	if (f->is_ambiguous() && c > 1 &&
		cmd.get<bool>("print-ambiguity"))
	{
		ss << "\nambiguity... number of trees: " << c << "\n\n";
		for (auto& n : f->ambiguous_nodes()) {
			ss << "\t `" << n.first.first << "` [" <<
				n.first.second[0] << "," <<
				n.first.second[1] << "]\n";
			size_t d = 0;
			for (auto ns : n.second) {
				ss << "\t\t " << d++ << "\t";
				for (auto nt : ns) ss << " `" <<
					nt.first << "`[" << nt.second[0]
					<< "," << nt.second[1] << "] ";
				ss << "\n";
			}
		}
		ss << "\n";
		cl.info(ss.str()), ss = {};
	}
	struct {
		bool graphs, facts, rules, terminals;
	} print(cmd.get<bool>("print-graphs"),
		cmd.get<bool>("tml-facts"),
		cmd.get<bool>("tml-rules"),
		cmd.get<bool>("terminals"));
	auto cb_next_g = [&f, &cl, &print](parser<char>::pgraph& g) {
		stringstream ss;
		f->remove_binarization(g);
		f->remove_recursive_nodes(g);
		if (print.graphs) {
			static size_t c = 1;
			ss << "parsed graph";
			if (c++ > 1) ss << " " << c;
			ss << ":";
			g.extract_trees()->to_print(ss);
			ss << "\n\n";
			cl.info(ss.str()), ss = {};
		}
		if (print.rules) to_tml_rules<char, char, parser<char>::pgraph>(
			ss << "TML rules:\n", g), ss << "\n", cl.info(ss.str());
		return true;
	};
	f->extract_graphs(f->root(), cb_next_g);
	if (print.facts) to_tml_facts<char, char>(ss << "TML facts:\n", *f);
	if (print.terminals) ss << "terminals parsed: \"" <<
		terminals_to_str(*f, f->root()) << "\"\n\n";
	cl.info(ss.str());
	return 0;
}

int tgf_run(int argc, char** argv) {

#ifdef DEBUG
	cout << "== DEBUG ====================================================="
							"=============\n";
	cout << "argc: " << argc << "\n";
	cout << "argv:";
	for (size_t i = 0; i != static_cast<size_t>(argc); ++i)
		cout << "\t" << i << ": `" << argv[i] << "`\n";
	cout << "= /DEBUG ====================================================="
							"=============\n\n";
#endif

	auto cmds = tgf_commands();
	auto options = tgf_options();

	// move argv into args and extract TGF filename if provided
	vector<string> args;
	string tgf_file{};
	bool provided = false;
	bool exists = false;
	for (int i = 0; i != argc; ++i)
		if (i == 1 && argv[i][0] != '-') {
			// first arg is TGF file or cmd
			if (cmds.find(argv[i]) != cmds.end())
				args.push_back(argv[i]);
			else provided = true,
				exists = filesystem::exists(tgf_file = argv[i]);
		} else args.push_back(argv[i]);

	cli cl("tgf", args, cmds, "show", options);
	cl.set_description("Tau Grammar Form (TGF) tool");
	cl.set_help_header("tgf <TGF file> ");

	// process args and exit if status not 0
	if (cl.process_args() != 0) return cl.status();

	auto opts = cl.get_processed_options();
	auto cmd  = cl.get_processed_command();

	// if --help/-h option is true, print help end exit
	if (opts["help"].get<bool>()) return cl.help(cout), 0;

	// error if command is invalid
	if (!cmd.ok()) return cl.error("invalid command", true);

	// if cmd's --help/-h option is true, print cmd's help and exit
	if (cmd.get<bool>("help")) return cl.help(cout, cmd), 0;

	// error if TGF file is not provided or does not exist
	if (!provided) return cl.error("no TGF file specified", true);
	if (!exists) return cl.error("TGF file does not exist ", true);

	if (cmd.name() == "show") {
		show(tgf_file,
			cmd.get<string>("start"),
			cmd.get<bool>("grammar"),
			cmd.get<bool>("nullable-ambiguity"));
	}

	else if (cmd.name() == "gen") {
		gen(cout, tgf_file,
			cmd.get<string>("name"),
			cmd.get<string>("start"),
			cmd.get<string>("char-type"),
			cmd.get<string>("terminal-type"),
			cmd.get<string>("decoder"),
			cmd.get<string>("encoder"));
	}

	else if (cmd.name() == "parse") {
		std::stringstream ss;
		string infile = cmd.get<string>("input");
		string inexp  = cmd.get<string>("input-expression");
		if (infile.size() && inexp.size())
			return cl.error("multiple inputs specified, use ei"
				"ther --input or --input-expression, not both");
		nonterminals nts;
		grammar g = tgf<char>::from_file(nts, tgf_file,
						cmd.get<string>("start"));
		if (cmd.get<bool>("grammar"))
			g.print_internal_grammar(ss << "\ngrammar:\n\n", "  ")
				<< "\n", cl.info(ss.str()), ss = {};
		parser p(g);
		std::unique_ptr<parser<char>::pforest> f;
		if (infile.size())
			if (infile == "-") // stdin
				f = p.parse(cin);
			else { // file
				f = p.parse(infile, MMAP_READ);
			}
		else // string
			f = p.parse(inexp.c_str(), inexp.size());
		if (cmd.get<bool>("print-input"))
			ss << "\ninput: \"" << p.get_input() << "\"\n",
			cl.info(ss.str());
		using lvl = decltype(p)::error::info_lvl;
		if (!p.found()) return cl.error(p.get_error().to_str(
			cmd.get<bool>("root-cause-error")
				? lvl::INFO_ROOT_CAUSE
				: cmd.get<bool>("detailed-error")
					? lvl::INFO_DETAILED
					: lvl::INFO_BASIC));
		parsed(std::move(f), cl, cmd);
	}
	return 0;
}

} // namespace idni