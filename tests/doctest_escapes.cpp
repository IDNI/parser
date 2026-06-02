// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "utility/escapes.h"

using namespace idni::escapes;

static std::string     dec (std::string_view s, const profile& p) { return decode(s, p).value(); }
static bool            ok  (std::string_view s, const profile& p) { return !decode(s, p).has_error(); }
static std::u32string  dcp (std::string_view s, const profile& p) { return u32decode(s, p).value(); }
static bool            cpok(std::string_view s, const profile& p) { return !u32decode(s, p).has_error(); }

// ---------------------------------------------------------------------------
TEST_SUITE("\\x lax (tgf_string: 1 or 2 hex digits)") {

	TEST_CASE("accepts 1 hex digit") {
		CHECK(dec("\\x4", tgf_string) == "\x04");
	}
	TEST_CASE("accepts 2 hex digits") {
		CHECK(dec("\\x41", tgf_string) == "A");
		CHECK(dec("\\xff", tgf_string) == "\xff");
	}
	TEST_CASE("rejects 0 digits") {
		CHECK_FALSE(ok("\\x", tgf_string));
	}
	TEST_CASE("stops at 2 digits — 3rd digit is literal") {
		CHECK(dec("\\x414", tgf_string) == "A4");
	}
	TEST_CASE("1 digit followed by non-hex is fine") {
		CHECK(dec("\\x4g", tgf_string) == "\x04g");
	}
}

// ---------------------------------------------------------------------------
TEST_SUITE("\\x fixed (exactly 2 hex digits)") {

	static constexpr profile fixed_x {
		.delim        = '"',
		.prefix       = '\\',
		.escape_chars = "",
		.hex_x        = hex_x_mode::fixed,
	};

	TEST_CASE("accepts exactly 2 hex digits") {
		CHECK(dec("\\x41", fixed_x) == "A");
	}
	TEST_CASE("rejects 1 digit") {
		CHECK_FALSE(ok("\\x4", fixed_x));
	}
	TEST_CASE("rejects non-hex second digit") {
		CHECK_FALSE(ok("\\x4g", fixed_x));
	}
	TEST_CASE("3rd digit is literal after the 2-digit escape") {
		CHECK(dec("\\x414", fixed_x) == "A4");
	}
}

// ---------------------------------------------------------------------------
TEST_SUITE("\\x greedy (c_like)") {

	TEST_CASE("accepts 1 hex digit") {
		CHECK(dec("\\x4", c_like) == "\x04");
	}
	TEST_CASE("accepts 2 hex digits") {
		CHECK(dec("\\x41", c_like) == "A");
	}
	TEST_CASE("stops at first non-hex char") {
		CHECK(dec("\\x41g", c_like) == "Ag");
	}
	TEST_CASE("rejects 0 digits") {
		CHECK_FALSE(ok("\\x", c_like));
	}
	TEST_CASE("rejects value above 0xFF") {
		CHECK_FALSE(ok("\\x100", c_like));
	}
	TEST_CASE("encode avoids ambiguous greedy hex before hex literal") {
		std::string raw;
		raw.push_back((char) 0x01);
		raw.push_back('0');
		std::string encoded = encode(raw, c_like);
		CHECK(encoded == "\\0010");
		CHECK(dec(encoded, c_like) == raw);
	}
}

// ---------------------------------------------------------------------------
TEST_SUITE("hex_u4_mode::scalar (tgf_string)") {

	TEST_CASE("accepts valid BMP scalar") {
		CHECK(dcp("\\u0041", tgf_string) == U"A");
	}
	TEST_CASE("rejects high surrogate") {
		CHECK_FALSE(cpok("\\uD800", tgf_string));
	}
	TEST_CASE("rejects low surrogate") {
		CHECK_FALSE(cpok("\\uDC00", tgf_string));
	}
	TEST_CASE("\\UHHHHHHHH accepts valid astral scalar") {
		CHECK(dcp("\\U0001F600", tgf_string) == U"\U0001F600");
	}
	TEST_CASE("\\UHHHHHHHH rejects value above U+10FFFF") {
		CHECK_FALSE(cpok("\\U00110000", tgf_string));
	}
}

// ---------------------------------------------------------------------------
TEST_SUITE("hex_u4_mode::utf16 (json)") {

	TEST_CASE("assembles surrogate pair for U+1F600") {
		CHECK(dcp("\\uD83D\\uDE00", json) == U"\U0001F600");
		CHECK(dec("\\uD83D\\uDE00", json) == "\xF0\x9F\x98\x80");
	}
	TEST_CASE("rejects lone high surrogate at end of input") {
		CHECK_FALSE(cpok("\\uD83D", json));
	}
	TEST_CASE("rejects lone low surrogate") {
		CHECK_FALSE(cpok("\\uDE00", json));
	}
	TEST_CASE("accepts regular BMP code point") {
		CHECK(dcp("\\u0041", json) == U"A");
	}
	TEST_CASE("custom prefix assembles surrogate pair") {
		static constexpr profile bang_json {
			.delim        = '"',
			.prefix       = '!',
			.escape_chars = "bfnrt/",
			.hex_u4       = hex_u4_mode::utf16,
		};
		CHECK(dcp("!uD83D!uDE00", bang_json) == U"\U0001F600");
	}
}

