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
#ifndef __IDNI_TGF_CLI_H__
#define __IDNI_TGF_CLI_H__
#include <string>
#include <vector>

#include "cli.h"
#include "repl.h"
#include "term_colors.h"
#include "tgf_repl_parser.generated.h"
#include "traverser.h"

namespace idni {

// TGF options and descriptions
cli::options tgf_options();

// TGF commands, their options and descriptions
cli::commands tgf_commands();

struct tgf_repl_evaluator {
	using parser_type = tgf_repl_parser::parser_type;
	using nonterminals_type = nonterminals<
		parser_type::char_type, parser_type::terminal_type>;
	using grammar_type = grammar<
		parser_type::char_type, parser_type::terminal_type>;
	using node_variant_t = std::variant<tgf_repl_parser::symbol_type>;
	using sp_node_type = idni::rewriter::sp_node<node_variant_t>;
	using traverser_t = traverser<node_variant_t, tgf_repl_parser>;

	std::string tgf_file;
	struct options {
		bool status             = true;
		bool colors             = true;
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
		bool debug           = false;
	} opt;

	tgf_repl_evaluator(const std::string& tgf_file);
	tgf_repl_evaluator(const std::string& tgf_file, options opt);

	void set_repl(repl<tgf_repl_evaluator>& r_);
	void reprompt();

	int eval(const traverser_t& n);
	int eval(const std::string& src);

	void parse(const char* input, size_t size);
	void parse(std::istream& instream);
	void parse(const std::string& infile);
	void parsed(std::unique_ptr<parser<char>::pforest> f);

	void reload();
	void reload(const std::string& new_tgf_file);

	void get_cmd(const traverser_t& n);
	void set_cmd(const traverser_t& n);
	void add_cmd(const traverser_t& n);
	void del_cmd(const traverser_t& n);
	void update_bool_opt_cmd(const traverser_t& n,
		const std::function<bool(bool&)>& update_fn);

	parser_type::parse_options get_parse_options() const;
	std::ostream& pretty_print(std::ostream& os,
		const parser_type::psptree& n, std::set<size_t> skip,
		bool nulls, size_t l);
	repl<tgf_repl_evaluator>* r = 0;
	std::shared_ptr<nonterminals_type> nts;
	std::shared_ptr<grammar_type> g;
	std::shared_ptr<parser_type> p;
	term::colors TC;
};

} // namespace idni
#endif // __IDNI_TGF_CLI_H__