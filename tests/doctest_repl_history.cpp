// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "utility/diagnostics.h"
#include "utility/repl.h"
#include "utility/repl_history.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <share.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace idni;

namespace {

std::string cleaned(std::string p) { std::filesystem::remove(p); return p; }

// Each case gets its own history file so persisted entries from one case
// never leak into another; the file is removed on both entry and exit.
struct tmp_history : repl_history {
	std::string path;
	explicit tmp_history(std::string p)
		: repl_history(cleaned(p)), path(std::move(p)) {}
	~tmp_history() { std::filesystem::remove(path); }
};

// An evaluator whose command ends with a period; any other text asks the
// REPL for more lines.
struct recording_evaluator {
	repl<recording_evaluator>* r = nullptr;
	std::vector<std::string> seen;
	diagnostics::result<int> eval(const std::string& s) {
		seen.push_back(s);
		diagnostics::result<int> res;
		res = s.find('.') == std::string::npos ? 2 : 0;
		return res;
	}
};

#ifdef _WIN32
// the MSVC CRT marks _open unsafe and asks for _sopen_s
int open_read(const char* p) {
	int fd = -1;
	return ::_sopen_s(&fd, p, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0) == 0
		? fd : -1;
}
int dup_fd(int fd) { return ::_dup(fd); }
int dup2_fd(int from, int to) { return ::_dup2(from, to); }
int close_fd(int fd) { return ::_close(fd); }
#else
int open_read(const char* p) { return ::open(p, O_RDONLY); }
int dup_fd(int fd) { return ::dup(fd); }
int dup2_fd(int from, int to) { return ::dup2(from, to); }
int close_fd(int fd) { return ::close(fd); }
#endif

// Runs the plain REPL with @p input as its stdin, which is a file, so the
// REPL takes its pipe path.
std::vector<std::string> run_repl_on(const std::string& input,
	const std::string& history)
{
	const std::string in_path = cleaned("doctest_repl_history_input.tmp");
	std::ofstream(in_path, std::ios::binary) << input;
	const int saved = dup_fd(0);
	const int in = open_read(in_path.c_str());
	REQUIRE(saved >= 0);
	REQUIRE(in >= 0);
	REQUIRE(dup2_fd(in, 0) == 0);
	close_fd(in);
	recording_evaluator ev;
	{
		repl<recording_evaluator> r(ev, "> ", history);
		r.echo = false;
		r.run();
	}
	dup2_fd(saved, 0);
	close_fd(saved);
	std::filesystem::remove(in_path);
	return ev.seen;
}

} // namespace

TEST_SUITE("repl: a leading byte order mark") {

	TEST_CASE("a first command over two lines reaches eval without the mark") {
		const std::string history = cleaned("doctest_repl_history_bom.tmp");
		auto seen = run_repl_on("\xEF\xBB\xBF" "a =\nb.\n", history);
		REQUIRE(seen.size() == 2);
		// the REPL gives the whole buffer again, the first line included
		CHECK(seen[0] == "a =");
		CHECK(seen[1] == "a =\nb.");
		std::ifstream f(history, std::ios::binary);
		const std::string stored((std::istreambuf_iterator<char>(f)),
			std::istreambuf_iterator<char>());
		CHECK(stored.find("\xEF\xBB\xBF") == std::string::npos);
		f.close();
		std::filesystem::remove(history);
	}
}

TEST_SUITE("repl_history: prev/next draft stash") {

	TEST_CASE("Up then store: draft is not a real entry") {
		tmp_history h("doctest_repl_history_1.tmp");
		h.store("cmd1");
		h.store("cmd2");
		auto t = h.prev("d");
		REQUIRE(t.has_value());
		CHECK(*t == "cmd2");
		h.store("cmd3");
		CHECK(h.size() == 3);
	}

	TEST_CASE("Up then Down restores the draft") {
		tmp_history h("doctest_repl_history_2.tmp");
		h.store("cmd1");
		h.store("cmd2");
		auto up = h.prev("d");
		REQUIRE(up.has_value());
		CHECK(*up == "cmd2");
		auto down = h.next();
		REQUIRE(down.has_value());
		CHECK(*down == "d");
		CHECK(h.size() == 2);
	}

	TEST_CASE("Up twice then Down twice restores the draft") {
		tmp_history h("doctest_repl_history_3.tmp");
		h.store("cmd1");
		h.store("cmd2");
		auto up1 = h.prev("d");
		REQUIRE(up1.has_value());
		CHECK(*up1 == "cmd2");
		auto up2 = h.prev(*up1);
		REQUIRE(up2.has_value());
		CHECK(*up2 == "cmd1");
		auto down1 = h.next();
		REQUIRE(down1.has_value());
		CHECK(*down1 == "cmd2");
		auto down2 = h.next();
		REQUIRE(down2.has_value());
		CHECK(*down2 == "d");
		CHECK(h.size() == 2);
	}
}
