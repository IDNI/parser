// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "net/line_reader.h"

#include <cstring>
#include <string>
#include <vector>

using namespace idni;

TEST_SUITE("line reader") {
	TEST_CASE("feed_lines gives each line and drops a line over the limit") {
		std::string line;
		bool dropping = false;
		std::vector<std::string> got;
		const char* data = "ab\ncdefgh\nij\nk";
		feed_lines(line, dropping, 4, data, std::strlen(data),
			[&](std::string& l) { got.push_back(l); },
			[&] { got.emplace_back("<too long>"); });
		CHECK(got == std::vector<std::string>{ "ab", "<too long>", "ij" });
		CHECK(line == "k");
	}
}
