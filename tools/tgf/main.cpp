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
#include <fstream>
#include "parser_gen.h"

#ifdef DEBUG
#	define DBG(x) x
#else
#	define DBG(x)
#endif

#define PBOOL(bval) (( bval ) ? "true" : "false")

using namespace std;
using namespace idni;


typedef variant<string, bool, int> value;

struct option {
	friend void init_commands();
	option() : value_(false) {}
	option(string name, char short_name, value dflt_value)
		: name_(name), short_name_(short_name), value_(dflt_value) {}
	option& desc(const string& d) { return description_ = d, *this; }
	const string& desc()   const { return description_; }
	const string& name()   const { return name_; }
	char short_name()      const { return short_name_; }
	bool matches(string n) const { return name_ == n; }
	bool matches(char c)   const { return short_name_ == c; }
	bool is_bool()   const { return holds_alternative<bool>(value_); }
	bool is_string() const { return holds_alternative<string>(value_); }
	bool is_int()    const { return holds_alternative<int>(value_); }
	void set_value(value v) { value_ = v; }
	template <typename T>
	T get_value() const { return get<T>(value_); }
private:
	string name_        = "";
	char   short_name_  = '\0';
	string description_ = "";
	value value_;
};

struct command {
	friend void init_commands();
	friend ostream& help(ostream& os, command cmd);
	command() {}
	command(string name, string description = "",
		map<string, option> opts = {}) :
			name_(name), description_(description), opts_(opts) {}
	command& description(string desc) {
		return description_ = desc, *this;
	}
	command& add_option(const option& o) {
		return opts_.insert({ o.name(), o }),  *this;
	}
	const string& name() const { return name_; }
	const string& description() const { return description_; }
	string long_for(char c) const {
		for (auto& opt : opts_) if (opt.second.matches(c))
			return opt.second.name();
		return "";
	}
	bool has(const string& n) const { return opts_.find(n) != opts_.end(); }
	option& operator[](const string& n) { return opts_[n]; }
	option& operator[](char c) { return opts_[long_for(c)]; }
	template <typename T>
	T get(const string& n) {
		option& o = opts_[n];
		return o.get_value<T>();
	}
	ostream& help(ostream& os) const {
		os << name() << "\n\n" << description() << "\n\n";
		os << "Command options:\n";
		for (auto& [_, opt] : opts_)
			os << "\t--" << opt.name() << "\t-" << opt.short_name()
				<< "\t" << opt.desc() << "\n";
		return os;
	}
	ostream& print(ostream& os) const {
		for (auto& [_, opt] : opts_) {
			os << "\t" << opt.name() << ": ";
			if (opt.is_bool()) os << PBOOL(opt.get_value<bool>());
			else if (opt.is_int()) os << opt.get_value<int>();
			else os << '"' << opt.get_value<string>() << '"';
			os << "\n";
		}
		return os;
	}
private:
	string  name_        = "";
	string  description_ = "";
	map<string, option> opts_ = {};
};

// checks case insensitively if the string represents a true value
bool is_true_value(string s) {
	vector<string> trues = { "1", "t", "on", "yes", "true" };
	transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
		return tolower(c); // char by char transform s string to lower
	});
	return find(trues.begin(), trues.end(), s) != trues.end();
}

// global variable commands containing list of commands and their opts and desc.
map<string, struct command> commands, commands_default;

ostream& help(ostream& os, command cmd = {}) {
	os << "parser-cli <TGF file> [ <command> [ <command options> ] ]\n";
	if (cmd.name().size()) return
		commands_default[cmd.name()].help(cout << "Command: ") << "\n";
	os << "Commands:\n";
	for (auto it : commands)
		os << "\t" << it.first << "\t\t" << it.second.description() << "\n";
	os << "\nCommand options:\n\t--help\t-h\tdetailed information about command\n";
	return os << "\n";
}

int error(string msg, bool print_help = false) {
	cerr << msg << endl;
	if (print_help) help(cerr);
	return 1;
}

