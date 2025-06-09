// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#include <cctype>
#include "charclasses.h"

namespace idni::charclasses {

template<> inline bool isalnum<char>(char c) { return std::isalnum(c); }
template<> inline bool isalpha<char>(char c) { return std::isalpha(c); }
template<> inline bool isblank<char>(char c) { return std::isblank(c); }
template<> inline bool iscntrl<char>(char c) { return std::iscntrl(c); }
template<> inline bool isdigit<char>(char c) { return std::isdigit(c); }
template<> inline bool isgraph<char>(char c) { return std::isgraph(c); }
template<> inline bool islower<char>(char c) { return std::islower(c); }
template<> inline bool isprint<char>(char c) { return std::isprint(c); }
template<> inline bool ispunct<char>(char c) { return std::ispunct(c); }
template<> inline bool isspace<char>(char c) { return std::isspace(c); }
template<> inline bool isupper<char>(char c) { return std::isupper(c); }
template<> inline bool isxdigit<char>(char c){ return std::isxdigit(c);}

// Follows simplified versions of char class functions for Unicode symbols
// TODO: use real Unicode character classes
template<> inline bool isalnum<char32_t>(char32_t c) {
	return !iseof<char32_t>(c) && (c > 160 || std::isalnum(c)); }
template<> inline bool isalpha<char32_t>(char32_t c) {
	return !iseof<char32_t>(c) && (c > 160 || std::isalpha(c)); }
template<> inline bool isblank<char32_t>(char32_t c) {
	return c < 128 && std::isblank(c); }
template<> inline bool iscntrl<char32_t>(char32_t c) {
	return c < 128 && std::iscntrl(c); }
template<> inline bool isdigit<char32_t>(char32_t c) {
	return c < 128 && std::isdigit(c); }
template<> inline bool isgraph<char32_t>(char32_t c) {
	return c < 128 && std::isgraph(c); }
template<> inline bool islower<char32_t>(char32_t c) {
	return c < 128 && std::islower(c); }
template<> inline bool isprint<char32_t>(char32_t c) {
	return !iseof<char32_t>(c) && (c > 160 || std::isprint(c)); }
template<> inline bool ispunct<char32_t>(char32_t c) {
	return c < 128 && std::ispunct(c); }
template<> inline bool isspace<char32_t>(char32_t c) {
	return c < 128 && std::isspace(c); }
template<> inline bool isupper<char32_t>(char32_t c) {
	return c < 128 && std::isupper(c); }
template<> inline bool isxdigit<char32_t>(char32_t c) {
	return c < 128 && std::isxdigit(c); }

} // idni::charclasses namespace
