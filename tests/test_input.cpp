// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#include "parser.h"

using namespace std;
using namespace idni;

auto noconv = [](parser<char32_t, char32_t>::input& in) {
	auto x = in.cur();
	//cout << " <" << to_std_string(x) << ">" << endl;
	return in.next(), vector<char32_t>{ x };
};
auto u8conv = [](parser<char, char32_t>::input& in) {
	vector<char32_t> r;
	utf8char s[4] = {0,0,0,0};
	char32_t ch;
	for (uint8_t p = 0; p != 3; ++p) {
		s[p] = in.cur();
		//cout << "\n\ns["<<(unsigned int)p<<"]:("<<(unsigned int)s[p]<<")\n";
		if (!is_mb_codepoint(s[p], p))
			return peek_codepoint(s, p+1, ch), in.next(), r = {ch};
		if (!in.next()) return r;
	}
	return in.next(), r;
};
auto bconv = [](parser<char, bool>::input& in) {
	vector<bool> r;
	for (int i = 7; i >= 0; --i) r.push_back(in.cur() & (1 << i));
	return in.next(), r;
};
auto b32conv = [](parser<char32_t, bool>::input& in) {
	vector<bool> r;
	for (int i = 31; i >= 0; --i) r.push_back(in.cur() & (1 << i));
	return in.next(), r;
};
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
auto tokenize = [](typename parser<char, token<char>>::input& in) {
	return tokenize_<char>(in);
};
auto tokenize32 = [](
	typename parser<char32_t, token<char32_t>>::input& in)
{
	return tokenize_<char32_t>(in);
};
template <typename C, typename T = C>
bool test(const char* msg, const basic_string<C>& istr,
	typename parser<C, T>::decoder_type decoder = 0, bool stream = false)
{
	using input_t = typename parser<C, T>::input;
	cout << "\"" << to_std_string(istr) << "\" " << msg << endl;
	basic_istringstream<C> is(istr);
	auto in = stream ? input_t(is, 0, decoder)
			: input_t(istr.c_str(), istr.size(), 0, decoder);
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
	};
	while (!in.teof()) terminal_out(in.tcur()), in.tnext();
	cout << "\n" << endl;
	return true;
}

int main() {
	const string    i  ( "Hello τ World!");
	const u32string i32(U"Hello τ World!");
	const bool stream = true;

	test<char>("<char>", "");
	test<char>("<char> stream", "", 0, stream);
	test<char>("<char>", "a");
	test<char>("<char> stream", "a", 0, stream);
	test<char>("<char>", i);
	test<char>("<char> stream", i, 0, stream);
	test<char32_t>("<char32_t>", i32);
	test<char32_t>("<char32_t> noconversion", i32, noconv);
	test<char, char32_t>("<char, char32_t> utf8decode", i, u8conv);
	test<char, char32_t>("<char, char32_t> utf8decode stream",
							i, u8conv, stream);
	test<char, bool>("<char, bool> binarize", i, bconv);
	test<char, bool>("<char, bool> binarize stream", i, bconv, stream);
	test<char32_t, bool>("<char32_t, bool>  binarize", i32, b32conv);
	test<char, token<char>>("<char, token<char>> tokenize", i, tokenize);
	test<char32_t, token<char32_t>>("<char32_t, token<char32_t>> tokenize",
							i32, tokenize32);
	cout << "\n" << endl;
}
