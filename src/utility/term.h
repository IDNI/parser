// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#ifndef __IDNI__PARSER__UTILITY__TERM_H__
#define __IDNI__PARSER__UTILITY__TERM_H__

// #define TERM_DEBUG 1
#ifdef TERM_DEBUG
#	define TDBG(x) x
#else
#	define TDBG(x)
#endif // TERM_DEBUG

namespace idni::term {

bool open();
void close();
void enable_getline_mode();
void disable_getline_mode();
void clear();
int in(char& c);
int in(char* s, size_t l);
void out(const char* data, size_t size);
void out(const std::string& str);
void clear_line();
void cursor_up(int n = 1);
void cursor_down(int n = 1);
void cursor_right(int n = 1);
void cursor_left(int n = 1);
std::pair<unsigned short, unsigned short> get_termsize();
bool is_tty();

} // namespace idni::term

#include "term.impl.h"
#endif // __IDNI__PARSER__UTILITY__TERM_H__