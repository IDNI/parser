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
#include "grammar.h"
namespace idni {

prods<char32_t> operator+(const prods<char32_t>& x,
	const std::basic_string<char>& s)
{
	assert(!x.empty());
	prods<char32_t> r(x);
	for (const auto& c : s) r = r + (char32_t) c;
	return r;
}
prods<char32_t> operator|(const prods<char32_t>& x,
	const std::basic_string<char>& s)
{
	assert(!x.empty());
	return x | prods<char32_t>(to_u32string(s));
}
prods<char32_t> operator&(const prods<char32_t>& x,
	const std::basic_string<char>& s)
{
	assert(!x.empty());
	return x & prods<char32_t>(to_u32string(s));
}

} // idni namespace
