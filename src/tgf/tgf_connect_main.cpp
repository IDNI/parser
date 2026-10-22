// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "tgf_cli_options.h"
#include "tgf_connect.h"
#include "utility/win_args.h"

namespace idni {

// A missing file named *.tgf counts too, so a typo gives the file error
// and not a resolve error.
static bool is_grammar_input(const std::string& input) {
	return input.ends_with(".tgf") || std::ifstream(input).good();
}

static int tgf_connect_run(int argc, char** argv) {
	auto& opts = idni::options();
	if (auto r = opts.declare(tgf_connect_option_set()); r.has_error())
		return tgf_repl_evaluator_base::print_report(r.report(), false,
			tgf_report_colors()), 1;
	int code = 0;
	auto read = tgf_read_args(argc, argv, tgf_connect_args_config(), code);
	if (!read) return code;
	parsed_args args = std::move(*read);

	std::string grammar_file;
	std::vector<std::string> addresses;
	for (auto& input : args.inputs) {
		if (!is_grammar_input(input)) {
			addresses.push_back(std::move(input));
			continue;
		}
		if (!grammar_file.empty()) return command_line_error(
			"tgf-connect takes one grammar file at most");
		grammar_file = std::move(input);
	}
	if (addresses.size() > 1) {
		command_line_error(tgf_one_address);
		opts.help(std::cerr);
		return 1;
	}
	if (!grammar_file.empty() && !std::ifstream(grammar_file).good())
		return command_line_error(tgf_missing_grammar_file);
	args.inputs = std::move(addresses);
	return tgf_connect_command(args, grammar_file);
}

static int tgf_connect_main(int argc, char** argv) {
#ifdef _WIN32
	return win_utf8_main(argc, argv, tgf_connect_run);
#else
	return tgf_connect_run(argc, argv);
#endif
}

} // namespace idni

int main(int argc, char** argv) {
	return idni::tgf_connect_main(argc, argv);
}
