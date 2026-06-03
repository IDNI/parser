// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_H__
#define __IDNI__PARSER__FORMAT__JSON_H__

#include <ostream>
#include <string>
#include <string_view>

#include "../../utility/escapes.h"
#include "../../utility/diagnostics.h"

namespace idni::format::json {

inline void escape(std::ostream& os, std::string_view s) {
	os << '"' << idni::escapes::encode(s, idni::escapes::json) << '"';
}

inline idni::escapes::decoded unescape(std::string_view body) {
	return idni::escapes::decode(body, idni::escapes::json);
}

/// Serialize a diagnostics report as JSON. With @p print_names = true,
/// each node carries an extra "message" field with the symbolic code name.
inline std::ostream& print(const diagnostics::report& r, std::ostream& os,
	bool print_names = false)
{
	os << "{\n  \"nodes\": [\n";
	const auto& nodes = r.nodes();
	const auto& attrs = r.attrs();
	for (size_t i = 0; i < nodes.size(); ++i) {
		if (i) os << ",\n";
		const auto& n = nodes[i];
		os << "    {\"tag\": " << static_cast<unsigned>(n.tag);
		if (print_names) {
			os << ", \"message\": ";
			escape(os, diagnostics::code_name(n.tag));
		}
		os << ", \"key\": ";
		escape(os, r.str(n.key));
		os << ", \"parent\": " << n.parent
		   << ", \"value\": " << n.value
		   << ", \"attrs\": [";
		for (uint8_t a = 0; a < n.attr_cnt; ++a) {
			if (a) os << ", ";
			const auto& at = attrs[n.attr_off + a];
			os << "{\"key\": ";
			escape(os, r.str(at.key));
			os << ", \"value\": " << at.value << "}";
		}
		os << "]}";
	}
	os << "\n  ]\n}";
	return os;
}

}

#endif // __IDNI__PARSER__FORMAT__JSON_H__
