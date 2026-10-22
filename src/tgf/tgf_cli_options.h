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
inline constexpr uint16_t tgf_default_port = 4000;

/// The commands of the tool, in the order of tgf_option_set().commands.
enum class tgf_command { grammar, parse, gen, repl, serve, connect, test };

/// The commands of the tool and the options that only the tool declares.
inline const option_set& tgf_option_set() {
	static const option_set v = {
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
	return v;
}

/// The entry of command @p c in tgf_option_set().commands.
inline const command_spec& tgf_command_spec(tgf_command c) {
	return tgf_option_set().commands[static_cast<size_t>(c)];
}

/// The options of tgf_repl_options.
inline const option_set& tgf_repl_option_set() {
	static const option_set v = { .options = {
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
	return v;
}

/// The owners whose options a REPL and a session see.
inline const std::array<std::string_view, 4> tgf_session_owners = {
	"grammar", "parse", "repl", "ui" };

/// The commands that read a grammar, so each grammar option shows on them.
inline const std::vector<std::string_view>& tgf_grammar_commands() {
	static const std::vector<std::string_view> v = {
		"grammar", "parse", "gen", "repl", "serve", "test" };
	return v;
}

/// One option on its commands. The short letter shows on each command of
/// @p lettered, or on every command when lettered is empty.
struct tgf_surface_row {
	std::string_view name;
	std::vector<std::string_view> commands;
	char letter = '\0';
	std::vector<std::string_view> lettered = {};
};

/// The commands of the options that the grammar, the parser and the
/// evaluator declare.
inline const std::vector<tgf_surface_row>& tgf_surface_rows() {
	static const std::vector<tgf_surface_row> v = {
		{ "auto-disambiguate", tgf_grammar_commands(), 'a', { "gen" } },
		{ "nodisambig-list", tgf_grammar_commands(), 'A', { "gen" } },
		{ "trim-terminals", tgf_grammar_commands() },
		{ "inline-char-classes", tgf_grammar_commands() },
		{ "derive-char-classes", tgf_grammar_commands(), 'd', { "parse" } },
		{ "trim", tgf_grammar_commands() },
		{ "trim-children", tgf_grammar_commands() },
		{ "trim-children-terminals", tgf_grammar_commands() },
		{ "dont-trim-terminals-of", tgf_grammar_commands() },
		{ "inline", tgf_grammar_commands() },
		{ "enabled-productions", tgf_grammar_commands(), 'G' },
		{ "start", tgf_grammar_commands(), 's',
			{ "grammar", "parse", "gen", "repl", "serve" } },
		{ "char-classes", tgf_grammar_commands() },
		{ "highlight-auto", tgf_grammar_commands() },
		{ "highlight", tgf_grammar_commands() },
		{ "dynamic", tgf_grammar_commands(), 'D', { "gen" } },
		{ "variants", tgf_grammar_commands() },
		{ "variant", tgf_grammar_commands() },
		{ "measure", { "grammar", "parse", "repl" }, 'm' },
		{ "error-verbosity", { "parse", "repl" }, 'v' },
		{ "tree-path", { "parse" }, 'P' },
		{ "print-terminals", { "parse" }, 't' },
		{ "print-ambiguity", { "parse", "repl" }, 'a' },
		{ "print-graphs", { "parse", "repl" }, 'p' },
		{ "print-tml-rules", { "parse", "repl" } },
		{ "print-tml-facts", { "parse", "repl" } },
		{ "print-input", { "parse" }, 'I' },
		{ "colors", { "grammar", "parse", "repl", "connect" }, 'c' },
		{ "json", { "grammar", "parse", "gen", "repl", "serve", "connect",
			"test" }, 'J' },
	};
	return v;
}

/// The command surfaces of the options that the grammar, the parser and the
/// evaluator declare, one per option and command.
inline const std::vector<std::pair<std::string_view, option_surface>>&
	tgf_surfaces()
{
	static const std::vector<std::pair<std::string_view, option_surface>>
		v = [] {
		std::vector<std::pair<std::string_view, option_surface>> out;
		for (const auto& row : tgf_surface_rows())
			for (const auto& cmd : row.commands) {
				const bool on = row.lettered.empty() || std::ranges::find(
					row.lettered, cmd) != row.lettered.end();
				out.push_back({ row.name,
					{ std::string(cmd), on ? row.letter : '\0' } });
			}
		return out;
	}();
	return v;
}


/// The program data of the command line of the tool.
inline const args_config& tgf_args_config() {
	static const args_config v = {
		.name = "tgf",
		.help_header = "tgf <TGF file>",
		.description = "Tau Grammar Format (TGF) tool",
		.default_command = tgf_command_spec(tgf_command::repl).name,
		.default_command_when_inputs =
			tgf_command_spec(tgf_command::repl).name,
		.global_surface = "",
	};
	return v;
}

/// The options without a connect surface that the client of `tgf connect`
/// reads from the environment.
inline const std::array<std::string_view, 1> tgf_connect_env_options = {
	"status" };

/// The options of the tgf-connect program: each option that the connect
/// command or the whole tool shows, and the environment options of the
/// client. Each one shows without a command.
inline option_set tgf_connect_option_set() {
	option_set out;
	for (const auto* set : { &tgf_option_set(), &tgf_repl_option_set() })
		for (const auto& o : set->options) {
			std::vector<option_surface> surfaces;
			auto take = [&surfaces](const option_surface& s) {
				if (s.command != "connect" && !s.command.empty())
					return;
				option_surface global = s;
				global.command.clear();
				// help prints the short letter of the first surface only
				if (surfaces.empty()) surfaces.push_back(global);
				else if (!surfaces[0].short_name)
					surfaces[0] = global;
			};
			for (const auto& s : o.surfaces) take(s);
			for (const auto& [name, s] : tgf_surfaces())
				if (name == o.name) take(s);
			const bool env = std::ranges::find(tgf_connect_env_options,
				o.name) != tgf_connect_env_options.end();
			if (surfaces.empty() && !env) continue;
			option_spec spec = o;
			spec.surfaces = std::move(surfaces);
			out.options.push_back(std::move(spec));
		}
	return out;
}

/// The program data of the command line of tgf-connect. The description
/// is the help of the connect command.
inline args_config tgf_connect_args_config() {
	return {
		.name = "tgf-connect",
		.help_header = "tgf-connect [<address>] [<TGF file>]",
		.description = tgf_command_spec(tgf_command::connect).help,
		.default_command = "",
		.default_command_when_inputs = "",
		.global_surface = "",
		.free_inputs = true,
	};
}

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
