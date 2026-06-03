// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_CLI_H__
#define __IDNI__PARSER__TGF__TGF_CLI_H__

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "utility/cli.h"
#include "utility/diagnostics.h"
#include "utility/repl.h"
#include "utility/term_colors.h"
#include "tgf_repl_parser.generated.h"
#include "tgf_cli_options.h"

namespace idni {

/// TGF entry point
int tgf_run(int argc, char** argv);

/// TGF options and descriptions
cli::options tgf_options();

/// TGF commands, their options and descriptions
cli::commands tgf_commands();

struct tgf_repl_evaluator {
	friend struct repl<tgf_repl_evaluator>;

	using parser_type = tgf_repl_parser::parser_type;
	using char_type = parser_type::char_type;
	using terminal_type = parser_type::terminal_type;
	using nonterminals_type = nonterminals<char_type, terminal_type>;
	using grammar_type = parser_type::grammar_type;
	using tree = tgf_repl_parser::tree;
	using trv  = tree::traverser;

	struct options {
		options() = default;

		bool gc                 = true;
		bool debug              = false;
		bool status             = true;
		bool continue_on_eof    = true;
		bool colors             = true;
		bool print_input        = false;
		bool print_terminals    = true;
		bool print_graphs       = true;
		bool print_ambiguity    = true;
		bool tml_rules          = false;
		bool tml_facts          = false;
		bool print_json         = false;
		bool measure            = false;
		bool measure_each_pos   = false;
#ifdef TAU_PARSER_MEASURE_SCOPES
		bool measure_forest     = true;
		bool measure_preprocess = true;
#else
		bool measure_forest     = false;
		bool measure_preprocess = false;
#endif
		std::string start{};
		parser_type::error::info_lvl error_verbosity =
			parser_type::error::info_lvl::INFO_BASIC;
		parse_tree_path tree_path = parse_tree_path::bintree_path;
		bool auto_disambiguate = true;
		std::set<std::string> nodisambig_list{};
		std::set<std::string> to_trim{};
		std::set<std::string> dont_trim_terminals_of{};
		std::set<std::string> to_trim_children{};
		std::set<std::string> to_trim_children_terminals{};
		std::set<std::vector<std::string>> to_inline{};
	};

	explicit tgf_repl_evaluator(std::string tgf_file);
	tgf_repl_evaluator(std::string tgf_file, options opt);
	tgf_repl_evaluator(parser_type& parser,
		std::string grammar_filename,
		std::string grammar_source);
	tgf_repl_evaluator(parser_type& parser,
		std::string grammar_filename,
		std::string grammar_source,
		options opt);

	[[nodiscard]] bool good() const noexcept { return p_ != nullptr; }

	parser_type& p() { return *p_; }
	const parser_type& p() const { return *p_; }
	grammar_type& g() { return p().get_grammar(); }
	const grammar_type& g() const { return p().get_grammar(); }

	[[nodiscard]] const std::string& filename() const noexcept;
	[[nodiscard]] const std::string& source() const noexcept;
	[[nodiscard]] bool has_fixed_grammar() const noexcept;

	bool reload();
	bool reload(const std::string& new_tgf_file);

	void flush_report();
	void set_repl(repl<tgf_repl_evaluator>& r_);
	void reprompt();

	int eval(const trv& n);
	int eval(const std::string& src);

	void parse(const char* input, size_t size);
	void parse(std::istream& instream);
	void parse(const std::string& infile);
	void parsed(parser_type::result& r);

	void get_cmd(const trv& n);
	void set_cmd(const trv& n);
	void add_cmd(const trv& n);
	void del_cmd(const trv& n);
	void update_bool_opt_cmd(const trv& n,
		const std::function<bool(bool&)>& update_fn);

	std::vector<std::string> treepath(const trv& tp) const;
	void update_opts_by_grammar_opts();

	parser_type::parse_options get_parse_options();
	std::ostream& pretty_print(std::ostream& os, tref n,
		std::set<size_t> skip, bool nulls, size_t l);

	size_t nt_id(const std::string& s);
	std::string nt_name(size_t id) const;

private:
	options opt;
	bool fixed_grammar = false;
	std::string tgf_filename;
	std::string grammar_source;

	repl<tgf_repl_evaluator>* r = nullptr;
	term::colors TC;
	idni::diagnostics::report report;

	std::unique_ptr<nonterminals_type> owned_nts;
	std::unique_ptr<grammar_type> owned_g;
	std::unique_ptr<parser_type> owned_p;
	parser_type* p_ = nullptr;

	bool load_file(const std::string& filename);
	void print_source() const;
};

/// Specialized parser entry point (compiled-in grammar; no load/reload)
int tgf_specialized_run(int argc, char** argv,
	tgf_repl_evaluator::parser_type& parser,
	const char* display_name,
	const char* grammar_source);

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_CLI_H__
