// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__RECODERS_H__
#define __IDNI__PARSER__RECODERS_H__
#include "parser.h"

namespace idni {

// UTF-8 -> UTF-32 decoder for parser<char, char32_t>.
//
// Decoding strategy: read the lead byte to derive the expected sequence
// length (1-4 bytes), then advance only over actual UTF-8 continuation
// bytes. Malformed input (invalid lead byte, missing continuation,
// truncation at EOF) emits U+FFFD and advances exactly one byte so that
// the bad byte is consumed but no good bytes are eaten.
inline std::vector<char32_t> utf8_to_u32_conv(
	parser<char, char32_t>::input& in)
{
	std::vector<char32_t> r;
	if (in.eof()) return r;
	idni::utf8char s[4] = { 0, 0, 0, 0 };
	s[0] = in.cur();
	// Expected sequence length from the lead byte. Anything not in
	// the 0xC2..0xF4 range is treated as a 1-byte (mostly invalid)
	// run; ASCII falls through here too with len = 1.
	size_t len = 1;
	if (s[0] >= 0x80) {
		if      (s[0] < 0xC2) len = 1;
		else if (s[0] < 0xE0) len = 2;
		else if (s[0] < 0xF0) len = 3;
		else if (s[0] <= 0xF4) len = 4;
	}
	// Always consume the lead byte.
	in.next();
	size_t got = 1;
	// Pull continuation bytes; stop if we hit EOF or a byte that
	// isn't a continuation. Non-continuation bytes are NOT consumed
	// so the next decode() call sees them.
	for (size_t i = 1; i < len; ++i) {
		if (in.eof()) break;
		s[i] = in.cur();
		if ((s[i] & 0xC0) != 0x80) break;
		in.next();
		got = i + 1;
	}
	char32_t ch;
	size_t decoded = idni::peek_codepoint(s, got, ch);
	if (decoded == 0 || decoded == static_cast<size_t>(-1))
		ch = 0xFFFD; // replacement character for malformed input
	return r = { ch };
}

inline std::basic_string<char> u32_to_utf8_conv(
	const std::vector<char32_t>& ts)
{
	std::stringstream ss;
	for (auto t : ts) ss << idni::to_string(t);
	return ss.str();
}

template<typename C, typename T>
typename parser<C, T>::options default_parser_options() {
	typename parser<C, T>::options o;
	if constexpr (std::is_same_v<C, char> && std::is_same_v<T, char32_t>) {
		o.codec.decode = utf8_to_u32_conv;
		o.codec.encode = u32_to_utf8_conv;
	}
	return o;
}

} // idni namespace
#endif // __IDNI__PARSER__RECODERS_H__
