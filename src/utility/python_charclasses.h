// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__PYTHON_CHARCLASSES_H__
#define __IDNI__PARSER__UTILITY__PYTHON_CHARCLASSES_H__

#include <string>
#include <string_view>

#include "unicode_properties.h"

// The character classes of the CPython str methods, from the Unicode
// tables of unicode_properties.h. A value above U+10FFFF is in no class.
namespace idni::charclasses::python {

/// Python str.isspace: White_Space, or the Bidi_Class WS, B or S.
inline bool isspace(char32_t c) {
	return unicode::property_word(c) & unicode::tables::UP_SPACE;
}

/// Python str.isprintable: U+0020, or no General_Category Cc, Cf, Cs,
/// Co, Cn, Zl, Zp or Zs.
inline bool isprintable(char32_t c) {
	return unicode::property_word(c) & unicode::tables::UP_PRINTABLE;
}

/// Python str.lower: append the full lowercase mapping of @p s to @p out,
/// with the Final_Sigma rule.
inline void lower(std::u32string_view s, std::u32string& out) {
	unicode::to_lower_full(s, out);
}

} // namespace idni::charclasses::python

#endif // __IDNI__PARSER__UTILITY__PYTHON_CHARCLASSES_H__
