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
#include <ranges>

#include "tgf_cli.h"
#include "parser_gen.h"
#include "parser_term_color_macros.h"
#include "parser_instance.h"

#define PBOOL(bval) (( bval ) ? "true" : "false")

using namespace std;

namespace idni {

using node_variant_t = std::variant<tgf_repl_parser::symbol_type>;
using sp_node_t = idni::rewriter::sp_node<node_variant_t>;
using parser_t = tgf_repl_parser;
using traverser_t = traverser<node_variant_t, parser_t>;
const auto& get_only_child = only_child_extractor<node_variant_t, parser_t>;
const auto& get_terminals = terminal_extractor<node_variant_t, parser_t>;
const auto& get_nonterminal = nonterminal_extractor<node_variant_t, parser_t>;

// configure CLI options and descriptions
//
cli::options tgf_options() {
	cli::options os;
	os["help"] = cli::option("help", 'h', false)
		.set_description("detailed information about options");
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
		cs[lcmd][lopt].set_description(d); };

	// ---------------------------------------------------------------------

	CMD(cli::command("grammar",  "show information about grammar"));

	auto help = OPT(cli::option("help",   'h', false));
		DESC("detailed information about command options");
	auto start = OPT(cli::option("start", 's', "start"));
		DESC("starting literal");
	auto print_grammar = OPT(cli::option("grammar", 'g', true));
		DESC("prints grammar");
	//OPT(cli::option("nullable-ambiguity", 'N', false));
	//	DESC("report possible nullable ambiguity");
	//auto char_type = OPT(cli::option("char-type", 'C', "char"));
	//	DESC("type of input character");
	//auto terminal_type = OPT(cli::option("terminal-type", 'T', "char"));
	//	DESC("type of terminal character");
//
	// ---------------------------------------------------------------------

	CMD(cli::command("parse", "parse an input string, file or stdin"));

	OPT(help);
	OPT(start);
	//OPT(char_type);
	//OPT(terminal_type);
	print_grammar.set(false);
	OPT(print_grammar);
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
	//OPT(char_type);
	//OPT(terminal_type);
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

	CMD(cli::command("repl",  "run TGF repl"));

	OPT(help);
	OPT(start);
	OPT(print_grammar);
	OPT(print_ambiguity);
	OPT(print_graphs);
	OPT(tml_rules);
	OPT(tml_facts);
	OPT(error_verbosity);
	OPT(cli::option("nullable-ambiguity", 'N', false));
		DESC("report possible nullable ambiguity");
	//OPT(char_type);
	//OPT(terminal_type);

	return cs;
}

// commands
//

int show(const string& tgf_file, const string start = "start",
	bool print_grammar = true, bool print_nullable_ambiguity = false,
	bool colors = true)
{
	//DBG(cout << tgf_file << " show" <<
	//	" --start " << start <<
	//	" --grammar " << PBOOL(print_grammar) <<
	//	" --nullable-ambiguity " << PBOOL(print_nullable_ambiguity) <<
	//	"\n";)
	nonterminals<char> nts;
	auto g = tgf<char>::from_file(nts, tgf_file, start);
	if (print_grammar)
		g.print_internal_grammar(cout, {}, true, term::colors(colors));
	if (print_nullable_ambiguity) g.check_nullable_ambiguity(cout);
	return 0;
}

parser<char>::error::info_lvl str2error_verbosity(const std::string& str) {
	if (str == "detailed")   return parser<char>::error::info_lvl::INFO_DETAILED;
	if (str == "root-cause") return parser<char>::error::info_lvl::INFO_ROOT_CAUSE;
	if (str != "basic") cout << "error: invalid error-verbosity: \"" << str << "\". setting to \"basic\"\n";
	return parser<char>::error::info_lvl::INFO_BASIC;
}

