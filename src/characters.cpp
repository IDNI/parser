// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <sstream>
#include <string.h>
#include "characters.h"
using namespace std;
namespace idni {

string to_string(int_t v) { stringstream ss; ss << v; return ss.str(); }
string to_string(const string_t& s) { return string(s.begin(), s.end()); }
string_t to_string_t(int_t v) {	return to_string_t(to_string(v)); }
string_t to_string_t(const string& s) {	return string_t(s.begin(), s.end()); }
string_t to_string_t(const char* s) { return to_string_t(string(s)); }
string_t to_string_t(char ch) {	return to_string_t(string{ ch }); }
string_t to_string_t(char32_t ch) {
	char_t s[4];
	size_t l = emit_codepoint(ch, s);
	if (l == (size_t) -1) return string_t();
	return string_t(s, l);
}
string_t to_string_t(const u32string& str) {
	basic_ostringstream<char_t> ss;
	auto it = str.begin();
	while (it != str.end()) emit_codepoint(ss, *(it++));
	return ss.str();
}
u32string to_u32string(const string_t& str) {
	basic_ostringstream<char32_t> ss;
	char32_t ch;
	const char_t* s = str.c_str();
	size_t chl, sl = str.size();
	while ((chl = peek_codepoint(s, sl, ch)) > 0) {
		sl -= chl;
		s += chl;
		ss.put(ch);
	}
	// if (chl == (size_t) -1) return U""; // throw invalid UTF-8?
	return ss.str();
}
bool is_mb_codepoint(const char_t ch) {	return ch >= 0x80; }
#define utf_cont(ch) (((ch) & 0xc0) == 0x80)
size_t peek_codepoint(const char_t* str, size_t l, char32_t &ch) {
	ch = -1;
  	if (!l) return 0;
	const char_t* end = str + l;
	unsigned char s[4] = { str[0] };
	ch = s[0];
	if  (ch < 0x80) return 1;
	if ((ch - 0xc2) > (0xf4 - 0xc2)) return -1;
	s[1] = *(str + 1);
	if  (ch < 0xe0) {
		if ((size_t)(str + 1) >= (size_t)end || !utf_cont(s[1]))
			return -1;
		ch = ((ch & 0x1f) << 6) | (s[1] & 0x3f);
		return 2;
	}
	s[2] = *(str + 2);
	if (ch < 0xf0) {
		if ((str + 2 >= end) || !utf_cont(s[1]) || !utf_cont(s[2]))
			return -1;
		if (ch == 0xed && s[1] > 0x9f) return -1;
		ch = ((ch & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] &0x3f);
		if (ch < 0x800) return -1;
		return 3;
	}
	s[3] = *(str + 3);
	if ((str + 3 >= end) ||
		!utf_cont(s[1]) || !utf_cont(s[2]) || !utf_cont(s[3]))
			return -1;
	if      (ch == 0xf0) { if (s[1] < 0x90) return -1; }
	else if (ch == 0xf4) { if (s[1] > 0x8f) return -1; }
	ch = ((ch&7)<<18) | ((s[1]&0x3f)<<12) | ((s[2]&0x3f)<<6) | (s[3]&0x3f);
	return 4;
}
size_t codepoint_size(char32_t ch) {
	return    ch < 0x80     ? 1
		: ch < 0x800    ? 2
		: ch < 0x10000  ? 3
		: ch < 0x110000 ? 4
		:                 0;
}
size_t emit_codepoint(char32_t ch, char_t *s) {
	if (ch < 0x80) {
		s[0] = (char_t) ch;
		return 1;
	} else if (ch < 0x800) {
		s[0] = (char_t) (0xC0 + (ch >> 6));
		s[1] = (char_t) (0x80 + (ch & 0x3F));
		return 2;
	} else if (ch < 0x10000) {
		s[0] = (char_t) (0xE0 + (ch >> 12));
		s[1] = (char_t) (0x80 + ((ch >> 6) & 0x3F));
		s[2] = (char_t) (0x80 + (ch & 0x3F));
		return 3;
	} else if (ch < 0x110000) {
		s[0] = (char_t) (0xF0 + (ch >> 18));
		s[1] = (char_t) (0x80 + ((ch >> 12) & 0x3F));
		s[2] = (char_t) (0x80 + ((ch >> 6) & 0x3F));
		s[3] = (char_t) (0x80 + (ch & 0x3F));
		return 4;
	} else return 0;
}
basic_ostream<char_t>& emit_codepoint(basic_ostream<char_t>& o, char32_t ch) {
	if (ch < 0x80)
		return o.put((char_t) ch);
	else if (ch < 0x800)
		return o.put((char_t) (0xC0 + (ch >> 6)))
			.put((char_t) (0x80 + (ch & 0x3F)));
	else if (ch < 0x10000)
		return o.put((char_t) (0xE0 + (ch >> 12)))
			.put((char_t) (0x80 + ((ch >> 6) & 0x3F)))
			.put((char_t) (0x80 + (ch & 0x3F)));
	else if (ch < 0x110000)
		return o.put((char_t) (0xF0 + (ch >> 18)))
			.put((char_t) (0x80 + ((ch >> 12) & 0x3F)))
			.put((char_t) (0x80 + ((ch >> 6) & 0x3F)))
			.put((char_t) (0x80 + (ch & 0x3F)));
	return o;
}
basic_ostream<char32_t>& operator<<(basic_ostream<char32_t>& ss, const char* c){
	for (const auto& ch : to_u32string(to_string_t(c))) ss.put(ch);
	return ss;
}
basic_ostream<char32_t>& operator<<(basic_ostream<char32_t>& ss,
	const std::string& s)
{
	for (const auto& ch : to_u32string(to_string_t(s))) ss.put(ch);
	return ss;
}

} // idni namespace
