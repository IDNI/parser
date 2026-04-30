// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// UTF-8 / UTF-16 codec tests.
// Buffers are exact-size on the heap so ASan/Valgrind catches any OOB read.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "init_test.h"
#include "utility/characters.h"
#include "parser.h"
#include "recoders.h"

#include <initializer_list>
#include <algorithm>

using idni::utf8char;
using idni::peek_codepoint;
using idni::emit_codepoint;
using idni::peek_codepoint_u16;
using idni::emit_codepoint_u16;

static size_t peek_exact(std::initializer_list<utf8char> bytes, char32_t& ch) {
	size_t l = bytes.size();
	auto* buf = new utf8char[l];
	std::copy(bytes.begin(), bytes.end(), buf);
	size_t r = peek_codepoint(buf, l, ch);
	delete[] buf;
	return r;
}

static size_t peek_exact_u16(std::initializer_list<char16_t> units,
	char32_t& ch)
{
	size_t l = units.size();
	auto* buf = new char16_t[l];
	std::copy(units.begin(), units.end(), buf);
	size_t r = peek_codepoint_u16(buf, l, ch);
	delete[] buf;
	return r;
}

TEST_SUITE("peek_codepoint truncation (no OOB read)") {
	TEST_CASE("2-byte lead with l=1 returns -1") {
		char32_t ch;
		CHECK(peek_exact({ 0xC2 }, ch) == (size_t)-1);
	}
	TEST_CASE("3-byte lead with l=1 or l=2 returns -1") {
		char32_t ch;
		CHECK(peek_exact({ 0xE0 }, ch) == (size_t)-1);
		CHECK(peek_exact({ 0xE0, 0xA0 }, ch) == (size_t)-1);
	}
	TEST_CASE("4-byte lead with l=1..3 returns -1") {
		char32_t ch;
		CHECK(peek_exact({ 0xF0 }, ch) == (size_t)-1);
		CHECK(peek_exact({ 0xF0, 0x90 }, ch) == (size_t)-1);
		CHECK(peek_exact({ 0xF0, 0x90, 0x80 }, ch) == (size_t)-1);
	}
}

TEST_SUITE("peek_codepoint correctness") {
	TEST_CASE("ASCII") {
		char32_t ch;
		CHECK(peek_exact({ 'A' }, ch) == 1);
		CHECK(ch == U'A');
	}
	TEST_CASE("2-byte: U+00A9 ©") {
		char32_t ch;
		CHECK(peek_exact({ 0xC2, 0xA9 }, ch) == 2);
		CHECK(ch == U'©');
	}
	TEST_CASE("3-byte: U+20AC €") {
		char32_t ch;
		CHECK(peek_exact({ 0xE2, 0x82, 0xAC }, ch) == 3);
		CHECK(ch == U'€');
	}
	TEST_CASE("4-byte: U+1F600 grinning face") {
		char32_t ch;
		CHECK(peek_exact({ 0xF0, 0x9F, 0x98, 0x80 }, ch) == 4);
		CHECK(ch == U'\U0001F600');
	}
	TEST_CASE("rejects overlong 2-byte for ASCII") {
		char32_t ch;
		CHECK(peek_exact({ 0xC0, 0xA0 }, ch) == (size_t)-1);
	}
	TEST_CASE("rejects surrogate U+D800 encoded as 3-byte") {
		char32_t ch;
		CHECK(peek_exact({ 0xED, 0xA0, 0x80 }, ch) == (size_t)-1);
	}
	TEST_CASE("rejects codepoint > U+10FFFF") {
		char32_t ch;
		CHECK(peek_exact({ 0xF4, 0x90, 0x80, 0x80 }, ch) == (size_t)-1);
	}
	TEST_CASE("rejects bad continuation byte") {
		char32_t ch;
		CHECK(peek_exact({ 0xC2, 0x20 }, ch) == (size_t)-1);
	}
}