int tgf_run(int argc, char** argv) {

//#ifdef DEBUG
//	cout << "== DEBUG ====================================================="
//							"=============\n";
//	cout << "argc: " << argc << "\n";
//	cout << "argv:";
//	for (size_t i = 0; i != static_cast<size_t>(argc); ++i)
//		cout << "\t" << i << ": `" << argv[i] << "`\n";
//	cout << "= /DEBUG ====================================================="
//							"=============\n\n";
//#endif

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

	cli cl("tgf", args, cmds, "repl", options);
	cl.set_description("Tau Grammar Form (TGF) tool");
	cl.set_help_header("tgf <TGF file>");

	// process args and exit if status not 0
	if (cl.process_args() != 0) return cl.status();

	auto opts = cl.get_processed_options();
	auto cmd  = cl.get_processed_command();

	// if --help/-h option is true, print help end exit
	if (opts["help"].get<bool>()) return cl.help(), 0;

	// error if command is invalid
	if (!cmd.ok()) return cl.error("invalid command", true);

	// if cmd's --help/-h option is true, print cmd's help and exit
	if (cmd.get<bool>("help")) return cl.help(cmd), 0;

	// error if TGF file is not provided or does not exist
	if (!provided) return cl.error("no TGF file specified", true);
	if (!exists) return cl.error("TGF file does not exist ", true);

	// pass cli options to repl evaluator if exists
	tgf_repl_evaluator::options tgf_repl_opt;
	if (cmd.has("status"))          	tgf_repl_opt.status =
		cmd.get<bool>("status");
	if (cmd.has("colors"))          	tgf_repl_opt.colors =
		cmd.get<bool>("colors");
	if (cmd.has("print-ambiguity")) 	tgf_repl_opt.print_ambiguity =
		cmd.get<bool>("print-ambiguity");
	if (cmd.has("print-graphs"))    	tgf_repl_opt.print_graphs =
		cmd.get<bool>("print-graphs");
	if (cmd.has("tml-rules"))       	tgf_repl_opt.tml_rules =
		cmd.get<bool>("tml-rules");
	if (cmd.has("tml-facts"))       	tgf_repl_opt.tml_facts =
		cmd.get<bool>("tml-facts");
	if (cmd.has("error-verbosity")) 	tgf_repl_opt.error_verbosity =
		str2error_verbosity(cmd.get<string>("error-verbosity"));
	if (cmd.has("start"))           	tgf_repl_opt.start =
		cmd.get<string>("start");
	tgf_repl_evaluator re(tgf_file, tgf_repl_opt);

	if (cmd.name() == "repl") {
		repl<decltype(re)> r(re, "tgf> ", ".tgf_history");
		re.set_repl(r);
		return r.run();
	}

	if (cmd.name() == "grammar") re.eval("internal-grammar");

	else if (cmd.name() == "gen") {
		std::vector<std::string> nodisambig_list;
		for (auto&& s : cmd.get<string>("nodisambig-list")
			| std::views::split(',')) nodisambig_list
					.emplace_back(s.begin(), s.end());
		generate_parser_cpp_from_file<char>(tgf_file,
			cmd.get<string>("start"), parser_gen_options
		{
			.output_dir          = cmd.get<string>("output-dir"),
			.output              = cmd.get<string>("output"),
			.name                = cmd.get<string>("name"),
			.ns                  = cmd.get<string>("namespace"),
			.char_type           = "char",
			.terminal_type       = "char",
			.decoder             = cmd.get<string>("decoder"),
			.encoder             = cmd.get<string>("encoder"),
			//.auto_disambiguate   = cmd.get<bool>("auto-disambiguate"),
			//.nodisambig_list     = nodisambig_list
		});
	}

	else if (cmd.name() == "parse") {
		stringstream ss;
		string infile = cmd.get<string>("input");
		string inexp  = cmd.get<string>("input-expression");
		if (infile.size() && inexp.size())
			return cl.error("multiple inputs specified, use ei"
				"ther --input or --input-expression, not both");
		if (cmd.get<bool>("grammar")) re.eval("i");
		if (cmd.get<bool>("print-input"))
			ss << "\ninput: \"" << re.p->get_input() << "\"\n",
			cl.info(ss.str());
		if (infile.size())
			if (infile == "-") // stdin
				re.parse(cin);
			else { // file
				re.parse(infile);
			}
		else // string
			re.parse(inexp.c_str(), inexp.size());
	}
	return 0;
}

// tgf_repl_evaluator impl

