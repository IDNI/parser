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
#include "parser.h"

using namespace std;

namespace idni {
const std::size_t grcprime = sizeof(std::size_t) == 8
                           ? 0x9e3779b97f4a7c15  // 64-bit golden ratio constant
                           : 0x9e3779b9;

prods<char32_t, char32_t> operator+(const prods<char32_t, char32_t>& x,
	const string& s)
{
	assert(!x.empty());
	prods<char32_t> r(x);
	for (const auto& c : to_u32string(s)) r = r + c;
	return r;
}
prods<char32_t, char32_t> operator|(const prods<char32_t, char32_t>& x,
	const string& s)
{
	assert(!x.empty());
	return x | prods<char32_t>(to_u32string(s));
}
prods<char32_t, char32_t> operator&(const prods<char32_t, char32_t>& x,
	const string& s)
{
	assert(!x.empty());
	return x & prods<char32_t>(to_u32string(s));
}

string to_std_string(const string& s) { return s; }
string to_std_string(const utf8string& s) { return to_string(s); }
string to_std_string(const u32string& s) {
	return to_std_string(to_utf8string(s)); }
string to_std_string(const char32_t& ch) {
	return to_std_string(to_utf8string(ch));}
template <>
string from_cstr<char>(const char* s) { return string(s); }
template <>
utf8string from_cstr<utf8char>(const char* s) {
	return to_utf8string(from_cstr<char>(s)); }
template <>
u32string from_cstr<char32_t>(const char* s) {
	return to_u32string(to_utf8string(s)); }
template <>
string from_str<char>(const string& s) { return s; }
template <>
utf8string from_str<utf8char>(const string& s) { return to_utf8string(s); }
template <>
u32string from_str<char32_t>(const string& s) {
	return to_u32string(to_utf8string(s)); }
template <>
string from_cstr<char>(const char32_t* s) { return to_std_string(u32string(s));}
template <>
utf8string from_cstr<utf8char>(const char32_t* s) {
	return to_utf8string(u32string(s)); }
template <>
u32string from_cstr<char32_t>(const char32_t* s) { return u32string(s); }
template <>
string from_str<char>(const u32string& s) { return to_std_string(s); }
template <>
utf8string from_str<utf8char>(const u32string& s) { return to_utf8string(s); }
template <>
u32string from_str<char32_t>(const u32string& s) { return s; }

} // idni namespace
