// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TREE_BUILDERS_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TREE_BUILDERS_TMPL_H__

#include <bit>
#include <charconv>
#include <cmath>
#include <unordered_map>

#include "json.h"

namespace idni::format::json {

// A constant is interned on each call and never cached, so a gc never
// leaves a cached tref dangling.
inline tref tree::_null()  { return get(json_parser::null_sym); }
inline tref tree::_true()  { return get(json_parser::true_sym); }
inline tref tree::_false() { return get(json_parser::false_sym); }

inline tref tree::build_bool(bool b) { return b ? _true() : _false(); }

inline tref tree::build_number(double n) {
	// JSON has no text for an infinity or a NaN
	if (!std::isfinite(n)) return _null();
	// 2^63 is exact as a double, and every double below it in magnitude
	// converts to int64 without overflow
	// exact: only a whole double is an integer
	if (std::bit_cast<uint64_t>(std::trunc(n))
			== std::bit_cast<uint64_t>(n) && std::fabs(n) < 0x1p63)
		return build_integer(static_cast<int64_t>(n));
	return get(node(json_parser::number, std::bit_cast<uint64_t>(n)));
}

inline tref tree::build_integer(int64_t n) {
	char buf[24];
	auto r = std::to_chars(buf, buf + sizeof(buf), n);
	// the double of the text, so a parse of the same text is one node
	return get(node(json_parser::number,
		std::bit_cast<uint64_t>(static_cast<double>(n)),
		std::string(buf, r.ptr)));
}

inline tref tree::build_string(std::string_view s) {
	return get(node(json_parser::str, 0, std::string(s)));
}

inline tref tree::build_array(const trefs& elements) {
	return get(json_parser::arr, elements);
}

inline tref tree::build_pair(std::string_view key, tref v) {
	return get(node(json_parser::object_pair, 0, std::string(key)), v);
}

inline tref tree::build_object(const trefs& pairs) {
	trefs members;
	members.reserve(pairs.size());
	std::unordered_map<std::string_view, size_t> at;
	for (tref p : pairs) {
		DBG(assert(get(p).is(json_parser::object_pair));)
		auto [it, added] = at.emplace(get(p).value.str, members.size());
		if (added) members.push_back(p);
		else members[it->second] = p;
	}
	return get(json_parser::object, members);
}

inline tref tree::set_member(tref object, std::string_view key, tref v) {
	DBG(assert(get(object).is_object());)
	trefs pairs = get(object).get_children();
	pairs.push_back(build_pair(key, v));
	return build_object(pairs);
}

} // namespace idni::format::json

#endif // __IDNI__PARSER__FORMAT__JSON_TREE_BUILDERS_TMPL_H__