void tgf_repl_evaluator::reprompt() {
	stringstream ss;
	if (opt.status)
		ss << TC_STATUS << "[ "
		<< TC_STATUS_FILE  << "\"" << tgf_file << "\"" << TC.CLEAR()
		<< TC_STATUS << " "
		<< TC_STATUS_START << opt.start << TC.CLEAR()
		<< TC_STATUS << " ]" << TC.CLEAR() << " ";
	ss << TC_PROMPT << "tgf>" << TC.CLEAR() << " ";
	if (r) r->prompt(ss.str());
}

ostream& tgf_repl_evaluator::pretty_print(ostream& os,
	const parser_type::psptree& n, std::set<size_t> skip = {},
	bool nulls = false, size_t l = 0)
{
	auto& value = n->value;
	if (skip.size() && n->value.first.nt() &&
		skip.find(n->value.first.n()) != skip.end())
			return os;
	if (!nulls && n->value.first.is_null()) return os;
	for (size_t t = 0; t < l; t++) os << "\t";
	if (n->value.first.nt())
		os << TC_NT << value.first << TC.CLEAR() << TC_NT_ID
			<< "(" << value.first.n() << ")" << TC.CLEAR();
	else if (value.first.is_null())
		os << TC_NULL << "null" << TC.CLEAR();
	else os << TC_T << value.first << TC.CLEAR();
	os << TC_RANGE << "[" << value.second[0] << ", "
		<< value.second[1] << "]" << TC.CLEAR() << "\n";
	for (auto& d : n->child) pretty_print(os, d, skip, nulls, l + 1);
	return os;
}

tgf_repl_evaluator::tgf_repl_evaluator(const string& tgf_file)
	: tgf_file(tgf_file), nts(shared_ptr<nonterminals_type>()),
		g(make_shared<grammar_type>(
			tgf<char>::from_file(*nts, tgf_file, opt.start))),
		p(make_shared<parser_type>(*g)) {}

tgf_repl_evaluator::tgf_repl_evaluator(const string& tgf_file, options opt)
	: tgf_file(tgf_file), opt(opt),
		nts(make_shared<nonterminals_type>()),
		g(make_shared<grammar_type>(
			tgf<char>::from_file(*nts, tgf_file, opt.start))),
		p(make_shared<parser_type>(*g))
{
	TC.set(opt.colors);
}

void tgf_repl_evaluator::set_repl(repl<tgf_repl_evaluator>& r_) {
	r = &r_;
	reprompt();
}

void tgf_repl_evaluator::parsed(unique_ptr<parser_type::pforest> f) {
	auto c = f->count_trees();
	stringstream ss;
	if (f->is_ambiguous() && c > 1 && opt.print_ambiguity) {
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
		cout << ss.str(), ss = {};
	}
	if (opt.print_terminals) ss << "parsed terminals: "
		<< TC_T << terminals_to_str(*f, f->root()) << TC_CLEARED_DEFAULT
		<< "\n";
	auto cb_next_g = [&f, &ss, this](parser_type::pgraph& g) {
		f->remove_binarization(g);
		f->remove_recursive_nodes(g);
		if (opt.print_graphs) pretty_print(ss << "parsed graph:\n",
			g.extract_trees(), {}, false, 1);
		if (opt.tml_rules)  to_tml_rules<char, char, parser_type::pgraph>(
			ss << "TML rules:\n", g), ss << "\n";
		return true;
	};
	f->extract_graphs(f->root(), cb_next_g);
	if (opt.tml_facts) to_tml_facts<char, char>(ss << "TML facts:\n", *f);
	cout << ss.str();
}

void tgf_repl_evaluator::parse(const char* input, size_t size) {
	//cout << "parsing: " << input << "\n";
	parser_type::parse_options po{ .start = nts->get(opt.start) };
	auto f = p->parse(input, size, po);
	if (p->found(po.start)) parsed(std::move(f));
	else cout << p->get_error().to_str(opt.error_verbosity) << endl;
}

void tgf_repl_evaluator::parse(istream& instream) {
	parser_type::parse_options po{ .start = nts->get(opt.start) };
	auto f = p->parse(instream, po);
	if (p->found(po.start)) parsed(std::move(f));
	else cout << p->get_error().to_str(opt.error_verbosity) << endl;
}

