// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_EVALUATOR_BASE_H__
#define __IDNI__PARSER__UTILITY__REPL_EVALUATOR_BASE_H__

#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

#include "utility/diagnostics.h"
#include "utility/options.h"
#include "utility/repl.h"
#include "utility/repl_history_policy.h"
#include "utility/repl_json.h"
#include "utility/repl_result.h"
#include "utility/term_colors.h"
#include "format/json/json.h"

namespace idni {

/// The options that every REPL evaluator reads.
struct repl_base_options {
	/// Show the status part of the prompt.
	bool status          = true;
	/// A text cut off at its end asks for more lines.
	bool continue_on_eof = true;
	bool colors          = true;
	/// Print and read JSON: no colors, no text on cout or cerr.
	bool json            = false;
};

/// Layer 1: the link of an evaluator to the front end that runs it. It
/// needs no parser.
struct repl_front_link {
	/// Set by repl<> and repl_ftxui<> through link_front().
	repl_front* front = nullptr;
	/// The history file and its filter of the front end.
	repl_history_policy history{};

	repl_front_link() = default;
	explicit repl_front_link(repl_history_policy h);
	virtual ~repl_front_link() = default;
	repl_front_link(const repl_front_link&) = delete;
	repl_front_link& operator=(const repl_front_link&) = delete;

	/// Set the prompt on the front end that runs.
	void reprompt();
protected:
	/// The prompt of the program, with its colors.
	virtual std::string prompt_text() const = 0;
};

/// Layer 2: the statement loop over the REPL parser @p repl_parser_t.
/// @p statement_nt is the nonterminal of one statement. A program with more
/// options passes its own @p options_t, derived from repl_base_options.
template <typename repl_parser_t, size_t statement_nt,
	typename options_t = repl_base_options>
struct repl_evaluator_base : repl_front_link {
	static_assert(std::derived_from<options_t, repl_base_options>);
	using trv = typename repl_parser_t::tree::traverser;
	using options = options_t;

	repl_evaluator_base(options_t o, repl_history_policy h);

	/// The evaluator interface of repl<> and repl_ftxui<>: run each
	/// statement of @p src. The value is 0 for a done text, 1 for a quit
	/// and 2 for a text that needs more lines.
	diagnostics::result<int> eval(const std::string& src);
	/// Move the pending report out and clear it.
	diagnostics::report take_report();
	/// Print the pending report and clear it.
	void flush_report();
	/// Print @p rep with the colors and the json option of this evaluator.
	void print_report(const diagnostics::report& rep) const;
	/// @p rep as one JSON line on stdout with @p json, else as text
	/// colored by @p tc: errors and warnings on stderr, info on stdout.
	static void print_report(const diagnostics::report& rep, bool json,
		const term::colors& tc, bool print_names = true);
	/// The same, with one color set for each of the three outputs.
	static void print_report(const diagnostics::report& rep, bool json,
		const diagnostics::sink_colors& tc, bool print_names = true);
	/// Symbolic name of @p s: "ok", "error", "incomplete" or "quit".
	static const char* status_name(cmd_status s);
	/// The status named @p name. An unknown name is cmd_status::error.
	static cmd_status status_of(std::string_view name);
	/// cmd_status::error when @p rep holds an error, else cmd_status::ok.
	static cmd_status report_status(const diagnostics::report& rep);
protected:
	/// Run one statement of eval() and print its answer.
	virtual cmd_result run_statement(const trv& s) = 0;
	/// Called with the parse of each REPL text that parses.
	virtual void on_parsed(typename repl_parser_t::result& r);
	/// Called at the end of each answer of eval().
	virtual void end_answer();

	/// Parse @p src as REPL text and run @p step on each statement until
	/// a quit or an incomplete answer. A text cut off at its end is
	/// cmd_status::incomplete when continue_on_eof is on, and its report
	/// holds the info node "incomplete input".
	eval_result run_statements(const std::string& src,
		const std::function<cmd_result(const trv&)>& step);
	/// Colors only in text mode with the colors option, and only on a
	/// terminal unless the user gave the `colors` option.
	void set_colors();

	options_t opt;
	/// The colors of the output on stdout.
	term::colors TC;
	/// The colors of the report, per output.
	diagnostics::sink_colors report_colors{ TC, TC, TC };
	diagnostics::report report;
};

} // namespace idni

#include "repl_evaluator_base.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__REPL_EVALUATOR_BASE_H__
