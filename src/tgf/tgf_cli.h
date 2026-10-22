// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_CLI_H__
#define __IDNI__PARSER__TGF__TGF_CLI_H__

#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "utility/diagnostics.h"
#include "utility/options.h"
#include "utility/repl_json.h"
#include "format/json/json.h"
#include "tgf_repl_parser.generated.h"
#include "tgf_repl_evaluator_base.h"
#include "tgf_cli_options.h"

namespace idni {

/// TGF entry point
int tgf_run(int argc, char** argv);

/// The main of the tgf program. Only main calls it, because on Windows it
/// reads the arguments from the command line of the process.
int tgf_main(int argc, char** argv);

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

/// The evaluator of repl<> and repl_ftxui<> that runs each statement on a
/// grammar it loads itself.
struct tgf_repl_evaluator : tgf_repl_evaluator_base {
	using parser_type = tgf_repl_parser::parser_type;
	using char_type = parser_type::char_type;
	using terminal_type = parser_type::terminal_type;
	using nonterminals_type = nonterminals<char_type, terminal_type>;
	using grammar_type = parser_type::grammar_type;
	using tree = tgf_repl_parser::tree;

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
	/// The start option of the grammar. Empty with no grammar.
	[[nodiscard]] std::string start_symbol() const;
	[[nodiscard]] bool has_fixed_grammar() const noexcept;

	/// Random id of the server session, empty in a standalone run. A
	/// non-empty id adds the hello.session field.
	std::string session_id{};

	/// Mark this evaluator as a server session: JSON only, no terminal
	/// colors, no file paths, and the given session id.
	void begin_server_session(const std::string& id);

	/// True when the option @p name exists and a REPL and a session see
	/// it: its owner is one of tgf_session_owners.
	[[nodiscard]] bool session_option(std::string_view name) const;

	bool reload(const std::string& new_tgf_file);

	/// Load a grammar from text and adopt it. @p name is a label for the
	/// grammar in state and hello. A later reload reloads the stored text.
	bool load_source(const std::string& name, const std::string& text);
	/// Load a grammar from the bytes of @p filename and keep those bytes
	/// as the source. A server uses this so a session and a reload see the
	/// bytes of the load, not a later change of the file.
	bool load_file_source(const std::string& filename);
	/// Data of the load command from source text: {"grammar", "loaded"}.
	tref load_source_data(const std::string& name,
		const std::string& text);
	/// Data of the reload command for a grammar loaded from source text.
	tref reload_source_data();

	/// Run one statement and return its data. Writes nothing to cout or
	/// cerr; errors and warnings go into the returned report.
	cmd_result run(const trv& n);
	/// Run one request text and return the data of each statement.
	eval_result run(const std::string& src);

	/// Every session option and its value as one object, in the order
	/// of the declaration.
	tref option_values() const;

	tref parsed(parser_type::result& r);
	tref parse(const char* input, size_t size);
	tref parse(std::istream& instream);
	tref parse(const std::string& infile);

	/// Data of get: the option @p name, or every option when it is empty.
	tref get_cmd(std::string_view name);
	/// Data of set: write the text @p text to the option @p name.
	tref set_cmd(std::string_view name,
		std::string_view text);
	/// Data of set: write @p v to the option @p name.
	tref set_option(std::string_view name,
		idni::option_value v);
	/// Data of add or delete: the entries of the list text @p text.
	tref list_cmd(std::string_view name,
		std::string_view text, bool add);
	/// Data of add or delete: add @p entries to the list option @p name,
	/// or remove them.
	tref edit_option_list(std::string_view name,
		const idni::option_list& entries, bool add);
	/// Data of toggle, enable and disable: the flag @p name gets
	/// @p update of its value.
	tref flag_cmd(std::string_view name,
		const std::function<bool(bool)>& update);
	/// True when @p name is a session option of the kind @p kind, or of
	/// any kind without one. Else an error goes to report.
	bool option_of_kind(std::string_view name,
		std::optional<option_kind> kind);

