// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__DIAGNOSTICS_H__
#define __IDNI__PARSER__UTILITY__DIAGNOSTICS_H__

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "defs.h"
#include "measure.h"
#include "term_colors.h"

namespace idni::diagnostics {

/// Discriminated tag for a @ref node. A node's role *and* (for metrics) its
/// unit are determined by which tag value it carries.
///
/// Layout: top two bits of @c uint16_t (mask @c 0xC000) encode severity.
///   00  0x0000..0x3FFF  errors
///   01  0x4000..0x7FFF  warnings
///   10  0x8000..0xBFFF  info
///   11  0xC000..0xFFFF  unused
/// Metric tags (@ref info_micros, @ref info_kb, @ref info_count) sit at the
/// start of the info band; @ref is_metric tells them apart from a plain
/// @ref info.
///
/// Timed scopes use @ref info_micros — a scope is just an @c info_micros node that
/// happens to have children attached. The structural role is determined by
/// the parent/child graph, not the tag.
enum class code : uint16_t {
	unknown                  = 0,
	invalid_argument         = 1,
	internal_error           = 2,
	io_error                 = 3,
	type_error               = 4,
	unsupported_operation    = 5,
	invalid_state            = 6,
	not_found                = 7,
	missing_type_information = 8,
	invalid_input_stream     = 9,
	invalid_output_stream    = 10,
	invalid_memory_access    = 11,
	out_of_range             = 12,
	unsat                    = 13,
	solver_error             = 14,
	parse_error              = 15,
	runtime_error            = 16,
	unknown_char_class       = 17,

	// 01 band — warnings
	warning                  = 0x4000,

	// 10 band — info message plus metric unit tags
	info                     = 0x8000, // unit-less informational message
	info_micros              = 0x8001, // time in microseconds (auto-scales
	                                // to µs / ms / s on print);
	                                // also the tag used by timed scopes
	info_kb                  = 0x8002, // memory, kilobytes
	info_count               = 0x8003, // counter, dimensionless
	// Adding a new metric tag: extend is_metric() (whitelist, not
	// range check) and PARSER_CODE_LABELS in parser_strings.h.
};

constexpr bool is_error(code c);
constexpr bool is_warning(code c);
constexpr bool is_info(code c);
constexpr bool is_metric(code c);
constexpr bool uses_micros_scale(code c);

/// Human-readable sentence-form label for @p c (e.g. "Parse error.",
/// "Unknown character class."). Capitalized first word, trailing
/// period. Strings live in @ref idni::parser_strings::dict_strings and
/// are the form emitted by @ref report::to_json as the `tag_name`
/// field. Returns "Unknown." for any value outside the @ref code enum.
const char* code_name(code c);

/// Single output slot for one severity band. Accepts either a stream
/// pointer (the common case) or any callable taking a string_view
/// (for adapters such as boost::log macros wrapped in a lambda).
/// A default-constructed sink is empty and silently drops its lines.
struct sink {
	sink() = default;
	sink(std::ostream* os);
	template <typename F,
		typename = std::enable_if_t<
			!std::is_convertible_v<F&&, std::ostream*>>>
	sink(F&& f) : write_(std::forward<F>(f)) {}
	explicit operator bool() const noexcept { return bool(write_); }
	void operator()(std::string_view line) const;
private:
	std::function<void(std::string_view)> write_;
};

/// Three sinks, one per severity band. Default-constructed sinks
/// are no-ops, so partial literals such as `sinks{.error = &cerr}`
/// are fine.
struct sinks {
	sink error;     // indent 0, one call per error node
	sink warning;   // indent 0, one call per warning node
	sink info;      // tab-indented per tree depth; info+metric+scope
};

struct attr {
	int_t   key = 0;
	int64_t value = 0;
};

struct node {
	code      tag      = code::unknown;
	int_t     key      = 0;
	int32_t   parent   = -1;
	int64_t   value    = 0;
	uint32_t  attr_off = 0;
	uint8_t   attr_cnt = 0;
};

struct report {
	using key = idni::int_t;
	static constexpr key none = 0;

	/// RAII handle for an open scope; dtor pops the node.
	/// @ref code::info_micros records elapsed µs; move-only, destroy LIFO.
	struct scope_guard {
		scope_guard() = default;  // disabled / no-op

