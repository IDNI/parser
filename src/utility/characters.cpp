// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <sstream>
#include <string.h>
#include "characters.h"

namespace idni {

template <>
inline std::string from_cstr<char>(const char* s) { return std::string(s); }
template <>
inline utf8string from_cstr<utf8char>(const char* s) {
	return to_utf8string(from_cstr<char>(s)); }
template <>
inline std::u32string from_cstr<char32_t>(const char* s) {
	return to_u32string(to_utf8string(s)); }
template <>
inline std::string from_str<char>(const std::string& s) { return s; }
template <>
inline utf8string from_str<utf8char>(const std::string& s) { return to_utf8string(s); }
template <>
inline std::u32string from_str<char32_t>(const std::string& s) {
	return to_u32string(to_utf8string(s)); }
template <>
inline std::string from_cstr<char>(const char32_t* s) { return to_std_string(std::u32string(s));}
template <>
inline utf8string from_cstr<utf8char>(const char32_t* s) {
	return to_utf8string(std::u32string(s)); }
template <>
inline std::u32string from_cstr<char32_t>(const char32_t* s) { return std::u32string(s); }
template <>
inline std::string from_str<char>(const std::u32string& s) { return to_std_string(s); }
template <>
inline utf8string from_str<utf8char>(const std::u32string& s) { return to_utf8string(s); }
template <>
inline std::u32string from_str<char32_t>(const std::u32string& s) { return s; }


inline std::string to_string(const utf8string& s) { return std::string(s.begin(), s.end()); }
inline std::string to_string(const std::u32string& s) { return to_string(to_utf8string(s)); }
inline std::string to_string(const char32_t& s) { return to_string(to_utf8string(s)); }
inline std::string to_string(const std::string& s) { return s; }
inline utf8string to_utf8string(int32_t v) { return to_utf8string(std::to_string(v)); }
inline utf8string to_utf8string(char ch) { return to_utf8string(std::string{ ch }); }
inline utf8string to_utf8string(const char* s) { return to_utf8string(std::string(s)); }
inline utf8string to_utf8string(const std::string& s) {
	return utf8string(s.begin(), s.end());
}
inline utf8string to_utf8string(char32_t ch) {
	if (ch == static_cast<char32_t>(0)) return {};
	utf8char s[4];
	size_t l = emit_codepoint(ch, s);
	if (l == (size_t) -1) return utf8string();
	return utf8string(s, l);
}
inline utf8string to_utf8string(const std::u32string& str) {
	std::basic_ostringstream<utf8char> ss;
	auto it = str.begin();
	while (it != str.end()) emit_codepoint(ss, *(it++));
	return ss.str();
}
inline std::u32string to_u32string(const utf8string& str) {
	std::basic_ostringstream<char32_t> ss;
	char32_t ch;
	const utf8char* s = str.c_str();
	size_t chl, sl = str.size();
	while ((chl = peek_codepoint(s, sl, ch)) > 0) {
		sl -= chl;
		s += chl;
		ss.put(ch);
	}
	// if (chl == (size_t) -1) return U""; // throw invalid UTF-8?
	return ss.str();
}
inline std::u32string to_u32string(const std::u32string& str) { return str; }
inline std::u32string to_u32string(const std::string& str) {
	return to_u32string(to_utf8string(str));
}

inline std::string to_std_string(const std::string& s) { return s; }
inline std::string to_std_string(const utf8string& s) { return to_string(s); }
inline std::string to_std_string(const std::u32string& s) {
	return to_std_string(to_utf8string(s)); }
inline std::string to_std_string(const char32_t& ch) {
	return to_std_string(to_utf8string(ch));}

#define utf_cont(ch) (((ch) & 0xc0) == 0x80)
inline bool is_mb_codepoint(utf8char ch, uint8_t p) {
	auto i = static_cast<unsigned int>(ch);
	if (p == 0) return i >= 0x80 && ((i - 0xc2) <= (0xf4 - 0xc2));
	if (p == 1) return i >= 0xe0 && utf_cont(i);
	if (p == 2) return i >= 0xf0 && utf_cont(i);
	if (p == 3) return utf_cont(i);
	return false;
}
inline size_t peek_codepoint(const utf8char* str, size_t l, char32_t& ch) {
	ch = -1;
  	if (!l) return 0;
	const utf8char* end = str + l;
	unsigned char s[4] = { str[0] };
	ch = s[0];
	if  (ch < 0x80) return 1;
	if ((ch - 0xc2) > (0xf4 - 0xc2)) return -1;
	s[1] = *(str + 1);
	if (ch < 0xe0) {
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
inline size_t codepoint_size(char32_t ch) {
	return    ch < 0x80     ? 1
		: ch < 0x800    ? 2
		: ch < 0x10000  ? 3
		: ch < 0x110000 ? 4
		:                 0;
}
inline size_t emit_codepoint(char32_t ch, utf8char *s) {
	if (ch < 0x80) {
		s[0] = (utf8char) ch;
		return 1;
	} else if (ch < 0x800) {
		s[0] = (utf8char) (0xC0 + (ch >> 6));
		s[1] = (utf8char) (0x80 + (ch & 0x3F));
		return 2;
	} else if (ch < 0x10000) {
		s[0] = (utf8char) (0xE0 + (ch >> 12));
		s[1] = (utf8char) (0x80 + ((ch >> 6) & 0x3F));
		s[2] = (utf8char) (0x80 + (ch & 0x3F));
		return 3;
	} else if (ch < 0x110000) {
		s[0] = (utf8char) (0xF0 + (ch >> 18));
		s[1] = (utf8char) (0x80 + ((ch >> 12) & 0x3F));
		s[2] = (utf8char) (0x80 + ((ch >> 6) & 0x3F));
		s[3] = (utf8char) (0x80 + (ch & 0x3F));
		return 4;
	} else return 0;
}
inline std::basic_ostream<utf8char>& emit_codepoint(std::basic_ostream<utf8char>& o,char32_t ch){
	if (ch < 0x80)
		return o.put((utf8char) ch);
	else if (ch < 0x800)
		return o.put((utf8char) (0xC0 + (ch >> 6)))
			.put((utf8char) (0x80 + (ch & 0x3F)));
	else if (ch < 0x10000)
		return o.put((utf8char) (0xE0 + (ch >> 12)))
			.put((utf8char) (0x80 + ((ch >> 6) & 0x3F)))
			.put((utf8char) (0x80 + (ch & 0x3F)));
	else if (ch < 0x110000)
		return o.put((utf8char) (0xF0 + (ch >> 18)))
			.put((utf8char) (0x80 + ((ch >> 12) & 0x3F)))
			.put((utf8char) (0x80 + ((ch >> 6) & 0x3F)))
			.put((utf8char) (0x80 + (ch & 0x3F)));
	return o;
}
inline std::basic_ostream<char32_t>& operator<<(std::basic_ostream<char32_t>& ss, const char* c){
	for (const auto& ch : to_u32string(to_utf8string(c))) ss.put(ch);
	return ss;
}
inline std::basic_ostream<char32_t>& operator<<(std::basic_ostream<char32_t>& ss,
	const std::string& s)
{
	for (const auto& ch : to_u32string(to_utf8string(s))) ss.put(ch);
	return ss;
}

} // idni namespace
