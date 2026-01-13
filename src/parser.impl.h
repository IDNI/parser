// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "parser.h"

namespace idni {

inline prods<char32_t, char32_t> operator+(const prods<char32_t, char32_t>& x,
	const std::string& s)
{
	assert(!x.empty());
	prods<char32_t> r(x);
	for (const auto& c : to_u32string(s)) r = r + c;
	return r;
}
inline prods<char32_t, char32_t> operator|(const prods<char32_t, char32_t>& x,
	const std::string& s)
{
	assert(!x.empty());
	return x | prods<char32_t>(to_u32string(s));
}
inline prods<char32_t, char32_t> operator&(const prods<char32_t, char32_t>& x,
	const std::string& s)
{
	assert(!x.empty());
	return x & prods<char32_t>(to_u32string(s));
}

} // idni namespace
