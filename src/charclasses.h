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

template <typename C> bool iseof(C c) {return c==std::char_traits<C>::eof();}
template <typename C> bool isalnum(C c);
template <typename C> bool isalpha(C c);
template <typename C> bool isblank(C c);
template <typename C> bool iscntrl(C c);
template <typename C> bool isdigit(C c);
template <typename C> bool isgraph(C c);
template <typename C> bool islower(C c);
template <typename C> bool isprint(C c);
template <typename C> bool ispunct(C c);
template <typename C> bool isspace(C c);
template <typename C> bool isupper(C c);
template <typename C> bool isxdigit(C c);

}} // idni::charclasses namespace
#endif //__IDNI__PARSER__CHARCLASSES_H__
