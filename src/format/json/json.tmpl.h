// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TMPL_H__

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>
#include <version>
#if !defined(__cpp_lib_to_chars) || __cpp_lib_to_chars < 201611L
#include <locale>
#include <sstream>
#endif

#include "json.h"

namespace idni::format::json {

inline void escape(std::ostream& os, std::string_view s) {
	os << '"' << idni::escapes::encode(s, idni::escapes::json) << '"';
}

inline idni::escapes::decoded unescape(std::string_view body) {
	return idni::escapes::decode(body, idni::escapes::json);
}

namespace detail {

// The parts of a JSON number text. The value is the digits of integral and
// fraction, times 10^(exp10 - fraction.size()).
struct decimal_parts {
	bool neg = false;
	std::string_view integral;  // one or more digits
	std::string_view fraction;  // the digits after '.', or empty
	int64_t exp10 = 0;          // saturated at -1000000 and 1000000
};

// No value for a text that is no JSON number.
inline std::optional<decimal_parts> split_decimal(std::string_view s) {
	auto digit = [&s](size_t i) {
		return i < s.size() && s[i] >= '0' && s[i] <= '9';
	};
	decimal_parts p;
	size_t i = 0;
	if (i < s.size() && s[i] == '-') { p.neg = true; ++i; }
	const size_t int_at = i;
	while (digit(i)) ++i;
	p.integral = s.substr(int_at, i - int_at);
	if (p.integral.empty()) return std::nullopt;
	if (i < s.size() && s[i] == '.') {
		const size_t at = ++i;
		while (digit(i)) ++i;
		p.fraction = s.substr(at, i - at);
	}
	if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
		const bool neg = ++i < s.size() && s[i] == '-';
		if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
		if (!digit(i)) return std::nullopt;
		// a saturated exponent still tells a tiny value from a huge one
		for (; digit(i); ++i)
			p.exp10 = std::min<int64_t>(p.exp10 * 10 + (s[i] - '0'),
				1000000);
		if (neg) p.exp10 = -p.exp10;
	}
	if (i != s.size()) return std::nullopt;
	return p;
}

// True when the JSON number @p s is not 0 and its magnitude is below 1.
// It reads the digits only, so a text of any length gives an answer.
inline bool number_below_one(std::string_view s) {
	const auto p = split_decimal(s);
	if (!p) return false;
	// the decimal exponent of the first nonzero digit
	int64_t lead = 0;
	if (size_t nz = p->integral.find_first_not_of('0');
			nz != std::string_view::npos)
		lead = static_cast<int64_t>(p->integral.size() - nz) - 1;
	else if (size_t nz = p->fraction.find_first_not_of('0');
			nz != std::string_view::npos)
		lead = -static_cast<int64_t>(nz) - 1;
	else return false;
	return lead + p->exp10 < 0;
}

} // namespace detail

// strtod would read the decimal point of the current locale, and the write
// side uses to_chars, so both directions must ignore it. libc++ ships
// to_chars for a double but not from_chars, hence the stream fallback.
inline bool parse_number(const std::string& s, double& d) {
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
	auto fc = std::from_chars(s.data(), s.data() + s.size(), d);
	if (fc.ptr != s.data() + s.size()) return false;
	if (fc.ec == std::errc{}) return true;
	if (fc.ec != std::errc::result_out_of_range) return false;
#else
	std::istringstream is(s);
	is.imbue(std::locale::classic());
	// from_chars refuses leading whitespace, and this path must agree
	is >> std::noskipws >> d;
	const bool failed = is.fail();
	is.clear();
	if (is.peek() != std::char_traits<char>::eof()) return false;
	if (!failed) return true;
	// libc++ fails on ERANGE of strtod and keeps its value: a subnormal,
	// or 0 below the smallest subnormal
	return detail::number_below_one(s);
#endif
	// from_chars gives out_of_range only below half the smallest subnormal
	// or above the largest double, and JSON sets no lower limit
	if (!detail::number_below_one(s)) return false;
	d = 0;
	return true;
}

inline read_options read_options::lenient(size_t max_int_digits,
	size_t max_depth)
{
	return { .nonfinite = true, .keep_float_form = true,
		.lone_surrogates = true, .max_int_digits = max_int_digits,
		.max_depth = max_depth };
}

inline double parse_number_saturating(const std::string& s) {
	const bool neg = !s.empty() && s[0] == '-';
	double d = 0;
	if (!parse_number(s, d))
		return neg ? -std::numeric_limits<double>::infinity()
			: std::numeric_limits<double>::infinity();
	return std::fpclassify(d) == FP_ZERO && neg ? -0.0 : d;
}

}

#endif // __IDNI__PARSER__FORMAT__JSON_TMPL_H__
