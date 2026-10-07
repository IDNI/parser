// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_CLI_H__
#define __IDNI__PARSER__TGF__TGF_CLI_H__

#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "utility/diagnostics.h"
#include "utility/options.h"
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

/// The options of tgf_repl_evaluator::options.
inline const option_set tgf_repl_option_set = { .options = {
	{ "print-terminals", "repl", true, "print the parsed terminals", {} },
	{ "status", "repl", true, "print status after each command", {} },
	{ "print-ambiguity", "repl", true, "print ambiguous nodes", {} },
	{ "print-graphs", "repl", true, "print the parsed graph", {} },
	{ "print-tml-rules", "repl", false,
		"print the forest as TML rules", {} },
	{ "print-tml-facts", "repl", false,
		"print the forest as TML facts", {} },
	{ "print-input", "repl", false, "print the input", {} },
	{ "colors", "ui", true, "use terminal colors", {} },
	{ "json", "cli", false, "print and read JSON", {} },
} };

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

		bool status             = true;
		bool continue_on_eof    = true;
		bool colors             = true;
		bool print_input        = false;
		bool print_terminals    = true;
		bool print_graphs       = true;
		bool print_ambiguity    = true;
		bool tml_rules          = false;
		bool tml_facts          = false;
		/// Print and read JSON: no colors, no output on cout or cerr.
		bool json               = false;
		/// True when the evaluator refuses commands that read a file path.
		bool no_server_paths    = false;
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
	/// Unbinds the grammar options, also of a grammar it does not own,
	/// and the options of tgf_repl_option_set.
	~tgf_repl_evaluator();
	tgf_repl_evaluator(const tgf_repl_evaluator&) = delete;
	tgf_repl_evaluator& operator=(const tgf_repl_evaluator&) = delete;

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
	/// The text of the loaded grammar: the stored source, or the bytes of
	/// the grammar file. Empty when no grammar is loaded. A server sends
	/// it to a session child so the child starts with the same grammar.
	[[nodiscard]] std::string grammar_text() const;
	/// Mark the stored text of the loaded grammar as the source of a
	/// reload, so a reload reads no file. A built-in grammar keeps its own
	/// source. The grammar name stays.
	void make_grammar_source_backed();
	/// The start option, or the start literal of the grammar when the
	/// option is empty. Empty with no grammar.
	[[nodiscard]] std::string start_symbol() const;
	[[nodiscard]] bool has_fixed_grammar() const noexcept;

	/// Random id of the server session, empty in a standalone run. A
	/// non-empty id adds the hello.session field.
	std::string session_id{};

	/// Mark this evaluator as a server session: JSON only, no terminal
	/// colors, no file paths, and the given session id.
	void begin_server_session(const std::string& id);

	/// Move the pending report out and clear it. The JSON front end uses
	/// it for the hello line, which carries the grammar-load report.
	diagnostics::report take_report();

	/// The directives the loaded grammar file carries, as read at load
	/// time; null with no grammar. The JSON front end sends it in hello
	/// and in the load and reload answers.
	[[nodiscard]] format::json::value directives_value() const;

	bool reload(const std::string& new_tgf_file);

	/// Load a grammar from text and adopt it. @p name is a label for the
	/// grammar in state and hello. A later reload reloads the stored text.
	bool load_source(const std::string& name, const std::string& text);
	/// Load a grammar from the bytes of @p filename and keep those bytes
	/// as the source. A server uses this so a session and a reload see the
	/// bytes of the load, not a later change of the file.
	bool load_file_source(const std::string& filename);
	/// Data of the load command from source text: {"grammar", "loaded"}.
	format::json::value load_source_data(const std::string& name,
		const std::string& text);
	/// Data of the reload command for a grammar loaded from source text.
	format::json::value reload_source_data();

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

	/// Every declared option and its value as one object, in the order
	/// of the declaration.
	format::json::value option_values() const;

	format::json::value parsed(parser_type::result& r, std::string& text);
	format::json::value parse(const char* input, size_t size,
		std::string& text);
	format::json::value parse(std::istream& instream, std::string& text);
	format::json::value parse(const std::string& infile,
		std::string& text);

	/// Data of get: the option @p name, or every option when it is empty.
	format::json::value get_cmd(std::string_view name);
	/// Data of set: write the text @p text to the option @p name.
	format::json::value set_cmd(std::string_view name,
		std::string_view text);
	/// Data of set: write @p v to the option @p name.
	format::json::value set_option(std::string_view name,
		idni::option_value v);
	/// Data of add or delete: the entries of the list text @p text.
	format::json::value list_cmd(std::string_view name,
		std::string_view text, bool add);
	/// Data of add or delete: add @p entries to the list option @p name,
	/// or remove them.
	format::json::value edit_option_list(std::string_view name,
		const idni::option_list& entries, bool add);
	/// Data of toggle, enable and disable: the flag @p name gets
	/// @p update of its value.
	format::json::value flag_cmd(std::string_view name,
		const std::function<bool(bool)>& update);

	shaping_options shaping();

	/// Bind the loaded grammar to options(), with no value of the grammar
	/// loaded before it.
	void bind_grammar_options();
	/// Write a grammar option through options(). An error goes to report.
	void set_grammar_option(std::string_view name, idni::option_value v);
	/// Declare tgf_repl_option_set and bind it to the fields of opt.
	void bind_repl_options();
	/// Declare parser_option_set and give each parse option its default,
	/// before the first parser of the evaluator binds.
	void reset_parse_options();
	/// The option values that argv gave, written again after each load.
	void set_argv_values(
		std::vector<std::pair<std::string, option_value>> values);

	std::ostream& pretty_print(std::ostream& os, tref n,
		std::set<size_t> skip, bool nulls, size_t l);

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
	/// True when the options of tgf_repl_option_set are bound to opt.
	bool repl_options_bound = false;
	/// True when the grammar came from stored text, not a file.
	bool grammar_from_source = false;
	std::string tgf_filename;
	std::string grammar_source;

	repl<tgf_repl_evaluator>* r = nullptr;
