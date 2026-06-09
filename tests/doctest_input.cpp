// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for parser input/decoder pipelines (char, char32_t, bool, token)

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"

using namespace std;
using namespace idni;

// ---------------------------------------------------------------------------
// decoder functions (from old test_input.cpp)
// ---------------------------------------------------------------------------

#ifndef __EMSCRIPTEN__
static auto noconv = [](parser<char32_t, char32_t>::input& in) {
	auto x = in.cur();
	return in.next(), vector<char32_t>{ x };
};
#endif

static auto u8conv = [](parser<char, char32_t>::input& in) {
	vector<char32_t> r;
	utf8char s[4] = {0,0,0,0};
	char32_t ch;
	for (uint8_t p = 0; p != 3; ++p) {
		s[p] = in.cur();
		if (!is_mb_codepoint(s[p], p))
			return peek_codepoint(s, p+1, ch), in.next(), r = {ch};
		if (!in.next()) return r;
	}
	return in.next(), r;
};

static auto bconv = [](parser<char, bool>::input& in) {
	vector<bool> r;
	for (int i = 7; i >= 0; --i) r.push_back(in.cur() & (1 << i));
	return in.next(), r;
};

#ifndef __EMSCRIPTEN__
static auto b32conv = [](parser<char32_t, bool>::input& in) {
	vector<bool> r;
	for (int i = 31; i >= 0; --i) r.push_back(in.cur() & (1 << i));
	return in.next(), r;
};
#endif

template<typename S>
struct token {
	enum type { NONE, WS, GREETINGS, ADDRESS, GREEK, PUNCT } ttype{ NONE };
	vector<string> tnames = {
		"NONE", "WS", "GREETINGS", "ADDRESS", "GREEK", "PUNCT"
	};
	basic_string<S> value{};
	string to_str() const {
		if (value.empty()) return {};
		stringstream ss;
		ss << tnames[ttype] << "("<< to_std_string(value) <<")";
		return ss.str();
	}
	bool operator==(const token& t) const {
		return ttype == t.ttype && value == t.value;
	}
};

template <typename S>
vector<token<S>> tokenize_(typename parser<S, token<S>>::input& in) {
	using token_t = token<S>;
	using namespace idni::charclasses;
	vector<token_t> r{ {} };
	token_t& t = r.back();
	S ch = in.cur();
	t.value = { ch };
	if (ispunct(ch)) t.ttype = token_t::PUNCT, in.next();
	else if (isspace(ch)) {
		t.ttype = token_t::WS;
		while (in.next() && isspace(ch = in.cur())) t.value += ch;
	} else if (isalnum(ch)) {
		while (in.next() && isalnum(ch = in.cur())) t.value += ch;
		if (t.value == from_cstr<S>("Hello")
		 || t.value == from_cstr<S>("Hi")) t.ttype = token_t::GREETINGS;
		else if (t.value == from_cstr<S>("World"))
					t.ttype = token_t::ADDRESS;
		else if (t.value == from_cstr<S>("τ")) t.ttype = token_t::GREEK;
	} else in.next();
	return r;
};

static auto tokenize = [](typename parser<char, token<char>>::input& in) {
	return tokenize_<char>(in);
};

#ifndef __EMSCRIPTEN__
static auto tokenize32 = [](
	typename parser<char32_t, token<char32_t>>::input& in)
{
	return tokenize_<char32_t>(in);
};
#endif

// ---------------------------------------------------------------------------
// helper: collect terminals from an input into a vector
// ---------------------------------------------------------------------------

template <typename C, typename T = C>
static vector<T> collect_terminals(const basic_string<C>& src,
	typename parser<C, T>::decoder_type decoder = nullptr,
	bool stream = false)
{
	using input_t = typename parser<C, T>::input;
	cout << "\"" << to_std_string(src) << "\"" << endl;
	vector<T> result;
	basic_stringstream<C> is(src);
	auto in = stream ? input_t(is, 0, decoder)
			 : input_t(src.c_str(), src.size(), 0, decoder);
	size_t counter = 0;
	auto terminal_out = [&counter](T c) {
		size_t n = counter++;
		if constexpr (is_same_v<T, bool>) {
			if (n % 4 == 0) cout << " ";
			if constexpr (is_same_v<C, char>) {
				if (n % 8  == 0) cout << "\n";
			}
			if constexpr (is_same_v<C, char32_t>) {
				if (n %  8 == 0) cout << " ";
				if (n % 32 == 0) cout << "\n";
			}
			cout << (c ? "1" : "0");
		} else if (n) cout << " ";
		if constexpr (is_same_v<T, char32_t>)
			cout << "'" << to_std_string(c) << "'";
		if constexpr (is_same_v<T, char>)
			cout << "'" << string{ c } << "'";
		if constexpr (is_same_v<T, token<C>>)
			cout << c.to_str();
		return c;
	};
	 while (!in.teof()) {
		result.push_back(terminal_out(in.tcur()));
		in.tnext();
	}
	return result;
}

