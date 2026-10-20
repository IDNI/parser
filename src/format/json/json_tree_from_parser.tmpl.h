// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TREE_FROM_PARSER_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TREE_FROM_PARSER_TMPL_H__

#include <bit>
#include <cmath>
#include <string>
#include <string_view>

#include "json.h"

namespace idni::format::json {

inline diagnostics::result<tref> tree::get(const parse_tree& t) {
	using pnode = json_parser::pnode;
	diagnostics::result<tref> R;
	// parse tree node -> JSON tree node; the subtree equality matches
	// the equal parse nodes that search_unique visits once
	subtree_unordered_map<pnode, tref> m;
	auto mapped = [&m](tref p) -> tref {
		auto it = m.find(p);
		return it == m.end() ? nullptr : it->second;
	};
	auto text = [](tref p) -> std::string {
		return trv(p) | trv::terminals;
	};
	auto leaf_kind = [](size_t nt) {
		switch (nt) {
		case json_parser::null_sym: case json_parser::true_sym:
		case json_parser::false_sym: case json_parser::number:
		case json_parser::str:
			return true;
		default: return false;
		}
	};
	auto transform = [&](tref p) {
		const parse_tree& pt = parse_tree::get(p);
		if (!pt.is_nt()) return true;
		tref x = nullptr;
		switch (pt.get_nt()) {
		case json_parser::null_sym:
		case json_parser::true_sym:
		case json_parser::false_sym:
			x = get(static_cast<type>(pt.get_nt()));
			break;
		case json_parser::number: {
			const std::string s = text(p);
			double d = 0;
			if (!parse_number(s, d)) {
				R.error(code::parse_error, "Invalid JSON number",
					{{label::value, s}});
				return false;
			}
			// -0 and 0 compare equal, so one node holds both
			if (std::fpclassify(d) == FP_ZERO) d = 0;
			if (s.find_first_of(".eE") != std::string::npos) {
				x = build_number(d);
				break;
			}
			// an integer literal outside int64 keeps its text too, so
			// it prints exactly
			x = get(node(json_parser::number, std::bit_cast<uint64_t>(d),
				s == "-0" ? "0" : s));
			break;
		}
		case json_parser::str: {
			auto dec = idni::escapes::decode(text(p), idni::escapes::json);
			if (dec.has_error()) {
				R.append(std::move(dec).report());
				return false;
			}
			x = build_string(std::move(dec).value());
			break;
		}
		case json_parser::start:
		case json_parser::value:
			if (auto c = trv(p) | trv::only_child; c)
				x = mapped(c.value());
			break;
		case json_parser::arr: {
			trefs el;
			for (tref v : (trv(p) | json_parser::values
					|| json_parser::value).values())
				el.push_back(mapped(v));
			x = build_array(el);
			break;
		}
		case json_parser::object_pair:
			x = build_pair(get(mapped(trv(p) | json_parser::str
				| trv::ref)).get_string(),
				mapped(trv(p) | json_parser::value | trv::ref));
			break;
		case json_parser::object: {
			trefs pairs;
			for (tref c : (trv(p) || json_parser::object_pair)
					.values())
				pairs.push_back(mapped(c));
			x = build_object(pairs);
			break;
		}
		default: return true;
		}
		if (x != nullptr) m.emplace(p, x);
		return true;
	};
	// the terminals of a leaf are read by the leaf itself
	auto into = [&leaf_kind](tref, tref parent = nullptr) {
		return parent == nullptr || !parse_tree::get(parent).is_nt()
			|| !leaf_kind(parse_tree::get(parent).get_nt());
	};
	post_order<pnode>(t.get()).search_unique(transform, into);
	if (R.has_error()) return R;
	tref x = mapped(t.get());
	if (x == nullptr) {
		R.error(code::parse_error, "No JSON value in the parse tree");
		return R;
	}
	return std::move(R).with_value(x);
}

inline diagnostics::result<tref> tree::get(json_parser::result& r) {
	diagnostics::result<tref> R;
	if (!r.found) {
		if (!r.report().nodes().empty()) R.append(std::move(r.report()));
		// The parser reports its syntax error itself.
		if (!R.has_error())
			R.error(code::parse_error, r.parse_error.to_str());
		return R;
	}
	tref n = r.get_shaped_tree2();
	if (!r.report().nodes().empty()) R.append(std::move(r.report()));
	auto x = get(parse_tree::get(n));
	tref v = x.has_value() ? x.value() : nullptr;
	R.append(std::move(x).report());
	if (R.has_error() || v == nullptr) return R;
	return std::move(R).with_value(v);
}

inline diagnostics::result<tref> tree::get(std::string_view text) {
	json_parser::parse_options po{ .start = json_parser::start };
	auto r = json_parser::instance().parse(text.data(), text.size(), po);
	return get(r);
}

} // namespace idni::format::json

#endif // __IDNI__PARSER__FORMAT__JSON_TREE_FROM_PARSER_TMPL_H__
