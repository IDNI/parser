// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TESTS__TGF_RUN_CAPTURE_H__
#define __IDNI__PARSER__TESTS__TGF_RUN_CAPTURE_H__

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../src/tgf/tgf_cli.h"

/// The exit code and the output of one tgf_run.
struct tgf_run_capture {
	int code = 0;
	std::string out;
	std::string err;
};

/// Run tgf_run on @p args with @p in as stdin. With @p err_to_out the
/// errors go to out in the order of the writes, else to err.
inline tgf_run_capture capture_tgf_run(const std::vector<std::string>& args,
	const std::string& in = "", bool err_to_out = true)
{
	std::vector<std::string> a = args;
	std::vector<char*> argv;
	for (auto& s : a) argv.push_back(s.data());
	std::ostringstream out, err;
	std::istringstream is(in);
	std::streambuf* old_out = std::cout.rdbuf(out.rdbuf());
	std::streambuf* old_err = std::cerr.rdbuf(
		err_to_out ? out.rdbuf() : err.rdbuf());
	std::streambuf* old_in = std::cin.rdbuf(is.rdbuf());
	tgf_run_capture r;
	r.code = idni::tgf_run(static_cast<int>(argv.size()), argv.data());
	std::cout.rdbuf(old_out);
	std::cerr.rdbuf(old_err);
	std::cin.rdbuf(old_in);
	r.out = out.str();
	r.err = err.str();
	return r;
}

#endif // __IDNI__PARSER__TESTS__TGF_RUN_CAPTURE_H__
