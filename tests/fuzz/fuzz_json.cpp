// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "format/json/json.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	using jt = idni::format::json::tree;
	auto r = jt::get(std::string_view(
		reinterpret_cast<const char*>(data), size));
	if (r.has_value()) {
		const std::string text = jt::get(r.value()).to_str();
		const std::string sorted = jt::get(r.value()).to_str(
			{ .sorted_keys = true, .ascii_only = true });
		(void)text; (void)sorted;
	}
	jt::gc_if_grown();
	return 0;
}
