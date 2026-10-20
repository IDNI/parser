// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__PYTHON_CHARCLASSES_H__
#define __IDNI__PARSER__UTILITY__PYTHON_CHARCLASSES_H__

#include <string>
#include <string_view>

#include "characters.h"
#include "diagnostics.h"
#include "unicode_properties.h"
#include "../parser_strings.h"

// The character classes of the CPython str methods, from the Unicode
// tables of unicode_properties.h. A value above U+10FFFF is in no class.
namespace idni::charclasses::python {

/// Python str.isdecimal: Numeric_Type Decimal, such as U+0661.
inline bool isdecimal(char32_t c) {
	return unicode::decimal_digit_value(c).has_value();
}

/// Python str.isdigit: Numeric_Type Decimal or Digit, such as U+00B2.
inline bool isdigit(char32_t c) {
	return unicode::has_digit_value(c);
}

/// Python str.isnumeric: Numeric_Type Decimal, Digit or Numeric, such as
/// U+00BD.
inline bool isnumeric(char32_t c) {
	return unicode::has_numeric_value(c);
}

/// Python str.isalpha: the General_Category Lu, Ll, Lt, Lm and Lo.
inline bool isalpha(char32_t c) {
	return unicode::is_letter(c);
}

/// Python str.isalnum: isalpha or isnumeric.
inline bool isalnum(char32_t c) {
	return isalpha(c) || isnumeric(c);
}

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

/// Python str.upper: append the full uppercase mapping of @p s to @p out.
inline void upper(std::u32string_view s, std::u32string& out) {
	unicode::to_upper_full(s, out);
}

namespace detail {
// Decodes @p s, maps the code points with @p map and encodes the result.
// The first byte that starts no UTF-8 code point gives code::invalid_utf8.
template <typename Map>
diagnostics::result<std::string> map_utf8(std::string_view s, Map map) {
	using label = parser_strings::label;
	diagnostics::result<std::string> r;
	std::u32string text;
	text.reserve(s.size());
	const auto* p = reinterpret_cast<const utf8char*>(s.data());
	for (size_t i = 0; i < s.size(); ) {
		char32_t cp;
		const size_t n = peek_codepoint(p + i, s.size() - i, cp);
		if (n == 0 || n == static_cast<size_t>(-1))
			return r.with_error(diagnostics::code::invalid_utf8,
				parser_strings::messages::escape_invalid_utf8,
				{ { label::offset, i } });
		text.push_back(cp);
		i += n;
	}
	std::u32string out;
	map(text, out);
	return r.with_value(to_std_string(out));
}
} // namespace detail

/// Python str.lower of the UTF-8 text @p s.
inline diagnostics::result<std::string> lower(std::string_view s) {
	return detail::map_utf8(s, [](std::u32string_view t, std::u32string& o) {
		lower(t, o); });
}

/// Python str.upper of the UTF-8 text @p s.
inline diagnostics::result<std::string> upper(std::string_view s) {
	return detail::map_utf8(s, [](std::u32string_view t, std::u32string& o) {
		upper(t, o); });
}

} // namespace idni::charclasses::python

#endif // __IDNI__PARSER__UTILITY__PYTHON_CHARCLASSES_H__
