// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Unit tests for the program codes of the diagnostics.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "debug_skip.h"
#include "utility/program_codes.h"
#include "format/json/json.h"

#include <array>
#include <sstream>
#include <string>
#include <string_view>

using namespace idni::diagnostics;

namespace {

bool has_error_message(const report& rep, std::string_view msg) {
	for (const auto& n : rep.nodes())
		if (is_error(n.tag) && rep.str(n.key) == msg) return true;
	return false;
}

} // namespace

// The registry is global to the process, so each case uses its own codes.
TEST_SUITE("program codes") {

	TEST_CASE("the parser codes stay inside the parser bands") {
		constexpr std::array parser_codes{ code::unknown,
			code::invalid_utf8, code::warning, code::info,
			code::info_scope };
		for (auto c : parser_codes) {
			const auto n = static_cast<uint16_t>(c);
			CHECK(((parser_error_band.first <= n
					&& n <= parser_error_band.last)
				||(parser_warning_band.first <= n
					&& n <= parser_warning_band.last)
				|| (parser_info_band.first <= n
					&& n <= parser_info_band.last)));
		}
	}

	TEST_CASE("a registered code gives its label through code_name") {
		const std::array codes{
			program_code_label{ program_error(0), "Unknown account." },
			program_code_label{ program_warning(0), "Low balance." },
			program_code_label{ program_info(0), "Synced." } };
		auto r = register_codes(codes);
		CHECK(r.has_value());
		CHECK_FALSE(r.report().has_error());
		CHECK(std::string_view(code_name(program_error(0)))
			== "Unknown account.");
		CHECK(std::string_view(code_name(program_warning(0)))
			== "Low balance.");
		CHECK(std::string_view(code_name(program_info(0)))
			== "Synced.");
	}

	TEST_CASE("a code that is not registered has no label") {
		CHECK(std::string_view(code_name(program_error(0x2FFF))).empty());
	}

	TEST_CASE("a report node carries the code and its label") {
		const std::array codes{
			program_code_label{ program_error(1), "Bad key." } };
		REQUIRE(register_codes(codes).has_value());
		report rep;
		rep.error(program_error(1), "the key has 31 bytes");
		REQUIRE(rep.nodes().size() >= 1);
		CHECK(rep.nodes().back().tag == program_error(1));
		CHECK(rep.has_error());
		std::ostringstream os;
		idni::format::json::print(rep, os, true);
		const std::string text = os.str();
		CHECK(text.find(R"({"tag":4097,"message":"Bad key.",)")
			!= std::string::npos);
	}

	TEST_CASE("a program warning code and info code reach the report") {
		const std::array codes{
			program_code_label{ program_warning(1), "Old node." },
			program_code_label{ program_info(1), "Fee paid." } };
		REQUIRE(register_codes(codes).has_value());
		report rep;
		rep.warning(program_warning(1), "the node runs 0.9");
		rep.info(program_info(1), "fee", size_t{ 12 });
		REQUIRE(rep.nodes().size() >= 2);
		const auto& w = rep.nodes()[rep.nodes().size() - 2];
		const auto& i = rep.nodes().back();
		CHECK(w.tag == program_warning(1));
		CHECK(i.tag == program_info(1));
		CHECK(i.value == 12);
		CHECK_FALSE(rep.has_error());
		std::ostringstream os;
		idni::format::json::print(rep, os, true);
		const std::string text = os.str();
		CHECK(text.find(R"({"tag":20481,"message":"Old node.",)")
			!= std::string::npos);
		CHECK(text.find(R"({"tag":36865,"message":"Fee paid.",)")
			!= std::string::npos);
	}

	TEST_CASE("a code outside the program bands is an error") {
		const std::array codes{
			program_code_label{ code::parse_error, "Mine." },
			program_code_label{ static_cast<code>(0xC000), "Unused." },
			program_code_label{ static_cast<code>(0x4FFF), "Parser." } };
		auto r = register_codes(codes);
		CHECK_FALSE(r.has_value());
		CHECK(has_error_message(r.report(), idni::parser_strings
			::messages::code_outside_program_bands));
		CHECK(std::string_view(code_name(code::parse_error))
			== "Parse error.");
		CHECK(std::string_view(code_name(static_cast<code>(0x4FFF)))
			.empty());
	}

	TEST_CASE("a code given twice is an error") {
		SUBCASE("twice in one call") {
			const std::array codes{
				program_code_label{ program_error(2), "One." },
				program_code_label{ program_error(2), "Two." } };
			auto r = register_codes(codes);
			CHECK_FALSE(r.has_value());
			CHECK(has_error_message(r.report(), idni::parser_strings
				::messages::code_registered_twice));
			CHECK(std::string_view(code_name(program_error(2)))
				.empty());
		}
		SUBCASE("again in a later call") {
			const std::array first{
				program_code_label{ program_error(3), "First." } };
			REQUIRE(register_codes(first).has_value());
			const std::array again{
				program_code_label{ program_error(3), "Again." } };
			auto r = register_codes(again);
			CHECK_FALSE(r.has_value());
			CHECK(has_error_message(r.report(), idni::parser_strings
				::messages::code_registered_twice));
			CHECK(std::string_view(code_name(program_error(3)))
				== "First.");
		}
	}

	TEST_CASE("a failed call registers no code of the call") {
		const std::array codes{
			program_code_label{ program_error(4), "Good." },
			program_code_label{ code::io_error, "Bad." } };
		CHECK_FALSE(register_codes(codes).has_value());
		CHECK(std::string_view(code_name(program_error(4))).empty());
	}

	TEST_CASE("a code without a label is an error") {
		const std::array codes{
			program_code_label{ program_error(5), nullptr } };
		auto r = register_codes(codes);
		CHECK_FALSE(r.has_value());
		CHECK(has_error_message(r.report(), idni::parser_strings
			::messages::code_without_label));
	}

	TEST_CASE("the severity of a program code follows its band") {
		CHECK(is_error(program_error(0)));
		CHECK(is_warning(program_warning(0)));
		CHECK(is_info(program_info(0)));
		CHECK_FALSE(is_metric(program_info(0)));

		CHECK(is_error(program_error(0x2FFF)));
		CHECK(is_warning(program_warning(0x2FFF)));
		CHECK(is_info(program_info(0x2FFF)));
		CHECK_FALSE(is_metric(program_info(0x2FFF)));

		CHECK(static_cast<uint16_t>(program_error(0x2FFF))
			== program_error_band.last);
		CHECK(static_cast<uint16_t>(program_warning(0x2FFF))
			== program_warning_band.last);
		CHECK(static_cast<uint16_t>(program_info(0x2FFF))
			== program_info_band.last);
	}

	TEST_CASE("a program code is a constant inside its band") {
		static_assert(program_error(0x2FFF) == static_cast<code>(0x3FFF));
		static_assert(program_code(program_warning_band, 1)
			== program_warning(1));
		static_assert(program_code(program_info_band, 0x2FFF)
			== static_cast<code>(0xBFFF));
		CHECK(program_code(program_error_band, 7) == program_error(7));
	}
}
