// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__PROGRAM_CODES_H__
#define __IDNI__PARSER__UTILITY__PROGRAM_CODES_H__

#include <cstdint>
#include <span>

#include "utility/diagnostics.h"

namespace idni::diagnostics {

/// Each severity band of code has a parser part and a program part, so a
/// program adds codes without a change of the parser enum.
struct code_band {
	uint16_t first;
	uint16_t last;
};

inline constexpr code_band parser_error_band    { 0x0000, 0x0FFF };
inline constexpr code_band program_error_band   { 0x1000, 0x3FFF };
inline constexpr code_band parser_warning_band  { 0x4000, 0x4FFF };
inline constexpr code_band program_warning_band { 0x5000, 0x7FFF };
inline constexpr code_band parser_info_band     { 0x8000, 0x8FFF };
inline constexpr code_band program_info_band    { 0x9000, 0xBFFF };

namespace detail {
// No definition: a call stops the constant evaluation, so the compile fails.
void code_number_past_its_band();
} // namespace detail

/// The code number @p n of band @p b, counted from 0. A number past the
/// band would give a code of the next severity, so it stops the compile.
consteval code program_code(code_band b, uint16_t n) {
	if (n > b.last - b.first) detail::code_number_past_its_band();
	return static_cast<code>(b.first + n);
}

/// The program error code number @p n, counted from 0.
consteval code program_error(uint16_t n) {
	return program_code(program_error_band, n);
}
consteval code program_warning(uint16_t n) {
	return program_code(program_warning_band, n);
}
consteval code program_info(uint16_t n) {
	return program_code(program_info_band, n);
}

/// One code of a program and its label. A report node carries the code
/// as "tag" and the label as "message".
struct program_code_label {
	code tag;
	/// Sentence form, ends with a period, for example "Unknown account.".
	const char* label;
};

/// Called once at start. code_name() then finds the label of a program
/// code. A code outside the program bands, or a code given twice, is an
/// error.
result<void> register_codes(std::span<const program_code_label> codes);

} // namespace idni::diagnostics

#include "program_codes.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__PROGRAM_CODES_H__
