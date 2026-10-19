// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__
#define __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>

#include "unicode_properties.generated.h"

// Unicode properties of one code point, from the tables of the Unicode
// version in unicode_properties.generated.h. A value above U+10FFFF has
// no property.
namespace idni::charclasses::unicode {

inline uint16_t property_word(char32_t cp) {
	if (cp > 0x10FFFF) return 0;
	return tables::props_stage2[static_cast<size_t>(
		tables::props_stage1[cp >> 8]) * 256 + (cp & 0xFF)];
}

/// The value 0 to 9 of a code point with Numeric_Type Decimal, such as
/// U+0661 or U+FF11.
inline std::optional<int> decimal_digit_value(char32_t cp) {
	const uint16_t w = property_word(cp);
	if (!(w & tables::UP_DECIMAL)) return std::nullopt;
	return w & tables::UP_DIGIT_VALUE;
}

/// True for Numeric_Type Decimal or Digit, such as U+00B2.
inline bool has_digit_value(char32_t cp) {
	return property_word(cp) & tables::UP_DIGIT;
}

/// The simple lowercase mapping of UnicodeData.txt, or @p cp itself.
inline char32_t to_lower(char32_t cp) {
	constexpr size_t n = std::size(tables::lower_simple) / 2;
	size_t lo = 0, hi = n;
	while (lo < hi) {
		const size_t mid = lo + (hi - lo) / 2;
		if (tables::lower_simple[2 * mid] < cp) lo = mid + 1;
		else hi = mid;
	}
	if (lo != n && tables::lower_simple[2 * lo] == cp)
		return tables::lower_simple[2 * lo + 1];
	return cp;
}

/// Append the full lowercase mapping of @p cp to @p out: the
/// unconditional mapping of SpecialCasing.txt, else the simple one.
/// U+0130 gives U+0069 U+0307. A mapping that needs the context, such
/// as the final sigma, is not here.
inline void to_lower_full(char32_t cp, std::u32string& out) {
	constexpr size_t stride = 5;
	const auto* end = std::end(tables::lower_full);
	for (const auto* e = std::begin(tables::lower_full); e != end;
			e += stride)
		if (e[0] == cp) {
			out.append(e + 2, e + 2 + e[1]);
			return;
		}
	out.push_back(to_lower(cp));
}

} // namespace idni::charclasses::unicode

#endif // __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__