		scope_guard(report& r, key name,
			code tag = code::info_micros,
			int64_t initial_value = 0);
		scope_guard(scope_guard&& o) noexcept;
		scope_guard(const scope_guard&)            = delete;
		scope_guard& operator=(const scope_guard&) = delete;
		// Move-assign would need to close the LHS scope first, but pop_scope
		// is LIFO - the LHS is rarely the stack top, so this is unsafe to add
		// without a stricter contract. Move-construct alone covers our uses.
		scope_guard& operator=(scope_guard&&)      = delete;
		~scope_guard();
	private:
		report*                r_     = nullptr;
		int32_t                idx_   = -1;
		bool                   timed_ = false;
		measures::steady_timer timer_;
	};

	/// Open a parent scope. Tag-driven: @ref code::info_micros auto-times via
	/// RAII; other tags push the @p value untouched. Returns a
	/// @ref scope_guard that pops on destruction.
	[[nodiscard]] scope_guard open(key name,
		code tag = code::info_micros, int64_t value = 0);
	[[nodiscard]] scope_guard open(std::string_view name,
		code tag = code::info_micros, int64_t value = 0);

	/// Like @ref open(key) but conditional. Disabled returns a no-op guard.
	[[nodiscard]] scope_guard open_if(bool enable, key name,
		code tag = code::info_micros, int64_t value = 0);
	[[nodiscard]] scope_guard open_if(bool enable,
		std::string_view name,
		code tag = code::info_micros, int64_t value = 0);

	template <typename F>
	auto step(bool enable, key name, F&& f) -> decltype(f());
	template <typename F>
	auto step(bool enable, std::string_view name, F&& f) -> decltype(f());

	/// Drop nodes and attrs and push a fresh root. Keeps interned
	/// dynamic strings so re-interning the same name across resets is
	/// cheap; use @ref clear to drop them too.
	void reset(std::string_view root_name = "root");
	void reset(key root);
	/// Like @ref reset but also drops the dynamic-string table.
	void clear();

	key intern(const char* s);
	key intern(std::string_view s);
	key intern_dynamic(std::string_view s);
	std::string_view str(key id) const;

	void count(key name, int_t v);
	void count(key name, int64_t v);
	void count(key name, size_t v);
	void count(std::string_view name, int_t v);
	void count(std::string_view name, int64_t v);
	void count(std::string_view name, size_t v);

	void kb(key name, int64_t kilobytes);
	void kb(std::string_view name, int64_t kilobytes);

	void error(code c, key msg, int64_t primary = 0,
		   std::initializer_list<attr> extra = {});
	void error(code c, std::string_view msg, int64_t primary = 0,
		   std::initializer_list<attr> extra = {});
	void error(code c, key msg, std::initializer_list<attr> extra);
	void error(code c, std::string_view msg,
		   std::initializer_list<attr> extra);

	void warning(key msg, int64_t primary = 0,
		     std::initializer_list<attr> extra = {});
	void warning(std::string_view msg, int64_t primary = 0,
		     std::initializer_list<attr> extra = {});
	void warning(key msg, std::initializer_list<attr> extra);
	void warning(std::string_view msg, std::initializer_list<attr> extra);

	void info(key msg, int64_t primary = 0,
		  std::initializer_list<attr> extra = {});
	void info(std::string_view msg, int64_t primary = 0,
		  std::initializer_list<attr> extra = {});
	void info(key msg, std::initializer_list<attr> extra);
	void info(std::string_view msg, std::initializer_list<attr> extra);

	void append(const report& other);
	void append(report&& other);

	[[nodiscard]] bool has_error() const;

	std::string format_message(size_t node_idx) const;

	/// Single pre-order walk over the tree. Errors and warnings emit
	/// flat at indent 0; info-band nodes (info, info_micros,
	/// info_count, info_kb) emit tab-indented per tree depth. Each
	/// line goes to the corresponding sink; empty sinks drop their
	/// lines silently.
	void print(const sinks&) const;
	/// Route all bands to @p os.
	void print(std::ostream& os) const;
	/// Route all bands to @p s.
	void print(const sink& s) const;
	/// Default routing: errors/warnings → std::cerr, info → std::cout.
	void print() const;
	/// Serialize the report tree as JSON. With @p print_names = true,
	/// each node carries an extra "tag_name" field with the symbolic
	/// @ref code name (e.g. "parse_error") alongside the numeric tag.
	std::ostream& to_json(std::ostream& os,
		bool print_names = false) const;

	[[nodiscard]] const std::vector<node>& nodes() const;

