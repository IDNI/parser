// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#ifndef __IDNI__PARSER__UTILITY__CHARACTERS_H__
#define __IDNI__PARSER__UTILITY__CHARACTERS_H__
#include <vector>
#include <string>
#include <cstdint>

namespace idni {

typedef unsigned char utf8char;
typedef std::basic_string<utf8char> utf8string;

// converting to utf8string
utf8string to_utf8string(int32_t v);
utf8string to_utf8string(char ch);
utf8string to_utf8string(const char* s);
utf8string to_utf8string(const std::string& s);
utf8string to_utf8string(char32_t ch);
utf8string to_utf8string(const std::u32string& str);
// u32string
std::u32string to_u32string(const std::u32string& str);
std::u32string to_u32string(const utf8string& str);
std::u32string to_u32string(const std::string& str);
// utf8string to other strings
std::string to_string(const std::string& s);
std::string to_string(const utf8string& s);
std::string to_string(const std::u32string& s);
std::string to_string(const char32_t& s);
std::string to_std_string(const std::string& s);
std::string to_std_string(const utf8string& s);
std::string to_std_string(const std::u32string& s);
std::string to_std_string(const char32_t& ch);
// from char*/string/char32_t*/u32string to string or u32string
template <typename CharT>
typename std::basic_string<CharT> from_cstr(const char *);
template <typename CharT>
typename std::basic_string<CharT> from_str(const std::string&);
template <typename CharT>
typename std::basic_string<CharT> from_cstr(const char32_t *);
template <typename CharT>
typename std::basic_string<CharT> from_str(const std::u32string&);

/**
 * checks if character is a begining of a multibyte codepoint or
 * if it is a part of a multibyte codepoint
 */
bool is_mb_codepoint(utf8char ch, uint8_t p = 0);
/**
 * convert const utf8char* sequence s of 1-4 utf8 code units into codepoint &ch
 * @param s string of unsigned chars containing utf8 text
 * @param l size of the s string
 * @param ch reference to a codepoint read from string
 * @return size (0, 1 - 4 bytes) or (size_t) -1 if illegal UTF8 code unit
 */
size_t peek_codepoint(const utf8char* s, size_t l, char32_t& ch);
/**
 * Returns size of a unicode codepoint in bytes
 * @return size (0-4)
 */
size_t codepoint_size(char32_t ch);
/**
 * Converts char32_t to a unsigned char *
 * @param ch unicode codepoint
 * @param s pointer to a utf8char (of size 4+ bytes) where the result is stored
 * @return byte size of the codepoint
 */
size_t emit_codepoint(char32_t ch, utf8char *s);
/**
 * Converts char32_t to a 1-4 utf8chars and outputs them into a stream
 * @param os output stream
 * @param ch unicode codepoint
 * @return output stream
 */
std::basic_ostream<utf8char>& emit_codepoint(
	std::basic_ostream<utf8char>& os, char32_t ch);
/**
 * Converts char32_t to a 1-4 utf8chars and outputs them into a vector
 * @param os resulting vector
 * @param ch unicode codepoint
 * @return output stream
 */
std::vector<utf8char>& emit_codepoint(std::vector<utf8char>& os, char32_t ch);
/**
 * convert const char* to u32string when streaming into char32_t stream
 */
std::basic_ostream<char32_t>& operator<<(std::basic_ostream<char32_t>& os,
	const char* s);
/**
 * convert const std::string to u32string when streaming into char32_t stream
 */
std::basic_ostream<char32_t>& operator<<(
	std::basic_ostream<char32_t>& os, const std::string& s);

} // idni namespace

#include "characters.impl.h"
#endif // __IDNI__PARSER__UTILITY__CHARACTERS_H__
