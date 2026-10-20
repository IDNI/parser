// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__
#define __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include "unicode_properties.generated.h"

// Unicode properties of one code point, from the tables of Unicode
// 17.0.0 in unicode_properties.generated.h. A program that must match
// another Unicode version can differ for the code points that are new or
// changed after that version. A value above U+10FFFF has no property.
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

/// True for the General_Category Lu, Ll, Lt, Lm and Lo. A mark, such as
/// U+0903, is no letter here.
inline bool is_letter(char32_t cp) {
	return property_word(cp) & tables::UP_LETTER;
}

/// True for Numeric_Type Decimal, Digit or Numeric, such as U+00BD or
/// U+2160.
inline bool has_numeric_value(char32_t cp) {
	return property_word(cp) & tables::UP_NUMERIC_VALUE;
}

/// True for the Cased property of DerivedCoreProperties.txt.
inline bool is_cased(char32_t cp) {
	return property_word(cp) & tables::UP_CASED;
}

/// True for the Case_Ignorable property of DerivedCoreProperties.txt.
inline bool is_case_ignorable(char32_t cp) {
	return property_word(cp) & tables::UP_CASE_IGNORABLE;
}

namespace detail {
// @p table holds pairs of a code point and its mapping, sorted by code point.
template <size_t N>
char32_t simple_mapping(const uint32_t (&table)[N], char32_t cp) {
	constexpr size_t n = N / 2;
	size_t lo = 0, hi = n;
	while (lo < hi) {
		const size_t mid = lo + (hi - lo) / 2;
		if (table[2 * mid] < cp) lo = mid + 1;
		else hi = mid;
	}
	if (lo != n && table[2 * lo] == cp) return table[2 * lo + 1];
	return cp;
}

// @p table holds rows of a code point, a length and three code points,
// sorted by code point. Gives the row of @p cp, or nullptr.
template <size_t N>
const uint32_t* full_mapping(const uint32_t (&table)[N], char32_t cp) {
	constexpr size_t stride = 5;
	constexpr size_t n = N / stride;
	size_t lo = 0, hi = n;
	while (lo < hi) {
		const size_t mid = lo + (hi - lo) / 2;
		if (table[stride * mid] < cp) lo = mid + 1;
		else hi = mid;
	}
	if (lo != n && table[stride * lo] == cp) return table + stride * lo;
	return nullptr;
}
} // namespace detail

/// The simple lowercase mapping of UnicodeData.txt, or @p cp itself.
inline char32_t to_lower(char32_t cp) {
	return detail::simple_mapping(tables::lower_simple, cp);
}

/// Append the full lowercase mapping of @p cp to @p out: the
/// unconditional mapping of SpecialCasing.txt, else the simple one.
/// U+0130 gives U+0069 U+0307. A mapping that needs the context, such
/// as the final sigma, is not here.
inline void to_lower_full(char32_t cp, std::u32string& out) {
	if (const uint32_t* e = detail::full_mapping(tables::lower_full, cp))
		out.append(e + 2, e + 2 + e[1]);
	else out.push_back(to_lower(cp));
}

/// Append the full lowercase mapping of each code point of @p text to
/// @p out, with the Final_Sigma rule of the Unicode standard: U+03A3
/// gives U+03C2 when a cased code point comes before it and none comes
/// after it, with the case-ignorable code points between them skipped.
inline void to_lower_full(std::u32string_view text, std::u32string& out) {
	constexpr char32_t capital_sigma = 0x03A3;
	for (size_t i = 0; i != text.size(); ++i) {
		if (text[i] != capital_sigma) {
			to_lower_full(text[i], out);
			continue;
		}
		size_t j = i;
		while (j != 0 && is_case_ignorable(text[j - 1])) --j;
		bool final_sigma = j != 0 && is_cased(text[j - 1]);
		if (final_sigma) {
			size_t k = i + 1;
			while (k != text.size() && is_case_ignorable(text[k])) ++k;
			final_sigma = k == text.size() || !is_cased(text[k]);
		}
		out.push_back(final_sigma ? 0x03C2 : 0x03C3);
	}
}

/// The simple uppercase mapping of UnicodeData.txt, or @p cp itself.
/// U+01C5 gives U+01C4.
inline char32_t to_upper(char32_t cp) {
	return detail::simple_mapping(tables::upper_simple, cp);
}

/// Append the full uppercase mapping of @p cp to @p out: the
/// unconditional mapping of SpecialCasing.txt, else the simple one.
/// U+00DF gives U+0053 U+0053, and U+FB01 gives U+0046 U+0049.
inline void to_upper_full(char32_t cp, std::u32string& out) {
	if (const uint32_t* e = detail::full_mapping(tables::upper_full, cp))
		out.append(e + 2, e + 2 + e[1]);
	else out.push_back(to_upper(cp));
}

/// Append the full uppercase mapping of each code point of @p text to
/// @p out. No uppercase mapping needs the context.
inline void to_upper_full(std::u32string_view text, std::u32string& out) {
	for (char32_t cp : text) to_upper_full(cp, out);
}

} // namespace idni::charclasses::unicode

#endif // __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_H__
