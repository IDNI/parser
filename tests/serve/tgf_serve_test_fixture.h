// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Shared fixture for the tgf serve and connect tests. Every platform runs
// `tgf serve --json --port 0` as a process and reads the port from its listening
// line, then connects with a plain socket.

#ifndef __IDNI__PARSER__TESTS__TGF_SERVE_TEST_FIXTURE_H__
#define __IDNI__PARSER__TESTS__TGF_SERVE_TEST_FIXTURE_H__

#include "../quiet_doctest.h"
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

// The options of an evaluator in a test: JSON output, terminal colors and
// the path restriction of a server session.
inline idni::tgf_repl_options repl_options(bool json = true,
	bool colors = true, bool no_server_paths = false)
{
	idni::tgf_repl_options opt;
	opt.json = json;
	opt.colors = colors;
	opt.no_server_paths = no_server_paths;
	return opt;
}

// The path of the tgf binary. ctest passes it through the environment so
// a Windows backslash needs no escaping; the build path is the fallback.
inline std::string tgf_binary_path() {
#ifdef _MSC_VER
	// the MSVC CRT marks getenv unsafe and asks for _dupenv_s
	char* raw = nullptr;
	std::size_t raw_size = 0;
	const bool read = _dupenv_s(&raw, &raw_size, "TGF_BINARY_PATH") == 0;
	const std::string p = read && raw ? raw : "";
	std::free(raw);
	if (!p.empty()) return p;
#else
	const char* p = std::getenv("TGF_BINARY_PATH");
	if (p && *p) return p;
#endif
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
		args.push_back("--json");
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
		auto v = idni::format::json::tree::get(std::string_view(line));
		REQUIRE(v.has_value());
		idni::tref l = idni::format::json::tree::get(v.value())
			.find("listening");
		REQUIRE(l != nullptr);
		idni::tref p = idni::format::json::tree::get(l).find("port");
		REQUIRE(p != nullptr);
		port = static_cast<uint16_t>(
			idni::format::json::tree::get(p).get_number());
	}

	uint16_t bound_port() const { return port; }

	child_process proc;
	uint16_t port = 0;
};

using jt = idni::format::json::tree;

// Parse one response line. A handle holds it, so a later gc keeps it.
inline idni::htref parse_line(const std::string& line) {
	auto p = jt::get(std::string_view(line));
	REQUIRE(p.has_value());
	return jt::geth(p.value());
}

// The member @p key of the object @p o, or nullptr when it is absent.
inline const jt* member(const jt& o, std::string_view key) {
	idni::tref m = o.find(key);
	return m ? &jt::get(m) : nullptr;
}
inline const jt* member(const jt* o, std::string_view key) {
	return o ? member(*o, key) : nullptr;
}
inline const jt* member(const idni::htref& o, std::string_view key) {
	return member(jt::get(o), key);
}

// True when a report node or one of its children has the key @p key.
inline bool json_node_has_key(const jt& node, const std::string& key) {
	std::vector<const jt*> pending{ &node };
	while (!pending.empty()) {
		const jt& current = *pending.back();
		pending.pop_back();
		if (auto k = member(current, "key"); k && k->is_string()
				&& k->get_string() == key) return true;
		if (auto ch = member(current, "children"); ch && ch->is_array())
			for (const jt& c : ch->children_trees())
				pending.push_back(&c);
	}
	return false;
}

// True when the report of @p resp, or of one of its eval results, holds
// the key @p key.
inline bool report_contains_key(const jt& resp, const std::string& key) {
	auto has_key = [&key](const jt& rep) {
		auto nodes = member(rep, "nodes");
		if (!nodes || !nodes->is_array()) return false;
		for (const jt& n : nodes->children_trees())
			if (json_node_has_key(n, key)) return true;
		return false;
	};
	if (auto rep = member(resp, "report"); rep && has_key(*rep))
		return true;
	// An eval response carries the report of each statement.
	if (auto res = member(resp, "results"); res && res->is_array())
		for (const jt& e : res->children_trees())
			if (auto rep = member(e, "report"); rep && has_key(*rep))
				return true;
	return false;
}
inline bool report_contains_key(const idni::htref& resp,
	const std::string& key)
{
	return report_contains_key(jt::get(resp), key);
}

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
