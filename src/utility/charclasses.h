// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__CHARCLASSES_H__
#define __IDNI__PARSER__UTILITY__CHARCLASSES_H__

#include <locale>

namespace idni::charclasses {

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

} // idni::charclasses namespace

#include "charclasses.impl.h"

#endif //__IDNI__PARSER__UTILITY__CHARCLASSES_H__