// configure commands, their options and descriptions
void init_commands() {
	auto& cs = commands;
	string lcmd = "", lopt = "";
	auto CMD  = [&cs, &lcmd](const struct command& cmd) {
		lcmd = cmd.name(), cs[lcmd] = cmd; };
	auto OPT  = [&cs, &lcmd, &lopt](const option& opt) {
		lopt = opt.name(), cs[lcmd].add_option(opt); };
	auto DESC = [&cs, &lcmd, &lopt](const string& d) {
		cs[lcmd].opts_[lopt].desc(d); };
	auto LAST = [&cs, &lcmd, &lopt]() {
		return cs[lcmd].opts_[lopt]; };

	// ---------------------------------------------------------------------

	CMD(command("show",  "show information about grammar / parser"));

	OPT(option("help",  'h',
		false)),
		DESC("detailed information about options");
	option help(LAST());
	OPT(option("start", 's',
		"start")),
		DESC("starting literal");
	option start(LAST());
	OPT(option("grammar", 'g',
		true)),
		DESC("prints grammar");
	option print_grammar(LAST());
	OPT(option("char-type", 'C',
		"char")),
		DESC("type of input character");
	option char_type(LAST());
	OPT(option("terminal-type", 'T',
		"char")),
		DESC("type of terminal character");
	option terminal_type(LAST());

	// ---------------------------------------------------------------------

	CMD(command("parse", "parse an input string, file or stdin"));

	OPT(help);
	OPT(start);
	OPT(char_type);
	OPT(terminal_type);
	print_grammar.set_value(false);
	OPT(print_grammar);
	OPT(option("input", 'i',
		"")),
		DESC("parse input from file or STDIN if -");
	OPT(option("input-expression", 'e',
		"")),
		DESC("parse input from provided string");
	OPT(option("print-ambiguity", 'a',
		true)),
		DESC("prints ambiguity info, incl. ambig. nodes");
	OPT(option("terminals", 't',
		true)),
		DESC("prints all parsed terminals serialized");
	OPT(option("print-graphs", 'p',
		false)),
		DESC("prints parsed graph");
	OPT(option("tml-rules", 'r',
		false)),
		DESC("prints parsed graph in tml rules");
	OPT(option("tml-facts", 'f',
		false)),
		DESC("prints parsed graph in tml facts");

	// ---------------------------------------------------------------------

	CMD(command("gen", "generate a parser code"));

	OPT(help);
	OPT(start);
	OPT(char_type);
	OPT(terminal_type);
	OPT(option("name", 'n',
		"my_parser")),
		DESC("name of the generated parser struct");
	OPT(option("decoder", 'd',
		"")),
		DESC("decoder function");
	OPT(option("encoder", 'e',
		"")),
		DESC("encoder function");
	OPT(option("output", 'o',
		"-")),
		DESC("output file");

	commands_default = commands;
}

bool is_opt_prefix(const string& opt) {
	return opt.size() >= 2 && opt[0] == '-';
}

bool opt_prefix(string& opt, bool& isshort) {
	isshort = false;
	if (opt.size() < 2 || opt[0] != '-') return false;
	if (opt[1] != '-') isshort = true, opt = opt.substr(1);
	else opt = opt.substr(2);
	return true;
}

int process_args(int argc_int, char** argv, command& cmdref, string& tgf_file) {

	if (argc_int < 0) error("invalid state: argc negative\n");
	size_t argc(argc_int);

#ifdef DEBUG
	cout << "== DEBUG ==================================================================\n";
	cout << "argc: " << argc << "\n";
	cout << "argv:\n";
	for (size_t i = 0; i != static_cast<size_t>(argc); ++i)
		cout << "\t" << i << " `" << argv[i] << "`\n";
	cout << "= /DEBUG ==================================================================\n\n";
#endif

	if (argc == 1) return error("no TGF file provided", true);
	tgf_file = argv[1];
	DBG(cout << "tgf_file: `" << tgf_file << "`\n";)

	if (tgf_file == "--help" || tgf_file == "-h")
		help(cout), exit(0);

	string arg_command = argc == 2 ? "show" : argv[2];
	DBG(cout << "command " << arg_command << std::endl;)

	if (arg_command == "--help" || arg_command == "-h")
		help(cout), exit(0);

	if (commands.find(arg_command) == commands.end())
		return error("unknown command: " + arg_command, true);
	command& cmd = commands[arg_command];

	option* cur = 0;
	for (size_t arg = 3; arg < argc; ) {
		string opt(argv[arg]);
		bool isshort;
		if (!opt_prefix(opt, isshort))
			return error("invalid option: " + opt);
		DBG(cout << "opt: " << opt << " isshort: " << PBOOL(isshort) << endl;)
		if (isshort) for (size_t o = 0; o != opt.size(); ++o) {
			string n = cmd.long_for(opt[o]);
			DBG(cout << "long_for " << opt[o] << ": " << n << endl;)
			if (n.size() == 0 || !cmd.has(n)) return
				error("invalid option: -" + opt[o]);
			DBG(cout << "cmd " << n << endl;);
			cur = &cmd[n];
			DBG(cout << "cur->name() " << cur->name() << endl;);
			if (cur->is_bool()) cur->set_value(true);
			else if (opt.size() > o + 1)
				return error("missing argument for option: -" +
							string{ opt[o] });
		}
		else { // long
			if (!cmd.has(opt))
				return error("invalid option: " + opt);
			cur = &cmd[opt];
			if (cur->is_bool()) cur->set_value(true);
		}
		// option argument if any
		++arg;
		DBG(cout << "arg: " << arg << " argc: " << argc << endl;)
		if (arg >= argc || is_opt_prefix(argv[arg])) { // no option argument
			DBG(cout << "no option argument?" << endl;)
			if (!cur->is_bool()) return error(
				"missing argument for option: --"+cur->name());
			continue;
		} else {
			string v = argv[arg];
			DBG(cout << "v: " << v << endl;)
			if (cur->is_string()) cur->set_value(v);
			else if (cur->is_bool())
				cur->set_value(is_true_value(v));
			else if (cur->is_int()) {
				stringstream is(v);
				int n = 0;
				if (!(is >> n)) return error("option --" +
					cur->name() +
					" argument is not a number: " + v);
				cur->set_value(n);
			}
			DBG(if (cur->is_string()) cout<<"cur->get_string(): " <<
				cur->get_value<string>() << "\n";)
		}
		++arg;
	}

	DBG(cout << "args processed. cmd.name(): " << cmd.name() << "\n";)
	cmdref = cmd;
	return 0;
}

