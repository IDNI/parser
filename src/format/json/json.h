// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_H__
#define __IDNI__PARSER__FORMAT__JSON_H__

#include <cassert>
#include <charconv>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "../../utility/escapes.h"
#include "../../utility/diagnostics.h"
#include "json_parser.generated.h"

namespace idni::format::json {

inline void escape(std::ostream& os, std::string_view s) {
	os << '"' << idni::escapes::encode(s, idni::escapes::json) << '"';
}

inline idni::escapes::decoded unescape(std::string_view body) {
	return idni::escapes::decode(body, idni::escapes::json);
}

/// A parsed or hand-built JSON value: one of null, bool, number, string,
/// array or object. Objects keep insertion order and last-write-wins on a
/// repeated key, matching how @ref set behaves.
struct value {
	enum class kind { null, boolean, number, string, array, object };

	value() = default;

	kind type() const { return kind_; }
	bool is_null()   const { return kind_ == kind::null; }
	bool is_bool()   const { return kind_ == kind::boolean; }
	bool is_number() const { return kind_ == kind::number; }
	bool is_string() const { return kind_ == kind::string; }
	bool is_array()  const { return kind_ == kind::array; }
	bool is_object() const { return kind_ == kind::object; }

	bool as_bool() const { assert(is_bool()); return bool_; }
	double as_number() const { assert(is_number()); return num_; }
	const std::string& as_string() const { assert(is_string()); return str_; }

	/// Number of elements (array) or members (object).
	size_t size() const {
		return kind_ == kind::object ? obj_.size() : arr_.size();
	}
	const value& operator[](size_t i) const { assert(is_array()); return arr_[i]; }
	std::vector<value>::const_iterator begin() const { return arr_.begin(); }
	std::vector<value>::const_iterator end()   const { return arr_.end(); }

	/// Object members in insertion order.
	const std::vector<std::pair<std::string, value>>& members() const {
		return obj_;
	}
	/// Looks up an object member by key. Returns nullptr, not a crash
	/// or an exception, when the key is absent.
	const value* find(std::string_view key) const {
		for (auto& kv : obj_) if (kv.first == key) return &kv.second;
		return nullptr;
	}

	static value null()          { return value(); }
	static value boolean(bool b) { value v; v.kind_ = kind::boolean; v.bool_ = b; return v; }
	static value number(double n){ value v; v.kind_ = kind::number; v.num_ = n; return v; }
	static value string(std::string s) {
		value v; v.kind_ = kind::string; v.str_ = std::move(s); return v;
	}
	static value array()  { value v; v.kind_ = kind::array;  return v; }
	static value object() { value v; v.kind_ = kind::object; return v; }

	/// Appends to an array value. Returns *this for chaining.
	value& push_back(value v) {
		assert(is_array());
		arr_.push_back(std::move(v));
		return *this;
	}
	/// Sets an object member, overwriting an existing one with the same
	/// key. Returns *this for chaining.
	value& set(std::string key, value v) {
		assert(is_object());
		for (auto& kv : obj_)
			if (kv.first == key) { kv.second = std::move(v); return *this; }
		obj_.emplace_back(std::move(key), std::move(v));
		return *this;
	}
private:
	kind kind_ = kind::null;
	bool bool_ = false;
	double num_ = 0;
	std::string str_{};
	std::vector<value> arr_{};
	std::vector<std::pair<std::string, value>> obj_{};
};

using code   = idni::diagnostics::code;
using result = idni::diagnostics::result<value>;

namespace detail {

using tree = json_parser::tree;
using trv  = tree::traverser;

inline value build(const trv& t, result& R);

inline value build_string(const trv& t, result& R) {
	std::string body = t | trv::terminals;
	auto dec = unescape(body);
	if (dec.has_error()) { R.append(std::move(dec).report()); return {}; }
	return value::string(std::move(dec).value());
}

/// @p t is a "value" node; it always has exactly one child, the actual
/// variant (true_sym/false_sym/null_sym/number/str/arr/object).
inline value build(const trv& t, result& R) {
	auto v = t | trv::only_child;
	switch (v | trv::nonterminal) {
	case json_parser::true_sym:  return value::boolean(true);
	case json_parser::false_sym: return value::boolean(false);
	case json_parser::null_sym:  return value::null();
	case json_parser::number: {
		std::string s = v | trv::terminals;
		double d = 0;
		// from_chars, not strtod: output already uses to_chars, so
		// neither direction depends on the current locale.
		auto fc = std::from_chars(s.data(), s.data() + s.size(), d);
		if (fc.ec != std::errc{} || fc.ptr != s.data() + s.size()) {
			R.error(code::parse_error, "invalid JSON number: " + s);
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

} // namespace detail

/// Parse a JSON text into a @ref value. On a syntax error, or an escape
/// (e.g. an unpaired \u surrogate) the grammar cannot catch, the result
/// carries no value and @ref result::has_error is true.
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
	detail::trv root(n);
	value v = detail::build(root | json_parser::value, R);
	// emplace() on an already-errored R throws: its own return path
	// dereferences the optional after resetting it back to empty.
	if (!R.has_error()) R.emplace(std::move(v));
	return R;
}

/// Serialize a @ref value as JSON, so a caller can build an arbitrary
/// object or array (e.g. {"id":1,"result":"...","report":{...}}) without
/// hand-writing braces.
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
