// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_CLI_H__
#define __IDNI__PARSER__TGF__TGF_CLI_H__

#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "utility/cli.h"
#include "utility/diagnostics.h"
#include "utility/repl.h"
#include "utility/term_colors.h"
#ifdef TAU_PARSER_HAS_FTXUI
#include "../utility/repl_ftxui.h"
#endif
#include "format/json/json.h"
#include "tgf_repl_parser.generated.h"
#include "tgf_cli_options.h"

namespace idni {

/// TGF entry point
int tgf_run(int argc, char** argv);

/// TGF options and descriptions
cli::options tgf_options();

/// TGF commands, their options and descriptions
cli::commands tgf_commands();

/// Status of one command, or of one request text as a whole.
enum class cmd_status { ok, error, incomplete, quit };

/// Symbolic name of @p s: "ok", "error", "incomplete" or "quit".
const char* cmd_status_name(cmd_status s);

/// Result of one statement.
struct cmd_result {
	std::string cmd;                        // long name: "set", "parse file"
	cmd_status status = cmd_status::ok;
	format::json::value data = format::json::value::object();
	std::string text;   // text mode only: colored output of parse,
			    // internal-grammar and unreachable
	diagnostics::report report;
};

/// Result of one request text (one or more statements).
struct eval_result {
	cmd_status status = cmd_status::ok;
	std::vector<cmd_result> results;
	diagnostics::report report;   // errors in the command text itself
};

/// One row for each REPL option, used by get_cmd() and render_text().
enum class option_kind { boolean, string_value, symbol_value, list, treepaths };

struct option_desc {
	size_t nt;          // tgf_repl_parser nonterminal
	const char* name;   // long name and JSON key: "print-graphs"
	const char* label;  // text label get prints: "print-graphs:  "
	option_kind kind;   // value shape of this option
};

/// The option row for @p name, or nullptr when no option has that name.
const option_desc* option_desc_by_name(std::string_view name);

struct tgf_repl_evaluator {
	friend struct repl<tgf_repl_evaluator>;
#ifdef TAU_PARSER_HAS_FTXUI
	friend struct repl_ftxui<tgf_repl_evaluator>;
#endif

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
		/// True when the evaluator is driven as a JSON API: no colors,
		/// no output on cout or cerr.
		bool json_api           = false;
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
		/// True when the user passed --auto-disambiguate on the command line.
		bool auto_disambiguate_user_set = false;
		std::set<std::string> to_trim{};
		std::set<std::string> dont_trim_terminals_of{};
		std::set<std::string> to_trim_children{};
		std::set<std::string> to_trim_children_terminals{};
		std::set<std::vector<std::string>> to_inline{};
	};

	explicit tgf_repl_evaluator(std::string tgf_file);
	tgf_repl_evaluator(std::string tgf_file, options opt);
	/// Construct an evaluator with no grammar loaded. A later load
	/// installs one.
	explicit tgf_repl_evaluator(options opt);
	tgf_repl_evaluator(parser_type& parser,
		std::string grammar_filename,
		std::string grammar_source);
	tgf_repl_evaluator(parser_type& parser,
		std::string grammar_filename,
		std::string grammar_source,
		options opt);

	[[nodiscard]] bool good() const noexcept { return p_ != nullptr; }
	/// True when a grammar is loaded.
	[[nodiscard]] bool has_grammar() const noexcept
		{ return grammar_loaded; }
	/// Report "no grammar loaded" and return false when no grammar is
	/// loaded. Commands that need productions call this first.
	bool require_grammar();

	parser_type& p() { return *p_; }
	const parser_type& p() const { return *p_; }
	grammar_type& g() { return p().get_grammar(); }
	const grammar_type& g() const { return p().get_grammar(); }

	[[nodiscard]] const std::string& filename() const noexcept;
	[[nodiscard]] const std::string& source() const noexcept;
	[[nodiscard]] const std::string& start_symbol() const noexcept;
	[[nodiscard]] bool has_fixed_grammar() const noexcept;

	/// Move the pending report out and clear it. The JSON front end uses
	/// it for the hello line, which carries the grammar-load report.
	diagnostics::report take_report();

	bool reload(const std::string& new_tgf_file);

	void flush_report();
	void set_repl(repl<tgf_repl_evaluator>& r_);
	void reprompt();

