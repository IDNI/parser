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
#include "recoders.h"

using namespace std;

namespace idni {

parser<char, char32_t>::decoder_type utf8_to_u32_conv =
	[](idni::parser<char, char32_t>::input& in) {
		vector<char32_t> r;
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

parser<char, char32_t>::encoder_type u32_to_utf8_conv =
	[](const vector<char32_t>& ts) {
		stringstream ss;
		for (auto t : ts) ss << idni::to_string(t);
		return ss.str();
	};

} // idni namespace
