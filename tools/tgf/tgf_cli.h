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
//#include "terminal_color.h"
#include "tgf_repl_parser.generated.h"

namespace idni {

// TGF options and descriptions
cli::options tgf_options();

// TGF commands, their options and descriptions
cli::commands tgf_commands();

struct tgf_repl_evaluator {
	using nonterminals_type = nonterminals<char>;
	using grammar_type = grammar<char>;
	using parser_type = parser<char>;
	using sp_node_type = tgf_repl_parsing::sp_node_type;

	std::string tgf_file;
	struct options {
		bool status          = true;
		bool colors          = true;
		bool print_ambiguity = true;
		bool print_graphs    = true;
		bool tml_rules       = false;
		bool tml_facts       = false;
		std::string start{"start"};
		parser_type::error::info_lvl error_verbosity =
			parser_type::error::info_lvl::INFO_BASIC;
		bool debug           = false;
	} opt;

	tgf_repl_evaluator(const std::string& tgf_file);
	tgf_repl_evaluator(const std::string& tgf_file, options opt);

	void set_repl(repl<tgf_repl_evaluator>& r_);
	void reprompt();

	int eval(const sp_node_type& n);
	int eval(const std::string& src);

	void parse(const char* input, size_t size);
	void parse(std::istream& instream);
	void parse(const std::string& infile);
	void parsed(std::unique_ptr<parser<char>::pforest> f);

	void reload();
	void reload(const std::string& new_tgf_file);

	void get_cmd(const sp_node_type& n);
	void set_cmd(const sp_node_type& n);
	void update_bool_opt_cmd(const sp_node_type& n,
		const std::function<bool(bool&)>& update_fn);

	repl<tgf_repl_evaluator>* r = 0;
	std::shared_ptr<nonterminals<char>> nts;
	std::shared_ptr<grammar<char>> g;
	std::shared_ptr<parser<char>> p;
};

} // namespace idni
#endif // __IDNI_TGF_CLI_H__