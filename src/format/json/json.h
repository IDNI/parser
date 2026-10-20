// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_H__
#define __IDNI__PARSER__FORMAT__JSON_H__

#include <compare>
#include <cstdint>
#include <functional>
#include <initializer_list>
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
/// A number keeps the bits of its double in `data`. An integer value
/// below 2^63 in magnitude keeps its decimal text in `str` too, and so
/// does an integer literal outside that range, so a value above 2^53
/// prints exactly. NaN, Infinity and -Infinity of the grammar variant
/// nonfinite are number nodes with no text, and NaN is a quiet NaN. A str
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

/// How tree::print writes a document. A profile fills every field, and a
/// caller can then change single fields. The default is minified(). With
/// sorted_keys and ascii_only, a document of strings and integers prints
/// the bytes of Python json.dumps with sort_keys, ensure_ascii and the
/// separators "," and ":". With floats repr, a float prints the bytes
/// of Python repr too.
struct print_options {
	/// Keys of every object in code point order. UTF-8 byte order is
	/// code point order, so std::string compare sorts them.
	bool sorted_keys = false;
	/// Every non-ASCII character as a `\u` escape. A byte that is not
	/// valid UTF-8 has no code point, so it prints as it is.
	bool ascii_only = false;
	/// Between two items of an array or an object. The two separators
	/// are views, so their text must outlive the print.
	std::string_view item_separator = ",";
	/// Between a key and its value.
	std::string_view key_separator = ":";
	/// 0: one line. n > 0: a new line for each item, n spaces for each
	/// level. An empty array or object stays on one line.
	size_t indent = 0;
	enum class float_format { shortest, repr };
	/// The text of a number node with no decimal text. shortest: the
	/// shortest round trip text of to_chars. repr: float_repr().
	float_format floats = float_format::shortest;
	/// False writes U+007F raw. With ascii_only it is always `\u007f`.
	bool escape_delete = true;
	/// What the printer does with the three bytes of a lone surrogate in
	/// a str (WTF-8, see read_options::lone_surrogates). keep_bytes: the
	/// bytes as they are. escape: `\udXXX`. fail: to_str_checked fails,
	/// and a stream print, which cannot fail, writes the escape.
	enum class lone_surrogate_policy { keep_bytes, escape, fail };
	lone_surrogate_policy lone_surrogate = lone_surrogate_policy::keep_bytes;
	/// On: a NaN or an infinity prints as NaN, Infinity or -Infinity,
	/// beside read_options::nonfinite. Off: to_str_checked fails, and a
	/// stream print writes null, so its text stays JSON.
	bool nonfinite = false;

	/// One line with no spaces: the default fields.
	static print_options minified();
	/// One line with a space after each separator. The bytes of Python
	/// json.dumps(v).
	static print_options normal();
	/// The bytes of Python json.dumps(v, indent=n). Python writes a new
	/// line for each item with indent=0, but pretty(0) gives one line.
	static print_options pretty(size_t n);
};

/// How tree::get reads a JSON text. Each field is off by default, so the
/// default options read RFC 8259 JSON as tree::get(std::string_view) does.
struct read_options {
	/// The grammar variant nonfinite: NaN, Infinity and -Infinity.
	bool nonfinite = false;
	/// A fraction or an exponent gives a float node with no text, also
	/// for an integer value, and -0.0 stays. A text out of the double range
	/// reads as an infinity, or as a zero with the sign of the text. An
	/// integer text keeps its digits, and its double is an infinity when
	/// it is out of range.
	bool keep_float_form = false;
	/// A lone surrogate escape gives the WTF-8 bytes of its code point in
	/// the str, as escapes::json_wtf8 decodes it. Such a str is no valid
	/// UTF-8. Raw input bytes of a surrogate stay invalid UTF-8.
	bool lone_surrogates = false;
	/// 0 is no limit. Else an integer text with more digits is an
	/// out_of_range error. Its attrs label::size and label::digits both
	/// give the digit count, which does not count the sign.
	size_t max_int_digits = 0;
	/// 0 is no limit. Else a deeper nest is an out_of_range error with the
	/// attr label::depth, the depth of the nest. Each open array and each
	/// open object is one level.
	size_t max_depth = 0;
	/// Every rule of these options, with the limits of the caller.
	static read_options lenient(size_t max_int_digits, size_t max_depth);
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
	/// Run the gc when the cache holds more than twice the nodes that
	/// the last gc kept, plus 4096. True when the gc ran. Call it from
	/// the one thread that runs the gc, outside every gc_pause.
	static bool gc_if_grown();

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
	/// The exact integer of a number node, from its text. No value for a
	/// number that is not an integer or that is outside int64.
	std::optional<int64_t> get_integer() const;
	/// The canonical decimal text of an integer number node of any size.
	/// No value for a float node, also for one with a whole value.
	std::optional<std::string_view> get_integer_text() const;
	/// The text of a str node, or the key of an object_pair node.
	const std::string& get_string() const;
	/// The value of an object_pair node.
	const tree& value_tree() const;
	/// The value of the member @p key of an object, or nullptr when the
	/// object has no such member.
	tref find(std::string_view key) const;

