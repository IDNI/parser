// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <cassert>
#include <string.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <ranges>

#include "parser.h"
#ifndef DEBUG
#include "utility/devhelpers.h"   // various helpers for converting forest
#endif
#include "tgf_cli.h"
#include "tgf.h"
#include "parser_gen.h"
#include "parser_term_color_macros.h"
#include "tgf_test.h"
#include "tgf_cli_options.h"
#include "parser_strings.h"

#define PBOOL(bval) (( bval ) ? "true" : "false")

using namespace std;

namespace idni {

using tt = tgf_repl_parser::tree::traverser;

// commands
//

static void print_diagnostics_report(
	const idni::diagnostics::report& report,
	bool json, bool print_names = true)
{
	if (report.nodes().empty()) return;
	if (json) report.to_json(cout, print_names) << '\n';
	else report.print();
}

int show(const string& tgf_file, string start = {},
	bool print_grammar = true,
	bool print_nullable_recursive_production = false,
	bool measure = false,
	bool print_json = false)
{
	nonterminals<char> nts;
	auto gr = tgf<char>::from_file(nts, tgf_file, measure || print_json);
	if (!gr.has_value()) {
		print_diagnostics_report(gr.report(), print_json);
		return 1;
	}
	const auto& g = gr.value();
	if (start.empty()) start = g.start_literal().to_std_string();
	if (print_grammar) g.print_internal_grammar_for(
		cout, start, {}, true);
	if (print_nullable_recursive_production)
		g.check_nullable_recursive_production(cout);
	print_diagnostics_report(gr.report(), print_json);
	return 0;
}

tgf_repl_evaluator::parser_type::error::info_lvl
	str2error_verbosity(const string& str)
{
	using lvl = tgf_repl_evaluator::parser_type::error::info_lvl;
	if (str == "detailed")   return lvl::INFO_DETAILED;
	if (str == "root-cause") return lvl::INFO_ROOT_CAUSE;
	if (str != "basic") cerr << "error: invalid error-verbosity: "
				"\"" << str << "\". setting to \"basic\"\n";
	return lvl::INFO_BASIC;
}

int run_tests(shared_ptr<tgf_repl_evaluator::parser_type> p,
	const vector<string>& files)
{
	tgf_test<char> t;
	int ret = 0;
	for (const auto& file : files) {
		std::cout << "running test: " << file << std::endl;
		auto r = t.run_from_file(p, file);
		if (r) ret = r;
	}
	return ret;
}

void print_version() { cout << tauparser::full_version << "\n"; }

void print_license() { cout << tauparser::license << "\n"; }

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

	// process CLI arguments
	// ---------------------------------------------------------------------


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
	cl.set_description("Tau Grammar Format (TGF) tool");
	cl.set_help_header("tgf <TGF file>");

	// process args and exit if status not 0
	if (cl.process_args() != 0) return cl.status();

	auto opts = cl.get_processed_options();
	auto cmd  = cl.get_processed_command();

	// if --version/v option is true, print version...
	// if --license/L option is true, print license...
	// if --help/-h   option is true, print help...
	//                                                 ...and exit
	bool quit = false;
	if (opts["version"].get<bool>()) quit = true, print_version();
	if (opts["license"].get<bool>()) quit = true, print_license();
	if (opts["help"]   .get<bool>()) quit = true, cl.help();
	if (quit) return 0;

	// error if command is invalid
	if (!cmd.ok()) return cl.error("invalid command", true);

	// if cmd's --help/-h option is true, print cmd's help and exit
	if (cmd.get<bool>("help")) return cl.help(cmd), 0;

	// error if TGF file is not provided or does not exist
	if (!provided) return cl.error("no TGF file specified", true);
	if (!exists) return cl.error("TGF file does not exist ", true);

	if (cmd.has("colors")) TC.set(cmd.get<bool>("colors"));

	// GRAMMAR COMMAND
	// ---------------------------------------------------------------------

	if (cmd.name() == "grammar")
		return show(tgf_file, cmd.get<string>("start"),
			cmd.get<bool>("grammar"),
			cmd.get<bool>("nullable"),
			cmd.get<bool>("measure"),
			cmd.get<bool>("json"));

	// GEN COMMAND
	// ---------------------------------------------------------------------

	if (cmd.name() == "gen") {
		vector<string> nodisambig_list;
		for (auto&& s : cmd.get<string>("nodisambig-list")
			| views::split(',')) nodisambig_list
					.emplace_back(s.begin(), s.end());
		string char_type     = cmd.get<string>("char-type");
		string terminal_type = cmd.get<string>("terminal-type");
		string decoder       = cmd.get<string>("decoder");
		string encoder       = cmd.get<string>("encoder");
		if (cmd.get<bool>("utf8")) {
			char_type     = "char";
			terminal_type = "char32_t";
			if (decoder.empty()) decoder = "idni::utf8_to_u32_conv";
			if (encoder.empty()) encoder = "idni::u32_to_utf8_conv";
		}
		const bool print_json = cmd.get<bool>("json");
		auto gr = generate_parser_cpp_from_file<char>(tgf_file,
			parser_gen_options{
			.output_dir          = cmd.get<string>("output-dir"),
			.output              = cmd.get<string>("output"),
			.name                = cmd.get<string>("name"),
			.ns                  = cmd.get<string>("namespace"),
			.char_type           = char_type,
			.terminal_type       = terminal_type,
			.decoder             = decoder,
			.encoder             = encoder,
			.auto_disambiguate   = cmd.get<bool>("auto-disambiguate"),
			.nodisambig_list     = nodisambig_list
		}, print_json);
		print_diagnostics_report(gr.report(), print_json);
		return gr.has_value() ? 0 : 1;
	}

