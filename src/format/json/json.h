// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_H__
#define __IDNI__PARSER__FORMAT__JSON_H__

#include <cassert>
#include <charconv>
#include <ostream>
#include <version>
#if !defined(__cpp_lib_to_chars) || __cpp_lib_to_chars < 201611L
#include <locale>
#include <sstream>
#endif
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "../../utility/escapes.h"
#include "../../utility/diagnostics.h"
#include "../../parser_strings.h"
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
using label  = idni::parser_strings::label;

using tree = json_parser::tree;
using trv  = tree::traverser;

inline value build(const trv& t, result& R);

inline value build_string(const trv& t, result& R) {
	std::string body = t | trv::terminals;
	auto dec = unescape(body);
	if (dec.has_error()) { R.append(std::move(dec).report()); return {}; }
	return value::string(std::move(dec).value());
}

// strtod would read the decimal point of the current locale, and the write
// side uses to_chars, so both directions must ignore it. libc++ ships
// to_chars for a double but not from_chars, hence the stream fallback.
inline bool parse_number(const std::string& s, double& d) {
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
	auto fc = std::from_chars(s.data(), s.data() + s.size(), d);
	return fc.ec == std::errc{} && fc.ptr == s.data() + s.size();
#else
	std::istringstream is(s);
	is.imbue(std::locale::classic());
	// from_chars refuses leading whitespace, and this path must agree
	is >> std::noskipws >> d;
	return !is.fail() && is.peek() == std::char_traits<char>::eof();
#endif
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
	trv root(n);
	value v = build(root | json_parser::value, R);
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

/// Build one nested report node. @p kids[i] lists the children of node i,
/// so the parent index never reaches the JSON.
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

/// Convert a diagnostics report into a nested JSON tree. A node with
/// parent -1 is a root; children is absent when a node has none. With
/// @p names = true each node carries a "message" field with its code name.
inline value to_value(const diagnostics::report& r, bool names = true) {
	const auto& nodes = r.nodes();
	std::vector<std::vector<size_t>> kids(nodes.size());
	for (size_t i = 0; i < nodes.size(); ++i)
		if (nodes[i].parent >= 0)
			kids[nodes[i].parent].push_back(i);
	auto roots = value::array();
	for (size_t i = 0; i < nodes.size(); ++i)
		if (nodes[i].parent < 0)
			roots.push_back(report_node_to_value(r, i, kids, names));
	auto v = value::object();
	v.set("nodes", std::move(roots));
	return v;
}

/// Serialize a diagnostics report as JSON on one line. With @p print_names
/// = true, each node carries an extra "message" field with its code name.
inline std::ostream& print(const diagnostics::report& r, std::ostream& os,
	bool print_names = false)
{
	return print(to_value(r, print_names), os);
}

}

#endif // __IDNI__PARSER__FORMAT__JSON_H__