#ifdef TAU_PARSER_HAS_FTXUI
	repl_ftxui<tgf_repl_evaluator>* r_ftx = nullptr;
#endif
	term::colors TC;
	idni::diagnostics::report report;
	/// Snapshot of the loaded grammar directives, taken when the grammar
	/// is installed or on the first read; null with no grammar.
	mutable format::json::value directives_{};
	/// True once directives_ holds the value for the loaded grammar.
	mutable bool directives_cached = false;

	std::unique_ptr<nonterminals_type> owned_nts;
	std::unique_ptr<grammar_type> owned_g;
	std::unique_ptr<parser_type> owned_p;
	parser_type* p_ = nullptr;
	/// The option values that argv gave, in argv order.
	std::vector<std::pair<std::string, option_value>> argv_values;

	/// Load a grammar file and adopt it, without printing. On success
	/// tgf_filename names @p filename.
	bool load_file(const std::string& filename);
	/// Write argv_values to the options of the loaded grammar and parser.
	void write_argv_values();
	/// Report "server path" and return false when paths are forbidden.
	bool paths_allowed();
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
/// request. With @p init_stdin, one init line is read first and applied
/// before the first hello. @p extra is appended to the first hello report.
inline constexpr size_t tgf_json_max_line = 16u * 1024 * 1024;
int tgf_json_loop(tgf_repl_evaluator& re, std::istream& in,
	std::ostream& out, size_t max_line = tgf_json_max_line,
	bool init_stdin = false,
	diagnostics::report extra = diagnostics::report{});

/// Write @p v as one JSON line and flush.
void json_write_line(std::ostream& os, const format::json::value& v);

/// The {"grammar","start"} state object that every response carries.
format::json::value state_value(const tgf_repl_evaluator& re);

/// Print @p report the way the text REPL does. With @p json true, print it
/// as one JSON line (with the code names when @p print_names is true).
/// Otherwise route errors and warnings to stderr and info to stdout, which
/// is @ref diagnostics::report::print. The client of `tgf serve` rebuilds a
/// report from its JSON and calls this too, so both print the same text.
void print_diagnostics_report(const diagnostics::report& report,
	bool json, bool print_names = true);

/// Text of one command result from its data only. The local REPL and the
/// remote evaluator of `tgf connect` share it, so the remote text matches
/// the local text.
void render_command_text(const cmd_result& r, std::ostream& os,
	const term::colors& TC,
	const std::set<size_t>* char_classes = nullptr);

/// One structured JSON request for one parsed REPL statement. A `load`
/// carries the field `file` and a `parse file` keeps that command name;
/// the client of `tgf serve` reads the file and rewrites both.
format::json::value statement_request(const tgf_repl_evaluator::trv& n);

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
