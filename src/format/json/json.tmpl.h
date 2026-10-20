// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TMPL_H__

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <variant>
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

inline value::kind value::type() const { return kind_; }
inline bool value::is_null()   const { return kind_ == kind::null; }
inline bool value::is_bool()   const { return kind_ == kind::boolean; }
inline bool value::is_number() const { return kind_ == kind::number; }
inline bool value::is_string() const { return kind_ == kind::string; }
inline bool value::is_array()  const { return kind_ == kind::array; }
inline bool value::is_object() const { return kind_ == kind::object; }

inline bool value::as_bool() const { assert(is_bool()); return bool_; }
inline double value::as_number() const { assert(is_number()); return num_; }
inline const std::string& value::as_string() const {
	assert(is_string()); return str_;
}

inline size_t value::size() const {
	return kind_ == kind::object ? obj_.size() : arr_.size();
}
inline const value& value::operator[](size_t i) const {
	assert(is_array()); return arr_[i];
}
inline std::vector<value>::const_iterator value::begin() const {
	return arr_.begin();
}
inline std::vector<value>::const_iterator value::end() const {
	return arr_.end();
}

inline const std::vector<std::pair<std::string, value>>& value::members()
	const
{
	return obj_;
}
inline const value* value::find(std::string_view key) const {
	for (auto& kv : obj_) if (kv.first == key) return &kv.second;
	return nullptr;
}

inline value value::null()          { return value(); }
inline value value::boolean(bool b) { value v; v.kind_ = kind::boolean; v.bool_ = b; return v; }
inline value value::number(double n){ value v; v.kind_ = kind::number; v.num_ = n; return v; }
inline value value::string(std::string s) {
	value v; v.kind_ = kind::string; v.str_ = std::move(s); return v;
}
inline value value::array()  { value v; v.kind_ = kind::array;  return v; }
inline value value::object() { value v; v.kind_ = kind::object; return v; }

inline value& value::push_back(value v) {
	assert(is_array());
	arr_.push_back(std::move(v));
	return *this;
}
inline value& value::set(std::string key, value v) {
	assert(is_object());
	for (auto& kv : obj_)
		if (kv.first == key) { kv.second = std::move(v); return *this; }
	obj_.emplace_back(std::move(key), std::move(v));
	return *this;
}

namespace detail {

template <typename T> inline constexpr bool is_vector_v = false;
template <typename T, typename A>
inline constexpr bool is_vector_v<std::vector<T, A>> = true;

template <typename T> inline constexpr bool is_variant_v = false;
template <typename... Ts>
inline constexpr bool is_variant_v<std::variant<Ts...>> = true;

template <typename T>
inline constexpr bool is_json_type_v = std::is_same_v<T, bool>
	|| std::unsigned_integral<T> || std::is_same_v<T, std::string>
	|| is_vector_v<T> || is_variant_v<T>;

// A valueless variant has no alternative to print, so it is null.
template <std::size_t I = 0, typename V>
value variant_to_json(const V& v) {
	if constexpr (I < std::variant_size_v<V>) {
		if (const auto* x = std::get_if<I>(&v)) return to_json(*x);
		return variant_to_json<I + 1>(v);
	} else return value::null();
}

template <typename V, std::size_t I = 0>
std::optional<V> variant_from_json(const value& v) {
	if constexpr (I < std::variant_size_v<V>) {
		using A = std::variant_alternative_t<I, V>;
		if (auto x = from_json<A>(v))
			return V(std::in_place_index<I>, std::move(*x));
		return variant_from_json<V, I + 1>(v);
	} else return std::nullopt;
}

} // namespace detail

template <typename T>
value to_json(const T& v) {
	static_assert(detail::is_json_type_v<T>, "to_json converts a bool, "
		"an unsigned integer, a std::string, or a std::vector or a "
		"std::variant of these types");
	if constexpr (std::is_same_v<T, bool>) return value::boolean(v);
	else if constexpr (std::unsigned_integral<T>)
		return value::number(static_cast<double>(v));
	else if constexpr (std::is_same_v<T, std::string>)
		return value::string(v);
	else if constexpr (detail::is_vector_v<T>) {
		auto a = value::array();
		for (const auto& e : v) a.push_back(to_json(e));
		return a;
	} else return detail::variant_to_json(v);
}

