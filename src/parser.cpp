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

prods<char32_t, char32_t> operator+(const prods<char32_t, char32_t>& x,
	const std::basic_string<char>& s)
{
	assert(!x.empty());
	prods<char32_t> r(x);
	for (const auto& c : to_u32string(s)) r = r + c;
	return r;
}
prods<char32_t, char32_t> operator|(const prods<char32_t, char32_t>& x,
	const std::basic_string<char>& s)
{
	assert(!x.empty());
	return x | prods<char32_t>(to_u32string(s));
}
prods<char32_t, char32_t> operator&(const prods<char32_t, char32_t>& x,
	const std::basic_string<char>& s)
{
	assert(!x.empty());
	return x & prods<char32_t>(to_u32string(s));
}

std::string to_std_string(const std::string& s) { return s; }
std::string to_std_string(const utf8string& s) { return to_string(s); }
std::string to_std_string(const std::u32string& s) {
	return to_std_string(to_utf8string(s)); }
std::string to_std_string(const char32_t& ch) {
	return to_std_string(to_utf8string(ch));}
template <>
std::string from_cstr<char>(const char* s) { return std::string(s); }
template <>
utf8string from_cstr<utf8char>(const char* s) {
	return to_utf8string(from_cstr<char>(s)); }
template <>
std::u32string from_cstr<char32_t>(const char* s) {
	return to_u32string(to_utf8string(s)); }
template <>
std::string from_str<char>(const std::string& s) {
	return s; }
template <>
utf8string from_str<utf8char>(const std::string& s) {
	return to_utf8string(s); }
template <>
std::u32string from_str<char32_t>(const std::string& s) {
	return to_u32string(to_utf8string(s)); }
template <>
std::string from_cstr<char>(const char32_t* s) {
	return to_std_string(std::u32string(s)); }
template <>
utf8string from_cstr<utf8char>(const char32_t* s) {
	return to_utf8string(std::u32string(s)); }
template <>
std::u32string from_cstr<char32_t>(const char32_t* s) {
	return std::u32string(s); }
template <>
std::string from_str<char>(const std::u32string& s) {
	return to_std_string(s); }
template <>
utf8string from_str<utf8char>(const std::u32string& s) {
	return to_utf8string(s); }
template <>
std::u32string from_str<char32_t>(const std::u32string& s) {
	return s; }


} // idni namespace
