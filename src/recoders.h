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
#ifndef __IDNI__PARSER__RECODERS_H__
#define __IDNI__PARSER__RECODERS_H__
#include "parser.h"

namespace idni {

extern parser<char, char32_t>::decoder_type utf8_to_u32_conv;
extern parser<char, char32_t>::encoder_type u32_to_utf8_conv;

} // idni namespace
#endif // __IDNI__PARSER__RECODERS_H__