template <typename T>
std::optional<T> from_json(const value& v) {
	static_assert(detail::is_json_type_v<T>, "from_json converts a bool, "
		"an unsigned integer, a std::string, or a std::vector or a "
		"std::variant of these types");
	if constexpr (std::is_same_v<T, bool>) {
		if (!v.is_bool()) return std::nullopt;
		return v.as_bool();
	} else if constexpr (std::unsigned_integral<T>) {
		if (!v.is_number()) return std::nullopt;
		const double d = v.as_number();
		// a double holds every whole number up to 2^53 exactly
		constexpr double exact = 9007199254740992.0;
		if (!(d >= 0) || d > exact || d != std::trunc(d)
			|| d > static_cast<double>(std::numeric_limits<T>::max()))
				return std::nullopt;
		return static_cast<T>(d);
	} else if constexpr (std::is_same_v<T, std::string>) {
		if (!v.is_string()) return std::nullopt;
		return v.as_string();
	} else if constexpr (detail::is_vector_v<T>) {
		if (!v.is_array()) return std::nullopt;
		T out;
		for (const auto& e : v) {
			auto x = from_json<typename T::value_type>(e);
			if (!x) return std::nullopt;
			out.push_back(std::move(*x));
		}
		return out;
	} else return detail::variant_from_json<T>(v);
}