	/// Write @p v as the value of the node at @p idx (no-op if @p idx is
	/// out of range). Used by @ref scope_guard to record elapsed time on
	/// close.
	void set_node_value(int32_t idx, int64_t v);

private:
	std::vector<node> nodes_;
	std::vector<attr> attrs_;
	std::vector<std::string> dyn_strings_;
	// Side index for intern_dynamic: maps string content → its negative
	// key. Keeps interning O(1); str(key) still indexes into dyn_strings_.
	std::unordered_map<std::string, int_t> dyn_strings_index_;
	std::vector<int32_t> scope_stack_;
	// Set by push_tagged when a node with an error tag is added. Lets
	// has_error() return in O(1). Reset by clear() / reset() / append().
	bool has_error_ = false;

	int32_t current_parent() const;

	/// Push a node of @p tag and make it the current parent. The default
	/// @c info_micros tag (timed) suits @ref scope_guard; @ref parent_guard
	/// uses an explicit tag for non-timed parenting.
	int32_t push_scope(key name,
		code tag = code::info_micros,
		int64_t initial_value = 0);

	/// Drop the top of @c scope_stack_ if it matches @p idx. Pure stack
	/// pop — value updates go through @ref set_node_value.
	void pop_scope(int32_t idx);

	int32_t push_tagged(code tag, key name, int64_t v,
		std::initializer_list<attr> extra = {});

	// Shared body for append(const&) and append(&&). The rvalue overload
	// passes an rvalue reference so this template can move dynamic
	// strings instead of copying them.
	template <typename Other>
	void append_impl(Other&& other);

	std::vector<std::vector<size_t>> build_children() const;

	// idx == SIZE_MAX (the default) folds across every root node — used
	// by print() entry. An explicit idx restricts the walk to that
	// subtree (used by the recursion).
	size_t max_label_width(
		const std::vector<std::vector<size_t>>& children,
		size_t lvl = 0, size_t idx = SIZE_MAX) const;

	struct value_parts {
		std::string num;
		std::string unit;
	};

	// Number and unit split for column alignment when printing.
	static value_parts format_value_parts(int64_t v, code c);

	// Format a node value with its unit suffix; time auto-scales to
	// µs/ms/s and memory (stored as KB) to kb/mb/gb/tb.
	static std::string format_value(int64_t v, code c);

	static int64_t checked_i64(size_t v);

	struct value_columns {
		size_t num_w  = 0;
		size_t unit_w = 0;
	};

	value_columns value_column_widths() const;

	mutable size_t info_row_ = 0;

	void print_node(const sinks& s,
		const std::vector<std::vector<size_t>>& children, size_t idx,
		size_t level, size_t label_w, value_columns cols) const;

	std::string color_info_line(const node& n, size_t level,
		size_t label_w, value_columns cols) const;
};

std::ostream& operator<<(std::ostream& os, const report& r);

/// A result mimics std::expected and additionally carries a unified report
/// (diagnostic messages and metrics) along with the value.
template <typename T>
struct result {
	using value_type        = T;
	using diagnostics_report = struct report;
	using scope_guard       = diagnostics_report::scope_guard;

	template <typename U> friend struct result;

	result() = default;

	explicit result(std::string_view name);

	result(result&& other) noexcept;
	result& operator=(result&& other) noexcept;

	result(const result&)		 = delete;
	result& operator=(const result&) = delete;

	explicit result(T value)
		requires(!std::is_same_v<std::decay_t<T>, std::string>);
	explicit result(diagnostics_report rep);

	~result();

	[[nodiscard]] bool has_value() const;

	explicit operator bool() const;

	operator T() const
		requires std::is_pointer_v<T>;

	bool operator==(std::nullptr_t) const
		requires std::is_pointer_v<T>;
	bool operator!=(std::nullptr_t) const
		requires std::is_pointer_v<T>;

	T&	 value() &;
	const T& value() const&;
	T&&	 value() &&;

	T& operator*() &
		requires(!std::is_pointer_v<T>);
	const T& operator*() const&
		requires(!std::is_pointer_v<T>);
	T&& operator*() &&
		requires(!std::is_pointer_v<T>);

	T* operator->()
		requires(!std::is_pointer_v<T>);
	const T* operator->() const
		requires(!std::is_pointer_v<T>);

	/// Set the success value in place. If the report already holds an
	/// error, the new value is silently dropped (see
	/// @ref enforce_error_no_value_invariant); a DBG-build assertion
	/// catches the misuse.
	T& emplace(T&& v);
	template <typename... Args>
	T& emplace(Args&&... args);

	/// Same drop-on-error invariant as @ref emplace.
	template <typename U = T>
	result& operator=(U&& v);

	void clear_value();