// ---------------------------------------------------------------------------
// TEST SUITE: char input & terminals
// ---------------------------------------------------------------------------

TEST_SUITE("input: char") {

	TEST_CASE("empty") {
		auto r = collect_terminals<char>("");
		CHECK(r.empty());
	}

	TEST_CASE("empty (stream)") {
		auto r = collect_terminals<char>("", nullptr, true);
		CHECK(r.empty());
	}

	TEST_CASE("single char") {
		auto r = collect_terminals<char>("a");
		REQUIRE(r.size() == 1u);
		CHECK(r[0] == 'a');
	}

	TEST_CASE("single char (stream)") {
		auto r = collect_terminals<char>("a", nullptr, true);
		REQUIRE(r.size() == 1u);
		CHECK(r[0] == 'a');
	}

	TEST_CASE("Hello τ World!") {
		auto r = collect_terminals<char>("Hello τ World!");
		// τ is 2 bytes in UTF-8 (0xCF 0x84), so 15 char terminals.
		std::string actual(r.begin(), r.end());
		CHECK(actual == "Hello τ World!");
	}

	TEST_CASE("Hello τ World! (stream)") {
		auto r = collect_terminals<char>("Hello τ World!", nullptr, true);
		std::string actual(r.begin(), r.end());
		CHECK(actual == "Hello τ World!");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: char32_t input & terminals
// ---------------------------------------------------------------------------

#ifndef __EMSCRIPTEN__
TEST_SUITE("input: char32_t") {

	TEST_CASE("empty") {
		auto r = collect_terminals<char32_t>(U"");
		CHECK(r.empty());
	}

	TEST_CASE("single char32_t") {
		auto r = collect_terminals<char32_t>(U"τ");
		REQUIRE(r.size() == 1u);
		CHECK(r[0] == U'τ');
	}

	TEST_CASE("Hello τ World!") {
		auto r = collect_terminals<char32_t>(U"Hello τ World!");
		std::u32string actual(r.begin(), r.end());
		CHECK(actual == U"Hello τ World!");
	}

	TEST_CASE("Hello τ World! with noconv decoder") {
		auto r = collect_terminals<char32_t, char32_t>(
			U"Hello τ World!", noconv);
		std::u32string actual(r.begin(), r.end());
		CHECK(actual == U"Hello τ World!");
	}
}
#endif

// ---------------------------------------------------------------------------
// TEST SUITE: char -> char32_t (UTF-8 decode)
// ---------------------------------------------------------------------------

TEST_SUITE("input: utf8 decode (char -> char32_t)") {

	TEST_CASE("empty") {
		auto r = collect_terminals<char, char32_t>("", u8conv);
		CHECK(r.empty());
	}

	TEST_CASE("ASCII") {
		auto r = collect_terminals<char, char32_t>("Hello", u8conv);
		REQUIRE(r.size() == 5u);
		CHECK(r[0] == U'H');
		CHECK(r[1] == U'e');
		CHECK(r[2] == U'l');
		CHECK(r[3] == U'l');
		CHECK(r[4] == U'o');
	}

	TEST_CASE("Greek τ") {
		auto r = collect_terminals<char, char32_t>("τ", u8conv);
		REQUIRE(r.size() == 1u);
		CHECK(r[0] == U'τ');
	}

	TEST_CASE("mixed") {
		auto r = collect_terminals<char, char32_t>(
			"Hello τ World!", u8conv);
		std::u32string actual(r.begin(), r.end());
		CHECK(actual == U"Hello τ World!");
	}

	TEST_CASE("stream mode") {
		auto r = collect_terminals<char, char32_t>(
			"Hi τ!", u8conv, true);
		std::u32string actual(r.begin(), r.end());
		CHECK(actual == U"Hi τ!");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: char -> bool (binarize)
// ---------------------------------------------------------------------------

TEST_SUITE("input: binarize (char -> bool)") {

	// bconv emits bits MSB-first; r[0] is the most significant bit.
	TEST_CASE("single byte") {
		auto r = collect_terminals<char, bool>("A", bconv);
		// 'A' = 0x41 = 0b01000001
		const std::vector<bool> expected{
			false, true, false, false, false, false, false, true };
		CHECK(r == expected);
	}

	TEST_CASE("stream mode") {
		auto r = collect_terminals<char, bool>("AB", bconv, true);
		// 'A' = 0b01000001, 'B' = 0b01000010
		const std::vector<bool> expected{
			false, true, false, false, false, false, false, true,
			false, true, false, false, false, false, true,  false };
		CHECK(r == expected);
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: char32_t -> bool (binarize)
// ---------------------------------------------------------------------------

#ifndef __EMSCRIPTEN__
TEST_SUITE("input: binarize32 (char32_t -> bool)") {

	// b32conv emits 32 bits MSB-first; r[0] is bit 31, r[31] is bit 0.
	// Helper: build the expected 32-bit MSB-first pattern from a value.
	auto bits32 = [](uint32_t v) {
		std::vector<bool> r;
		r.reserve(32);
		for (int i = 31; i >= 0; --i) r.push_back((v >> i) & 1u);
		return r;
	};

	TEST_CASE("single char32_t") {
		auto r = collect_terminals<char32_t, bool>(U"A", b32conv);
		// U'A' = 0x00000041
		CHECK(r == bits32(0x00000041));
	}

	TEST_CASE("Greek char32_t") {
		auto r = collect_terminals<char32_t, bool>(U"τ", b32conv);
		// U'τ' = U+03C4 = 0x000003C4
		CHECK(r == bits32(0x000003C4));
	}
}
#endif

// ---------------------------------------------------------------------------
// TEST SUITE: char -> token<char> (tokenize)
// ---------------------------------------------------------------------------

TEST_SUITE("input: tokenize (char -> token<char>)") {

	TEST_CASE("empty") {
		auto r = collect_terminals<char, token<char>>("", tokenize);
		CHECK(r.empty());
	}

	TEST_CASE("Hello τ World!") {
		auto r = collect_terminals<char, token<char>>(
			"Hello τ World!", tokenize);
		// τ is 2 bytes in UTF-8 at char level (0xCF 0x84); neither byte
		// is alnum/space/punct in the default C locale, so each falls
		// through to a NONE token of width 1. Final sequence:
		//   GREETINGS("Hello"), WS, NONE, NONE, WS, ADDRESS("World"),
		//   PUNCT("!")
		REQUIRE(r.size() == 7u);
		CHECK(r[0].ttype == token<char>::GREETINGS);
		CHECK(r[0].value == "Hello");
		CHECK(r[1].ttype == token<char>::WS);
		CHECK(r[2].ttype == token<char>::NONE);
		CHECK(r[3].ttype == token<char>::NONE);
		CHECK(r[4].ttype == token<char>::WS);
		CHECK(r[5].ttype == token<char>::ADDRESS);
		CHECK(r[5].value == "World");
		CHECK(r[6].ttype == token<char>::PUNCT);
		CHECK(r[6].value == "!");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: char32_t -> token<char32_t> (tokenize32)
// ---------------------------------------------------------------------------

#ifndef __EMSCRIPTEN__
TEST_SUITE("input: tokenize32 (char32_t -> token<char32_t>)") {

	TEST_CASE("Hello τ World!") {
		auto r = collect_terminals<char32_t, token<char32_t>>(
			U"Hello τ World!", tokenize32);
		REQUIRE(r.size() == 6u);
		CHECK(r[0].ttype == token<char32_t>::GREETINGS);
		CHECK(r[0].value == U"Hello");
		CHECK(r[1].ttype == token<char32_t>::WS);
		CHECK(r[1].value == U" ");
		CHECK(r[2].ttype == token<char32_t>::GREEK);
		CHECK(r[2].value == U"τ");
		CHECK(r[3].ttype == token<char32_t>::WS);
		CHECK(r[3].value == U" ");
		CHECK(r[4].ttype == token<char32_t>::ADDRESS);
		CHECK(r[4].value == U"World");
		CHECK(r[5].ttype == token<char32_t>::PUNCT);
		CHECK(r[5].value == U"!");
	}
}
#endif