inline value build_string(const trv& t, result& R) {
	std::string body = t | trv::terminals;
	auto dec = unescape(body);
	if (dec.has_error()) { R.append(std::move(dec).report()); return {}; }
	return value::string(std::move(dec).value());
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

inline value build(const trv& t, result& R) {
	auto v = t | trv::only_child;
	switch (v | trv::nonterminal) {
	case json_parser::true_sym:  return value::boolean(true);
	case json_parser::false_sym: return value::boolean(false);
	case json_parser::null_sym:  return value::null();
	case json_parser::number: {
		std::string s = v | trv::terminals;
		double d = 0;
		if (!parse_number(s, d)) {
			R.error(code::parse_error, "Invalid JSON number",
				{{label::value, s}});
			return {};
		}
		return value::number(d);
	}
	case json_parser::str: return build_string(v, R);
	case json_parser::arr: {
		value r = value::array();
		if (auto vs = v | json_parser::values; vs.has_value())
			for (auto& e : (vs || json_parser::value)()) {
				r.push_back(build(e, R));
				if (R.has_error()) return r;
			}
		return r;
	}
	case json_parser::object: {
		value r = value::object();
		for (auto& p : (v || json_parser::object_pair)()) {
			value key = build_string(p | json_parser::str, R);
			// a key that failed to unescape is not a string; stop
			// before as_string() reads a null value as one.
			if (R.has_error()) return r;
			r.set(key.as_string(), build(p | json_parser::value, R));
			if (R.has_error()) return r;
		}
		return r;
	}
	default: assert(false); return {};
	}
}

inline result parse(std::string_view s) {
	result R;
	auto& p = json_parser::instance();
	json_parser::parse_options po{ .start = json_parser::start };
	auto pr = p.parse(s.data(), s.size(), po);
	if (!pr.found) {
		if (!pr.report().nodes().empty()) R.append(std::move(pr.report()));
		R.error(code::parse_error, pr.parse_error.to_str());
		return R;
	}
	tref n = pr.get_shaped_tree2();
	if (!pr.report().nodes().empty()) R.append(std::move(pr.report()));
	trv root(n);
	value v = build(root | json_parser::value, R);
	// emplace() on an already-errored R throws: its own return path
	// dereferences the optional after resetting it back to empty.
	if (!R.has_error()) R.emplace(std::move(v));
	return R;
}

inline std::ostream& print(const value& v, std::ostream& os) {
	switch (v.type()) {
	case value::kind::null:    os << "null"; break;
	case value::kind::boolean: os << (v.as_bool() ? "true" : "false"); break;
	case value::kind::number: {
		char buf[32];
		auto r = std::to_chars(buf, buf + sizeof(buf), v.as_number());
		os.write(buf, r.ptr - buf);
		break;
	}
	case value::kind::string: escape(os, v.as_string()); break;
	case value::kind::array: {
		os << '[';
		bool first = true;
		for (auto& e : v) {
			if (!first) os << ',';
			first = false;
			print(e, os);
		}
		os << ']';
		break;
	}
	case value::kind::object: {
		os << '{';
		bool first = true;
		for (auto& kv : v.members()) {
			if (!first) os << ',';
			first = false;
			escape(os, kv.first);
			os << ':';
			print(kv.second, os);
		}
		os << '}';
		break;
	}
	}
	return os;
}

inline value report_node_to_value(const diagnostics::report& r, size_t i,
	const std::vector<std::vector<size_t>>& kids, bool names)
{
	const auto& n = r.nodes()[i];
	auto v = value::object();
	v.set("tag", value::number(static_cast<uint16_t>(n.tag)));
	if (names) v.set("message",
		value::string(diagnostics::code_name(n.tag)));
	v.set("key", value::string(std::string(r.str(n.key))))
	 .set("value", value::number(static_cast<double>(n.value)));
	auto attrs = value::array();
	for (uint8_t a = 0; a < n.attr_cnt; ++a) {
		const auto& at = r.attrs()[n.attr_off + a];
		auto av = value::object();
		av.set("key", value::string(std::string(r.str(at.key))));
		// a text label stores an interned string key in value
		if (parser_strings::is_text_label(at.key))
			av.set("value", value::string(std::string(
				r.str(static_cast<int_t>(at.value)))));
		else av.set("value", value::number(
			static_cast<double>(at.value)));
		attrs.push_back(std::move(av));
	}
	v.set("attrs", std::move(attrs));
	if (!kids[i].empty()) {
		auto ch = value::array();
		for (size_t k : kids[i])
			ch.push_back(report_node_to_value(r, k, kids, names));
		v.set("children", std::move(ch));
	}
	return v;
}

inline value to_value(const diagnostics::report& r, bool names) {
	const auto& nodes = r.nodes();
	std::vector<std::vector<size_t>> kids(nodes.size());
	for (size_t i = 0; i < nodes.size(); ++i)
		if (nodes[i].parent >= 0)
			kids[static_cast<size_t>(nodes[i].parent)].push_back(i);
	auto roots = value::array();
	for (size_t i = 0; i < nodes.size(); ++i)
		if (nodes[i].parent < 0)
			roots.push_back(report_node_to_value(r, i, kids, names));
	auto v = value::object();
	v.set("nodes", std::move(roots));
	return v;
}

inline void report_node_from_value(diagnostics::report& r,
	const value& n)
{
	using diagnostics::attr_in;
	using diagnostics::code;
	code tag = code::unknown;
	if (auto t = n.find("tag"); t && t->is_number())
		tag = static_cast<code>(
			static_cast<uint16_t>(t->as_number()));
	diagnostics::report::key key = 0;
	if (auto k = n.find("key"); k && k->is_string())
		key = r.intern(k->as_string());
	int64_t node_value = 0;
	if (auto v = n.find("value"); v && v->is_number())
		node_value = static_cast<int64_t>(v->as_number());
	std::vector<attr_in> attrs;
	if (auto a = n.find("attrs"); a && a->is_array())
		for (const auto& at : *a) {
			auto ak = at.find("key");
			auto av = at.find("value");
			if (!ak || !ak->is_string()) continue;
			diagnostics::report::key akey =
				r.intern(ak->as_string());
			if (av && av->is_string())
				attrs.emplace_back(akey,
					std::string_view(av->as_string()));
			else if (av && av->is_number())
				attrs.emplace_back(akey,
					static_cast<int_t>(
						av->as_number()));
		}
	r.push_node(tag, key, node_value, attrs);
	if (auto ch = n.find("children"); ch && ch->is_array())
		for (const auto& c : *ch) report_node_from_value(r, c);
	r.pop_node();
}

inline diagnostics::report report_from_value(const value& v) {
	diagnostics::report r;
	auto nodes = v.find("nodes");
	if (!nodes || !nodes->is_array()) return r;
	for (const auto& n : *nodes) report_node_from_value(r, n);
	return r;
}

}

#endif // __IDNI__PARSER__FORMAT__JSON_TMPL_H__