void tgf_repl_evaluator::parse(const string& infile) {
	parser_type::parse_options po{ .start = nts->get(opt.start) };
	auto f = p->parse(infile, po);
	if (p->found(po.start)) parsed(std::move(f));
	else cout << p->get_error().to_str(opt.error_verbosity) << endl;
}

void tgf_repl_evaluator::reload(const string& new_tgf_file) {
	if (tgf_file != new_tgf_file) tgf_file = new_tgf_file;
	nts = make_shared<nonterminals_type>();
	g = make_shared<grammar_type>(tgf<char>::from_file(*nts, tgf_file, opt.start));
	p = make_shared<parser_type>(*g);
	cout << "loaded: " << tgf_file << "\n";
}

void tgf_repl_evaluator::reload() {
	reload(tgf_file);
}

size_t get_opt(traverser_t t) {
	traverser_t bool_opt = t | tgf_repl_parser::bool_option;
	if (bool_opt.has_value()) t = bool_opt;
	else {
		traverser_t list_opt = t | tgf_repl_parser::list_option;
		if (list_opt.has_value()) t = list_opt;
	}
	return t | get_only_child | get_nonterminal;
}

void tgf_repl_evaluator::get_cmd(const traverser_t& n) {
	using lvl = parser_type::error::info_lvl;
	static auto pbool = [] (bool b) { return b ? "on" : "off"; };
	static auto pverb = [] (lvl v) {
		switch (v) {
		case lvl::INFO_BASIC:      return "basic";
		case lvl::INFO_DETAILED:   return "detailed";
		case lvl::INFO_ROOT_CAUSE: return "root-cause";
		default: return "unknown";
		}
	};
	static auto plist = [] (const std::set<std::string>& l) {
		if (l.empty()) return std::string("(empty)");
		std::stringstream ss;
		bool first = true;
		for (auto& s : l) ss << (first ? first = false, "" : ", ") << s;
		return ss.str();
	};
	static std::map<size_t,	std::function<void()>> printers = {
	{ tgf_repl_parser::debug_opt,   [this]() { cout <<
		"show debug:        " << pbool(opt.debug) << "\n"; } },
	{ tgf_repl_parser::status_opt,   [this]() { cout <<
		"show status:       " << pbool(opt.status) << "\n"; } },
	{ tgf_repl_parser::colors_opt,   [this]() { cout <<
		"colors:            " << pbool(opt.colors) << "\n"; } },
	{ tgf_repl_parser::print_terminals_opt, [this]() { cout <<
		"print-terminals:   " << pbool(opt.print_terminals) << "\n"; } },
	{ tgf_repl_parser::print_graphs_opt, [this]() { cout <<
		"print-graphs:      " << pbool(opt.print_graphs) << "\n"; } },
	{ tgf_repl_parser::print_ambiguity_opt, [this]() { cout <<
		"print-ambiguity:   " << pbool(opt.print_ambiguity) << "\n"; } },
	{ tgf_repl_parser::print_rules_opt, [this]() { cout <<
		"print-rules:       " << pbool(opt.tml_rules) << "\n"; } },
	{ tgf_repl_parser::print_facts_opt, [this]() { cout <<
		"print-facts:       " << pbool(opt.tml_facts) << "\n"; } },
	{ tgf_repl_parser::auto_disambiguate_opt, [this]() { cout <<
		"auto-disambiguate: " << pbool(g->opt.auto_disambiguate) << "\n"; } },
	{ tgf_repl_parser::nodisambig_list_opt, [this]() { cout <<
		"nodisambig-list:   " << plist(g->opt.nodisambig_list) << "\n"; } },
	{ tgf_repl_parser::error_verbosity_opt, [this]() { cout <<
		"error-verbosity:   " << pverb(opt.error_verbosity) << "\n"; } }};
	auto option = n | tgf_repl_parser::option;
	if (!option.has_value()) option = n | tgf_repl_parser::bool_option;
	if (!option.has_value()) option = n | tgf_repl_parser::list_option;
	if (!option.has_value()) { for (auto& [_, v] : printers) v(); return; }
	printers[get_opt(option)]();
}

bool set_bool_value(bool& val, const size_t& vt) {
	if      (vt == tgf_repl_parser::option_value_true) val = true;
	else if (vt == tgf_repl_parser::option_value_false) val = false;
	else cout << "error: invalid bool value: " << vt << " = " <<
		parser_instance<tgf_repl_parser>().name(vt) << "\n";
	return val;
};