	// builders

	static tref _null();
	static tref _true();
	static tref _false();
	static tref build_bool(bool b);
	/// -0 becomes 0, so both are one node and print as 0. An infinity
	/// or a NaN becomes null.
	/// An integral @p n with |n| < 2^63 keeps its decimal text.
	static tref build_number(double n);
	static tref build_integer(int64_t n);
	/// A float node of @p d with no text, so -0.0 and an integer value
	/// stay floats. Every NaN gives the one quiet NaN node.
	static tref build_float(double d);
	/// A number node with the decimal text of an integer of any size.
	/// The text is canonical: an optional '-', then 0 or digits with no
	/// leading zero. -0 is no canonical text. The double of the node is an
	/// infinity when the integer is out of range, as a lenient read gives.
	static diagnostics::result<tref> build_integer(std::string_view decimal);
	static tref build_string(std::string_view s);
	static tref build_array(const trefs& elements);
	static tref build_pair(std::string_view key, tref v);
	/// An object of the object_pair nodes @p pairs. A repeated key keeps
	/// its first position and its last value.
	static tref build_object(const trefs& pairs);
	/// A copy of @p object with the member @p key set to @p v.
	static tref set_member(tref object, std::string_view key, tref v);
	/// A diagnostics report as a nested JSON object. A node with parent -1
	/// is a root. children is absent when a node has none. With @p names
	/// each node carries a "message" field with its code name.
	static tref build_report(const diagnostics::report& r,
		bool names = true);
	/// The report node @p i with its children as a JSON object. @p kids[i]
	/// lists the children of node i, so the parent index never reaches
	/// the JSON.
	static tref build_report_node(const diagnostics::report& r, size_t i,
		const std::vector<std::vector<size_t>>& kids, bool names = true);

	// from parser

	/// The JSON tree of a shaped parse tree whose root is a start node, a
	/// value node or the node of one JSON value, such as a str.
	static diagnostics::result<tref> get(const parse_tree& t,
		const read_options& o = {});
	static diagnostics::result<tref> get(json_parser::result& r,
		const read_options& o = {});
	/// Parse a JSON text. On a syntax error or a bad escape the result
	/// carries no value.
	static diagnostics::result<tref> get(std::string_view text);
	/// Parse a JSON text with the options @p o. With nonfinite the
	/// parser of detail::nonfinite_grammar() reads the text.
	static diagnostics::result<tref> get(std::string_view text,
		const read_options& o);

	// printers

	/// The JSON text on one line, without recursion.
	std::ostream& print(std::ostream& os) const;
	std::string to_str() const;
	std::ostream& print(std::ostream& os, const print_options& o) const;
	std::string to_str(const print_options& o) const;
	/// to_str, or an error for a text that @p o cannot write.
	diagnostics::result<std::string> to_str_checked(
		const print_options& o) const;
	/// The node structure, one node per line.
	std::ostream& print_tree(std::ostream& os, size_t s = 0) const;
	std::ostream& dump(std::ostream& os, bool subtree = true) const;
	std::string dump_to_str(bool subtree = true) const;

	/// Tree navigation: `| nt` gives the first child with the nonterminal
	/// nt, `|| nt` all of them, `| extractor` a value.
	struct traverser {
		using type = node::type;

		template <typename result_type>
		struct extractor {
			using function = std::function<
				result_type(const traverser&)>;
			extractor(const function& e) : e(e) {}
			result_type operator()(const traverser& t) const
				{ return e(t); }
		private:
			function e;
		};

		traverser();
		traverser(tref r);
		traverser(const tree& r);
		traverser(const htref& r);
		traverser(const trefs& n);

		bool has_value() const;
		explicit operator bool() const;
		tref value() const;
		const tree& value_tree() const;
		const tree& operator[](size_t n) const;
		const trefs& values() const;
		std::vector<traverser> traversers() const;
		std::vector<traverser> operator()() const;
		bool is(size_t nt) const;
		bool empty() const;
		size_t size() const;

