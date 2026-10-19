// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__
#define __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__

#include <cstddef>
#include <cstdint>
#include <optional>

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

} // namespace idni::charclasses::unicode

#endif // __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__