	[[nodiscard]] diagnostics_report&      report() &;
	[[nodiscard]] const diagnostics_report& report() const&;
	[[nodiscard]] diagnostics_report&&     report() &&;

	/// Semantic @p code applies to errors only; warnings and infos are
	/// message-only (severity is @ref node_kind).
	/// NOTE: an error on a result that already holds a value will discard
	/// that value (see @ref enforce_error_no_value_invariant).
	void error(code c, std::string_view msg, int64_t primary = 0,
		   std::initializer_list<attr> extra = {});
	void error(code c, std::string_view msg,
		   std::initializer_list<attr> extra);
	void warning(std::string_view msg, int64_t primary = 0,
		     std::initializer_list<attr> extra = {});
	void warning(std::string_view msg, std::initializer_list<attr> extra);
	void info(std::string_view msg, int64_t primary = 0,
		  std::initializer_list<attr> extra = {});
	void info(std::string_view msg, std::initializer_list<attr> extra);

	[[nodiscard]] bool has_error() const;

	void print(std::ostream& os) const;
	void print(const sink& s) const;
	void print(const sinks& s) const;
	void print() const;

	/// @ref print then return @ref has_value(). Default routing:
	/// errors/warnings → @c std::cerr, info → @c std::cout.
	[[nodiscard]] bool print_and_ok() const;
	[[nodiscard]] bool print_and_ok(std::ostream& os) const;
	[[nodiscard]] bool print_and_ok(const sink& s) const;
	[[nodiscard]] bool print_and_ok(const sinks& s) const;

	[[nodiscard]] bool is_well_formed() const;

	/// Open a parent scope on this result's report. Tag-driven: info_micros
	/// auto-times; other tags use @p value as the final node value.
	[[nodiscard]] scope_guard open(report::key name,
		code tag = code::info_micros, int64_t value = 0);
	[[nodiscard]] scope_guard open(std::string_view name,
		code tag = code::info_micros, int64_t value = 0);
	/// Like @ref open but conditional. Returns a no-op guard when `enable` is `false`.
	[[nodiscard]] scope_guard open_if(bool enable,
		report::key name,
		code tag = code::info_micros, int64_t value = 0);
	[[nodiscard]] scope_guard open_if(bool enable,
		std::string_view name,
		code tag = code::info_micros, int64_t value = 0);

	/// Convenience wrapper: open_if(enable, name) + run @p f + return its
	/// result. Mirrors @ref report::step.
	template <typename F>
	auto step(bool enable, report::key name, F&& f) -> decltype(f());
	template <typename F>
	auto step(bool enable, std::string_view name, F&& f) -> decltype(f());

	void append(diagnostics_report&& child);

	/// Merge a child @ref result's report into this one. The child's value
	/// is dropped;
	/// use the underlying primitives if you need it. Attaches the child's
	/// subtree under whichever scope is currently open on this result.
	template <typename U>
	result& merge(result<U>&& child);

	/// Stream-style shorthand for @ref merge. Returns @c *this so multiple
	/// merges can be chained: `r << child_a << child_b;`.
	template <typename U>
	result& operator<<(result<U>&& child);

	/// Take @p child's value (if any), merge its report, return the value.
	template <typename U>
	[[nodiscard]] std::optional<U> merge_take(result<U>&& child);

private:
	void ensure_report_root();
	void enforce_error_no_value_invariant();

	std::optional<T> value_ {};
	diagnostics_report diag_rep_ {};
};

template <typename T>
[[nodiscard]] result<T> fail(struct report rep);

template <typename T>
[[nodiscard]] result<T> error(code c, std::string_view msg, int64_t primary = 0,
			      std::initializer_list<attr> extra = {});

/// Forward @p src's diagnostic report into a new @ref result<T> whose value
/// is @p value on success, absent on failure. The value/error invariant
/// (@ref result::enforce_error_no_value_invariant) still applies, so when
/// @p src carries an error @p value is dropped.
template <typename T, typename U>
[[nodiscard]] result<T> forward_as(result<U>&& src, T value);

/// Print messages (if any) then return the value or @c std::exit @p exit_code.
/// All three bands (errors, warnings, info) are routed to @p os — which
/// defaults to @c std::cerr, so info lines also land on stderr during the
/// exit path. Pass @c std::cout if you need normal info routing.
template <typename T>
T exit_on_fail(result<T>&& r,
	int exit_code = 1,
	std::ostream& os = std::cerr);

} // namespace idni::diagnostics

#include "diagnostics.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__DIAGNOSTICS_H__