	// prepare repl evaluator options and initialize repl evaluator
	// ---------------------------------------------------------------------

	// pass cli options to repl evaluator if exists
	tgf_repl_evaluator::options tgf_repl_opt;
	if (cmd.has("status"))                  tgf_repl_opt.status =
		cmd.get<bool>("status");
	if (cmd.has("colors"))                  tgf_repl_opt.colors =
		cmd.get<bool>("colors");
	if (cmd.has("measure"))                 tgf_repl_opt.measure =
		cmd.get<bool>("measure");
	if (cmd.has("json"))                    tgf_repl_opt.print_json =
		cmd.get<bool>("json");
	if (cmd.has("print-input"))             tgf_repl_opt.print_input =
		cmd.get<bool>("print-input");
	if (cmd.has("print-ambiguity"))         tgf_repl_opt.print_ambiguity =
		cmd.get<bool>("print-ambiguity");
	if (cmd.has("print-graphs"))            tgf_repl_opt.print_graphs =
		cmd.get<bool>("print-graphs");
	if (cmd.has("tml-rules"))               tgf_repl_opt.tml_rules =
		cmd.get<bool>("tml-rules");
	if (cmd.has("tml-facts"))               tgf_repl_opt.tml_facts =
		cmd.get<bool>("tml-facts");
	if (cmd.has("error-verbosity"))         tgf_repl_opt.error_verbosity =
		str2error_verbosity(cmd.get<string>("error-verbosity"));
	if (cmd.has("start"))
		tgf_repl_opt.start = cmd.get<string>("start");
	if (cmd.has("tree-path"))
		tgf_repl_opt.tree_path =
			cmd.get<string>("tree-path") == "forest"
				? parse_tree_path::forest_path
				: parse_tree_path::bintree_path;
	if (cmd.has("auto-disambiguate"))
		tgf_repl_opt.auto_disambiguate =
			cmd.get<bool>("auto-disambiguate");
	tgf_repl_evaluator re(tgf_file, tgf_repl_opt);
	if (!re.good()) return re.flush_report(), 1;

	// TEST COMMAND
	// ---------------------------------------------------------------------

	if (cmd.name() == "test") {
		auto files = cl.get_files();
		if (!files.size()) return cl.error(
			"test command needs at least one file as an argument");
		int ret = run_tests(re.p, files);
		re.flush_report();
		return ret;
	}

	// REPL COMMAND
	// ---------------------------------------------------------------------

	if (cmd.name() == "repl") {
		if (cmd.get<string>("evaluate").size()) {
			re.flush_report();
			int ret = re.eval(cmd.get<string>("evaluate"));
			re.flush_report();
			return ret;
		}
		re.flush_report();
		repl<decltype(re)> r(re, "tgf> ", ".tgf_history");
		re.set_repl(r);
		int ret = r.run();
		re.flush_report();
		return ret;
	}

	// PARSE COMMAND
	// ---------------------------------------------------------------------

