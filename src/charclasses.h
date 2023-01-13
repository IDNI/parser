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
#ifndef __IDNI__PARSER__CHARCLASSES_H__
#define __IDNI__PARSER__CHARCLASSES_H__
namespace idni { namespace charclasses {

template <typename CharT>
bool iseof(CharT c) { return c == std::char_traits<CharT>::eof(); }
template <typename CharT>
bool isalnum(CharT c);
template <typename CharT>
bool isalpha(CharT c);
template <typename CharT>
bool isblank(CharT c);
template <typename CharT>
bool iscntrl(CharT c);
template <typename CharT>
bool isdigit(CharT c);
template <typename CharT>
bool isgraph(CharT c);
template <typename CharT>
bool islower(CharT c);
template <typename CharT>
bool isprint(CharT c);
template <typename CharT>
bool ispunct(CharT c);
template <typename CharT>
bool isspace(CharT c);
template <typename CharT>
bool isupper(CharT c);
template <typename CharT>
bool isxdigit(CharT c);

}} // idni::charclasses namespace
#endif //__IDNI__PARSER__CHARCLASSES_H__