TEST_SUITE("UTF-16 codec") {
	TEST_CASE("BMP roundtrip") {
		char16_t buf[2];
		CHECK(emit_codepoint_u16(U'A', buf) == 1);
		CHECK(buf[0] == u'A');

		char32_t ch;
		CHECK(peek_exact_u16({ u'A' }, ch) == 1);
		CHECK(ch == U'A');
	}
	TEST_CASE("non-BMP: U+1F600 → surrogate pair") {
		char16_t buf[2];
		CHECK(emit_codepoint_u16(U'\U0001F600', buf) == 2);
		CHECK(buf[0] == 0xD83D);
		CHECK(buf[1] == 0xDE00);

		char32_t ch;
		CHECK(peek_exact_u16({ 0xD83D, 0xDE00 }, ch) == 2);
		CHECK(ch == U'\U0001F600');
	}
	TEST_CASE("rejects unpaired high surrogate (truncated)") {
		char32_t ch;
		CHECK(peek_exact_u16({ 0xD83D }, ch) == (size_t)-1);
	}
	TEST_CASE("rejects unpaired low surrogate") {
		char32_t ch;
		CHECK(peek_exact_u16({ 0xDC00 }, ch) == (size_t)-1);
	}
	TEST_CASE("rejects bad surrogate pair (high+ASCII)") {
		char32_t ch;
		CHECK(peek_exact_u16({ 0xD83D, u'A' }, ch) == (size_t)-1);
	}
	TEST_CASE("emit rejects surrogate codepoint") {
		char16_t buf[2];
		CHECK(emit_codepoint_u16(0xD800, buf) == 0);
	}
	TEST_CASE("emit rejects > U+10FFFF") {
		char16_t buf[2];
		CHECK(emit_codepoint_u16(0x110000, buf) == 0);
	}
}

// Helper: feed bytes through utf8_to_u32_conv and return both the decoded
// codepoints and the byte position the input ends up at (so tests can
// assert that malformed leads do not over-consume follow-up bytes).
struct decode_result {
	std::vector<char32_t> codepoints;
	size_t consumed;
};
static decode_result run_decoder(const char* data, size_t len) {
	idni::parser<char, char32_t>::input in(data, len);
	decode_result out;
	while (!in.eof()) {
		auto cps = idni::utf8_to_u32_conv(in);
		out.codepoints.insert(out.codepoints.end(), cps.begin(), cps.end());
	}
	out.consumed = in.pos();
	return out;
}

