// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Shared fixture for the tgf serve and connect tests. Every platform runs
// `tgf serve --port 0` as a process and reads the port from its listening
// line, then connects with a plain socket.

#ifndef __IDNI__PARSER__TESTS__TGF_SERVE_TEST_FIXTURE_H__
#define __IDNI__PARSER__TESTS__TGF_SERVE_TEST_FIXTURE_H__

#include "doctest.h"
#include "tgf/tgf_cli.h"
#include "format/json/json.h"
#include "tgf_serve_process.h"

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tgf_serve_test {

inline std::string grammar_path() {
	return std::string(PROJECT_SOURCE_DIR) + "/tests/fixtures/tiny.tgf";
}

inline idni::tgf_repl_evaluator::options json_options() {
	idni::tgf_repl_evaluator::options opt;
	opt.json_api = true;
	opt.print_json = true;
	return opt;
}

// The path of the tgf binary. ctest passes it through the environment so
// a Windows backslash needs no escaping; the build path is the fallback.
inline std::string tgf_binary_path() {
	const char* p = std::getenv("TGF_BINARY_PATH");
	if (p && *p) return p;
#ifdef _WIN32
	return std::string(PROJECT_BINARY_DIR) + "/tgf.exe";
#else
	return std::string(PROJECT_BINARY_DIR) + "/tgf";
#endif
}

// A server on a free port, started as a real process. @p grammar
// overrides the shared fixture path, so a test can start a server from
// its own copy of a grammar file.
struct serve_fixture {
	serve_fixture(size_t max_sessions = 16,
		size_t max_line = 16777216,
		const std::string& log_dir = "", bool no_log = true,
		const std::string& grammar = "",
		const std::vector<std::string>& extra = {})
	{
		std::vector<std::string> args;
		args.push_back(grammar.empty() ? grammar_path() : grammar);
		args.push_back("serve");
		args.push_back("--port");
		args.push_back("0");
		args.push_back("--max-sessions");
		args.push_back(std::to_string(max_sessions));
		args.push_back("--max-line");
		args.push_back(std::to_string(max_line));
		if (!log_dir.empty()) {
			args.push_back("--log-dir");
			args.push_back(log_dir);
		}
		if (no_log) args.push_back("--no-log");
		for (const auto& a : extra) args.push_back(a);
		REQUIRE(proc.start(tgf_binary_path(), args));
		std::string line;
		REQUIRE(proc.read_stdout_line(line, 30000));
		auto v = idni::format::json::parse(line);
		REQUIRE(v.has_value());
		auto p = v.value().find("listening");
		REQUIRE(p != nullptr);
		port = static_cast<uint16_t>(p->as_number());
	}

	uint16_t bound_port() const { return port; }

	child_process proc;
	uint16_t port = 0;
};

// A unique temporary directory, so the log test works on both platforms.
inline std::string make_temp_dir(const std::string& prefix) {
#ifdef _WIN32
	char buf[MAX_PATH] = {};
	::GetTempPathA(MAX_PATH, buf);
	std::string base(buf);
	// Forward slashes avoid the TGF string escape on a later load.
	for (auto& c : base) if (c == '\\') c = '/';
	std::string path = base + prefix
		+ std::to_string(::GetCurrentProcessId());
	::_mkdir(path.c_str());
	return path;
#else
	std::string tmpl = "/tmp/" + prefix + "XXXXXX";
	REQUIRE(::mkdtemp(tmpl.data()) != nullptr);
	return tmpl;
#endif
}

} // namespace tgf_serve_test

#endif // __IDNI__PARSER__TESTS__TGF_SERVE_TEST_FIXTURE_H__