	shaping_options shaping();

	/// Bind the loaded grammar to options(), with no value of the grammar
	/// loaded before it.
	void bind_grammar_options();
	/// Write a grammar option through options(). An error goes to report.
	void set_grammar_option(std::string_view name, idni::option_value v);
	/// Declare tgf_repl_option_set and bind it to the fields of opt.
	void bind_repl_options();
	/// Declare parser_option_set() and give each parse option its default,
	/// before the first parser of the evaluator binds.
	void reset_parse_options();
	/// The option values that argv gave, written again after each load.
	void set_argv_values(
		std::vector<std::pair<std::string, option_value>> values);
	/// Write the argv values of the owner grammar to the current
	/// repository. An error goes to report.
	void write_argv_values();

	std::string nt_name(size_t id) const;

	/// One string for the production at index @p p, with no colors and
	/// no node ids, for example "expr => expr '+' term.".
	std::string production_string(size_t p) const;
	/// One entry of the production_ids array that parallels a
	/// productions array: index, head id, conjunct literal ids, guard
	/// and conjunctive.
	tref production_id_entry(size_t p) const;
	/// The {key: @p symbol, "productions", "production_ids"} data of the
	/// productions @p ps.
	tref productions_value(const char* key,
		const std::string& symbol, const std::set<size_t>& ps) const;

private:
	bool fixed_grammar = false;
	bool grammar_loaded = false;
	/// True when the options of tgf_repl_option_set are bound to opt.
	bool repl_options_bound = false;
	/// True when the grammar came from stored text, not a file.
	bool grammar_from_source = false;
	std::string tgf_filename;
	std::string grammar_source;

	std::unique_ptr<nonterminals_type> owned_nts;
	std::unique_ptr<grammar_type> owned_g;
	std::unique_ptr<parser_type> owned_p;
	parser_type* p_ = nullptr;
	/// The option values that argv gave, in argv order.
	std::vector<std::pair<std::string, option_value>> argv_values;

	/// Load a grammar file and adopt it, without printing. On success
	/// tgf_filename names @p filename.
	bool load_file(const std::string& filename);
	/// Report "server path" and return false when paths are forbidden.
	bool paths_allowed();
	/// Data of the load and reload commands: {"grammar", "loaded"}.
	tref reload_data(const std::string& new_tgf_file);
	/// The {"grammar", "loaded"} data of a load of @p name.
	tref load_answer(bool loaded,
		const std::string& name) const;

	tref state() const override;
	cmd_result run_statement(const trv& s) override;
	void on_parsed(tgf_repl_parser::result& r) override;
	void end_answer() override;
};

/// Specialized parser entry point (compiled-in grammar; no load/reload)
int tgf_specialized_run(int argc, char** argv,
	tgf_repl_evaluator::parser_type& parser,
	const char* display_name,
	const char* grammar_source);

/// Run the JSON request loop on @p in until end of input or a quit
/// request. With @p init_stdin, one init line is read first and applied
/// before the first hello. @p extra is appended to the first hello report.
inline constexpr size_t tgf_json_max_line = repl_json_max_line;
int tgf_json_loop(tgf_repl_evaluator& re, std::istream& in,
	std::ostream& out, size_t max_line = tgf_json_max_line,
	bool init_stdin = false,
	diagnostics::report extra = diagnostics::report{});

/// The {"grammar","start"} state object that every response carries.
tref state_value(const tgf_repl_evaluator& re);

/// One structured JSON request for one parsed REPL statement. A `load`
/// carries the field `file` and a `parse file` keeps that command name;
/// the client of `tgf serve` reads the file and rewrites both.
tref statement_request(const tgf_repl_evaluator::trv& n);

/// The JSON form of an option value: a bool, a number, a string or an
/// array of strings.
tref build_option(const option_value& v);
/// The option value of the JSON @p v. Nothing for another kind, a
/// negative number, a fraction or a number over 2^53 or the largest size_t.
std::optional<option_value> option_of(tref v);

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_CLI_H__