std::string unquote(const string& q) {
	istringstream iss(q);
	string u;
	return iss >> quoted(u), u;
};

void tgf_repl_evaluator::set_cmd(const traverser_t& n) {
	auto option = n | tgf_repl_parser::option;
	auto v  = n | tgf_repl_parser::option_value;
	auto vt = v | get_only_child | get_nonterminal;
	static std::map<size_t,	std::function<void()>> setters = {
	{ tgf_repl_parser::debug_opt,          [this, &vt]() {
		set_bool_value(opt.debug, vt); } },
	{ tgf_repl_parser::status_opt,          [this, &vt]() {
		set_bool_value(opt.status, vt); } },
	{ tgf_repl_parser::colors_opt,          [this, &vt]() {
		TC.set(set_bool_value(opt.colors, vt)); } },
	{ tgf_repl_parser::print_terminals_opt, [this, &vt]() {
		set_bool_value(opt.print_terminals, vt); } },
	{ tgf_repl_parser::print_graphs_opt, [this, &vt]() {
		set_bool_value(opt.print_graphs, vt); } },
	{ tgf_repl_parser::print_ambiguity_opt, [this, &vt]() {
		set_bool_value(opt.print_ambiguity, vt); } },
	{ tgf_repl_parser::print_rules_opt, [this, &vt]() {
		set_bool_value(opt.tml_rules, vt); } },
	{ tgf_repl_parser::print_facts_opt, [this, &vt]() {
		set_bool_value(opt.tml_facts, vt); } },
	{ tgf_repl_parser::auto_disambiguate_opt, [this, &vt]() {
		set_bool_value(g->opt.auto_disambiguate, vt); } },
	{ tgf_repl_parser::nodisambig_list_opt, [this, &v]() {
		g->opt.nodisambig_list.clear();
		for (const auto& s : (v
			| tgf_repl_parser::symbol_list
			|| tgf_repl_parser::symbol).traversers())
				g->opt.nodisambig_list.insert(s | get_terminals);
	} },
	{ tgf_repl_parser::error_verbosity, [this, &v]() {
		auto vrb = v | tgf_repl_parser::error_verbosity;
		if (!vrb.has_value()) {
			cout << "error: invalid error verbosity value\n"; return; }
		auto vrb_type = vrb | get_only_child | get_nonterminal;
		using lvl = tgf_repl_parser::parser_type::error::info_lvl;
		switch (vrb_type) {
		case tgf_repl_parser::basic_sym:
			opt.error_verbosity = lvl::INFO_BASIC; break;
		case tgf_repl_parser::detailed_sym:
			opt.error_verbosity = lvl::INFO_DETAILED; break;
		case tgf_repl_parser::root_cause_sym:
			opt.error_verbosity = lvl::INFO_ROOT_CAUSE; break;
		default: cout << "error: invalid error verbosity value\n"; return;
		}
	}}};
	setters[get_opt(option)]();
	get_cmd(n);
}

void tgf_repl_evaluator::add_cmd(const traverser_t& n) {
	auto option = n | tgf_repl_parser::list_option;
	auto add = [](auto& s, auto& l) { l.insert(s | get_terminals); };
	static std::map<size_t, std::function<void()>> adders = {
	{ tgf_repl_parser::nodisambig_list_opt, [this, &n, &add]() {
		auto v  = n | tgf_repl_parser::symbol_list;
		for (const auto& s : (v
			|| tgf_repl_parser::symbol).traversers())
				add(s, g->opt.nodisambig_list);
	}}};
	adders[get_opt(option)]();
	get_cmd(n);
}

void tgf_repl_evaluator::del_cmd(const traverser_t& n) {
	auto option = n | tgf_repl_parser::list_option;
	auto del = [](auto& s, auto& l) { l.erase(s | get_terminals); };
	static std::map<size_t,	std::function<void()>> deleters = {
	{ tgf_repl_parser::nodisambig_list_opt, [this, &n, &del]() {
		for (const auto& s : (n | tgf_repl_parser::symbol_list
			|| tgf_repl_parser::symbol).traversers())
				del(s, g->opt.nodisambig_list);
	}}};
	deleters[get_opt(option)]();
	get_cmd(n);
}

