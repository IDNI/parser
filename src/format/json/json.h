// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_H__
#define __IDNI__PARSER__FORMAT__JSON_H__

#include <compare>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../../utility/escapes.h"
#include "../../utility/diagnostics.h"
#include "../../utility/tree.h"
#include "../../parser_strings.h"
#include "json_parser.generated.h"

namespace idni::format::json {

void escape(std::ostream& os, std::string_view s);

idni::escapes::decoded unescape(std::string_view body);

/// A parsed or hand-built JSON value: one of null, bool, number, string,
/// array or object. Objects keep insertion order and last-write-wins on a
/// repeated key, matching how @ref set behaves.
struct value {
	enum class kind { null, boolean, number, string, array, object };

	value() = default;

	kind type() const;
	bool is_null()   const;
	bool is_bool()   const;
	bool is_number() const;
	bool is_string() const;
	bool is_array()  const;
	bool is_object() const;

	bool as_bool() const;
	double as_number() const;
	const std::string& as_string() const;

	/// Number of elements (array) or members (object).
	size_t size() const;
	const value& operator[](size_t i) const;
	std::vector<value>::const_iterator begin() const;
	std::vector<value>::const_iterator end()   const;

	/// Object members in insertion order.
	const std::vector<std::pair<std::string, value>>& members() const;
	/// Looks up an object member by key. Returns nullptr, not a crash
	/// or an exception, when the key is absent.
	const value* find(std::string_view key) const;

	static value null();
	static value boolean(bool b);
	static value number(double n);
	static value string(std::string s);
	static value array();
	static value object();

	/// Appends to an array value. Returns *this for chaining.
	value& push_back(value v);
	/// Sets an object member, overwriting an existing one with the same
	/// key. Returns *this for chaining.
	value& set(std::string key, value v);
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

using parse_tree = json_parser::tree;
using trv  = parse_tree::traverser;

/// A node of a JSON document tree. The kind is a json_parser nonterminal:
/// null_sym, true_sym, false_sym, number, str, arr, object or object_pair.
/// A number keeps the bits of its double in `data`. A str
/// keeps its decoded text in `str`. An object_pair keeps its
/// key in `str` and has one child, the value. An arr has its elements as children, an
/// object its object_pair nodes, both in order.
struct node {
	using type = json_parser::nonterminal;

	const type nt;
	const uint64_t data;
	const std::string str;
	const uint64_t hash;

	explicit node(type nt = json_parser::null_sym, uint64_t data = 0,
		std::string str = {});

	/// The name of the nonterminal @p nt.
	static const std::string& name(size_t nt);
	const std::string& name() const;
	size_t get_nt() const;
	uint64_t hashit() const;

	/// By hash, then nt, data and str.
	std::weak_ordering operator<=>(const node& that) const;
	bool operator==(const node& that) const;
};

/// A JSON document as an interned lcrs tree. A tref of it is valid until
/// a gc of bintree<node> drops it, so a document that outlives the step
/// that built it is held by an htref.
struct tree : public lcrs_tree<node> {
	using node_type = node;
	using base_t    = lcrs_tree<node>;
	using type      = node::type;

	// direct tree API

	tref get() const;
	static const tree& get(const tref id);
	static const tree& get(const htref& h);
	static htref geth(tref id);
	/// Number of interned nodes.
	static size_t m_size();

	static tref get(const node& v);
	static tref get(const node& v, tref ch);
	static tref get(const node& v, tref ch1, tref ch2);
	static tref get(const node& v, const trefs& ch, tref r = nullptr);
	static tref get(type nt);
	static tref get(type nt, const trefs& ch, tref r = nullptr);

	// children

	tref_range<node> children() const;
	tree_range<tree> children_trees() const;
	const tree& operator[](size_t n) const;
	const tree& child_tree(size_t n) const;
	const tree& first_tree() const;
	const tree& second_tree() const;
	const tree& only_child_tree() const;

	// fast access helpers

	bool is(size_t nt) const;
	bool is_null() const;
	bool is_bool() const;
	bool is_number() const;
	bool is_string() const;
	bool is_array() const;
	bool is_object() const;
	type get_type() const;
	bool get_bool() const;
	double get_number() const;
	/// The text of a str node, or the key of an object_pair node.
	const std::string& get_string() const;
	/// The value of an object_pair node.
	const tree& value_tree() const;
	/// The value of the member @p key of an object, or nullptr when the
	/// object has no such member.
	tref find(std::string_view key) const;
};

/// The JSON value of @p v: a bool, an unsigned integer, a std::string, or
/// a std::vector or a std::variant of these types.
template <typename T> value to_json(const T& v);
/// The value of @p v as a T, or nothing when the JSON has another shape.
template <typename T> std::optional<T> from_json(const value& v);

value build_string(const trv& t, result& R);

bool parse_number(const std::string& s, double& d);

/// @p t is a "value" node; it always has exactly one child, the actual
/// variant (true_sym/false_sym/null_sym/number/str/arr/object).
value build(const trv& t, result& R);

/// Parse a JSON text into a @ref value. On a syntax error, or an escape
/// (e.g. an unpaired \u surrogate) the grammar cannot catch, the result
/// carries no value and @ref result::has_error is true.
result parse(std::string_view s);

/// Serialize a @ref value as JSON, so a caller can build an arbitrary
/// object or array (e.g. {"id":1,"result":"...","report":{...}}) without
/// hand-writing braces.
std::ostream& print(const value& v, std::ostream& os);

/// Build one nested report node. @p kids[i] lists the children of node i,
/// so the parent index never reaches the JSON.
value report_node_to_value(const diagnostics::report& r, size_t i,
	const std::vector<std::vector<size_t>>& kids, bool names);

/// Convert a diagnostics report into a nested JSON tree. A node with
/// parent -1 is a root; children is absent when a node has none. With
/// @p names = true each node carries a "message" field with its code name.
value to_value(const diagnostics::report& r, bool names = true);

/// Rebuild one nested report node from JSON and append it under the
/// current parent of @p r. The children recurse, so the tree nests again.
/// A missing tag reads as @ref diagnostics::code::unknown; the message
/// field is ignored because the tag carries the code.
void report_node_from_value(diagnostics::report& r, const value& n);

/// Rebuild a diagnostics report from its nested JSON form, the reverse of
/// @ref to_value. The @c message field is ignored: the tag carries the
/// code.
diagnostics::report report_from_value(const value& v);

/// Serialize a diagnostics report as JSON on one line. With @p print_names
/// = true, each node carries an extra "message" field with its code name.
std::ostream& print(const diagnostics::report& r, std::ostream& os,
	bool print_names = false);

}

#include "json.tmpl.h"
#include "json_tree.tmpl.h"

#endif // __IDNI__PARSER__FORMAT__JSON_H__
