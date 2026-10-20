// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TREE_FROM_PARSER_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TREE_FROM_PARSER_TMPL_H__

#include <bit>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "json.h"

namespace idni::format::json {

inline diagnostics::result<tref> tree::get(const parse_tree& t,
	const read_options& o)
{
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
	// The exact value of a number with a fraction or an exponent as the
	// decimal text of an integer. No value when the number is no integer.
	auto exact_integer = [](const detail::decimal_parts& p)
			-> std::optional<std::string>
	{
		std::string digits(p.integral);
		digits += p.fraction;
		if (digits.find_first_not_of('0') == std::string::npos)
			return "0";
		const int64_t exp = p.exp10 - static_cast<int64_t>(p.fraction.size());
		if (exp < 0) {
			const size_t cut = static_cast<size_t>(-exp);
			if (cut > digits.size() || digits.find_first_not_of('0',
				digits.size() - cut) != std::string::npos)
					return std::nullopt;
			digits.resize(digits.size() - cut);
		} else if (exp > 20) return std::nullopt;
		else digits.append(static_cast<size_t>(exp), '0');
		digits.erase(0, digits.find_first_not_of('0'));
		if (digits.empty()) return "0";
		return p.neg ? "-" + digits : digits;
	};
	auto leaf_kind = [](size_t nt) {
		switch (nt) {
		case json_parser::null_sym: case json_parser::true_sym:
		case json_parser::false_sym: case json_parser::number:
		case json_parser::nonfinite: case json_parser::str:
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
			const auto fr = trv(p) | json_parser::fractional;
			const auto ep = trv(p) | json_parser::exponent;
			if (o.keep_float_form) {
				double d = parse_number_saturating(s);
				if (fr || ep) {
					x = get(node(json_parser::number,
						std::bit_cast<uint64_t>(d)));
					break;
				}
				// an integer -0 is the integer 0
				if (std::fpclassify(d) == FP_ZERO) d = 0;
				x = get(node(json_parser::number,
					std::bit_cast<uint64_t>(d), s == "-0" ? "0" : s));
				break;
			}
			double d = 0;
			if (!parse_number(s, d)) {
				R.error(code::parse_error, "Invalid JSON number",
					{{label::value, s}});
				return false;
			}
			// -0 and 0 compare equal, so one node holds both
			if (std::fpclassify(d) == FP_ZERO) d = 0;
			if (fr || ep) {
				// the integer text only when the double is the number
				// exactly, so 9007199254740993.0 gets none
				const auto parts = detail::split_decimal(s);
				const auto ex = parts ? exact_integer(*parts)
					: std::optional<std::string>{};
				// exact: only a whole double is an integer
				if (ex && std::bit_cast<uint64_t>(std::trunc(d))
						== std::bit_cast<uint64_t>(d)
					&& std::fabs(d) < 0x1p63
					&& std::to_string(static_cast<int64_t>(d)) == *ex)
						x = build_integer(static_cast<int64_t>(d));
				else x = get(node(json_parser::number,
					std::bit_cast<uint64_t>(d)));
				break;
			}
			// an integer literal outside int64 keeps its text too, so
			// it prints exactly
			x = get(node(json_parser::number, std::bit_cast<uint64_t>(d),
				s == "-0" ? "0" : s));
			break;
		}
		case json_parser::nonfinite: {
			const std::string s = text(p);
			// one NaN pattern, so equal texts intern as one node
			const double d = s == "NaN"
				? std::numeric_limits<double>::quiet_NaN()
				: s == "Infinity" ? std::numeric_limits<double>::infinity()
				: -std::numeric_limits<double>::infinity();
			x = get(node(json_parser::number, std::bit_cast<uint64_t>(d)));
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

inline diagnostics::result<tref> tree::get(json_parser::result& r,
	const read_options& o)
{
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
	auto x = get(parse_tree::get(n), o);
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

inline diagnostics::result<tref> tree::get(std::string_view text,
	const read_options& o)
{
	parser<char, char32_t>& p = o.nonfinite ? detail::nonfinite_parser()
		: json_parser::instance();
	parser<char, char32_t>::parse_options po{ .start = json_parser::start };
	auto r = p.parse(text.data(), text.size(), po);
	return get(r, o);
}

namespace detail {

inline grammar<char, char32_t>& nonfinite_grammar() {
	static grammar<char, char32_t>& g = []() -> grammar<char, char32_t>& {
		static grammar<char, char32_t> v(json_parser_data::nts(),
			json_parser_data::productions(),
			json_parser_data::start_symbol(),
			json_parser_data::char_classes(),
			json_parser_data::grammar_options());
		auto r = v.set_variant("nonfinite");
		DBG(assert(r.has_value());)
		(void)r;
		return v;
	}();
	return g;
}

inline parser<char, char32_t>& nonfinite_parser() {
	static parser<char, char32_t> p(nonfinite_grammar(),
		json_parser_data::make_parser_options());
	return p;
}

} // namespace detail

} // namespace idni::format::json

#endif // __IDNI__PARSER__FORMAT__JSON_TREE_FROM_PARSER_TMPL_H__
