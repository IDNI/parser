// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "utility/python_charclasses.h"
#include "utility/unicode_properties.h"

#include <string>
#include <string_view>
#include <utility>

using namespace idni::charclasses::unicode;
namespace python = idni::charclasses::python;

// The expected values come from the Unicode Character Database. Python
// unicodedata and the str methods give the same values for these points.
TEST_SUITE("unicode properties") {

	TEST_CASE("decimal_digit_value") {
		CHECK(decimal_digit_value(U'0') == 0);
		CHECK(decimal_digit_value(U'9') == 9);
		CHECK(decimal_digit_value(0xFF11) == 1);
		CHECK(decimal_digit_value(0x0661) == 1);
		CHECK(decimal_digit_value(0x0669) == 9);
		CHECK_FALSE(decimal_digit_value(0x00B2).has_value());
		CHECK_FALSE(decimal_digit_value(0x2460).has_value());
		CHECK_FALSE(decimal_digit_value(0x2160).has_value());
		CHECK_FALSE(decimal_digit_value(U'a').has_value());
		CHECK_FALSE(decimal_digit_value(0x0007).has_value());
		CHECK_FALSE(decimal_digit_value(0x110000).has_value());
	}

	TEST_CASE("has_digit_value") {
		for (char32_t cp : { U'1', char32_t{ 0xFF11 }, char32_t{ 0x0661 },
				char32_t{ 0x00B2 }, char32_t{ 0x2460 },
				char32_t{ 0x1369 }, char32_t{ 0x10A40 } })
		{
			CAPTURE(static_cast<uint32_t>(cp));
			CHECK(has_digit_value(cp));
		}
		for (char32_t cp : { U'a', U' ', char32_t{ 0x2160 },
				char32_t{ 0x2028 }, char32_t{ 0x0007 },
				char32_t{ 0xD800 }, char32_t{ 0x110000 } })
		{
			CAPTURE(static_cast<uint32_t>(cp));
			CHECK_FALSE(has_digit_value(cp));
		}
	}

	TEST_CASE("python::isspace") {
		for (char32_t cp : { U' ', U'\t', U'\n', char32_t{ 0x001C },
				char32_t{ 0x001F }, char32_t{ 0x0085 },
				char32_t{ 0x00A0 }, char32_t{ 0x2028 },
				char32_t{ 0x2029 }, char32_t{ 0x3000 } })
		{
			CAPTURE(static_cast<uint32_t>(cp));
			CHECK(python::isspace(cp));
		}
		for (char32_t cp : { U'a', U'1', char32_t{ 0x0007 },
				char32_t{ 0x200B }, char32_t{ 0xFF11 },
				char32_t{ 0x110000 } })
		{
			CAPTURE(static_cast<uint32_t>(cp));
			CHECK_FALSE(python::isspace(cp));
		}
	}

	TEST_CASE("python::isprintable") {
		for (char32_t cp : { U' ', U'a', U'~', char32_t{ 0x00B2 },
				char32_t{ 0x0130 }, char32_t{ 0xFF11 },
				char32_t{ 0x1F600 } })
		{
			CAPTURE(static_cast<uint32_t>(cp));
			CHECK(python::isprintable(cp));
		}
		for (char32_t cp : { char32_t{ 0x0007 }, char32_t{ 0x007F },
				char32_t{ 0x00A0 }, char32_t{ 0x00AD },
				char32_t{ 0x2028 }, char32_t{ 0x2029 },
				char32_t{ 0x3000 }, char32_t{ 0xD800 },
				char32_t{ 0xE000 }, char32_t{ 0x0378 },
				char32_t{ 0x110000 } })
		{
			CAPTURE(static_cast<uint32_t>(cp));
			CHECK_FALSE(python::isprintable(cp));
		}
	}

	TEST_CASE("to_lower") {
		CHECK(to_lower(U'A') == U'a');
		CHECK(to_lower(U'a') == U'a');
		CHECK(to_lower(0x0130) == 0x0069);
		CHECK(to_lower(0x2160) == 0x2170);
		CHECK(to_lower(0x0410) == 0x0430);
		CHECK(to_lower(0x10400) == 0x10428);
		CHECK(to_lower(0xFF11) == 0xFF11);
		CHECK(to_lower(0x0007) == 0x0007);
		CHECK(to_lower(0x110000) == 0x110000);
	}

	TEST_CASE("to_lower_full") {
		std::u32string out;
		to_lower_full(0x0130, out);
		CHECK(out == std::u32string{ 0x0069, 0x0307 });
		to_lower_full(U'A', out);
		CHECK(out == std::u32string{ 0x0069, 0x0307, U'a' });
		std::u32string one;
		to_lower_full(0x00DF, one);
		CHECK(one == std::u32string{ 0x00DF });
		one.clear();
		to_lower_full(0x03A3, one);
		CHECK(one == std::u32string{ 0x03C3 });
	}

	TEST_CASE("is_cased and is_case_ignorable") {
		CHECK(is_cased(U'A'));
		CHECK(is_cased(0x03A3));
		CHECK(is_cased(0x02B0));
		CHECK_FALSE(is_cased(U'1'));
		CHECK_FALSE(is_cased(U'.'));
		CHECK(is_case_ignorable(U'\''));
		CHECK(is_case_ignorable(U'.'));
		CHECK(is_case_ignorable(0x0301));
		CHECK(is_case_ignorable(0x00AD));
		CHECK(is_case_ignorable(0x02B0));
		CHECK_FALSE(is_case_ignorable(U'A'));
		CHECK_FALSE(is_case_ignorable(U' '));
	}

	// The expected strings come from Python str.lower.
	TEST_CASE("to_lower_full of a text has the final sigma") {
		const std::pair<std::u32string, std::u32string> cases[] = {
			{ U"\u039F\u0394\u039F\u03A3", U"\u03BF\u03B4\u03BF\u03C2" },
			{ U"\u03A3", U"\u03C3" },
			{ U"\u0391\u03A3.", U"\u03B1\u03C2." },
			{ U"\u0391\u03A3 \u0392", U"\u03B1\u03C2 \u03B2" },
			{ U"\u03A3.", U"\u03C3." },
			{ U"\u0130", U"\u0069\u0307" },
			{ U"\u0391\u0301\u03A3", U"\u03B1\u0301\u03C2" },
			{ U"\u0391'\u03A3", U"\u03B1'\u03C2" },
			{ U"\u0391\u03A3\u0301\u0392", U"\u03B1\u03C3\u0301\u03B2" },
			{ U"\u0391\u03A3\u0301", U"\u03B1\u03C2\u0301" },
			{ U"\u0391\u03A3\u0391", U"\u03B1\u03C3\u03B1" },
			{ U"\u0391\u03A31", U"\u03B1\u03C21" },
			{ U"1\u03A3", U"1\u03C3" },
			{ U"\u0391\u03A3\u03A3", U"\u03B1\u03C3\u03C2" },
			{ U"\u0391\u03A3\u00AD\u0392", U"\u03B1\u03C3\u00AD\u03B2" },
			{ U"ABC", U"abc" },
			{ U"", U"" },
		};
		for (const auto& [text, lower] : cases) {
			CAPTURE(text.size());
			std::u32string out;
			to_lower_full(std::u32string_view(text), out);
			CHECK(out == lower);
		}
		std::u32string out = U"x";
		to_lower_full(U"\u0391\u03A3", out);
		CHECK(out == U"x\u03B1\u03C2");
	}
}
