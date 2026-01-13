// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__RECODERS_H__
#define __IDNI__PARSER__RECODERS_H__
#include "parser.h"

namespace idni {

inline static parser<char, char32_t>::decoder_type utf8_to_u32_conv =
	[](idni::parser<char, char32_t>::input& in) {
		std::vector<char32_t> r;
		idni::utf8char s[4] = { 0, 0, 0, 0 };
		char32_t ch;
		for (uint8_t p = 0; p != 3; ++p) {
			s[p] = in.cur();
			if (!idni::is_mb_codepoint(s[p], p))
				return idni::peek_codepoint(s, p + 1, ch),
					in.next(), r = { ch };
			if (!in.next()) return r;
		}
		return in.next(), r;
	};

inline static parser<char, char32_t>::encoder_type u32_to_utf8_conv =
	[](const std::vector<char32_t>& ts) {
		std::stringstream ss;
		for (auto t : ts) ss << idni::to_string(t);
		return ss.str();
	};

} // idni namespace
#endif // __IDNI__PARSER__RECODERS_H__