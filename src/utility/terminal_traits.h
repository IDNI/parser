// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__TERMINAL_TRAITS_H__
#define __IDNI__PARSER__UTILITY__TERMINAL_TRAITS_H__

#include <cstdio>
#include <string>
#include <vector>

#include "utf8char_traits.h"

namespace idni {

/// The traits of a bit stream, so std::char_traits<bool> is never named.
/// The standard allows no such specialization, and libc++ has none.
struct bit_traits {
	using char_type = bool;
	using int_type = int;

	static constexpr void assign(char_type& r, const char_type& a) noexcept {
		r = a;
	}
	static constexpr bool eq(char_type a, char_type b) noexcept {
		return a == b;
	}
	static constexpr bool lt(char_type a, char_type b) noexcept {
		return a < b;
	}
	static constexpr char_type to_char_type(int_type c) noexcept {
		return c != 0;
	}
	static constexpr int_type to_int_type(char_type c) noexcept {
		return c ? 1 : 0;
	}
	static constexpr bool eq_int_type(int_type a, int_type b) noexcept {
		return a == b;
	}
	/// No bit has this value, so the end of a stream stays apart.
	static constexpr int_type eof() noexcept { return -1; }
	static constexpr int_type not_eof(int_type c) noexcept {
		return eq_int_type(c, eof()) ? 0 : c;
	}
};

/// The traits of a terminal or input type @p T and the string of its
/// terminals.
template <typename T>
struct terminal_traits {
	using string_type = std::basic_string<T>;
};

/// A character type: std::char_traits.
template <typename C>
struct char_terminal_traits {
	using type = std::char_traits<C>;
	using string_type = std::basic_string<C>;
};

template <> struct terminal_traits<char> : char_terminal_traits<char> {};
template <> struct terminal_traits<wchar_t> : char_terminal_traits<wchar_t> {};
template <> struct terminal_traits<char8_t> : char_terminal_traits<char8_t> {};
template <> struct terminal_traits<char16_t>
	: char_terminal_traits<char16_t> {};
template <> struct terminal_traits<char32_t>
	: char_terminal_traits<char32_t> {};

template <>
struct terminal_traits<unsigned char> {
	using type = utf8char_traits;
	using string_type = std::basic_string<unsigned char>;
};

template <>
struct terminal_traits<bool> {
	using type = bit_traits;
	// std::basic_string<bool> needs std::char_traits<bool>
	using string_type = std::vector<bool>;
};

template <typename T>
using terminal_char_traits = typename terminal_traits<T>::type;

/// The string of terminals of type @p T: std::basic_string<T> for a
/// character type, std::vector<bool> for a bit.
template <typename T>
using terminal_string = typename terminal_traits<T>::string_type;

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__TERMINAL_TRAITS_H__