void tgf_repl_evaluator::update_bool_opt_cmd(
	const traverser_t& n,
	const std::function<bool(bool&)>& update_fn)
{
	auto option_type = n | tgf_repl_parser::bool_option
		| get_only_child | get_nonterminal;
	switch (option_type) {
	case tgf_repl_parser::debug_opt:             update_fn(opt.debug); break;
	case tgf_repl_parser::status_opt:            update_fn(opt.status); break;
	case tgf_repl_parser::colors_opt:     TC.set(update_fn(opt.colors)); break;
	case tgf_repl_parser::print_terminals_opt:   update_fn(opt.print_terminals); break;
	case tgf_repl_parser::print_graphs_opt:      update_fn(opt.print_graphs); break;
	case tgf_repl_parser::print_ambiguity_opt:   update_fn(opt.print_ambiguity); break;
	case tgf_repl_parser::print_rules_opt:       update_fn(opt.tml_rules); break;
	case tgf_repl_parser::print_facts_opt:       update_fn(opt.tml_facts); break;
	case tgf_repl_parser::auto_disambiguate_opt: update_fn(g->opt.auto_disambiguate); break;
	default: cout << ": unknown bool option\n"; break;
	}
	get_cmd(n);
}

void version() { cout << "TGF version: " << GIT_DESCRIBED << "\n"; }

