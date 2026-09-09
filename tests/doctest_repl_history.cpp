// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "utility/repl_history.h"

#include <cstdio>
#include <filesystem>

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

} // namespace

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
