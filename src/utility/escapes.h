// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__ESCAPES_H__
#define __IDNI__PARSER__UTILITY__ESCAPES_H__

#include <string>
#include <string_view>
#include "diagnostics.h"

namespace idni::escapes {

enum class hex_x_mode {
	off,
	lax,   // \xH[H] (1-2 digits)
	fixed, // \xHH (2 digits)
	greedy // \x... (C-style)
};
enum class hex_u4_mode {
	off,
	scalar,	// Unicode scalar
	utf16	// UTF-16 pair assembly
};
enum class lenient_mode {
	off,  // unknown \c → error (C)
	drop, // unknown \c → c  (JavaScript)
	keep  // unknown \c → \c (PHP/Perl/Ruby)
};

struct profile {
	// active quote char; == prefix means doubled-delimiter mode (CSV)
	char delim = '"';
	// escape introducer; '\0' = no escaping
	char prefix = '\\';
	// letters after prefix; control letters map to their code, others map to themselves
	std::string_view escape_chars = "abfnrtv/";
	lenient_mode lenient = lenient_mode::off;
	hex_x_mode  hex_x  = hex_x_mode::off;
	hex_u4_mode hex_u4 = hex_u4_mode::off;
	bool	    hex_U8 = false;
	bool	    octal  = false;
};

// Built-in profiles
inline constexpr profile tgf_string {
	.delim	      = '"',
	.prefix	      = '\\',
	.escape_chars = "abfnrtv/",
	.lenient      = lenient_mode::off,
	.hex_x	      = hex_x_mode::lax,
	.hex_u4	      = hex_u4_mode::scalar,
	.hex_U8	      = true,
	.octal	      = false,
};

inline constexpr profile tgf_char {
	.delim	      = '\'',
	.prefix	      = '\\',
	.escape_chars = "abfnrtv/",
	.lenient      = lenient_mode::off,
	.hex_x	      = hex_x_mode::lax,
	.hex_u4	      = hex_u4_mode::scalar,
	.hex_U8	      = true,
	.octal	      = false,
};

inline constexpr profile json {
	.delim	      = '"',
	.prefix	      = '\\',
	.escape_chars = "bfnrt/",
	.lenient      = lenient_mode::off,
	.hex_x	      = hex_x_mode::off,
	.hex_u4	      = hex_u4_mode::utf16,
	.hex_U8	      = false,
	.octal	      = false,
};

inline constexpr profile csv {
	.delim	      = '"',
	.prefix	      = '"', // prefix == delim: doubled-quote mode
	.escape_chars = "",
	.lenient      = lenient_mode::off,
	.hex_x	      = hex_x_mode::off,
	.hex_u4	      = hex_u4_mode::off,
	.hex_U8	      = false,
	.octal	      = false,
};

inline constexpr profile c_like {
	.delim	      = '"',
	.prefix	      = '\\',
	.escape_chars = "abfnrtv'/?",
	.lenient      = lenient_mode::off,
	.hex_x	      = hex_x_mode::greedy,
	.hex_u4	      = hex_u4_mode::scalar,
	.hex_U8	      = true,
	.octal	      = true,
};

using u32decoded = diagnostics::result<std::u32string>;
using decoded    = diagnostics::result<std::string>;

u32decoded    u32decode(std::string_view body, const profile& p);
decoded       decode(std::string_view body, const profile& p);
std::string   encode(std::u32string_view cps, const profile& p);
decoded       utf8encode(std::string_view utf8, const profile& p);
std::string   encode(std::string_view bytes, const profile& p);

} // namespace idni::escapes

#include "escapes.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__ESCAPES_H__
