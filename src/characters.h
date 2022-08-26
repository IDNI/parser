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
#ifndef __IDNI__CHARACTERS_H__
#define __IDNI__CHARACTERS_H__
#include <array>
#include <iostream>

namespace idni {

typedef int32_t int_t;

typedef std::basic_ostream<char> ostream_t;

typedef unsigned char utf8char_t;
typedef std::basic_string<utf8char_t> utf8string_t;

typedef utf8char_t char_t;
typedef utf8string_t string_t;

typedef char_t* cstr;
typedef const char_t* ccs;
typedef ccs* pccs;
typedef std::array<ccs, 2> ccs_range;

std::string to_string_(int_t v);

// utf8 string to other strings
std::u32string to_u32string(const string_t& str);
std::string to_string(const string_t& s);

// conversions from any type into utf8 string
string_t to_string_t(int_t v);
string_t to_string_t(const std::string& s);
string_t to_string_t(const char* s);
string_t to_string_t(char ch);
string_t to_string_t(char32_t ch);
string_t to_string_t(const std::u32string& str);

string_t unquote(string_t str);
string_t to_lower_first(string_t s);
string_t to_upper_first(string_t s);
int_t hex_to_int_t(ccs str, size_t len = 2);

unsigned char* strdup(const unsigned char* src);
size_t strlen(const unsigned char *src);
unsigned char* strcpy(unsigned char* dst, const unsigned char* src);
int strncmp(const unsigned char* str1, const unsigned char* str2, size_t num);

// only for comparing with const char* literals containing ASCII
int strncmp(const unsigned char* str1, const char* str2, size_t num);
int strcmp(const unsigned char* str1, const char* str2);

bool is_mb_codepoint(const char_t str);
size_t peek_codepoint(ccs s, size_t l, char32_t &ch);
size_t codepoint_size(char32_t ch);
size_t emit_codepoint(char32_t ch, char_t *s);
std::basic_ostream<char_t>& emit_codepoint(std::basic_ostream<char_t>& os,
	char32_t ch);

bool isalnum(ccs s, size_t n, size_t& l);
bool isalpha(ccs s, size_t n, size_t& l);
bool isprint(ccs s, size_t n, size_t& l);
bool isspace(ccs s, size_t n, size_t& l);
bool isprint(char32_t ch);

}      // idni namespace
#endif // __IDNI__CHARACTERS_H__
