// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <iostream>
#include <utility>

#include "program_args.h"

namespace idni {

inline diagnostics::result<program_args> read_program_args(int argc,
	char** argv, const program_identity& who, const args_config& cfg)
{
	diagnostics::result<program_args> r;
	auto& opts = idni::options();
	const std::string env_prefix = detail::option_env_name("", who.name)
		+ "_";
	if (!r.merge_ok(opts.load_env(env_prefix))) return r;
	auto parsed = r.merge_take(opts.parse_args(argc, argv, cfg));
	if (!parsed) return r;
	program_args pa{ std::move(*parsed) };
	if (opts.get<bool>("help")) pa.info = info_flag::help;
	else if (opts.get<bool>("version")) pa.info = info_flag::version;
	else if (opts.get<bool>("license")) pa.info = info_flag::license;
	return r.with_value(std::move(pa));
}

inline void print_info(const program_identity& who, const program_args& pa)
{
	switch (pa.info) {
	case info_flag::none: break;
	case info_flag::help:
		idni::options().help(std::cout, pa.args.command_given
			? pa.args.command : std::string());
		break;
	case info_flag::version: std::cout << who.version << "\n"; break;
	case info_flag::license:
		std::cout << who.license;
		if (!who.license.ends_with('\n')) std::cout << "\n";
		break;
	}
}

} // namespace idni
