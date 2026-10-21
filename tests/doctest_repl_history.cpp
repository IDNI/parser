// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "utility/diagnostics.h"
#include "utility/repl.h"
#include "utility/repl_history.h"
#include "utility/repl_run.h"

#include <csignal>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
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
		: repl_history(repl_history_policy{ cleaned(p) }),
		  path(std::move(p)) {}
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
		repl<recording_evaluator> r(ev, "> ", { history });
		r.echo = false;
		r.run();
	}
	dup2_fd(saved, 0);
	close_fd(saved);
	std::filesystem::remove(in_path);
	return ev.seen;
}

// An evaluator whose command "loop." runs up to 20 steps and asks for a
// stop between two steps. The first loop raises SIGINT at its third step.
struct looping_evaluator {
	repl<looping_evaluator>* r = nullptr;
	bool raise_once = true;
	std::vector<int> steps;
	std::vector<bool> stop_at_start;
	diagnostics::result<int> eval(const std::string& s) {
		if (s == "loop.") {
			stop_at_start.push_back(r->stop_requested());
			int step = 0;
			while (step < 20 && !r->stop_requested()) {
				if (++step == 3 && raise_once)
					raise_once = false, std::raise(SIGINT);
			}
			steps.push_back(step);
		}
		diagnostics::result<int> res;
		res = 0;
		return res;
	}
};

} // namespace

TEST_SUITE("repl: stop_requested") {

	TEST_CASE("a SIGINT stops the loop of one command only") {
		const std::string in_path = cleaned("doctest_repl_stop_input.tmp");
		std::ofstream(in_path, std::ios::binary) << "loop.\nloop.\n";
		const int saved = dup_fd(0);
		const int in = open_read(in_path.c_str());
		REQUIRE(saved >= 0);
		REQUIRE(in >= 0);
		REQUIRE(dup2_fd(in, 0) == 0);
		close_fd(in);
		auto marker = [](int) {};
		using handler_t = void (*)(int);
		const handler_t before = std::signal(SIGINT, +marker);
		looping_evaluator ev;
		{
			repl<looping_evaluator> r(ev, "> ", { "" });
			r.echo = false;
			r.run();
			CHECK_FALSE(r.stop_requested());
		}
		const handler_t after = std::signal(SIGINT, before);
		dup2_fd(saved, 0);
		close_fd(saved);
		std::filesystem::remove(in_path);
		CHECK(after == +marker);
		CHECK(ev.steps == std::vector<int>{ 3, 20 });
		CHECK(ev.stop_at_start == std::vector<bool>{ false, false });
	}
}

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