TEST_SUITE("utf8_to_u32_conv recoder") {
	TEST_CASE("ASCII passes through") {
		auto r = run_decoder("hi", 2);
		CHECK(r.codepoints.size() == 2);
		CHECK(r.codepoints[0] == U'h');
		CHECK(r.codepoints[1] == U'i');
		CHECK(r.consumed == 2);
	}
	TEST_CASE("2-byte UTF-8 -> single codepoint") {
		// é = U+00E9 = 0xC3 0xA9
		const char data[] = "\xC3\xA9";
		auto r = run_decoder(data, 2);
		CHECK(r.codepoints.size() == 1);
		CHECK(r.codepoints[0] == U'é');
		CHECK(r.consumed == 2);
	}
	TEST_CASE("3-byte UTF-8 -> single codepoint") {
		// € = U+20AC = 0xE2 0x82 0xAC
		const char data[] = "\xE2\x82\xAC";
		auto r = run_decoder(data, 3);
		CHECK(r.codepoints.size() == 1);
		CHECK(r.codepoints[0] == U'€');
		CHECK(r.consumed == 3);
	}
	TEST_CASE("4-byte UTF-8 -> single codepoint (regression: old bound p!=3 truncated)") {
		// 😀 = U+1F600 = 0xF0 0x9F 0x98 0x80
		const char data[] = "\xF0\x9F\x98\x80";
		auto r = run_decoder(data, 4);
		CHECK(r.codepoints.size() == 1);
		CHECK(r.codepoints[0] == U'\U0001F600');
		CHECK(r.consumed == 4);
	}
	TEST_CASE("CP-1252 stray byte: emit U+FFFD, do NOT eat next byte") {
		// 0xE9 alone (CP-1252 'é') followed by '_'. Old code consumed
		// the '_' trying to complete a 3-byte sequence and dropped it.
		const char data[] = "\xE9_";
		auto r = run_decoder(data, 2);
		REQUIRE(r.codepoints.size() == 2);
		CHECK(r.codepoints[0] == 0xFFFD);
		CHECK(r.codepoints[1] == U'_');
		CHECK(r.consumed == 2);
	}
	TEST_CASE("CP-1252 stray byte before space: space preserved") {
		// 0xE9 followed by ' '. Old code ate the space too.
		const char data[] = "\xE9 _";
		auto r = run_decoder(data, 3);
		REQUIRE(r.codepoints.size() == 3);
		CHECK(r.codepoints[0] == 0xFFFD);
		CHECK(r.codepoints[1] == U' ');
		CHECK(r.codepoints[2] == U'_');
	}
	TEST_CASE("Truncated 2-byte sequence at EOF -> U+FFFD") {
		const char data[] = "\xC3";
		auto r = run_decoder(data, 1);
		REQUIRE(r.codepoints.size() == 1);
		CHECK(r.codepoints[0] == 0xFFFD);
	}
	TEST_CASE("Truncated 4-byte sequence at EOF -> U+FFFD") {
		const char data[] = "\xF0\x9F";
		auto r = run_decoder(data, 2);
		REQUIRE(r.codepoints.size() == 1);
		CHECK(r.codepoints[0] == 0xFFFD);
	}
	TEST_CASE("Lead byte followed by non-continuation: only lead consumed") {
		// 0xC3 followed by ASCII 'A' (not a continuation byte).
		const char data[] = "\xC3" "A";
		auto r = run_decoder(data, 2);
		REQUIRE(r.codepoints.size() == 2);
		CHECK(r.codepoints[0] == 0xFFFD);
		CHECK(r.codepoints[1] == U'A');
	}
	TEST_CASE("Mixed valid + malformed input round-trips") {
		// Valid 'a', valid é (UTF-8), stray 0xE9, valid 'b'
		const char data[] = "a" "\xC3\xA9" "\xE9" "b";
		auto r = run_decoder(data, 5);
		REQUIRE(r.codepoints.size() == 4);
		CHECK(r.codepoints[0] == U'a');
		CHECK(r.codepoints[1] == U'é');
		CHECK(r.codepoints[2] == 0xFFFD);
		CHECK(r.codepoints[3] == U'b');
	}
}

TEST_SUITE("UTF-8 ↔ UTF-16 conversions") {
	TEST_CASE("ASCII roundtrip") {
		std::string s = "hello";
		auto u16 = idni::to_u16string(s);
		CHECK(u16 == u"hello");
		CHECK(idni::to_string(u16) == s);
	}
	TEST_CASE("BMP roundtrip: ©€") {
		std::string s = "\xC2\xA9\xE2\x82\xAC";   // © €
		auto u16 = idni::to_u16string(s);
		CHECK(u16 == u"©€");
		CHECK(idni::to_string(u16) == s);
	}
	TEST_CASE("non-BMP roundtrip: 😀") {
		std::string s = "\xF0\x9F\x98\x80";
		auto u16 = idni::to_u16string(s);
		CHECK(u16.size() == 2); // surrogate pair
		CHECK(u16[0] == 0xD83D);
		CHECK(u16[1] == 0xDE00);
		CHECK(idni::to_string(u16) == s);
	}
	TEST_CASE("u32 ↔ u16 roundtrip") {
		std::u32string u32 = U"a©€\U0001F600";
		auto u16 = idni::to_u16string(u32);
		CHECK(idni::to_u32string(u16) == u32);
	}
}