	/// Run one statement and return its data. Writes nothing to cout or
	/// cerr; errors and warnings go into the returned report.
	cmd_result run(const trv& n);
	/// Run one request text. @p each runs after each statement, so the
	/// text REPL prints and flushes in the current order.
	eval_result run(const std::string& src,
		const std::function<void(cmd_result&)>& each = {});

	/// Render @p r as text. Success output is byte-identical to REPL text.
	void render_text(const cmd_result& r, std::ostream& os) const;

	idni::diagnostics::result<int> eval(const std::string& src);

	/// Option value as JSON, by option nonterminal.
	format::json::value option_value(size_t o) const;
	/// All 23 REPL options as one object, in the order get prints them.
	format::json::value option_values() const;

	format::json::value parsed(parser_type::result& r, std::string& text);
	format::json::value parse(const char* input, size_t size,
		std::string& text);
	format::json::value parse(std::istream& instream, std::string& text);
	format::json::value parse(const std::string& infile,
		std::string& text);

	format::json::value get_cmd(const trv& n);
	format::json::value set_cmd(const trv& n);
	format::json::value add_cmd(const trv& n);
	format::json::value del_cmd(const trv& n);
	format::json::value update_bool_opt_cmd(const trv& n,
		const std::function<bool(bool&)>& update_fn);

	std::vector<std::string> treepath(const trv& tp) const;
	/// Shaping options from the REPL lists in opt. update_opts_by_grammar_opts()
	/// fills them from the grammar lists.
	shaping_options shaping();

	void update_opts_by_grammar_opts();
	void apply_auto_disambiguate();

	parser_type::parse_options get_parse_options();
	std::ostream& pretty_print(std::ostream& os, tref n,
		std::set<size_t> skip, bool nulls, size_t l);

	size_t nt_id(const std::string& s);
	std::string nt_name(size_t id) const;

	/// One string for the production at index @p p, with no colors and
	/// no node ids, for example "expr => expr '+' term.".
	std::string production_string(size_t p) const;
	/// One entry of the production_ids array that parallels a
	/// productions array: index, head id, conjunct literal ids, guard
	/// and conjunctive.
	format::json::value production_id_entry(size_t p) const;

private:
	options opt;
	bool fixed_grammar = false;
	bool grammar_loaded = false;
	std::string tgf_filename;
	std::string grammar_source;

	repl<tgf_repl_evaluator>* r = nullptr;
#ifdef TAU_PARSER_HAS_FTXUI
	repl_ftxui<tgf_repl_evaluator>* r_ftx = nullptr;
#endif
	term::colors TC;
	idni::diagnostics::report report;

	std::unique_ptr<nonterminals_type> owned_nts;
	std::unique_ptr<grammar_type> owned_g;
	std::unique_ptr<parser_type> owned_p;
	parser_type* p_ = nullptr;

	bool load_file(const std::string& filename);
	/// Load a grammar file and adopt it, without printing. @p new_tgf_file
	/// is the file to load; on success tgf_filename names it.
	bool load_grammar(const std::string& new_tgf_file);
	/// Data of the load and reload commands: {"grammar", "loaded"}.
	format::json::value reload_data(const std::string& new_tgf_file);
	void print_source(std::ostream& os) const;
};

/// Specialized parser entry point (compiled-in grammar; no load/reload)
int tgf_specialized_run(int argc, char** argv,
	tgf_repl_evaluator::parser_type& parser,
	const char* display_name,
	const char* grammar_source);

/// Run the JSON request loop on @p in until end of input or a quit
/// request. Writes one response line per request and flushes each.
int tgf_json_loop(tgf_repl_evaluator& re, std::istream& in,
	std::ostream& out);

/// Write @p v as one JSON line and flush.
void json_write_line(std::ostream& os, const format::json::value& v);

/// The {"grammar","start"} state object that every response carries.
format::json::value state_value(const tgf_repl_evaluator& re);

/// Response of one eval request: {"id","status","results":[...],
/// "state":{...},"report"}.
format::json::value json_eval_response(const format::json::value& id,
	const eval_result& er, const format::json::value& state);

/// Response of one one-shot CLI command: {"cmd","status","result",
/// "state":{...},"report"}.
format::json::value json_result_response(cmd_status status,
	const std::string& cmd,
	const format::json::value& result,
	const format::json::value& state,
	const diagnostics::report& report);

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_CLI_H__
