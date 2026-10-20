// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <algorithm>
#include <array>

#include "program_codes.h"

namespace idni::diagnostics {

inline result<void> register_codes(std::span<const program_code_label> codes)
{
	using msgs = idni::parser_strings::messages;
	constexpr std::array program_bands{
		program_error_band, program_warning_band, program_info_band };
	auto& labels = detail::program_code_labels();
	result<void> r;
	for (size_t i = 0; i < codes.size(); ++i) {
		const auto n = static_cast<uint16_t>(codes[i].tag);
		const int_t v = n;
		if (std::ranges::none_of(program_bands, [n](code_band b) {
				return b.first <= n && n <= b.last; }))
			r.error(code::invalid_argument,
				msgs::code_outside_program_bands, v);
		else if (labels.contains(n) || std::ranges::any_of(
				codes.first(i), [&codes, i](const auto& p) {
					return p.tag == codes[i].tag; }))
			r.error(code::invalid_argument,
				msgs::code_registered_twice, v);
		if (codes[i].label == nullptr)
			r.error(code::invalid_argument,
				msgs::code_without_label, v);
	}
	if (r.report().has_error()) return r;
	// No lock: a program registers its codes once at start, before it
	// starts a thread that reads them through code_name().
	for (const auto& p : codes)
		labels.emplace(static_cast<uint16_t>(p.tag), p.label);
	return r.with_value();
}

} // namespace idni::diagnostics
