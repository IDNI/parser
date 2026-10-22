// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_CLI_OPTIONS_H__
#define __IDNI__PARSER__TGF__TGF_CLI_OPTIONS_H__

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "defs.h"
#include "tgf_repl_evaluator_base.h"
#include "../utility/options.h"
#include "../utility/program_args.h"
#include "../utility/term_colors.h"

namespace idni {

/// The server of `tgf connect` without an address.
inline constexpr const char* tgf_default_host = "localhost";
inline constexpr std::uint16_t tgf_default_port = 4000;

/// The commands of the tool, in the order of tgf_option_set.commands.
enum class tgf_command { grammar, parse, gen, repl, serve, connect, test };

/// The commands of the tool and the options that only the tool declares.
inline const option_set tgf_option_set = {
	.options = {
		{ "help", "global", false,
			"detailed information about options",
			{ { "", 'h' }, { "grammar", 'h' }, { "parse", 'h' },
			  { "gen", 'h' }, { "repl", 'h' }, { "serve", 'h' },
			  { "connect", 'h' }, { "test", 'h' } }, false },
		{ "version", "global", false, "print version and exit",
			{ { "", 'v' } }, false },
		{ "license", "global", false, "print license and exit",
			{ { "", 'L' } }, false },

		{ "char-class-report", "cli", false,
			"print the derived character class report",
			{ { "grammar", 'C' } } },
		{ "grammar", "cli", true, "print the grammar",
			{ { "grammar", 'g', true }, { "parse", 'g', false },
			  { "repl", 'g', false } } },
		{ "nullable", "cli", false,
			"report nullable recursive productions",
			{ { "grammar", 'N' } } },
		{ "input", "cli", std::string(),
			"parse input from a file or STDIN if -",
			{ { "parse", 'i' } } },
		{ "input-expression", "cli", std::string(),
			"parse input from a provided string",
			{ { "parse", 'e' } }, true, true },
		{ "evaluate", "cli", std::string(),
			"run a REPL command and quit", { { "repl", 'e' } }, true,
			true },
		{ "legacy-repl", "cli", false, "use the legacy terminal REPL",
			{ { "repl", 'X' }, { "connect", 'X' } } },
		{ "session", "cli", std::string(),
			"session id of a tgf serve session",
			{ { "repl" }, { "connect" } }, false },
		{ "init-stdin", "cli", false,
			"read one init line before the first hello, used by "
			"tgf serve", { { "repl" } }, false },

		{ "char-type", "generator", std::string("char"),
			"input character type (e.g. char, char32_t)",
			{ { "gen", 'C' } } },
		{ "terminal-type", "generator", std::string("char"),
			"terminal character type (e.g. char, char32_t)",
			{ { "gen", 'T' } } },
		{ "utf8", "generator", false,
			"shorthand: --char-type char --terminal-type char32_t "
			"with utf8 recoders", { { "gen", 'U' } } },
		{ "name", "generator", std::string(),
			"name of the generated parser struct",
			{ { "gen", 'n' } } },
		{ "output-dir", "generator", std::string(), "output directory",
			{ { "gen", 'O' } } },
		{ "output", "generator", std::string(),
			"output file for the parser", { { "gen", 'o' } } },
		{ "namespace", "generator", std::string(),
			"namespace for the generated parser",
			{ { "gen", 'N' } } },
		{ "header-only", "generator", true,
			"keep the parser in a single header",
			{ { "gen", 'H' } } },
		{ "treemr", "generator", false,
			"add a matcher() member built on "
			"idni::treemr::matcher_for", { { "gen" } } },
		{ "decoder", "generator", std::string(), "decoder function",
			{ { "gen", 'd' } } },
		{ "encoder", "generator", std::string(), "encoder function",
			{ { "gen", 'e' } } },

		{ "port", "serve", std::size_t{ tgf_default_port },
			"TCP port of the server, 0 lets serve pick a free port",
			{ { "serve", 'p' }, { "connect", 'p' } } },
		{ "host", "serve", std::string(tgf_default_host),
			"host of the server, which binds 127.0.0.1 only: use "
			"port forwarding or tailscale serve to reach it from "
			"another machine", { { "connect", 'H' } } },
		{ "max-sessions", "serve", std::size_t{ 16 },
			"refuse a new session at this count", { { "serve" } } },
		{ "idle-timeout", "serve", std::size_t{ 30 },
			"end a session after this many minutes with no traffic",
			{ { "serve" } } },
		{ "session-memory", "serve", std::size_t{ 0 },
			"address space limit of a session in MB, 0 means no "
			"limit", { { "serve" } } },
		{ "max-line", "serve", std::size_t{ 16777216 },
			"largest request line in bytes, a longer line is an "
			"error", { { "serve" } } },
		{ "write-timeout", "serve", std::size_t{ 30 },
			"close a connection that does not drain its output "
			"within this many seconds, 0 disables",
			{ { "serve" } } },
		{ "first-line-timeout", "serve", std::size_t{ 30 },
			"close a connection that sends no first line within "
			"this many seconds, 0 disables", { { "serve" } } },
		{ "max-pending", "serve", std::size_t{ 16 },
			"refuse a new connection while this many have sent no "
			"first line", { { "serve" } } },
		{ "max-open-requests", "serve", std::size_t{ 64 },
			"stop reading a client while this many of its requests "
			"wait for an answer", { { "serve" } } },
		{ "max-answer", "serve", std::size_t{ 16777216 },
			"largest answer line in bytes: a longer line of a "
			"session ends it, connect refuses it",
			{ { "serve" }, { "connect" } } },
		{ "max-stderr-line", "serve", std::size_t{ 1048576 },
			"largest stderr line of a session in bytes, the tail "
			"of a longer line is dropped", { { "serve" } } },
		{ "detached-buffer", "serve", std::size_t{ 16777216 },
			"answers in bytes that a session keeps while no client "
			"is attached", { { "serve" } } },
		{ "end-timeout", "serve", std::size_t{ 5 },
			"stop a session that does not exit within this many "
			"seconds after its end", { { "serve" } } },
		{ "log-dir", "serve", std::string(),
			"directory of the session logs and server.log, empty "
			"uses ~/.tau/tgf/logs", { { "serve" } } },
		{ "no-log", "serve", false, "write no session logs",
			{ { "serve" } } },
		{ "debug-sessions", "serve", false,
			"print each session log line (in, out, stderr) on stdout",
			{ { "serve", 'd' } } },
	},
	.commands = {
		{ "grammar", "show information about grammar" },
		{ "parse", "parse an input string, file or stdin, exit with 1 "
			"when it does not parse" },
		{ "gen", "generate a parser code" },
		{ "repl", "run TGF repl" },
		{ "serve", "serve TGF sessions on a TCP port" },
		{ "connect", "run a REPL in a tgf serve session at "
			"[tgf://]<host>[:<port>], localhost:4000 by default", true },
		{ "test", "run tester (with provided tgf.test file)" },
	},
};

/// The options of tgf_repl_options.
inline const option_set tgf_repl_option_set = { .options = {
	{ "print-terminals", "repl", tgf_repl_options{}.print_terminals,
		"print the parsed terminals", {} },
	{ "status", "repl", tgf_repl_options{}.status,
		"print status after each command", {} },
	{ "print-ambiguity", "repl", tgf_repl_options{}.print_ambiguity,
		"print ambiguous nodes", {} },
	{ "print-graphs", "repl", tgf_repl_options{}.print_graphs,
		"print the parsed graph", {} },
	{ "print-tml-rules", "repl", tgf_repl_options{}.tml_rules,
		"print the forest as TML rules", {} },
	{ "print-tml-facts", "repl", tgf_repl_options{}.tml_facts,
		"print the forest as TML facts", {} },
	{ "print-input", "repl", tgf_repl_options{}.print_input,
		"print the input", {} },
	{ "colors", "ui", tgf_repl_options{}.colors, "use terminal colors", {} },
	{ "json", "cli", tgf_repl_options{}.json, "print and read JSON", {} },
} };

/// The entry of command @p c in tgf_option_set.commands.
inline const command_spec& tgf_command_spec(tgf_command c) {
	return tgf_option_set.commands[static_cast<size_t>(c)];
}

/// The owners whose options a REPL and a session see.
inline const std::array<std::string_view, 4> tgf_session_owners = {
	"grammar", "parse", "repl", "ui" };

/// The command surfaces of the options that the grammar, the parser and the
/// evaluator declare.
inline const std::vector<std::pair<std::string_view, option_surface>>
	tgf_surfaces = {
	{ "auto-disambiguate", { "grammar" } },
	{ "auto-disambiguate", { "parse" } },
	{ "auto-disambiguate", { "gen", 'a' } },
	{ "auto-disambiguate", { "repl" } },
	{ "auto-disambiguate", { "serve" } },
	{ "auto-disambiguate", { "test" } },
	{ "nodisambig-list", { "grammar" } },
	{ "nodisambig-list", { "parse" } },
	{ "nodisambig-list", { "gen", 'A' } },
	{ "nodisambig-list", { "repl" } },
	{ "nodisambig-list", { "serve" } },
	{ "nodisambig-list", { "test" } },
	{ "trim-terminals", { "grammar" } },
	{ "trim-terminals", { "parse" } },
	{ "trim-terminals", { "gen" } },
	{ "trim-terminals", { "repl" } },
	{ "trim-terminals", { "serve" } },
	{ "trim-terminals", { "test" } },
	{ "inline-char-classes", { "grammar" } },
	{ "inline-char-classes", { "parse" } },
	{ "inline-char-classes", { "gen" } },
	{ "inline-char-classes", { "repl" } },
	{ "inline-char-classes", { "serve" } },
	{ "inline-char-classes", { "test" } },
	{ "derive-char-classes", { "grammar" } },
	{ "derive-char-classes", { "parse", 'd' } },
	{ "derive-char-classes", { "gen" } },
	{ "derive-char-classes", { "repl" } },
	{ "derive-char-classes", { "serve" } },
	{ "derive-char-classes", { "test" } },
	{ "trim", { "grammar" } },
	{ "trim", { "parse" } },
	{ "trim", { "gen" } },
	{ "trim", { "repl" } },
	{ "trim", { "serve" } },
	{ "trim", { "test" } },
	{ "trim-children", { "grammar" } },
	{ "trim-children", { "parse" } },
	{ "trim-children", { "gen" } },
	{ "trim-children", { "repl" } },
	{ "trim-children", { "serve" } },
	{ "trim-children", { "test" } },
	{ "trim-children-terminals", { "grammar" } },
	{ "trim-children-terminals", { "parse" } },
	{ "trim-children-terminals", { "gen" } },
	{ "trim-children-terminals", { "repl" } },
	{ "trim-children-terminals", { "serve" } },
	{ "trim-children-terminals", { "test" } },
	{ "dont-trim-terminals-of", { "grammar" } },
	{ "dont-trim-terminals-of", { "parse" } },
	{ "dont-trim-terminals-of", { "gen" } },
	{ "dont-trim-terminals-of", { "repl" } },
	{ "dont-trim-terminals-of", { "serve" } },
	{ "dont-trim-terminals-of", { "test" } },
	{ "inline", { "grammar" } },
	{ "inline", { "parse" } },
	{ "inline", { "gen" } },
	{ "inline", { "repl" } },
	{ "inline", { "serve" } },
	{ "inline", { "test" } },
	{ "enabled-productions", { "grammar", 'G' } },
	{ "enabled-productions", { "parse", 'G' } },
	{ "enabled-productions", { "gen", 'G' } },
	{ "enabled-productions", { "repl", 'G' } },
	{ "enabled-productions", { "serve", 'G' } },
	{ "enabled-productions", { "test", 'G' } },
	{ "start", { "grammar", 's' } },
	{ "start", { "parse", 's' } },
	{ "start", { "gen", 's' } },
	{ "start", { "repl", 's' } },
	{ "start", { "serve", 's' } },
	{ "start", { "test" } },
	{ "char-classes", { "grammar" } },
	{ "char-classes", { "parse" } },
	{ "char-classes", { "gen" } },
	{ "char-classes", { "repl" } },
	{ "char-classes", { "serve" } },
	{ "char-classes", { "test" } },
	{ "highlight-auto", { "grammar" } },
	{ "highlight-auto", { "parse" } },
	{ "highlight-auto", { "gen" } },
	{ "highlight-auto", { "repl" } },
	{ "highlight-auto", { "serve" } },
	{ "highlight-auto", { "test" } },
	{ "highlight", { "grammar" } },
	{ "highlight", { "parse" } },
	{ "highlight", { "gen" } },
	{ "highlight", { "repl" } },
	{ "highlight", { "serve" } },
	{ "highlight", { "test" } },
	{ "dynamic", { "grammar" } },
	{ "dynamic", { "parse" } },
	{ "dynamic", { "gen", 'D' } },
	{ "dynamic", { "repl" } },
	{ "dynamic", { "serve" } },
	{ "dynamic", { "test" } },
	{ "variants", { "grammar" } },
	{ "variants", { "parse" } },
	{ "variants", { "gen" } },
	{ "variants", { "repl" } },
	{ "variants", { "serve" } },
	{ "variants", { "test" } },
	{ "variant", { "grammar" } },
	{ "variant", { "parse" } },
	{ "variant", { "gen" } },
	{ "variant", { "repl" } },
	{ "variant", { "serve" } },
	{ "variant", { "test" } },

	{ "measure", { "grammar", 'm' } },
	{ "measure", { "parse", 'm' } },
	{ "measure", { "repl", 'm' } },
	{ "error-verbosity", { "parse", 'v' } },
	{ "error-verbosity", { "repl", 'v' } },
	{ "tree-path", { "parse", 'P' } },

	{ "print-terminals", { "parse", 't' } },
	{ "print-ambiguity", { "parse", 'a' } },
	{ "print-ambiguity", { "repl", 'a' } },
	{ "print-graphs", { "parse", 'p' } },
	{ "print-graphs", { "repl", 'p' } },
	{ "print-tml-rules", { "parse" } },
	{ "print-tml-rules", { "repl" } },
	{ "print-tml-facts", { "parse" } },
	{ "print-tml-facts", { "repl" } },
	{ "print-input", { "parse", 'I' } },
	{ "colors", { "grammar", 'c' } },
	{ "colors", { "parse", 'c' } },
	{ "colors", { "repl", 'c' } },
	{ "colors", { "connect", 'c' } },
	{ "json", { "grammar", 'J' } },
	{ "json", { "parse", 'J' } },
	{ "json", { "gen", 'J' } },
	{ "json", { "repl", 'J' } },
	{ "json", { "serve", 'J' } },
	{ "json", { "connect", 'J' } },
	{ "json", { "test", 'J' } }
};

/// The program data of the command line of the tool.
inline const args_config tgf_args_config = {
	.name = "tgf",
	.help_header = "tgf <TGF file>",
	.description = "Tau Grammar Format (TGF) tool",
	.default_command = tgf_command_spec(tgf_command::repl).name,
	.default_command_when_inputs =
		tgf_command_spec(tgf_command::repl).name,
	.global_surface = "",
};

/// The colors of a report without an evaluator, per output. A `colors`
/// value from argv or `TGF_COLORS` holds for each output, a pipe included.
/// Without one, an output has colors only when it is a terminal.
inline diagnostics::sink_colors tgf_report_colors() {
	using diagnostics::color_mode;
	auto& opts = idni::options();
	const bool on = opts.find("colors") != nullptr
		&& opts.get<bool>("colors");
	return diagnostics::output_colors(!on ? color_mode::off
		: opts.given("colors") ? color_mode::always
		: color_mode::terminal);
}

/// The command line errors that both programs give.
inline constexpr std::string_view tgf_missing_grammar_file =
	"TGF file does not exist";
inline constexpr std::string_view tgf_one_address =
	"connect takes one server address at most";

/// Print @p msg as a command line error and return the exit code 1.
inline int command_line_error(std::string_view msg) {
	std::cerr << msg << "\n";
	return 1;
}

/// Read the environment (`TGF_*`) and @p argv with read_program_args(),
/// print the report, then the text of an info flag. Without a value the
/// program ends with @p code.
inline std::optional<parsed_args> tgf_read_args(int argc, char** argv,
	const args_config& cfg, int& code)
{
	code = 1;
	const program_identity tgf{ "tgf", tauparser::full_version,
		tauparser::license };
	auto r = read_program_args(argc, argv, tgf, cfg);
	tgf_repl_evaluator_base::print_report(r.report(), false,
		tgf_report_colors());
	if (!r.has_value()) return std::nullopt;
	if (r.value().info != info_flag::none) {
		print_info(tgf, r.value());
		code = 0;
		return std::nullopt;
	}
	return std::move(r.value().args);
}

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_CLI_OPTIONS_H__