int show(string tgf_file, string start = "start", bool print_grammar = true) {
	DBG(cout << "show " << tgf_file <<
		" --start " << start <<
		" --grammar " << PBOOL(print_grammar) << "\n";)
	nonterminals<char> nts;
	auto g = tgf<char>::from_file(nts, tgf_file, start);
	if (print_grammar) g.print_internal_grammar(cout);
	return 0;
}

int gen(ostream& os, string tgf_file, string name = "my_parser",
	string start = "start", string char_type = "char",
	string terminal_type = "char", string decoder = "", string encoder = "")
{
	DBG(cout << "gen " << tgf_file <<
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

int parsed(std::unique_ptr<parser<char>::pforest> f, command& cmd) {
	auto c = f->count_trees();
	if (f->is_ambiguous() && c > 1 &&
		cmd.get<bool>("print-ambiguity"))
	{
		cout << "ambiguity... number of trees: " << c << "\n";
		for (auto& n : f->ambiguous_nodes()) {
			cout << "\t `" << n.first.first << "` [" <<
				n.first.second[0] << "," <<
				n.first.second[1] << "]\n";
			size_t d = 0;
			for (auto ns : n.second) {
				cout << "\t\t " << d++ << "\t";
				for (auto nt : ns) cout << " `" <<
					nt.first << "`[" << nt.second[0]
					<< "," << nt.second[1] << "] ";
				cout << "\n";
			}
		}

	}
	struct {
		bool graphs, facts, rules, terminals;
	} print(cmd.get<bool>("print-graphs"),
		cmd.get<bool>("tml-facts"),
		cmd.get<bool>("tml-rules"),
		cmd.get<bool>("terminals"));
	auto cb_next_g = [&f, &print](parser<char>::pgraph& g) {
		f->remove_binarization(g);
		f->remove_recursive_nodes(g);
		if (print.graphs) {
			static size_t c = 1;
			cout << "parsed graph";
			if (c++ > 1) cout << " " << c;
			cout << ":";
			g.extract_trees()->to_print(cout);
			cout << "\n\n";
		}
		if (print.rules) to_tml_rules<char, char, parser<char>::pgraph>(
			cout << "TML rules:\n", g), cout << "\n";
		return true;
	};
	f->extract_graphs(f->root(), cb_next_g);

	if (print.facts) to_tml_facts<char, char>(cout << "TML facts:\n", *f),
		cout << "\n";

	if (print.terminals) cout << "terminals parsed: \"" <<
		terminals_to_str(*f, f->root()) << "\"\n\n";
	return 0;
}

int main(int argc, char** argv) {

	init_commands();
	command cmd;
	string tgf_file;
	int status = process_args(argc, argv, cmd, tgf_file);
	//DBG(cout << "process_args status: " << status << "\n";)
	DBG(cmd.print(cout << "command options:\t") << endl;)
	if (status) return status;

	if (cmd.get<bool>("help")) {
		help(cout, cmd);
		return 0;
	}

	if (cmd.name() == "show") {
		show(tgf_file,
			cmd.get<string>("start"),
			cmd.get<bool>("grammar"));
	} else if (cmd.name() == "gen") {
		gen(cout, tgf_file,
			cmd.get<string>("name"),
			cmd.get<string>("start"),
			cmd.get<string>("char-type"),
			cmd.get<string>("terminal-type"),
			cmd.get<string>("decoder"),
			cmd.get<string>("encoder"));
	} else if (cmd.name() == "parse") {
		string infile = cmd.get<string>("input");
		string inexp  = cmd.get<string>("input-expression");
		if (infile.size() && inexp.size())
			return error(argv[0],"multiple inputs specified, use ei"
				"ther --input or --input-expression, not both");
		nonterminals nts;
		grammar g = tgf<char>::from_file(nts, tgf_file,
						cmd.get<string>("start"));
		if (cmd.get<bool>("grammar"))
			g.print_internal_grammar(cout << "grammar:\n") << endl;
		parser p(g);
		std::unique_ptr<parser<char>::pforest> f;
		if (infile.size())
			if (infile == "-") // stdin
				f = p.parse(cin);
			else { // file
				int fd;
				if ((fd = ::open(infile.c_str(), O_RDONLY))==-1)
					return error("failed to open file " +
						infile + "':" +strerror(errno));
				f = p.parse(fd);
			}
		else // string
			f = p.parse(inexp.c_str(), inexp.size());
		if (!p.found()) return error(p.get_error().to_str());
		parsed(move(f), cmd);
	}
	return 0;
}
