// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#ifndef __IDNI__PARSER__TGF__TGF_CLI_H__
#define __IDNI__PARSER__TGF__TGF_CLI_H__
#include <string>
#include <vector>

#include "utility/cli.h"
#include "utility/repl.h"
#include "utility/term_colors.h"
#include "tgf_repl_parser.generated.h"

namespace idni {

/// TGF options and descriptions
cli::options tgf_options();

/// TGF commands, their options and descriptions
cli::commands tgf_commands();

struct tgf_repl_evaluator {
	using parser_type = tgf_repl_parser::parser_type;
	using nonterminals_type = nonterminals<
		parser_type::char_type, parser_type::terminal_type>;
	using grammar_type = grammar<
		parser_type::char_type, parser_type::terminal_type>;
	using tree = tgf_repl_parser::tree;
	using trv  = tree::traverser;

	std::string tgf_file;
	struct options {
		bool debug              = false;
		bool status             = true;
		bool colors             = true;
		bool print_input        = false;
		bool print_terminals    = true;
		bool print_graphs       = true;
		bool print_ambiguity    = true;
		bool tml_rules          = false;
		bool tml_facts          = false;
		bool measure            = false;
		bool measure_each_pos   = false;
		bool measure_forest     = false;
		bool measure_preprocess = false;
		std::string start{"start"};
		parser_type::error::info_lvl error_verbosity =
			parser_type::error::info_lvl::INFO_BASIC;
		std::set<std::string> nodisambig_list{};
		std::set<std::string> to_trim{};
		std::set<std::string> dont_trim_terminals_of{};
		std::set<std::string> to_trim_children{};
		std::set<std::string> to_trim_children_terminals{};
		std::set<std::vector<std::string>> to_inline{};
	} opt;

	tgf_repl_evaluator(const std::string& tgf_file);
	tgf_repl_evaluator(const std::string& tgf_file, options opt);

	void set_repl(repl<tgf_repl_evaluator>& r_);
	void reprompt();

	int eval(const trv& n);
	int eval(const std::string& src);

	void parse(const char* input, size_t size);
	void parse(std::istream& instream);
	void parse(const std::string& infile);
	void parsed(parser_type::result& r);

	void reload();
	void reload(const std::string& new_tgf_file);

	void get_cmd(const trv& n);
	void set_cmd(const trv& n);
	void add_cmd(const trv& n);
	void del_cmd(const trv& n);
	void update_bool_opt_cmd(const trv& n,
		const std::function<bool(bool&)>& update_fn);

	std::vector<std::string> treepath(const trv& tp) const;

	void update_opts_by_grammar_opts();

	parser_type::parse_options get_parse_options() const;
	std::ostream& pretty_print(std::ostream& os, tref n,
		std::set<size_t> skip, bool nulls, size_t l);
	repl<tgf_repl_evaluator>* r = 0;
	std::shared_ptr<nonterminals_type> nts;
	std::shared_ptr<grammar_type> g;
	std::shared_ptr<parser_type> p;
	term::colors TC;
};

} // namespace idni
#endif // __IDNI__PARSER__TGF__TGF_CLI_H__