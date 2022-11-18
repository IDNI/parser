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
#include <cctype>
#include "characters.h"
#include "charclasses.h"
namespace idni { namespace charclasses {

template <typename CharT>
bool iseof(CharT c) { return c == std::char_traits<CharT>::eof(); }
template bool iseof<char>(char c);
template bool iseof<utf8char>(utf8char c);
template bool iseof<char32_t>(char32_t c);
template<> bool isalnum<char>(char c) { return ::isalnum(c); }
template<> bool isalpha<char>(char c) { return ::isalpha(c); }
template<> bool isblank<char>(char c) { return ::isblank(c); }
template<> bool iscntrl<char>(char c) { return ::iscntrl(c); }
template<> bool isdigit<char>(char c) { return ::isdigit(c); }
template<> bool isgraph<char>(char c) { return ::isgraph(c); }
template<> bool islower<char>(char c) { return ::islower(c); }
template<> bool isprint<char>(char c) { return ::isprint(c); }
template<> bool ispunct<char>(char c) { return ::ispunct(c); }
template<> bool isspace<char>(char c) { return ::isspace(c); }
template<> bool isupper<char>(char c) { return ::isupper(c); }
template<> bool isxdigit<char>(char c){ return ::isxdigit(c);}
// utf8char
template<> bool isalnum<utf8char>(utf8char c) { return ::isalnum(c); }
template<> bool isalpha<utf8char>(utf8char c) { return ::isalpha(c); }
template<> bool isblank<utf8char>(utf8char c) { return ::isblank(c); }
template<> bool iscntrl<utf8char>(utf8char c) { return ::iscntrl(c); }
template<> bool isdigit<utf8char>(utf8char c) { return ::isdigit(c); }
template<> bool isgraph<utf8char>(utf8char c) { return ::isgraph(c); }
template<> bool islower<utf8char>(utf8char c) { return ::islower(c); }
template<> bool isprint<utf8char>(utf8char c) { return ::isprint(c); }
template<> bool ispunct<utf8char>(utf8char c) { return ::ispunct(c); }
template<> bool isspace<utf8char>(utf8char c) { return ::isspace(c); }
template<> bool isupper<utf8char>(utf8char c) { return ::isupper(c); }
template<> bool isxdigit<utf8char>(utf8char c){ return ::isxdigit(c);}
// Follows simplified versions of char class functions for Unicode symbols 
// TODO: use real Unicode character classes
template<> bool isalnum<char32_t>(char32_t c) {
	return !iseof<char32_t>(c) && (c > 160 || ::isalnum(c)); }
template<> bool isalpha<char32_t>(char32_t c) {
	return !iseof<char32_t>(c) && (c > 160 || ::isalpha(c)); }
template<> bool isblank<char32_t>(char32_t c) {
	return c < 128 && ::isblank(c); }
template<> bool iscntrl<char32_t>(char32_t c) {
	return c < 128 && ::iscntrl(c); }
template<> bool isdigit<char32_t>(char32_t c) {
	return c < 128 && ::isdigit(c); }
template<> bool isgraph<char32_t>(char32_t c) {
	return c < 128 && ::isgraph(c); }
template<> bool islower<char32_t>(char32_t c) {
	return c < 128 && ::islower(c); }
template<> bool isprint<char32_t>(char32_t c) {
	return !iseof<char32_t>(c) && (c > 160 || ::isprint(c)); }
template<>bool ispunct<char32_t>(char32_t c) {
	return c < 128 && ::ispunct(c); }
template<>bool isspace<char32_t>(char32_t c) {
	return c < 128 && ::isspace(c); }
template<>bool isupper<char32_t>(char32_t c) {
	return c < 128 && ::isupper(c); }
template<>bool isxdigit<char32_t>(char32_t c) {
	return c < 128 && ::isxdigit(c); }

}} // idni::charclasses namespace