// TODO (LOW) write proper help messages
void help(size_t nt = tgf_repl_parser::help_sym) {
	static const std::string bool_options =
		"  status                 show status                        on/off\n"
		"  colors                 use term colors                    on/off\n"
		"  print-ambiguity        prints ambiguous nodes             on/off\n"
		"  print-terminals        prints parsed terminals            on/off\n"
		"  print-graphs           prints parsed graphs               on/off\n"
		"  print-rules            prints parsed forest as TML rules  on/off\n"
		"  print-facts            prints parsed forest as TML facts  on/off\n";
	static const std::string list_options =
		"  nodisambig-list        list of nodes to keep ambiguous    symbol1, symbol2...\n";
	static const std::string all_available_options = std::string{} +
		"Available options:\n" + bool_options + list_options +
		"  error-verbosity        parse errors verbosity             basic/detailed/root-cause\n";
	static const std::string bool_available_options = std::string{} +
		"Available options:\n" + bool_options;
	static const std::string list_available_options = std::string{} +
		"Available options:\n" + list_options;
	switch (nt) {
	case tgf_repl_parser::help_sym: cout
		<< "tgf commands:\n"
		<< "  help or h                    print this help\n"
		<< "  help <command>               print help for a command\n"
		<< "  quit, q, exit or e           exit the repl\n"
		<< "  version or v                 print version\n"
		<< "\n"
		<< "settings commands:\n"
		<< "  get                          get options' values\n"
		<< "  set                          set option's value\n"
		<< "  toggle                       toggle option's value\n"
		<< "  enable                       set option's value to on\n"
		<< "  disable                      set option's value to off\n"
		<< "  add                          add value to the list\n"
		<< "  delete                       remove value from the list\n"
		<< "\n"
		<< "grammar commands:\n"
		<< "  grammar or n                 show TGF grammar\n"
		<< "  internal-grammar or ig or i  show TGF grammar\n"
		<< "  start or s                   show or change start symbol\n"
		<< "  unreachable or u             show unreachable productions\n"
		<< "\n"
		<< "parsing commands:\n"
		<< "  parse                        parse input\n"
		<< "  parse file or pf or f        parse input file\n"
		<< "\n";
		break;
	case tgf_repl_parser::version_sym: cout
		<< "version or v prints out current TGF commit id\n";
		break;
	case tgf_repl_parser::quit_sym: cout
		<< "command: quit or exit\n"
		<< "short: q or e\n"
		<< "\texits the repl\n";
		break;
	case tgf_repl_parser::get_sym: cout
		<< "command: get [<option>]\n"
		<< "\tprints the value of the given option\n"
		<< "\tprints all option values if no option provided\n"
		<< "\n"
		<< all_available_options;
		break;
	case tgf_repl_parser::set_sym: cout
		<< "command: set <option> [=] <value>\n"
		<< "\tsets value of the given option\n"
		<< "\n"
		<< all_available_options;
		break;
	case tgf_repl_parser::toggle_sym: cout
		<< "command: toggle <option>\n"
		<< "short: tog\n"
		<< "\t toggles value between on/off of the given option\n"
		<< "\n"
		<< bool_available_options;
		break;
	case tgf_repl_parser::enable_sym: cout
		<< "command: enable <option>\n"
		<< "short: en\n"
		<< "\tsets the value of the given option to on\n"
		<< "\n"
		<< bool_available_options;
		break;
	case tgf_repl_parser::disable_sym: cout
		<< "command: disable <option>\n"
		<< "short: dis\n"
		<< "\tsets the value of the given option to off\n"
		<< "\n"
		<< bool_available_options;
		break;
	case tgf_repl_parser::add_sym: cout
		<< "command: add <option> <value>\n"
		<< "\tadds the value to the given option list\n"
		<< "\n"
		<< list_available_options;
		break;
	case tgf_repl_parser::del_sym: cout
		<< "command: delete <option> <value>\n"
		<< "or: del, remove, rem or rm\n"
		<< "\tremoves the value from the given option list\n"
		<< "\n"
		<< list_available_options;
		break;
	case tgf_repl_parser::load_sym: cout
		<< "command: file \"TGF filepath\"\n"
		<< "short: f\n"
		<< "\tload a TGF file from drive\n";
		break;
	case tgf_repl_parser::start_sym: cout
		<< "command: start [<start symbol>]\n"
		<< "short: s\n"
		<< "\tset a new start symbol for parsing"
		<< "\tprint the current start symbol if no argument\n";
		break;
	case tgf_repl_parser::grammar_sym: cout
		<< "command: grammar\n"
		<< "short: g\n"
		<< "\tprints the actual TGF file\n";
		break;
	case tgf_repl_parser::internal_grammar_sym: cout
		<< "command: internal-grammar [<start symbol>]\n"
		<< "short: ig or i\n"
		<< "\tprints the internal grammar\n"
		<< "\tif start symbol provided prints the internal sub-grammar\n";
		break;
	case tgf_repl_parser::unreachable_sym: cout
		<< "command: unreachable [<symbol>]\n"
		<< "short: u\n"
		<< "\tprints unreachable production rules for provided symbol\n"
		<< "\tif no symbol provided prints unreachable rules for start symbol\n";
		break;
	case tgf_repl_parser::parse_sym: cout
		<< "command: parse <input>\n"
		<< "short: p\n"
		<< "\tparse the given input\n";
		break;
	case tgf_repl_parser::parse_file_sym: cout
		<< "command: parse file \"<input file>\"\n"
		<< "short: pf or f\n"
		<< "\tparse the given input file\n";
		break;
	}
}