		static const extractor<tref>             ref;
		static const extractor<trefs>            refs;
		static const extractor<htref>            handle;
		/// The traverser must hold a value.
		static const extractor<const tree&>      Tree;
		static const extractor<traverser>        dump;
		static const extractor<type>             nt;
		static const extractor<std::string>      string;
		static const extractor<double>           number;
		static const extractor<bool>             boolean;
		static const extractor<traverser>        only_child;
		static const extractor<traverser>        first;
		static const extractor<traverser>        second;
		static const extractor<traverser>        children;
		static const extractor<tref_range<node>> children_range;
		static const extractor<tree_range<tree>> children_trees_range;
		/// The value of the member @p key of an object; no value when
		/// the object has no such member.
		static extractor<traverser> member(std::string_view key);
		/// @p fn applied to each value; a nullptr result is dropped.
		static extractor<traverser> f(const auto& fn);

		traverser operator|(size_t nt) const;
		traverser operator||(size_t nt) const;
		template <typename result_type>
		result_type operator|(const extractor<result_type>& e) const;
		template <typename result_type>
		result_type operator||(const extractor<result_type>& e) const;
	private:
		void set_values(const trefs& n);
		bool has_value_ = true;
		trefs values_{};
	};

	traverser operator()() const;
private:
	// The print of the stream and of the checked text. A null @p rep
	// takes the fallback of each check.
	std::ostream& print_into(std::ostream& os, const print_options& o,
		diagnostics::report* rep) const;
};

using tt = tree::traverser;

namespace detail {

/// The json.tgf grammar with the variant nonfinite: NaN, Infinity and
/// -Infinity are values. It is a grammar object of its own, so the grammar
/// of json_parser::instance() stays on the variant standard.
grammar<char, char32_t>& nonfinite_grammar();
/// The parser of nonfinite_grammar().
parser<char, char32_t>& nonfinite_parser();

} // namespace detail

// free queries

bool is(tref n, size_t nt);
std::function<bool(tref)> is(size_t nt);
bool is(tref n, std::initializer_list<size_t> nts);
std::function<bool(tref)> is(std::initializer_list<size_t> nts);

/// The string member @p key of @p object, or nullptr when the member is
/// absent or holds no string.
const std::string* string_member(tref object, std::string_view key);

/// Rebuild a diagnostics report from its JSON form, the reverse of
/// @ref tree::build_report. The message field is ignored: the tag carries
/// the code. A missing tag reads as @ref diagnostics::code::unknown.
diagnostics::report report_of(tref v);

/// Append the report node of the JSON object @p n, with its children,
/// under the current parent of @p r. A missing tag reads as
/// @ref diagnostics::code::unknown. The message field is ignored.
void report_node_of(diagnostics::report& r, tref n);

std::ostream& operator<<(std::ostream& os, const node& n);
/// The JSON text of @p t.
std::ostream& operator<<(std::ostream& os, const tree& t);
std::ostream& operator<<(std::ostream& os, const tt& t);

/// Write @p v as one line: the JSON text, one '\n', then a flush. On
/// Windows std::cout goes to binary mode first, so no '\r' joins the '\n'.
std::ostream& write_line(tref v, std::ostream& os);

/// Write a diagnostics report as JSON on one line. With @p print_names
/// each node carries a "message" field with its code name.
std::ostream& print(const diagnostics::report& r, std::ostream& os,
	bool print_names = false);

/// The JSON value of @p v: a bool, an unsigned integer, a std::string, or
/// a std::vector or a std::variant of these types.
template <typename T> value to_json(const T& v);
/// The value of @p v as a T, or nothing when the JSON has another shape.
template <typename T> std::optional<T> from_json(const value& v);

value build_string(const trv& t, result& R);

/// Read the JSON number @p s. A value below the smallest double reads as 0.
/// A value above the largest double is an error.
bool parse_number(const std::string& s, double& d);

/// Read the JSON number @p s. A value below the smallest double reads as
/// a zero with the sign of @p s, and a value above the largest double as
/// an infinity with that sign.
double parse_number_saturating(const std::string& s);

/// The text of Python repr(float) for @p d: the shortest digits that read
/// back to @p d, in plain notation for a decimal exponent from -4 to 15,
/// else in exponent notation with a sign and at least two digits. A
/// float has a point or an exponent: 1.0, 1e+16. NaN and an infinity
/// give nan, inf and -inf.
std::string float_repr(double d);

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

}

#include "json.tmpl.h"
#include "json_tree.tmpl.h"
#include "json_tree_builders.tmpl.h"
#include "json_tree_from_parser.tmpl.h"
#include "json_tree_printers.tmpl.h"

#endif // __IDNI__PARSER__FORMAT__JSON_H__