// ---------------------------------------------------------------------------
TEST_SUITE("built-in profile spot-checks") {

	TEST_CASE("JSON rejects \\x") {
		CHECK_FALSE(ok("\\x41", json));
	}
	TEST_CASE("JSON rejects octal") {
		CHECK_FALSE(ok("\\101", json));
	}
	TEST_CASE("JSON rejects \\U") {
		CHECK_FALSE(cpok("\\U00000041", json));
	}
	TEST_CASE("TGF accepts \\x41") {
		CHECK(dec("\\x41", tgf_string) == "A");
	}
}

// ---------------------------------------------------------------------------
TEST_SUITE("round-trips") {

	TEST_CASE("byte-domain 0x00..0xFF via tgf_char") {
		for (int b = 0; b <= 0xFF; b++) {
			std::string raw(1, (char)(unsigned char)b);
			std::string encoded = encode(raw, tgf_char);
			auto r = decode(encoded, tgf_char);
			REQUIRE(!r.has_error());
			CHECK(r.value() == raw);
		}
	}

	TEST_CASE("codepoint-domain ASCII controls via tgf_string") {
		for (char32_t cp = 0; cp < 0x20; cp++) {
			std::u32string in(1, cp);
			std::string encoded = encode(in, tgf_string);
			auto r = u32decode(encoded, tgf_string);
			REQUIRE(!r.has_error());
			CHECK(r.value() == in);
		}
	}

	TEST_CASE("codepoint-domain BMP scalars via tgf_string") {
		for (char32_t cp = 0x80; cp <= 0x2FFF; cp++) {
			if (cp >= 0xD800 && cp <= 0xDFFF) continue;
			std::u32string in(1, cp);
			std::string encoded = encode(in, tgf_string);
			auto r = u32decode(encoded, tgf_string);
			REQUIRE(!r.has_error());
			CHECK(r.value() == in);
		}
	}

	TEST_CASE("codepoint-domain astral scalars via tgf_string") {
		for (char32_t cp : { U'\U0001F600', U'\U00010000', U'\U0010FFFF' }) {
			std::u32string in(1, cp);
			std::string encoded = encode(in, tgf_string);
			auto r = u32decode(encoded, tgf_string);
			REQUIRE(!r.has_error());
			CHECK(r.value() == in);
		}
	}

	TEST_CASE("JSON raw UTF-8 encode+decode round-trip") {
		std::u32string in = U"\U0001F600 hello";
		std::string encoded = encode(in, json);
		auto r = u32decode(encoded, json);
		REQUIRE(!r.has_error());
		CHECK(r.value() == in);
	}
}

// ---------------------------------------------------------------------------
TEST_SUITE("encode: 8-bit / multibyte passthrough (cp1250, UTF-8)") {

	TEST_CASE("every high byte passes through raw — tgf_string") {
		for (int b = 0x80; b <= 0xFF; b++) {
			std::string raw(1, (char)(unsigned char)b);
			CHECK(encode(raw, tgf_string) == raw);
		}
	}

	TEST_CASE("every high byte passes through raw — tgf_char") {
		for (int b = 0x80; b <= 0xFF; b++) {
			std::string raw(1, (char)(unsigned char)b);
			CHECK(encode(raw, tgf_char) == raw);
		}
	}

	TEST_CASE("every high byte passes through raw — c_like") {
		for (int b = 0x80; b <= 0xFF; b++) {
			std::string raw(1, (char)(unsigned char)b);
			CHECK(encode(raw, c_like) == raw);
		}
	}

	TEST_CASE("every high byte passes through raw — csv") {
		for (int b = 0x80; b <= 0xFF; b++) {
			std::string raw(1, (char)(unsigned char)b);
			CHECK(encode(raw, csv) == raw);
		}
	}

	TEST_CASE("UTF-8 two-byte sequence passes through raw (U+00E9 é)") {
		std::string utf8 = "\xc3\xa9";
		CHECK(encode(utf8, tgf_string) == utf8);
		CHECK(dec(encode(utf8, tgf_string), tgf_string) == utf8);
	}

	TEST_CASE("UTF-8 four-byte sequence passes through raw (U+1F600 😀)") {
		std::string utf8 = "\xf0\x9f\x98\x80";
		CHECK(encode(utf8, tgf_string) == utf8);
		CHECK(dec(encode(utf8, tgf_string), tgf_string) == utf8);
	}

	TEST_CASE("cp1250 byte sequence passes through raw") {
		// Š š Ž ž ą ż in Windows-1250
		std::string cp1250 = "\x8a\x9a\x8e\x9e\xb9\xbf";
		CHECK(encode(cp1250, tgf_string) == cp1250);
		CHECK(dec(encode(cp1250, tgf_string), tgf_string) == cp1250);
	}

	TEST_CASE("regression: prefix char is still escaped (tgf_string)") {
		CHECK(encode("\\", tgf_string) == "\\\\");
	}

	TEST_CASE("regression: delimiter char is still escaped (tgf_string)") {
		CHECK(encode("\"", tgf_string) == "\\\"");
	}

	TEST_CASE("regression: control bytes 0x00..0x1F and 0x7F are still escaped (tgf_string)") {
		for (int b = 0x00; b <= 0x1F; b++) {
			std::string raw(1, (char)(unsigned char)b);
			auto enc = encode(raw, tgf_string);
			CHECK(enc != raw);
			CHECK(dec(enc, tgf_string) == raw);
		}
		{
			std::string raw(1, (char)(unsigned char)0x7F);
			auto enc = encode(raw, tgf_string);
			CHECK(enc != raw);
			CHECK(dec(enc, tgf_string) == raw);
		}
	}
}
