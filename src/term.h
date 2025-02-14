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
#ifndef __IDNI_TERM_H__
#define __IDNI_TERM_H__

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

#endif // __IDNI_TERM_H__