TEST_SUITE("repl_history: policy") {

	bool not_key_import(std::string_view line) {
		return !line.starts_with("key import");
	}

	std::string file_text(const std::string& path) {
		std::ifstream f(path, std::ios::binary);
		return { std::istreambuf_iterator<char>(f),
			std::istreambuf_iterator<char>() };
	}

	TEST_CASE("keep drops a line from the memory and from the file") {
		const std::string path = cleaned("doctest_repl_history_keep.tmp");
		{
			repl_history h(repl_history_policy{ path, not_key_import });
			h.store("cmd1");
			h.store("key import secret");
			CHECK(h.size() == 1);
			h.store("cmd2");
			CHECK(h.size() == 2);
			auto up1 = h.prev("");
			REQUIRE(up1.has_value());
			CHECK(*up1 == "cmd2");
			auto up2 = h.prev(*up1);
			REQUIRE(up2.has_value());
			CHECK(*up2 == "cmd1");
			CHECK_FALSE(h.prev(*up2).has_value());
		}
		const std::string stored = file_text(path);
		CHECK(stored == "cmd1\ncmd2\n");
		CHECK(stored.find("key import") == std::string::npos);
		repl_history again(repl_history_policy{ path, not_key_import });
		CHECK(again.size() == 2);
		std::filesystem::remove(path);
	}

#ifndef _WIN32
	TEST_CASE("a store into a new history file makes it with mode 0600") {
		namespace sfs = std::filesystem;
		const std::string path = cleaned("doctest_repl_history_mode_new.tmp");
		{
			repl_history h(repl_history_policy{ path });
			h.store("cmd1");
		}
		CHECK(file_text(path) == "cmd1\n");
		CHECK((sfs::status(path).permissions() & sfs::perms::all)
			== (sfs::perms::owner_read | sfs::perms::owner_write));
		sfs::remove(path);
	}

	TEST_CASE("a store into an existing history file gives it mode 0600") {
		namespace sfs = std::filesystem;
		const std::string path = cleaned("doctest_repl_history_mode_old.tmp");
		{ std::ofstream f(path); f << "cmd0\n"; }
		const auto mode_0644 = sfs::perms::owner_read | sfs::perms::owner_write
			| sfs::perms::group_read | sfs::perms::others_read;
		sfs::permissions(path, mode_0644);
		{
			repl_history h(repl_history_policy{ path });
			h.store("cmd1");
		}
		CHECK(file_text(path) == "cmd0\ncmd1\n");
		CHECK((sfs::status(path).permissions() & sfs::perms::all)
			== (sfs::perms::owner_read | sfs::perms::owner_write));
		sfs::remove(path);
	}
#endif

	TEST_CASE("a history file that cannot be written gives a warning") {
		const std::string path = "doctest_repl_history_no_folder/history";
		repl_history h(repl_history_policy{ path });
		auto r = h.store("cmd1");
		CHECK(r.has_value());
		CHECK_FALSE(r.report().has_error());
		std::ostringstream os;
		r.report().print(os, term::colors(false));
		CHECK(os.str().find("Cannot write file") != std::string::npos);
		CHECK(os.str().find(path) != std::string::npos);
		CHECK(os.str().find("errno=") != std::string::npos);
		CHECK(h.size() == 1);
		CHECK_FALSE(std::filesystem::exists(path));
	}

	TEST_CASE("a UTF-8 file name names one file to write and to read") {
		namespace sfs = std::filesystem;
		const std::string path = "doctest_repl_history_\xc3\xa9.tmp";
		const sfs::path native(fs::to_path(path));
		std::error_code ec;
		sfs::remove(native, ec);
		{
			repl_history h(repl_history_policy{ path });
			REQUIRE(h.store("cmd1").has_value());
		}
		std::ifstream f(native, std::ios::binary);
		const std::string stored((std::istreambuf_iterator<char>(f)),
			std::istreambuf_iterator<char>());
		f.close();
		CHECK(stored == "cmd1\n");
		repl_history again(repl_history_policy{ path });
		CHECK(again.size() == 1);
		sfs::remove(native, ec);
	}

	TEST_CASE("a dropped line moves the cursor to the end") {
		repl_history h(repl_history_policy{ "", not_key_import });
		h.store("cmd1");
		h.store("cmd2");
		REQUIRE(h.prev("").has_value());
		h.store("key import secret");
		auto up = h.prev("");
		REQUIRE(up.has_value());
		CHECK(*up == "cmd2");
	}

	TEST_CASE("an empty file keeps the history in memory only") {
		namespace fs = std::filesystem;
		std::error_code ec;
		const fs::path home = fs::current_path(ec);
		const fs::path dir = home / "doctest_repl_history_memory.tmp";
		fs::remove_all(dir, ec);
		REQUIRE(fs::create_directory(dir, ec));
		fs::current_path(dir, ec);
		REQUIRE_FALSE(ec);
		{
			repl_history h{};
			h.store("cmd1");
			h.store("cmd2");
			CHECK(h.size() == 2);
		}
		const bool nothing_written = fs::is_empty(dir, ec);
		fs::current_path(home, ec);
		fs::remove_all(dir, ec);
		CHECK(nothing_written);
	}
}

