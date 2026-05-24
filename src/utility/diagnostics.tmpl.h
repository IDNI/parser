// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <utility>

#include "diagnostics.h"
#include "../parser_strings.h"

#include "../format/json.h"

namespace idni::diagnostics {

namespace {

constexpr uint16_t code_band(code c) {
	return static_cast<uint16_t>(c) & 0xC000u;
}

// Tab width for diagnostics tree printing (matches project style).
constexpr size_t print_tab_width = 8;

} // namespace

constexpr bool is_error(code c) { return code_band(c) == 0x0000u; }
constexpr bool is_warning(code c) { return code_band(c) == 0x4000u; }
constexpr bool is_info(code c) { return code_band(c) == 0x8000u; }
// Whitelist, not range check: if a new metric tag is added to `code`,
// extend this predicate too.
constexpr bool is_metric(code c) {
	return c == code::info_micros || c == code::info_kb
		|| c == code::info_count;
}

constexpr bool uses_micros_scale(code c) { return c == code::info_micros; }

inline const char* code_name(code c) {
	using sk = idni::parser_strings::static_key;
	sk k = sk::code_unknown;
	switch (c) {
#define PARSER_CODE_LABEL_CASE(id, label) \
	case code::id: k = sk::code_##id; break;
	PARSER_CODE_LABELS(PARSER_CODE_LABEL_CASE)
#undef PARSER_CODE_LABEL_CASE
	}
	return idni::parser_strings::dict(
		idni::parser_strings::key_id(k)).c_str();
}

inline sink::sink(std::ostream* os)
	: write_(os ? std::function<void(std::string_view)>(
			[os](std::string_view s) { *os << s << '\n'; })
		    : std::function<void(std::string_view)>{}) {}

inline void sink::operator()(std::string_view line) const {
	if (write_) write_(line);
}

inline int64_t report::checked_i64(size_t v) {
	return v > static_cast<size_t>(std::numeric_limits<int64_t>::max())
		? std::numeric_limits<int64_t>::max()
		: static_cast<int64_t>(v);
}

inline report::scope_guard::scope_guard(report& r, key name,
	code tag, int64_t initial_value)
	: r_(&r), idx_(r.push_scope(name, tag, initial_value)),
	  timed_(tag == code::info_micros)
{
	if (timed_) timer_.start();
}

inline report::scope_guard::scope_guard(scope_guard&& o) noexcept
	: r_(o.r_), idx_(o.idx_), timed_(o.timed_),
	  timer_(std::move(o.timer_))
{
	o.r_ = nullptr;
}

inline report::scope_guard::~scope_guard() {
	if (!r_) return;
	if (timed_) r_->set_node_value(idx_,
		static_cast<int64_t>(timer_.stop() * 1000));
	r_->pop_scope(idx_);
}

inline report::scope_guard report::open(key name, code tag, int64_t value) {
	return scope_guard(*this, name, tag, value);
}

inline report::scope_guard report::open(std::string_view name, code tag,
	int64_t value)
{
	return open(intern(name), tag, value);
}

inline report::scope_guard report::open_if(bool enable, key name,
	code tag, int64_t value)
{
	return enable ? open(name, tag, value) : scope_guard{};
}

inline report::scope_guard report::open_if(bool enable,
	std::string_view name, code tag, int64_t value)
{
	return enable ? open(intern(name), tag, value) : scope_guard{};
}

template <typename F>
auto report::step(bool enable, key name, F&& f) -> decltype(f()) {
	auto _ = open_if(enable, name);
	return f();
}

template <typename F>
auto report::step(bool enable, std::string_view name, F&& f)
	-> decltype(f())
{
	// Skip intern when disabled — keeps the `enable=false` path
	// allocation-free.
	if (!enable) return f();
	return step(enable, intern(name), std::forward<F>(f));
}

inline void report::reset(std::string_view root_name) {
	reset(intern(root_name));
}

inline void report::reset(key root) {
	nodes_.clear();
	attrs_.clear();
	scope_stack_.clear();
	has_error_ = false;
	push_scope(root);
}

inline void report::clear() {
	nodes_.clear();
	attrs_.clear();
	dyn_strings_.clear();
	dyn_strings_index_.clear();
	scope_stack_.clear();
	has_error_ = false;
}

inline report::key report::intern(const char* s) {
	if (!s || !*s) return none;
	key k = idni::parser_strings::dict(s);
	return k != none ? k : intern_dynamic(s);
}

inline report::key report::intern(std::string_view s) {
	if (s.empty()) return none;
	key k = idni::parser_strings::dict(s);
	return k != none ? k : intern_dynamic(s);
}

inline report::key report::intern_dynamic(std::string_view s) {
	if (s.empty()) return none;
	std::string key_str(s);
	auto it = dyn_strings_index_.find(key_str);
	if (it != dyn_strings_index_.end()) return it->second;
	dyn_strings_.emplace_back(s);
	key id = -static_cast<key>(dyn_strings_.size());
	dyn_strings_index_.emplace(std::move(key_str), id);
	return id;
}

inline std::string_view report::str(key id) const {
	if (id > 0) return idni::parser_strings::dict(id);
	if (id < 0) {
		size_t i = static_cast<size_t>(-id - 1);
		return i < dyn_strings_.size() ? dyn_strings_[i] : std::string_view{};
	}
	return {};
}

inline void report::count(key name, int_t v) {
	count(name, static_cast<int64_t>(v));
}

inline void report::count(key name, int64_t v) {
	push_tagged(code::info_count, name, v);
}

inline void report::count(key name, size_t v) {
	count(name, checked_i64(v));
}

inline void report::count(std::string_view name, int_t v) {
	count(intern(name), v);
}

inline void report::count(std::string_view name, int64_t v) {
	count(intern(name), v);
}

inline void report::count(std::string_view name, size_t v) {
	count(intern(name), v);
}

inline void report::kb(key name, int64_t kilobytes) {
	DBG(assert(kilobytes >= 0);)
	push_tagged(code::info_kb, name, kilobytes);
}

inline void report::kb(std::string_view name, int64_t kilobytes) {
	kb(intern(name), kilobytes);
}

inline void report::error(code c, key msg, int64_t primary,
	std::initializer_list<attr> extra)
{
	DBG(assert(is_error(c));)
	push_tagged(c, msg, primary, extra);
}

inline void report::error(code c, std::string_view msg, int64_t primary,
	std::initializer_list<attr> extra)
{
	error(c, intern_dynamic(msg), primary, extra);
}

inline void report::warning(key msg, int64_t primary,
	std::initializer_list<attr> extra)
{
	push_tagged(code::warning, msg, primary, extra);
}

inline void report::warning(std::string_view msg, int64_t primary,
	std::initializer_list<attr> extra)
{
	warning(intern_dynamic(msg), primary, extra);
}

inline void report::info(key msg, int64_t primary,
	std::initializer_list<attr> extra)
{
	push_tagged(code::info, msg, primary, extra);
}

inline void report::info(std::string_view msg, int64_t primary,
	std::initializer_list<attr> extra)
{
	info(intern_dynamic(msg), primary, extra);
}

inline void report::error(code c, key msg,
	std::initializer_list<attr> extra)
{
	error(c, msg, 0, extra);
}

inline void report::error(code c, std::string_view msg,
	std::initializer_list<attr> extra)
{
	error(c, msg, 0, extra);
}

inline void report::warning(key msg, std::initializer_list<attr> extra) {
	warning(msg, 0, extra);
}

inline void report::warning(std::string_view msg,
	std::initializer_list<attr> extra)
{
	warning(msg, 0, extra);
}

inline void report::info(key msg, std::initializer_list<attr> extra) {
	info(msg, 0, extra);
}

inline void report::info(std::string_view msg,
	std::initializer_list<attr> extra)
{
	info(msg, 0, extra);
}

template <typename Other>
void report::append_impl(Other&& other) {
	constexpr bool is_rvalue = std::is_rvalue_reference_v<Other&&>;
	if (other.nodes_.empty()) return;
	has_error_ = has_error_ || other.has_error_;
	size_t base      = nodes_.size();
	size_t attr_base = attrs_.size();
	int32_t attach_parent = current_parent();
	size_t dyn_base = dyn_strings_.size();
	DBG(assert(attr_base + other.attrs_.size()
		<= std::numeric_limits<uint32_t>::max());)
	if constexpr (is_rvalue) {
		dyn_strings_.insert(dyn_strings_.end(),
			std::make_move_iterator(other.dyn_strings_.begin()),
			std::make_move_iterator(other.dyn_strings_.end()));
	} else {
		dyn_strings_.insert(dyn_strings_.end(),
			other.dyn_strings_.begin(),
			other.dyn_strings_.end());
	}
	// Rebuild the side index for the newly-appended dynamic strings.
	for (size_t i = 0; i < other.dyn_strings_.size(); ++i) {
		int_t new_id = -static_cast<int_t>(dyn_base + i + 1);
		dyn_strings_index_.emplace(dyn_strings_[dyn_base + i], new_id);
	}
	auto remap_key = [&](int_t k) -> int_t {
		if (k >= 0) return k;
		size_t i = static_cast<size_t>(-k - 1);
		if (i >= other.dyn_strings_.size()) return none;
		return -static_cast<int_t>(dyn_base + i + 1);
	};
	nodes_.reserve(base + other.nodes_.size());
	for (auto n : other.nodes_) {
		if (n.parent < 0)
			n.parent = attach_parent;
		else
			n.parent += static_cast<int32_t>(base);
		if (n.attr_cnt)
			n.attr_off = static_cast<decltype(n.attr_off)>(
				n.attr_off + attr_base);
		n.key = remap_key(n.key);
		nodes_.push_back(n);
	}
	size_t attr_dst = attrs_.size();
	attrs_.insert(attrs_.end(),
		other.attrs_.begin(), other.attrs_.end());
	for (size_t i = attr_dst; i < attrs_.size(); ++i)
		attrs_[i].key = remap_key(attrs_[i].key);
}

inline void report::append(const report& other) {
	append_impl(other);
}

inline void report::append(report&& other) {
	append_impl(std::move(other));
}

inline bool report::has_error() const {
	return has_error_;
}

inline std::string report::format_message(size_t node_idx) const {
	if (node_idx >= nodes_.size()) return {};
	const auto& n = nodes_[node_idx];
	if (!is_error(n.tag) && !is_warning(n.tag)) return {};
	std::string out;
	auto msg = str(n.key);
	if (!msg.empty()) out.append(msg);
	else              out += "(unnamed)";
	std::string extras;
	auto append_extra = [&](const std::string& part) {
		if (!extras.empty()) extras += ' ';
		extras += part;
	};
	for (uint8_t i = 0; i < n.attr_cnt; ++i) {
		const auto& a = attrs_[n.attr_off + i];
		append_extra(std::string(str(a.key)) + '='
			+ std::to_string(a.value));
	}
	// parse_error nodes always carry a byte offset (loc=0 is the first
	// byte of input, a common error site). Other errors store whatever
	// the producer chose in `value`, so render that as a plain value=
	// only when it is meaningfully non-zero.
	if (n.tag == code::parse_error && n.key != none)
		append_extra("loc=" + std::to_string(n.value));
	else if (n.value != 0)
		append_extra("value=" + std::to_string(n.value));
	if (!extras.empty()) out += " (" + extras + ')';
	return out;
}

inline void report::print(const sinks& s) const {
	if (nodes_.empty()) return;
	// Three passes over nodes_ — children index, label-width pass, value-
	// column pass — kept separate for clarity. Reports stay small enough
	// that fusing them is not worth the complexity.
	auto children = build_children();
	size_t label_w = max_label_width(children);
	auto cols = value_column_widths();
	info_row_ = 0;
	for (size_t i = 0; i < nodes_.size(); ++i)
		if (nodes_[i].parent < 0)
			print_node(s, children, i, 0, label_w, cols);
}

inline void report::print(std::ostream& os) const {
	print({ .error = &os, .warning = &os, .info = &os });
}

inline void report::print(const sink& s) const {
	print({ s, s, s });
}

inline void report::print() const {
	print({ .error = &std::cerr, .warning = &std::cerr,
		.info = &std::cout });
}

inline std::ostream& report::to_json(std::ostream& os,
	bool print_names) const
{
	os << "{\n  \"nodes\": [\n";
	for (size_t i = 0; i < nodes_.size(); ++i) {
		if (i) os << ",\n";
		const auto& n = nodes_[i];
		os << "    {\"tag\": " << static_cast<unsigned>(n.tag);
		if (print_names) {
			os << ", \"message\": ";
			idni::format::json::escape(os, code_name(n.tag));
		}
		os << ", \"key\": ";
		idni::format::json::escape(os, str(n.key));
		os << ", \"parent\": " << n.parent
		   << ", \"value\": " << n.value
		   << ", \"attrs\": [";
		for (uint8_t a = 0; a < n.attr_cnt; ++a) {
			if (a) os << ", ";
			const auto& at = attrs_[n.attr_off + a];
			os << "{\"key\": ";
			idni::format::json::escape(os, str(at.key));
			os << ", \"value\": " << at.value << "}";
		}
		os << "]}";
	}
	os << "\n  ]\n}";
	return os;
}

inline const std::vector<node>& report::nodes() const {
	return nodes_;
}

inline void report::set_node_value(int32_t idx, int64_t v) {
	if (idx >= 0 && static_cast<size_t>(idx) < nodes_.size())
		nodes_[static_cast<size_t>(idx)].value = v;
}

inline int32_t report::current_parent() const {
	return scope_stack_.empty() ? -1 : scope_stack_.back();
}

inline int32_t report::push_scope(key name, code tag, int64_t initial_value) {
	int32_t idx = push_tagged(tag, name, initial_value);
	scope_stack_.push_back(idx);
	return idx;
}

inline void report::pop_scope(int32_t idx) {
	// LIFO requirement: an out-of-order pop leaves the outer guard's
	// scope as current. Loud in debug, silent skip in release.
	DBG(assert(scope_stack_.empty() || scope_stack_.back() == idx);)
	if (!scope_stack_.empty() && scope_stack_.back() == idx)
		scope_stack_.pop_back();
}

inline int32_t report::push_tagged(code tag, key name, int64_t v,
	std::initializer_list<attr> extra)
{
	// Catches only the explicitly-unused 0xC000 band; out-of-enum
	// values inside the valid bands still pass.
	DBG(assert(code_band(tag) != 0xC000u);)
	if (is_error(tag)) has_error_ = true;
	node n;
	n.tag    = tag;
	n.key    = name;
	n.parent = current_parent();
	n.value  = v;
	if (extra.size()) {
		DBG(assert(attrs_.size() <= std::numeric_limits<
			decltype(n.attr_off)>::max());)
		DBG(assert(extra.size() <= std::numeric_limits<
			decltype(n.attr_cnt)>::max());)
		n.attr_off = static_cast<decltype(n.attr_off)>(attrs_.size());
		n.attr_cnt = static_cast<decltype(n.attr_cnt)>(extra.size());
		for (const auto& a : extra) attrs_.push_back(a);
	}
	nodes_.push_back(n);
	return static_cast<int32_t>(nodes_.size() - 1);
}

inline std::vector<std::vector<size_t>> report::build_children() const {
	std::vector<std::vector<size_t>> children(nodes_.size());
	for (size_t i = 0; i < nodes_.size(); ++i) {
		int32_t p = nodes_[i].parent;
		if (p >= 0)
			children[static_cast<size_t>(p)].push_back(i);
	}
	return children;
}

inline size_t report::max_label_width(
	const std::vector<std::vector<size_t>>& children,
	size_t lvl, size_t idx) const
{
	if (idx >= nodes_.size()) {
		// Top-level entry (caller passed defaults): fold over every
		// root in the forest. This handles reports built without an
		// initial reset() — multiple parent==-1 nodes coexist.
		size_t best = 0;
		for (size_t i = 0; i < nodes_.size(); ++i)
			if (nodes_[i].parent < 0)
				best = std::max(best,
					max_label_width(children, 0, i));
		return best;
	}
	const auto& n = nodes_[idx];
	// Errors/warnings render flat at column 0 and don't influence
	// the info-band metric column.
	if (is_error(n.tag) || is_warning(n.tag)) return lvl;
	size_t w = lvl * print_tab_width + str(n.key).size() + 1;
	size_t best = w;
	for (size_t i : children[idx]) {
		const auto& c = nodes_[i];
		if (is_error(c.tag) || is_warning(c.tag)) continue;
		size_t cw = max_label_width(children, lvl + 1, i);
		best = std::max(best, cw);
	}
	return best;
}

inline report::value_parts report::format_value_parts(int64_t v, code c) {
	value_parts p;
	auto two_decimals = [](int64_t whole, int64_t hundredths)
	{
		char buf[41];
		std::snprintf(buf, sizeof(buf), "%lld.%02lld",
			static_cast<long long>(whole),
			static_cast<long long>(hundredths));
		return std::string(buf);
	};
	switch (c) {
	case code::info_kb: {
		auto scale_kb = [&](int64_t div, const char* unit) {
			int64_t whole = v / div;
			int64_t rem   = v % div;
			int64_t hundredths = (rem * 100 + div / 2) / div;
			if (hundredths >= 100) { ++whole; hundredths -= 100; }
			p.num  = two_decimals(whole, hundredths);
			p.unit = unit;
		};
		if (v == 0) p.num = "0", p.unit = "kb";
		else if (v < 1024)
			p.num = std::to_string(v), p.unit = "kb";
		else if (v < 1024 * 1024)
			scale_kb(1024, "Mb");
		else if (v < 1024LL * 1024 * 1024)
			scale_kb(1024LL * 1024, "Gb");
		else
			scale_kb(1024LL * 1024 * 1024, "Tb");
		break;
	}
	case code::info_micros:
		// Zero is shown as "0 ms"
		if (v == 0) p.num = "0", p.unit = "ms";
		else if (v < 1000)
			p.num = std::to_string(v), p.unit = "\u00b5s";
		else if (v < 1'000'000) {
			int64_t whole = v / 1000;
			int64_t hundredths = ((v % 1000) + 5) / 10;
			if (hundredths >= 100) { ++whole; hundredths -= 100; }
			p.num = two_decimals(whole, hundredths);
			p.unit = "ms";
		} else {
			int64_t whole = v / 1000000;
			int64_t hundredths = ((v % 1000000) + 5000) / 10000;
			if (hundredths >= 100) { ++whole; hundredths -= 100; }
			p.num = two_decimals(whole, hundredths);
			p.unit = "s";
		}
		break;
	default:
		p.num = std::to_string(v);
		break;
	}
	return p;
}

inline std::string report::format_value(int64_t v, code c) {
	auto p = format_value_parts(v, c);
	if (p.unit.empty()) return p.num;
	return p.num + ' ' + p.unit;
}

inline report::value_columns report::value_column_widths() const {
	constexpr size_t min_value_field_width = 6;
	value_columns cols{ min_value_field_width, 0 };
	for (const auto& n : nodes_) {
		if (is_error(n.tag) || is_warning(n.tag)) continue;
		auto p = format_value_parts(n.value, n.tag);
		cols.num_w  = std::max(cols.num_w, p.num.size());
		cols.unit_w = std::max(cols.unit_w, p.unit.size());
	}
	return cols;
}

inline std::string report::color_info_line(const node& n, size_t level,
	size_t label_w, value_columns cols) const
{
	const bool even = (info_row_ & 1) != 0;
	++info_row_;
	auto label_fg = [&]() -> std::string {
		return even ? TC.DIM() : TC.DEFAULT();
	};
	auto metric_fg = [&](code c) -> std::string {
		if (c == code::info_kb)
			return even ? TC.CYAN() : TC.LIGHT_CYAN();
		if (c == code::info_count)
			return even ? TC.DARK_GRAY() : TC.LIGHT_GRAY();
		return label_fg();
	};
	std::string line(level, '\t');
	auto key = str(n.key);
	line += label_fg() + std::string(key) + ':' + TC.CLEAR();
	// A root-level info_micros node with value=0 is the structural
	// marker pushed by reset(); it has no measured time, so suppress
	// the value/unit columns (and their label padding) to avoid the
	// misleading "0 ms" noise.
	const bool suppress_value =
		level == 0 && n.value == 0 && n.tag == code::info_micros;
	if (!suppress_value) {
		size_t pos = level * print_tab_width + key.size() + 1;
		if (pos < label_w) line.append(label_w - pos, ' ');
		line += ' ';
		auto p = format_value_parts(n.value, n.tag);
		auto val_fg = metric_fg(n.tag);
		if (p.num.size() < cols.num_w)
			line.append(cols.num_w - p.num.size(), ' ');
		line += val_fg + p.num + TC.CLEAR();
		if (!p.unit.empty()) {
			line += ' ';
			if (p.unit.size() < cols.unit_w)
				line.append(cols.unit_w - p.unit.size(), ' ');
			line += val_fg + p.unit + TC.CLEAR();
		}
	}
	if (n.attr_cnt > 0) {
		line += ' ';
		line += TC.DIM();
		line += '(';
		for (uint8_t i = 0; i < n.attr_cnt; ++i) {
			const auto& a = attrs_[n.attr_off + i];
			if (i) line += ' ';
			line += std::string(str(a.key));
			line += '=';
			line += std::to_string(a.value);
		}
		line += ')';
		line += TC.CLEAR();
	}
	return line;
}

inline void report::print_node(const sinks& s,
	const std::vector<std::vector<size_t>>& children, size_t idx,
	size_t level, size_t label_w, value_columns cols) const
{
	const auto& n = nodes_[idx];
	if (is_error(n.tag)) {
		s.error(TC.RED() + format_message(idx) + TC.CLEAR());
	} else if (is_warning(n.tag)) {
		s.warning(TC.YELLOW() + format_message(idx) + TC.CLEAR());
	} else {
		s.info(color_info_line(n, level, label_w, cols));
	}
	for (size_t i : children[idx])
		print_node(s, children, i, level + 1, label_w, cols);
}

inline std::ostream& operator<<(std::ostream& os, const report& r) {
	r.print(os);
	return os;
}

template <typename T>
result<T>::result(std::string_view name) {
	diag_rep_.reset(name);
}

template <typename T>
result<T>::result(result&& other) noexcept
	: value_(std::move(other.value_)),
	  diag_rep_(std::move(other.diag_rep_)) {}

template <typename T>
result<T>& result<T>::operator=(result&& other) noexcept {
	if (this != &other) {
		value_    = std::move(other.value_);
		diag_rep_ = std::move(other.diag_rep_);
	}
	return *this;
}

template <typename T>
result<T>::result(T value)
	requires(!std::is_same_v<std::decay_t<T>, std::string>)
	: value_(std::move(value)) {}

template <typename T>
result<T>::result(typename result<T>::diagnostics_report rep)
	: diag_rep_(std::move(rep)) {
	this->enforce_error_no_value_invariant();
}

template <typename T>
result<T>::~result() = default;

template <typename T>
bool result<T>::has_value() const {
	return value_.has_value();
}

template <typename T>
result<T>::operator bool() const {
	return has_value();
}

template <typename T>
result<T>::operator T() const
	requires std::is_pointer_v<T>
{
	return value_.has_value() ? value_.value() : nullptr;
}

template <typename T>
bool result<T>::operator==(std::nullptr_t) const
	requires std::is_pointer_v<T>
{
	return !value_.has_value();
}

template <typename T>
bool result<T>::operator!=(std::nullptr_t) const
	requires std::is_pointer_v<T>
{
	return value_.has_value();
}

template <typename T>
T& result<T>::value() & {
	return value_.value();
}

template <typename T>
const T& result<T>::value() const& {
	return value_.value();
}

template <typename T>
T&& result<T>::value() && {
	return std::move(value_.value());
}

template <typename T>
T& result<T>::emplace(T&& v) {
	DBG(assert(!diag_rep_.has_error()
		&& "emplace on errored result silently drops the value");)
	value_.emplace(std::move(v));
	this->enforce_error_no_value_invariant();
	return value();
}

template <typename T>
template <typename... Args>
T& result<T>::emplace(Args&&... args) {
	DBG(assert(!diag_rep_.has_error()
		&& "emplace on errored result silently drops the value");)
	value_.emplace(std::forward<Args>(args)...);
	this->enforce_error_no_value_invariant();
	return value();
}

template <typename T>
template <typename U>
result<T>& result<T>::operator=(U&& v) {
	DBG(assert(!diag_rep_.has_error()
		&& "assigning to an errored result silently drops the value");)
	value_.emplace(std::forward<U>(v));
	this->enforce_error_no_value_invariant();
	return *this;
}

template <typename T>
void result<T>::clear_value() {
	value_.reset();
}

template <typename T>
T& result<T>::operator*() &
	requires(!std::is_pointer_v<T>)
{
	return value();
}

template <typename T>
const T& result<T>::operator*() const&
	requires(!std::is_pointer_v<T>)
{
	return value();
}

template <typename T>
T&& result<T>::operator*() &&
	requires(!std::is_pointer_v<T>)
{
	return std::move(value());
}

template <typename T>
T* result<T>::operator->()
	requires(!std::is_pointer_v<T>)
{
	return &value();
}

template <typename T>
const T* result<T>::operator->() const
	requires(!std::is_pointer_v<T>)
{
	return &value();
}

template <typename T>
typename result<T>::diagnostics_report& result<T>::report() & {
	return diag_rep_;
}

template <typename T>
const typename result<T>::diagnostics_report&
result<T>::report() const& {
	return diag_rep_;
}

template <typename T>
typename result<T>::diagnostics_report&& result<T>::report() && {
	return std::move(diag_rep_);
}

template <typename T>
void result<T>::ensure_report_root() {
	if (diag_rep_.nodes().empty()) diag_rep_.reset("root");
}

template <typename T>
void result<T>::error(code c, std::string_view msg, int64_t primary,
		      std::initializer_list<attr> extra) {
	ensure_report_root();
	diag_rep_.error(c, msg, primary, extra);
	this->enforce_error_no_value_invariant();
}

template <typename T>
void result<T>::warning(std::string_view msg, int64_t primary,
			std::initializer_list<attr> extra) {
	ensure_report_root();
	diag_rep_.warning(msg, primary, extra);
}

template <typename T>
void result<T>::info(std::string_view msg, int64_t primary,
		     std::initializer_list<attr> extra) {
	ensure_report_root();
	diag_rep_.info(msg, primary, extra);
}

template <typename T>
void result<T>::error(code c, std::string_view msg,
		      std::initializer_list<attr> extra) {
	error(c, msg, 0, extra);
}

template <typename T>
void result<T>::warning(std::string_view msg,
			std::initializer_list<attr> extra) {
	warning(msg, 0, extra);
}

template <typename T>
void result<T>::info(std::string_view msg,
		     std::initializer_list<attr> extra) {
	info(msg, 0, extra);
}

template <typename T>
bool result<T>::has_error() const {
	return diag_rep_.has_error();
}

template <typename T>
void result<T>::print(std::ostream& os) const {
	if (!diag_rep_.nodes().empty()) diag_rep_.print(os);
}

template <typename T>
void result<T>::print(const sink& s) const {
	if (!diag_rep_.nodes().empty()) diag_rep_.print(s);
}

template <typename T>
void result<T>::print(const sinks& s) const {
	if (!diag_rep_.nodes().empty()) diag_rep_.print(s);
}

template <typename T>
void result<T>::print() const {
	if (!diag_rep_.nodes().empty()) diag_rep_.print();
}

template <typename T>
bool result<T>::print_and_ok() const {
	print();
	return has_value();
}

template <typename T>
bool result<T>::print_and_ok(std::ostream& os) const {
	print(os);
	return has_value();
}

template <typename T>
bool result<T>::print_and_ok(const sink& s) const {
	print(s);
	return has_value();
}

template <typename T>
bool result<T>::print_and_ok(const sinks& s) const {
	print(s);
	return has_value();
}

template <typename T>
bool result<T>::is_well_formed() const {
	return value_.has_value() || diag_rep_.has_error();
}

template <typename T>
typename result<T>::scope_guard result<T>::open_if(bool enable,
	report::key name, code tag, int64_t value)
{
	return diag_rep_.open_if(enable, name, tag, value);
}

template <typename T>
typename result<T>::scope_guard result<T>::open_if(bool enable,
	std::string_view name, code tag, int64_t value)
{
	return diag_rep_.open_if(enable, name, tag, value);
}

template <typename T>
typename result<T>::scope_guard result<T>::open(report::key name, code tag,
	int64_t value)
{
	return diag_rep_.open(name, tag, value);
}

template <typename T>
typename result<T>::scope_guard result<T>::open(std::string_view name,
	code tag, int64_t value)
{
	return diag_rep_.open(name, tag, value);
}

template <typename T>
template <typename F>
auto result<T>::step(bool enable, report::key name, F&& f) -> decltype(f()) {
	return diag_rep_.step(enable, name, std::forward<F>(f));
}

template <typename T>
template <typename F>
auto result<T>::step(bool enable, std::string_view name, F&& f)
	-> decltype(f())
{
	return diag_rep_.step(enable, name, std::forward<F>(f));
}

template <typename T>
void result<T>::append(typename result<T>::diagnostics_report&& child) {
	diag_rep_.append(std::move(child));
	this->enforce_error_no_value_invariant();
}

template <typename T>
template <typename U>
result<T>& result<T>::merge(result<U>&& child) {
	diag_rep_.append(std::move(child.diag_rep_));
	this->enforce_error_no_value_invariant();
	return *this;
}

template <typename T>
template <typename U>
result<T>& result<T>::operator<<(result<U>&& child) {
	return merge(std::move(child));
}

template <typename T>
template <typename U>
std::optional<U> result<T>::merge_take(result<U>&& child) {
	std::optional<U> v;
	if (child.has_value()) v = std::move(child).value();
	merge(std::move(child));
	return v;
}

// Invariant: a result holding an error carries no value. If both are present
// after a mutation, the value is dropped silently.
template <typename T>
void result<T>::enforce_error_no_value_invariant() {
	if (value_.has_value() && diag_rep_.has_error()) value_.reset();
}

template <typename T>
result<T> fail(struct report rep) {
	return result<T>(std::move(rep));
}

template <typename T>
result<T> error(code c, std::string_view msg, int64_t primary,
		std::initializer_list<attr> extra) {
	report r;
	r.error(c, msg, primary, extra);
	return fail<T>(std::move(r));
}

template <typename T, typename U>
result<T> forward_as(result<U>&& src, T value) {
	if (!src.has_value()) {
		auto rep = std::move(src).report();
		return result<T>(std::move(rep));
	}
	result<T> r(std::move(value));
	auto rep = std::move(src).report();
	r.append(std::move(rep));
	return r;
}

template <typename T>
T exit_on_fail(result<T>&& r, int exit_code, std::ostream& os)
{
	if (!r.print_and_ok(os)) std::exit(exit_code);
	return std::move(r).value();
}

} // namespace idni::diagnostics
