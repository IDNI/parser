// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TREE_BUILDERS_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TREE_BUILDERS_TMPL_H__

#include <bit>
#include <charconv>
#include <cmath>
#include <optional>
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

inline tref tree::build_report(const diagnostics::report& r, bool names) {
	const auto& nodes = r.nodes();
	std::vector<std::vector<size_t>> kids(nodes.size());
	for (size_t i = 0; i < nodes.size(); ++i)
		if (nodes[i].parent >= 0)
			kids[static_cast<size_t>(nodes[i].parent)].push_back(i);
	trefs roots;
	for (size_t i = 0; i < nodes.size(); ++i)
		if (nodes[i].parent < 0)
			roots.push_back(build_report_node(r, i, kids, names));
	return build_object({ build_pair("nodes", build_array(roots)) });
}

inline tref tree::build_report_node(const diagnostics::report& r, size_t i,
	const std::vector<std::vector<size_t>>& kids, bool names)
{
	// The members of node n without its children.
	auto fields = [&r, names](size_t n) {
		const auto& nd = r.nodes()[n];
		trefs f{ build_pair("tag", build_integer(
			static_cast<uint16_t>(nd.tag))) };
		if (names) f.push_back(build_pair("message",
			build_string(diagnostics::code_name(nd.tag))));
		f.push_back(build_pair("key", build_string(r.str(nd.key))));
		// a message has no counter, and the reader takes a missing value as 0
		if (!diagnostics::is_info_message(nd.tag) || nd.value != 0)
			f.push_back(build_pair("value", build_integer(nd.value)));
		trefs attrs;
		for (uint8_t a = 0; a < nd.attr_cnt; ++a) {
			const auto& at = r.attrs()[nd.attr_off + a];
			// a text label stores an interned string key in value
			tref av = parser_strings::is_text_label(at.key)
				? build_string(r.str(static_cast<int_t>(at.value)))
				: build_integer(at.value);
			attrs.push_back(build_object({
				build_pair("key", build_string(r.str(at.key))),
				build_pair("value", av) }));
		}
		f.push_back(build_pair("attrs", build_array(attrs)));
		return f;
	};
	// A node in progress, its members, its finished children and the
	// index of its next child. Children are built before their parent.
	struct frame {
		size_t node;
		trefs members;
		trefs children{};
		size_t next = 0;
	};
	tref root = nullptr;
	std::vector<frame> open;
	open.push_back({ i, fields(i) });
	while (!open.empty()) {
		frame& f = open.back();
		if (f.next != kids[f.node].size()) {
			const size_t k = kids[f.node][f.next++];
			open.push_back({ k, fields(k) });
			continue;
		}
		trefs members = std::move(f.members);
		if (!f.children.empty()) members.push_back(
			build_pair("children", build_array(f.children)));
		tref x = build_object(members);
		open.pop_back();
		if (open.empty()) root = x;
		else open.back().children.push_back(x);
	}
	return root;
}

//------------------------------------------------------------------------------

inline diagnostics::report report_of(tref v) {
	diagnostics::report r;
	tref nodes = v ? tree::get(v).find("nodes") : nullptr;
	if (nodes == nullptr || !tree::get(nodes).is_array()) return r;
	for (tref n : tree::get(nodes).children()) report_node_of(r, n);
	return r;
}

inline void report_node_of(diagnostics::report& r, tref n) {
	using diagnostics::attr_in;
	using diagnostics::code;
	if (n == nullptr || !tree::get(n).is_object()) return;
	// The JSON can come from the network, so a number is read only as an
	// exact integer: a cast of an out of range double is undefined.
	auto integer = [](tref x) -> std::optional<int64_t> {
		if (x == nullptr || !tree::get(x).is_number()) return std::nullopt;
		return tree::get(x).get_integer();
	};
	// Open the report node of the JSON object n.
	auto push = [&r, &integer](const tree& n) {
		code tag = code::unknown;
		if (auto t = integer(n.find("tag")); t && *t >= 0 && *t <= 0xFFFF)
			tag = static_cast<code>(*t);
		diagnostics::report::key key = 0;
		if (auto k = string_member(n.get(), "key"); k)
			key = r.intern(*k);
		const int64_t node_value = integer(n.find("value")).value_or(0);
		std::vector<attr_in> attrs;
		if (tref a = n.find("attrs"); a && tree::get(a).is_array())
			for (const tree& at : tree::get(a).children_trees()) {
				auto ak = string_member(at.get(), "key");
				if (!ak) continue;
				diagnostics::report::key akey = r.intern(*ak);
				tref av = at.find("value");
				if (av && tree::get(av).is_string())
					attrs.emplace_back(akey, std::string_view(
						tree::get(av).get_string()));
				else if (auto v = integer(av)) {
					diagnostics::attr num;
					num.key = akey;
					num.value = *v;
					attrs.emplace_back(num);
				}
			}
		r.push_node(tag, key, node_value, attrs);
	};
	// Only the children members are entered, so every object on the way
	// is a report node. It opens on the way down and closes on the way up,
	// so its children nest under it.
	auto into = [](tref n) {
		const tree& t = tree::get(n);
		return !t.is(json_parser::object_pair)
			|| t.get_string() == "children";
	};
	auto enter = [&push](tref n) {
		if (tree::get(n).is_object()) push(tree::get(n));
	};
	auto leave = [&r](tref n) {
		if (tree::get(n).is_object()) r.pop_node();
	};
	pre_order<node>(n).visit(enter, into, leave);
}

} // namespace idni::format::json

#endif // __IDNI__PARSER__FORMAT__JSON_TREE_BUILDERS_TMPL_H__