TEST_SUITE("repl_front") {
	struct counting_front : idni::repl_front {
		std::string prompt;
		int clears = 0;
		void set_prompt(const std::string& p) override { prompt = p; }
		void clear() override { ++clears; }
		bool stop_requested() override { return false; }
	};
	struct front_evaluator { idni::repl_front* front = nullptr; };
	struct typed_evaluator { idni::repl<typed_evaluator>* r = nullptr; };

	TEST_CASE("link_front sets the member front and calls no fallback") {
		counting_front f;
		front_evaluator e;
		bool fell_back = false;
		idni::link_front(e, &f, [&](auto&) { fell_back = true; });
		REQUIRE(e.front == &f);
		CHECK_FALSE(fell_back);
		e.front->set_prompt("tgf> ");
		e.front->clear();
		CHECK(f.prompt == "tgf> ");
		CHECK(f.clears == 1);
	}

	TEST_CASE("link_front calls the fallback when the evaluator has no front") {
		counting_front f;
		typed_evaluator e;
		int calls = 0;
		idni::link_front(e, &f, [&](typed_evaluator& x) {
			CHECK(&x == &e);
			++calls;
		});
		CHECK(calls == 1);
		CHECK(e.r == nullptr);
	}

	TEST_CASE("repl is a repl_front") {
		CHECK(std::is_base_of_v<idni::repl_front,
			idni::repl<typed_evaluator>>);
		CHECK(std::is_base_of_v<idni::repl_front,
			idni::repl<front_evaluator>>);
	}
}

TEST_SUITE("repl: the history warnings of a run") {

	TEST_CASE("run gives the warning of each lost history line") {
		const std::string in_path = cleaned("doctest_repl_run_input.tmp");
		std::ofstream(in_path, std::ios::binary) << "a.\nb.\n";
		const int saved = dup_fd(0);
		const int in = open_read(in_path.c_str());
		REQUIRE(saved >= 0);
		REQUIRE(in >= 0);
		REQUIRE(dup2_fd(in, 0) == 0);
		close_fd(in);
		recording_evaluator ev;
		const std::string history = "doctest_repl_history_no_folder/history";
		diagnostics::result<int> run;
		{
			repl<recording_evaluator> r(ev, "> ", { history });
			r.echo = false;
			run = r.run();
		}
		dup2_fd(saved, 0);
		close_fd(saved);
		std::filesystem::remove(in_path);
		CHECK(ev.seen == std::vector<std::string>{ "a.", "b." });
		REQUIRE(run.has_value());
		CHECK(run.value() == 0);
		CHECK_FALSE(run.report().has_error());
		std::ostringstream os;
		run.report().print(os, term::colors(false));
		const std::string text = os.str();
		size_t lost = 0;
		for (size_t p = text.find("Cannot write file");
				p != std::string::npos;
				p = text.find("Cannot write file", p + 1)) ++lost;
		CHECK(lost == 2u);
	}
}

TEST_SUITE("run_repl") {

	// An evaluator that keeps one front end pointer and counts its prompts.
	struct prompting_evaluator {
		repl_front* front = nullptr;
		int prompts = 0;
		std::vector<std::string> seen;
		void reprompt() { ++prompts; front->set_prompt("t> "); }
		diagnostics::result<int> eval(const std::string& s) {
			seen.push_back(s);
			diagnostics::result<int> res;
			res = 0;
			return res;
		}
	};

	TEST_CASE("the line REPL runs, sets the prompt and gives the history warnings") {
		// This test has no FTXUI, so both values of legacy run the line REPL.
		for (const bool legacy : { true, false }) {
			CAPTURE(legacy);
			const std::string in_path = cleaned("doctest_run_repl_input.tmp");
			std::ofstream(in_path, std::ios::binary) << "a.\nb.\n";
			const int saved = dup_fd(0);
			const int in = open_read(in_path.c_str());
			REQUIRE(saved >= 0);
			REQUIRE(in >= 0);
			REQUIRE(dup2_fd(in, 0) == 0);
			close_fd(in);
			prompting_evaluator ev;
			std::ostringstream out;
			auto old = std::cout.rdbuf(out.rdbuf());
			auto run = run_repl(ev, legacy, repl_history_policy{
				"doctest_repl_history_no_folder/history" });
			std::cout.rdbuf(old);
			dup2_fd(saved, 0);
			close_fd(saved);
			std::filesystem::remove(in_path);
			CHECK(ev.prompts >= 1);
			CHECK(ev.seen == std::vector<std::string>{ "a.", "b." });
			REQUIRE(run.has_value());
			CHECK(run.value() == 0);
			CHECK_FALSE(run.report().has_error());
			CHECK_FALSE(run.report().nodes().empty());
		}
	}
}
