// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "net/line_support.h"

#include <filesystem>
#include <string>
#include <string_view>

using namespace idni;
using jt = format::json::tree;

static bool lower_hex(std::string_view s) {
	for (const char c : s)
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
			return false;
	return true;
}

static bool digits(std::string_view s) {
	for (const char c : s) if (c < '0' || c > '9') return false;
	return !s.empty();
}

static std::string member_text(tref v, std::string_view key) {
	const std::string* f = format::json::string_member(v, key);
	return f ? *f : std::string();
}

TEST_SUITE("line support: ids and times") {
	TEST_CASE("a session id is 32 lowercase hex characters") {
		const auto a = new_session_id();
		const auto b = new_session_id();
		REQUIRE(a.has_value());
		REQUIRE(b.has_value());
		CHECK(a->size() == 32);
		CHECK(lower_hex(*a));
		CHECK(*a != *b);
	}
	TEST_CASE("the timestamp is UTC ISO 8601 with milliseconds") {
		const std::string t = line_log::timestamp();
		REQUIRE(t.size() == 24);
		CHECK(digits(t.substr(0, 4)));
		CHECK(t[4] == '-');
		CHECK(t[10] == 'T');
		CHECK(t[19] == '.');
		CHECK(digits(t.substr(20, 3)));
		CHECK(t[23] == 'Z');
	}
	TEST_CASE("the start time fits a file name") {
		const std::string t = line_log::start_time();
		REQUIRE(t.size() == 15);
		CHECK(digits(t.substr(0, 8)));
		CHECK(t[8] == '-');
		CHECK(digits(t.substr(9)));
	}
}

TEST_SUITE("line support: log entries") {
	TEST_CASE("a text entry holds the time, the direction and the line") {
		tref v = line_log::text("in", "{\"cmd\":\"a\"}");
		CHECK(member_text(v, "dir") == "in");
		CHECK(member_text(v, "line") == "{\"cmd\":\"a\"}");
		CHECK(member_text(v, "t").size() == 24);
	}
	TEST_CASE("a session event names the event and the session") {
		tref v = line_log::session_event("new", "abc");
		CHECK(member_text(v, "dir") == "event");
		CHECK(member_text(v, "event") == "new");
		CHECK(member_text(v, "session") == "abc");
	}
	TEST_CASE("open_session and append write one JSON line") {
		auto dir = fs::temp_folder("line_support_");
		REQUIRE(dir.has_value());
		const std::filesystem::path folder(dir.value());
		const std::string folder_text = folder.string();
		auto log = line_log::open_session(folder_text, "abc");
		REQUIRE(log.has_value());
		auto os = std::move(log).value();
		CHECK(line_log::append(os,
			line_log::session_event("new", "abc")));
		os.close();
		size_t files = 0;
		for (const auto& e : std::filesystem::directory_iterator(folder)) {
			++files;
			const std::string name = e.path().filename().string();
			CHECK(name.size() == 15 + 1 + 3 + 4);
			CHECK(name.substr(15) == "-abc.log");
			auto text = fs::read(fs::to_path(e.path().string()));
			REQUIRE(text.has_value());
			CHECK(text.value().find("\"event\":\"new\"")
				!= std::string::npos);
			CHECK(text.value().back() == '\n');
#ifndef _WIN32
			CHECK((static_cast<unsigned>(std::filesystem::status(e.path())
				.permissions()) & 0777u) == 0600u);
#endif
		}
		CHECK(files == 1);
		std::filesystem::remove_all(folder);
	}
	TEST_CASE("append to a closed stream is false") {
		fs::file_ostream os;
		CHECK_FALSE(line_log::append(os,
			line_log::session_event("new", "abc")));
	}
}
