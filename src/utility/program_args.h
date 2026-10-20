// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__PROGRAM_ARGS_H__
#define __IDNI__PARSER__UTILITY__PROGRAM_ARGS_H__

#include <string>
#include <string_view>

#include "diagnostics.h"
#include "options.h"

namespace idni {

/// The texts that make a program known on its command line.
struct program_identity {
	/// The program name, for example "tgf". It gives the prefix of the
	/// environment variables: "tgf" reads `TGF_*`.
	std::string_view name;
	/// The text that --version prints, without the last line end.
	std::string version;
	/// The text that --license prints. A line end follows when the text
	/// does not end with one.
	std::string_view license;
};

/// The info flags, in the order of their priority.
enum class info_flag { none, help, version, license };

/// The arguments of a program run.
struct program_args {
	parsed_args args;
	/// The first info flag that argv gives, in the order help, version,
	/// license. A program prints it with print_info() and ends with 0.
	info_flag info = info_flag::none;
};

/// Write the environment and @p argv through the options of the thread.
/// It prints nothing: the report holds every message.
diagnostics::result<program_args> read_program_args(int argc, char** argv,
	const program_identity& who, const args_config& cfg);

/// Print the text of @p pa.info on std::cout. The help names the command
/// when argv gives one.
void print_info(const program_identity& who, const program_args& pa);

} // namespace idni

#include "program_args.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__PROGRAM_ARGS_H__