int tgf_repl_evaluator::eval(const traverser_t& s) {
	switch (s | get_nonterminal) {
	case tgf_repl_parser::help: {
		auto optarg = s | tgf_repl_parser::cmd_symbol
			| get_only_child
			| get_nonterminal;
		if (optarg) help(optarg);
		else help();
		break;
	}
	case tgf_repl_parser::version:       version(); break;
	case tgf_repl_parser::get:           get_cmd(s); break;
	case tgf_repl_parser::set:           set_cmd(s); break;
	case tgf_repl_parser::toggle:
		update_bool_opt_cmd(s, [](bool& b){ return b = !b; }); break;
	case tgf_repl_parser::enable:
		update_bool_opt_cmd(s, [](bool& b){ return b = true; }); break;
	case tgf_repl_parser::disable:
		update_bool_opt_cmd(s, [](bool& b){ return b = false; }); break;
	case tgf_repl_parser::add:           add_cmd(s); break;
	case tgf_repl_parser::del:           del_cmd(s); break;
	case tgf_repl_parser::quit: return cout << "Quit.\n", 1;
	case tgf_repl_parser::reload_cmd: reload(); break;
	case tgf_repl_parser::load_cmd: {
		auto n = s | tgf_repl_parser::filename;
		auto filename = unquote(n | get_terminals);
		DBG(assert(filename.size());)
		reload(filename);
		break;
	}
	case tgf_repl_parser::start_cmd: {
		auto n = s | tgf_repl_parser::symbol;
		string start;
		if (n.has_value() && (start = n | get_terminals).size())
			cout << "start symbol set: " << TC_NT
				<< (opt.start = start) << TC_DEFAULT << "\n";
		else
			cout << "start symbol: " << TC_NT << opt.start
				<< TC_DEFAULT << "\n";
		break;
	}
	case tgf_repl_parser::internal_grammar_cmd: {
		auto n = s | tgf_repl_parser::symbol;
		std::string start = n.has_value() ? n | get_terminals
							: opt.start;
		g->print_internal_grammar_for(cout
			<< "\ninternal grammar for symbol "
		 	<< TC_NT << start << TC_DEFAULT << ":\n",
			start, "  ", true, TC);
		break;
	}
	case tgf_repl_parser::grammar_cmd: {
		cout << "grammar:\n";
		ifstream f(tgf_file);
		if (f) {
			string line;
			while (getline(f, line)) cout << line << "\n";
			cout << "\n";
		}
		else cout << "error: could not open file: " << tgf_file << "\n";
		break;
	}
	case tgf_repl_parser::unreachable_cmd: {
		auto n = s | tgf_repl_parser::symbol;
		std::string start = n.has_value() ? n | get_terminals
							: opt.start;
		auto unreachable = g->unreachable_productions(g->nt(start));
		if (unreachable.size()) {
			cout << "unreachable production rules for symbol: "
				<< TC_NT << start << TC_DEFAULT << "\n";
			for (auto& p : unreachable) g->print_production(
				cout << "  ", p, true, TC) << "\n";
		}
		else cout << "all production rules reachable for symbol: "
			<< TC_NT << start << TC_DEFAULT << "\n";
		break;
	}
	case tgf_repl_parser::parse_cmd: {
		auto input = s | tgf_repl_parser::parse_input | get_terminals;
		parse(input.c_str(), input.size());
		break;
	}
	case tgf_repl_parser::parse_file_cmd: {
		auto n = s | tgf_repl_parser::filename;
		auto filename = unquote(n | get_terminals);
		DBG(assert(filename.size());)
		parse(filename);
		break;
	}
	default: cout << "error: unknown command\n"; break;
	}
	return 0;
}

int tgf_repl_evaluator::eval(const string& src) {
	static tgf_repl_parser rp;
	int quit = 0;
	auto f = rp.parse(src.c_str(), src.size());
	if (!rp.found()) cout << rp.get_error().to_str() << "\n";
	else if (f) {

		auto c = f->count_trees();
		stringstream ss;
		if (f->is_ambiguous() && c > 1) {
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
			cout << ss.str(), ss = {};
		}
		if (opt.debug) {
			auto cb_next_g = [&f, this](parser_type::pgraph& g) {
				stringstream ss;
				f->remove_binarization(g);
				f->remove_recursive_nodes(g);
				ss << "parsed graph:";
				g.extract_trees()->to_print(ss, 0, {
					tgf_repl_parser::_, tgf_repl_parser::__ });
				ss << "\n\n";
				cout << ss.str();
				return true;
			};
			f->extract_graphs(f->root(), cb_next_g);
			//g->print_internal_grammar(cout << "\ngrammar:\n\n", "  ");
		}
		char dummy; // as a dummy transformer
		auto source = idni::rewriter::make_node_from_forest<
			tgf_repl_parser, char, tgf_repl_parser::node_type,
			node_variant_t>(dummy, f.get());
		auto t  = traverser_t(source);
		//std::cout << "t.has_value(): " << t.has_value() << "\n";
		//std::cout << "t.size(): " << t.values().size() << "\n";
		auto ts = t | get_terminals;
		//std::cout << "parsed terminals: " << ts << "\n";
		auto statements = t || tgf_repl_parser::statement;
		//std::cout << "statements.has_value(): " << statements.has_value() << "\n";
		//std::cout << "statements.size(): " << statements.values().size() << "\n";

		for (const auto& statement : statements())
			if ((quit = eval(statement | get_only_child)))
				return quit;
	}
	if (quit == 0) reprompt();
	return quit;
}

} // namespace idni