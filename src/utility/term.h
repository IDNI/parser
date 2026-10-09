// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__TERM_H__
#define __IDNI__PARSER__UTILITY__TERM_H__

#include <cstddef>
#include <string>

// #define TERM_DEBUG 1
#ifdef TERM_DEBUG
#	define TDBG(x) x
#else
#	define TDBG(x)
#endif // TERM_DEBUG

namespace idni::term {

/// Drops a UTF-8 byte order mark at the start of @p s. PowerShell writes one
/// before the text it pipes into a program.
inline void drop_utf8_bom(std::string& s) {
	if (s.starts_with("\xEF\xBB\xBF")) s.erase(0, 3);
}

bool open();
void close();
void enable_getline_mode();
void disable_getline_mode();
void clear();
/// Byte count read, or -1 on error: read(2) returns a signed count.
std::ptrdiff_t in(char& c);
std::ptrdiff_t in(char* s, size_t l);
void out(const char* data, size_t size);
void out(const std::string& str);
void clear_line();
void cursor_up(size_t n = 1);
void cursor_down(size_t n = 1);
void cursor_right(size_t n = 1);
void cursor_left(size_t n = 1);
std::pair<unsigned short, unsigned short> get_termsize();
bool is_tty();

} // namespace idni::term

#include "term.impl.h"
#endif // __IDNI__PARSER__UTILITY__TERM_H__