	if (cmd.name() == "parse") {
		string infile = cmd.get<string>("input");
		string inexp  = cmd.get<string>("input-expression");
		if (infile.size() && inexp.size())
			return cl.error("multiple inputs specified, use ei"
				"ther --input or --input-expression, not both");
		if (cmd.get<bool>("grammar")) re.eval("i");
		if (infile.size())
			if (infile == "-") // stdin
				re.parse(cin);
			else { // file
				re.parse(infile);
			}
		else // string
			re.parse(inexp.c_str(), inexp.size());
		re.flush_report();
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
	if (r) r->set_prompt(ss.str());
}

ostream& tgf_repl_evaluator::pretty_print(ostream& os, tref n,
	set<size_t> skip = {}, bool nulls = false, size_t l = 0)
{
	const auto& t = tree::get(n);
	if (skip.size() && t.value.first.nt() &&
		skip.find(t.value.first.n()) != skip.end())
			return os;
	if (!nulls && t.value.first.is_null()) return os;
	for (size_t t = 0; t < l; t++) os << "\t";
	if (t.value.first.nt())
		os << TC_NT << t.value.first << TC.CLEAR() << TC_NT_ID
			<< "(" << t.value.first.n() << ")" << TC.CLEAR();
	else if (t.value.first.is_null())
		os << TC_NULL << "null" << TC.CLEAR();
	else os << TC_T << t.value.first << TC.CLEAR();
	os << TC_RANGE << "[" << t.value.second[0] << ", "
		<< t.value.second[1] << "]" << TC.CLEAR() << "\n";
	for (tref d : t.children()) pretty_print(os, d, skip, nulls, l + 1);
	return os;
}

static tgf_repl_evaluator::parser_type::options utf8_parser_options() {
	tgf_repl_evaluator::parser_type::options o;
	if constexpr (
		!std::is_same_v<tgf_repl_evaluator::parser_type::char_type,
		                tgf_repl_evaluator::parser_type::terminal_type>)
	{
		o.chars_to_terminals = idni::utf8_to_u32_conv;
		o.terminals_to_chars = idni::u32_to_utf8_conv;
	}
	return o;
}

void tgf_repl_evaluator::flush_report() {
	print_diagnostics_report(report, opt.print_json);
	report.clear();
}

bool tgf_repl_evaluator::init_grammar(const string& filename) {
	using char_t = parser_type::char_type;
	using term_t = parser_type::terminal_type;
	auto next_nts = make_shared<nonterminals_type>();
	auto gr = tgf<char_t, term_t>::from_file(*next_nts, filename,
		opt.measure || opt.print_json);
	if (!gr.has_value()) {
		report.append(std::move(gr).report());
		return false;
	}
	auto next_g = make_shared<grammar_type>(std::move(gr).value());
	auto next_p = make_shared<parser_type>(*next_g, utf8_parser_options());
	report.append(std::move(gr).report());
	nts = std::move(next_nts);
	g = std::move(next_g);
	p = std::move(next_p);
	return true;
}

tgf_repl_evaluator::tgf_repl_evaluator(const string& tgf_file)
	: tgf_file(tgf_file)
{
	if (!init_grammar(tgf_file)) return;
	update_opts_by_grammar_opts();
	g->opt.auto_disambiguate = this->opt.auto_disambiguate;
}

tgf_repl_evaluator::tgf_repl_evaluator(const string& tgf_file, options opt)
	: tgf_file(tgf_file), opt(opt)
{
	TC.set(opt.colors);
	if (!init_grammar(tgf_file)) return;
	update_opts_by_grammar_opts();
	g->opt.auto_disambiguate = opt.auto_disambiguate;
}

void tgf_repl_evaluator::update_opts_by_grammar_opts() {
	auto ntids2strs = [this] (const set<size_t>& ntids) {
		set<string> r;
		for (const auto& id : ntids) r.insert(nts->get(id));
		return r;
	};
	opt.to_trim          = ntids2strs(g->opt.shaping.to_trim);
	opt.to_trim_children = ntids2strs(g->opt.shaping.to_trim_children);
	opt.dont_trim_terminals_of =
		ntids2strs(g->opt.shaping.dont_trim_terminals_of);
	opt.to_trim_children_terminals =
		ntids2strs(g->opt.shaping.to_trim_children_terminals);
	opt.to_inline.clear();
	for (const auto& tp : g->opt.shaping.to_inline) {
		vector<string> v;
		for (const auto& s : tp) v.push_back(nts->get(s));
		opt.to_inline.insert(std::move(v));
	}
	if (opt.start.size() == 0)
		opt.start = g->start_literal().to_std_string();
}

void tgf_repl_evaluator::set_repl(repl<tgf_repl_evaluator>& r_) {
	r = &r_;
	reprompt();
}

void tgf_repl_evaluator::parsed(parser_type::result& r) {
	if (!r.good() || !r.found) {
		report.append(std::move(r.report()));
		return;
	}
	auto f = r.get_forest();
	stringstream ss;
	if (opt.print_input) ss << "input: \"" << r.get_input() << "\"\n";
	if (opt.print_ambiguity) r.print_ambiguous_nodes(ss);
	if (opt.print_terminals) ss << "parsed terminals: "
		<< TC_T << to_std_string(r.get_terminals())
		<< TC_CLEARED_DEFAULT << "\n";
	using c_t = parser_type::char_type;
	using t_t = parser_type::terminal_type;
	auto cb_next_g = [&r, &ss, this](parser_type::pgraph& g) {
		r.inline_grammar_transformations(g);
		auto t = g.extract_trees();
		//if (opt.print_graphs) pretty_print(ss << "parsed graph:\n",
		//	t, {}, false, 1);
		if (opt.tml_rules) to_tml_rules<c_t, t_t, parser_type::pgraph>(
			ss << "TML rules:\n", g), ss << "\n";
		return true;
	};
	if (opt.tml_rules) f->extract_graphs(f->root(), cb_next_g);
	if (opt.tml_facts) to_tml_facts<c_t, t_t>(ss << "TML facts:\n", r);
	if (opt.print_graphs) {
		auto str2ntids = [this](const set<string>& list) {
			set<size_t> r;
			for (const auto& s : list) r.insert(nts->get(s));
			return r;
		};
		shaping_options sopt;
		sopt.trim_terminals = g->opt.shaping.trim_terminals;
		sopt.inline_char_classes = g->opt.shaping.inline_char_classes;
		if (opt.to_trim.size()) sopt.to_trim = str2ntids(opt.to_trim);
		else sopt.to_trim = g->opt.shaping.to_trim;
		sopt.to_trim_children = opt.to_trim_children.size()
			? str2ntids(opt.to_trim_children)
			: g->opt.shaping.to_trim_children;
		sopt.dont_trim_terminals_of = opt.dont_trim_terminals_of.size()
			? str2ntids(opt.dont_trim_terminals_of)
			: g->opt.shaping.dont_trim_terminals_of;
		sopt.to_trim_children_terminals =
				opt.to_trim_children_terminals.size()
			? str2ntids(opt.to_trim_children_terminals)
			: g->opt.shaping.to_trim_children_terminals;
		if (opt.to_inline.size()) {
			for (const auto& tp : opt.to_inline) {
				vector<size_t> v;
				for (const auto& s : tp)
					v.push_back(nts->get(s));
				sopt.to_inline.insert(v);
			}
		}
		else sopt.to_inline = g->opt.shaping.to_inline;
		pretty_print(ss << "parsed graph:\n",
			r.get_shaped_tree2(sopt), {}, false, 1);
	}
	cout << ss.str();
	report.append(std::move(r.report()));
}

tgf_repl_evaluator::parser_type::parse_options
	tgf_repl_evaluator::get_parse_options() const
{
	parser_type::parse_options po{
		.start              = nts->get(opt.start),
		.measure            = opt.measure,
		.measure_each_pos   = opt.measure_each_pos,
		.measure_forest     = opt.measure_forest,
		.measure_preprocess = opt.measure_preprocess,
		.debug              = opt.debug,
		.error_verbosity    = opt.error_verbosity,
		.tree_path          = opt.tree_path
	};
	if (opt.measure) { /// `opt.measure` is a master ENABLE for now
		po.measure_scopes       = true;
		po.measure_counters     = true;
		po.measure_forest       = true;
		po.measure_preprocess   = true;
	}
	return po;
}

void tgf_repl_evaluator::parse(const char* input, size_t size) {
	if (!good()) return;
	//cout << "parsing: " << input << "\n";
	auto po = get_parse_options();
	auto r = p->parse(input, size, po);
	parsed(r);
}

void tgf_repl_evaluator::parse(istream& instream) {
	if (!good()) return;
	auto po = get_parse_options();
	auto r = p->parse(instream, po);
	parsed(r);
}

void tgf_repl_evaluator::parse(const string& infile) {
	if (!good()) return;
	auto po = get_parse_options();
	auto r = p->parse(infile, po);
	parsed(r);
}

bool tgf_repl_evaluator::reload(const string& new_tgf_file) {
	if (!init_grammar(new_tgf_file)) {
		cout << "reload failed: " << new_tgf_file << "\n";
		return false;
	}
	tgf_file = new_tgf_file;
	update_opts_by_grammar_opts();
	g->opt.auto_disambiguate = opt.auto_disambiguate;
	cout << "loaded: " << tgf_file << "\n";
	return true;
}

bool tgf_repl_evaluator::reload() {
	return reload(tgf_file);
}

pair<tgf_repl_parser::nonterminal, tt> get_opt(const tt& t) {
	using p = tgf_repl_parser;
	static const map<p::nonterminal, p::nonterminal> ov{
		{ p::bool_option,      p::bool_value },
		{ p::list_option,      p::symbol_list },
		{ p::treepaths_option, p::treepath_list },
		{ p::enum_ev_option,   p::error_verbosity }
	};
	for (auto it = ov.begin(); it != ov.end(); ++it)
		if (auto x = t | it->first; x.has_value())
			return { static_cast<p::nonterminal>(
					x | tt::only_child | tt::nonterminal),
				t | it->second };
	return { p::nul, t };
}

void tgf_repl_evaluator::get_cmd(const tt& n) {
	using p = tgf_repl_parser;
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
	static auto plist = [] (const set<string>& l) {
		if (l.empty()) return string("(empty)");
		stringstream ss;
		bool first = true;
		for (auto& s : l) ss << (first ? first = false, "" : ", ") << s;
		return ss.str();
	};
	static auto ptreepaths = [](
		const set<vector<string>>& l)
	{
		if (l.empty()) return string("(empty)");
		stringstream ss;
		bool first = true;
		for (auto& tp : l) {
			ss << (first ? first = false, "" : ", ");
			bool first_s = true;
			for (auto& s : tp) ss
				<< (first_s ? first_s = false, "" : " > ") << s;
		}
		return ss.str();
	};
	static map<size_t,	function<void()>> printers = {
	{ p::enabled_prods_opt, [this]() { cout <<
		"enabled_productions:    " << plist(g->opt.enabled_guards) << "\n"; } },
	{ p::debug_opt,   [this]() { cout <<
		"show debug:             " << pbool(opt.debug) << "\n"; } },
	{ p::status_opt,   [this]() { cout <<
		"show status:            " << pbool(opt.status) << "\n"; } },
	{ p::colors_opt,   [this]() { cout <<
		"colors:                 " << pbool(opt.colors) << "\n"; } },
	{ p::measure_parsing_opt, [this]() { cout <<
		"measure-parsing:        " << pbool(opt.measure) << "\n"; } },
	{ p::measure_each_pos_opt, [this]() { cout <<
		"measure-each:           " << pbool(opt.measure_each_pos) << "\n"; } },
	{ p::measure_forest_opt, [this]() { cout <<
		"measure-forest:         " << pbool(opt.measure_forest) << "\n"; } },
	{ p::measure_preprocess_opt, [this]() { cout <<
		"measure-preprocess:     " << pbool(opt.measure_preprocess) << "\n"; } },
	{ p::print_terminals_opt, [this]() { cout <<
		"print-terminals:        " << pbool(opt.print_terminals) << "\n"; } },
	{ p::print_graphs_opt, [this]() { cout <<
		"print-graphs:           " << pbool(opt.print_graphs) << "\n"; } },
	{ p::print_ambiguity_opt, [this]() { cout <<
		"print-ambiguity:        " << pbool(opt.print_ambiguity) << "\n"; } },
	{ p::print_rules_opt, [this]() { cout <<
		"print-rules:            " << pbool(opt.tml_rules) << "\n"; } },
	{ p::print_facts_opt, [this]() { cout <<
		"print-facts:            " << pbool(opt.tml_facts) << "\n"; } },
	{ p::trim_terminals_opt, [this]() { cout <<
		"trim-terminals:         " << pbool(g->opt.shaping.trim_terminals) << "\n"; } },
	{ p::inline_cc_opt, [this]() { cout <<
		"inline-char-classes:    " << pbool(g->opt.shaping.inline_char_classes) << "\n"; } },
	{ p::trim_opt, [this]() { cout <<
		"trim:                   " << plist(opt.to_trim) << "\n"; } },
	{ p::trim_children_opt, [this]() { cout <<
		"trim-children:          " << plist(opt.to_trim_children) << "\n"; } },
	{ p::trim_children_terminals_opt, [this]() { cout <<
		"trim-children-terminals:" << plist(opt.to_trim_children_terminals) << "\n"; } },
	{ p::inline_opt, [this]() { cout <<
		"inline:                 " << ptreepaths(opt.to_inline) << "\n"; } },
	{ p::auto_disambiguate_opt, [this]() { cout <<
		"auto-disambiguate:      " << pbool(g->opt.auto_disambiguate) << "\n"; } },
	{ p::nodisambig_list_opt, [this]() { cout <<
		"nodisambig-list:        " << plist(opt.nodisambig_list) << "\n"; } },
	{ p::error_verbosity_opt, [this]() { cout <<
		"error-verbosity:        " << pverb(opt.error_verbosity) << "\n"; } }};
	if (!n) { for (auto& [_, v] : printers) v(); return; }
	auto [o, _] = get_opt(n);
	printers[o]();
}

bool get_bool_value(const tt& t) {
	return (t | tt::only_child | tt::nonterminal)
			== tgf_repl_parser::true_value;
}

string unquote(const string& s) {
	stringstream ss;
	if (s.size() < 2 || s[0] != '"' || s[s.size() - 1] != '"') return s;
	size_t l = s.size() - 2;
	for (size_t i = 1; i != s.size() - 1; ++i) {
		if (i != l && s[i] == '\\') {
			switch (s[++i]) {
			case 'r': ss << '\r'; break;
			case 'n': ss << '\n'; break;
			case 't': ss << '\t'; break;
			case 'b': ss << '\b'; break;
			case 'f': ss << '\f'; break;
			default: ss << s[i];
			}
		} else ss << s[i];
	}
	return ss.str();
};

vector<string> tgf_repl_evaluator::treepath(const tt& tp) const {
	vector<string> v;
	for (const auto& s : (tp || tgf_repl_parser::symbol)())
		v.push_back(s | tt::terminals);
	return v;
}

void tgf_repl_evaluator::set_cmd(const tt& n) {
	using p = tgf_repl_parser;
	auto [o, v] = get_opt(n);
	switch (o) {
	case p::debug_opt:
		opt.debug = get_bool_value(v); break;
	case p::status_opt:
		opt.status = get_bool_value(v); break;
	case p::colors_opt:
		TC.set((opt.colors = get_bool_value(v))); break;
	case p::print_terminals_opt:
		opt.print_terminals = get_bool_value(v); break;
	case p::print_graphs_opt:
		opt.print_graphs = get_bool_value(v); break;
	case p::print_ambiguity_opt:
		opt.print_ambiguity = get_bool_value(v); break;
	case p::print_rules_opt:
		opt.tml_rules = get_bool_value(v); break;
	case p::print_facts_opt:
		opt.tml_facts = get_bool_value(v); break;
	case p::measure_parsing_opt:
		opt.measure = get_bool_value(v); break;
	case p::measure_each_pos_opt:
		opt.measure_each_pos = get_bool_value(v); break;
	case p::measure_forest_opt:
		opt.measure_forest = get_bool_value(v); break;
	case p::measure_preprocess_opt:
		opt.measure_preprocess = get_bool_value(v); break;
	case p::trim_terminals_opt:
		g->opt.shaping.trim_terminals = get_bool_value(v); break;
	case p::inline_cc_opt:
		g->opt.shaping.inline_char_classes = get_bool_value(v); break;
	case p::trim_opt:
		opt.to_trim.clear();
		for (const auto& s : (v || p::symbol)())
			opt.to_trim.insert(s | tt::terminals);
		break;
	case p::enabled_prods_opt: {
		std::set<std::string> grds;
		for (const auto& s : (v || p::symbol)())
			grds.insert(s | tt::terminals);
		if (grds != g->opt.enabled_guards)
			g->set_enabled_productions(grds);
		break;
	}
	case p::trim_children_opt:
		opt.to_trim_children.clear();
		for (const auto& s : (v || p::symbol)())
			opt.to_trim_children.insert(s | tt::terminals);
		break;
	case p::trim_children_terminals_opt:
		opt.to_trim_children_terminals.clear();
		for (const auto& s : (v || p::symbol)())
			opt.to_trim_children_terminals.insert(s | tt::terminals);
		break;
	case p::inline_opt:
		opt.to_inline.clear();
		for (const auto& tp : (v || p::treepath)())
			opt.to_inline.insert(treepath(tp));
		break;
	case p::auto_disambiguate_opt:
		g->opt.auto_disambiguate = get_bool_value(v); break;
	case p::nodisambig_list_opt:
		g->opt.nodisambig_list.clear();
		for (const auto& s : (v || p::symbol)())
			opt.nodisambig_list.insert(s | tt::terminals);
		break;
	case p::error_verbosity_opt: {
		auto vrb = v | p::error_verbosity;
		if (!vrb.has_value()) {
			cout << "error: invalid error verbosity value\n"; return; }
		auto vrb_type = vrb | tt::only_child | tt::nonterminal;
		using lvl = p::parser_type::error::info_lvl;
		switch (vrb_type) {
		case p::basic_sym:
			opt.error_verbosity = lvl::INFO_BASIC; break;
		case p::detailed_sym:
			opt.error_verbosity = lvl::INFO_DETAILED; break;
		case p::root_cause_sym:
			opt.error_verbosity = lvl::INFO_ROOT_CAUSE; break;
		default: cout << "error: invalid error verbosity value\n";
			return;
		}
		break;
	}
	default: assert(false);
	};
	get_cmd(n);
}

void tgf_repl_evaluator::add_cmd(const tt& n) {
	using p = tgf_repl_parser;
	auto [o, v] = get_opt(n);
	set<string> empty{};
	if (o == p::inline_opt) {
		for (const auto& tp : (v || p::treepath)())
			opt.to_inline.insert(treepath(tp));
		get_cmd(n);
		return;
	}
	if (o == p::enabled_prods_opt) {
		for (const auto& s : (v || p::symbol)())
			g->opt.enabled_guards.insert(s | tt::terminals);
		g->set_enabled_productions(g->opt.enabled_guards);
		get_cmd(n);
		return;
	}
	auto& l(o == p::nodisambig_list_opt ? opt.nodisambig_list :
		o == p::trim_opt            ? opt.to_trim :
		o == p::trim_children_opt   ? opt.to_trim_children :
		o == p::trim_children_terminals_opt
						? opt.to_trim_children_terminals
						: empty);
	for (const auto& s : (v || p::symbol)())
		l.insert(s | tt::terminals);
	get_cmd(n);
}

void tgf_repl_evaluator::del_cmd(const tt& n) {
	using p = tgf_repl_parser;
	auto [o, v] = get_opt(n);
	if (o == p::inline_opt) {
		for (const auto& tp : (v || p::treepath)())
			opt.to_inline.erase(treepath(tp));
		get_cmd(n);
		return;
	}
	if (o == p::enabled_prods_opt) {
		for (const auto& s : (v || p::symbol)())
			g->opt.enabled_guards.erase(s | tt::terminals);
		g->set_enabled_productions(g->opt.enabled_guards);
		get_cmd(n);
		return;
	}
	set<string> empty{};
	auto& l(o == p::nodisambig_list_opt ? opt.nodisambig_list :
		o == p::trim_opt            ? opt.to_trim :
		o == p::trim_children_opt   ? opt.to_trim_children :
		o == p::trim_children_terminals_opt
						? opt.to_trim_children_terminals
						: empty);
	for (const auto& s : (n || p::symbol)())
		l.erase(s | tt::terminals);
	get_cmd(n);
}

void tgf_repl_evaluator::update_bool_opt_cmd(
	const tt& n,
	const function<bool(bool&)>& update_fn)
{
	using p = tgf_repl_parser;
	auto option_type = n | tgf_repl_parser::bool_option
		| tt::only_child | tt::nonterminal;
	switch (option_type) {
	case p::debug_opt:             update_fn(opt.debug); break;
	case p::status_opt:            update_fn(opt.status); break;
	case p::colors_opt:     TC.set(update_fn(opt.colors)); break;
	case p::print_terminals_opt:   update_fn(opt.print_terminals); break;
	case p::print_graphs_opt:      update_fn(opt.print_graphs); break;
	case p::print_ambiguity_opt:   update_fn(opt.print_ambiguity); break;
	case p::print_rules_opt:       update_fn(opt.tml_rules); break;
	case p::print_facts_opt:       update_fn(opt.tml_facts); break;
	case p::measure_parsing_opt:   update_fn(opt.measure); break;
	case p::measure_each_pos_opt:  update_fn(opt.measure_each_pos); break;
	case p::measure_forest_opt:    update_fn(opt.measure_forest); break;
	case p::measure_preprocess_opt:update_fn(opt.measure_preprocess); break;
	case p::auto_disambiguate_opt: update_fn(g->opt.auto_disambiguate); break;
	case p::trim_terminals_opt:    update_fn(g->opt.shaping.trim_terminals); break;
	case p::inline_cc_opt:         update_fn(g->opt.shaping.inline_char_classes); break;
	default: cout << ": unknown bool option\n"; break;
	}
	get_cmd(n);
}

// TODO (LOW) write proper help messages
void help(size_t nt = tgf_repl_parser::help_sym) {
	using p = tgf_repl_parser;
	static const string bool_options =
		"  status                 show status                        on/off\n"
		"  colors                 use term colors                    on/off\n"
		"  print-ambiguity        prints ambiguous nodes             on/off\n"
		"  print-terminals        prints parsed terminals            on/off\n"
		"  print-graphs           prints parsed graphs               on/off\n"
		"  print-rules            prints parsed forest as TML rules  on/off\n"
		"  print-facts            prints parsed forest as TML facts  on/off\n"
		"  measure-parsing        measures parsing time              on/off\n"
		"  measure-each-pos       measures parsing time of each pos  on/off\n"
		"  measure-forest         measures forest building time      on/off\n"
		"  measure-preprocess     measures forest preprocess time    on/off\n"
		"  trim-terminals         trim terminals                     on/off\n"
		"  inline-char-classes    inline character classes           on/off\n";
	static const string list_options =
		"  nodisambig-list        list of nodes to keep ambiguous    symbol1, symbol2...\n"
		"  trim                   list of nodes to trim              symbol1, symbol2...\n"
		"  trim-children          list of nodes to trim children     symbol1, symbol2...\n";
	static const string treepaths_options =
		"  inline                 list of tree paths to inline       symbol1 > ch1 > ch2, symbol2...\n";
	static const string enum_ev_option =
		"  error-verbosity        parse errors verbosity             basic/detailed/root-cause\n";
	static const string all_available_options = string{} +
		"Available options:\n" + bool_options + list_options
			+ treepaths_options + enum_ev_option;
	static const string bool_available_options = string{} +
		"Available options:\n" + bool_options;
	static const string list_and_treepaths_available_options =
		string{} +
		"Available options:\n" + list_options + treepaths_options;
	switch (nt) {
	case p::help_sym: cout
		<< "tgf commands:\n"
		<< "  help or h                    print this help\n"
		<< "  help <command>               print help for a command\n"
		<< "  quit, q, exit or e           exit the repl\n"
		<< "  version or v                 print version\n"
		<< "  clear or cls                 clears the screen\n"
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
		<< "  grammar or g                 show TGF grammar\n"
		<< "  internal-grammar or ig or i  show TGF grammar\n"
		<< "  start or s                   show or change start symbol\n"
		<< "  unreachable or u             show unreachable productions\n"
		<< "\n"
		<< "parsing commands:\n"
		<< "  parse or p                   parse input\n"
		<< "  parse file or pf or f        parse input file\n"
		<< "\n";
		break;
	case p::version_sym: cout
		<< "version or v prints out current TGF commit id\n";
		break;
	case p::quit_sym: cout
		<< "command: quit or exit\n"
		<< "short: q or e\n"
		<< "\texits the repl\n";
		break;
	case p::clear_sym: cout
		<< "command: clear\n"
		<< "short: cls\n"
		<< "\tclears the screen\n";
		break;
	case p::get_sym: cout
		<< "command: get [<option>]\n"
		<< "\tprints the value of the given option\n"
		<< "\tprints all option values if no option provided\n"
		<< "\n"
		<< all_available_options;
		break;
	case p::set_sym: cout
		<< "command: set <option> [=] <value>\n"
		<< "\tsets value of the given option\n"
		<< "\n"
		<< all_available_options;
		break;
	case p::toggle_sym: cout
		<< "command: toggle <option>\n"
		<< "short: tog\n"
		<< "\t toggles value between on/off of the given option\n"
		<< "\n"
		<< bool_available_options;
		break;
	case p::enable_sym: cout
		<< "command: enable <option>\n"
		<< "short: en\n"
		<< "\tsets the value of the given option to on\n"
		<< "\n"
		<< bool_available_options;
		break;
	case p::disable_sym: cout
		<< "command: disable <option>\n"
		<< "short: dis\n"
		<< "\tsets the value of the given option to off\n"
		<< "\n"
		<< bool_available_options;
		break;
	case p::add_sym: cout
		<< "command: add <option> <value>\n"
		<< "\tadds the value to the given option list\n"
		<< "\n"
		<< list_and_treepaths_available_options;
		break;
	case p::del_sym: cout
		<< "command: delete <option> <value>\n"
		<< "or: del, remove, rem or rm\n"
		<< "\tremoves the value from the given option list\n"
		<< "\n"
		<< list_and_treepaths_available_options;
		break;
	case p::load_sym: cout
		<< "command: file \"TGF filepath\"\n"
		<< "short: f\n"
		<< "\tload a TGF file from drive\n";
		break;
	case p::start_sym: cout
		<< "command: start [<start symbol>]\n"
		<< "short: s\n"
		<< "\tset a new start symbol for parsing"
		<< "\tprint the current start symbol if no argument\n";
		break;
	case p::grammar_sym: cout
		<< "command: grammar\n"
		<< "short: g\n"
		<< "\tprints the actual TGF file\n";
		break;
	case p::igrammar_sym: cout
		<< "command: internal-grammar [<start symbol>]\n"
		<< "short: ig or i\n"
		<< "\tprints the internal grammar\n"
		<< "\tif start symbol provided prints the internal sub-grammar\n";
		break;
	case p::unreachable_sym: cout
		<< "command: unreachable [<symbol>]\n"
		<< "short: u\n"
		<< "\tprints unreachable production rules for provided symbol\n"
		<< "\tif no symbol provided prints unreachable rules for start symbol\n";
		break;
	case p::parse_sym: cout
		<< "command: parse <input>\n"
		<< "short: p\n"
		<< "\tparse the given input\n";
		break;
	case p::parse_file_sym: cout
		<< "command: parse file \"<input file>\"\n"
		<< "short: pf or f\n"
		<< "\tparse the given input file\n";
		break;
	}
}

static std::string_view repl_cmd_scope_name(size_t nt) {
	return tgf_repl_parser::instance().name(nt);
}

int tgf_repl_evaluator::eval(const tt& s) {
	using p = tgf_repl_parser;
	const auto nt = s | tt::nonterminal;
	auto _ = report.open_if(opt.measure, repl_cmd_scope_name(nt));
	int ret = 0;
	switch (nt) {
	case p::quit_cmd: ret = (cout << "Quit.\n", 1); break;
	case p::clear_cmd: if (r) r->clear(); break;
	case p::help_cmd: {
		auto optarg = s | p::help_arg
				| tt::only_child | tt::nonterminal;
		if (optarg) help(optarg);
		else help();
		break;
	}
	case tgf_repl_parser::version_cmd: print_version(); break;
	case tgf_repl_parser::license_cmd: print_license(); break;
	case tgf_repl_parser::get_cmd:     get_cmd(s | tgf_repl_parser::option); break;
	case tgf_repl_parser::set_cmd:     set_cmd(s); break;
	case tgf_repl_parser::toggle_cmd:
		update_bool_opt_cmd(s, [](bool& b){ return b = !b; }); break;
	case p::enable_cmd:
		update_bool_opt_cmd(s, [](bool& b){ return b = true; }); break;
	case p::disable_cmd:
		update_bool_opt_cmd(s, [](bool& b){ return b = false; }); break;
	case p::add_cmd:     add_cmd(s); break;
	case p::del_cmd:     del_cmd(s); break;
	case p::reload_cmd: reload(); break;
	case p::load_cmd: {
		auto n = s | p::filename;
		auto filename = unquote(n | tt::terminals);
		DBG(assert(filename.size());)
		reload(filename);
		break;
	}
	case p::start_cmd: {
		auto n = s | p::symbol;
		string start;
		if (n.has_value() && (start = n | tt::terminals).size())
			cout << "start symbol set: " << TC_NT
				<< (opt.start = start) << TC_DEFAULT << "\n";
		else
			cout << "start symbol: " << TC_NT << opt.start
				<< TC_DEFAULT << "\n";
		break;
	}
	case p::igrammar_cmd: {
		auto n = s | p::symbol;
		string start = n.has_value() ? n | tt::terminals : opt.start;
		g->print_internal_grammar_for(cout
			<< "\ninternal grammar for symbol "
		 	<< TC_NT << start << TC_DEFAULT << ":\n",
			start, "  ", true, TC);
		break;
	}
	case p::grammar_cmd: {
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
	case p::unreachable_cmd: {
		auto n = s | p::symbol;
		string start = n.has_value() ? n | tt::terminals : opt.start;
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
	case p::parse_cmd: {
		string input{};
		auto i = s | p::parse_input;
		if (auto seq = i | p::parse_input_char_seq;
			seq.has_value()) input = seq | tt::terminals;
		else if (auto qstr = i | p::quoted_string;
			qstr.has_value()) input = unquote(qstr
				|| p::quoted_string_char
				|| tt::terminals);
		//if (opt.debug) cout << "input: " << input << "\n";
		parse(input.c_str(), input.size());
		break;
	}
	case p::parse_file_cmd: {
		auto n = s | p::filename;
		auto filename = unquote(n | tt::terminals);
		DBG(assert(filename.size());)
		parse(filename);
		break;
	}
	default: cout << "error: unknown command\n"; break;
	}
	return ret;
}

int tgf_repl_evaluator::eval(const string& src) {
	static tgf_repl_parser rp;
	int quit = 0;
	auto r = rp.parse(src.c_str(), src.size());
	if (!r.found) {
		report.append(std::move(r.report()));
		flush_report();
	} else {
		r.print_ambiguous_nodes(cout);
		if (opt.debug) {
			pretty_print(cout << "input command graph:\n",
				r.get_shaped_tree2(), {}, false, 1);
			//g->print_internal_grammar(cout << "\ngrammar:\n\n", "  ");
		}
		tref ref = r.get_shaped_tree2();
		auto t = tt(ref);
		auto statements = t || tgf_repl_parser::statement;
		for (const auto& statement : statements()) {
			quit = eval(statement | tt::only_child);
			flush_report();
			if (quit == 1) return quit;
		}
	}
	std::cout << std::endl;
	if (quit == 0) reprompt();
	return quit;
}

} // namespace idni
