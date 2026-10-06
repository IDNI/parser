// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__

#include <cassert>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <map>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>

#include "../parser_strings.h"

namespace idni {

namespace detail {

/// The kind of an option is the index of its alternative in option_value.
enum class option_kind : std::size_t { flag, numeric, text, list };

inline option_kind kind_of(const option_value& v) {
	return static_cast<option_kind>(v.index());
}

template <typename T>
constexpr bool is_option_type_v = std::is_same_v<T, bool>
	|| std::is_same_v<T, std::size_t> || std::is_same_v<T, std::string>
	|| std::is_same_v<T, option_list>;

// set() takes a literal or an int too, so a caller needs no cast.
template <typename T, typename D = std::decay_t<T>>
using option_store_t = std::conditional_t<std::is_same_v<D, bool>, bool,
	std::conditional_t<std::is_integral_v<D>, std::size_t,
	std::conditional_t<std::is_convertible_v<const D&, std::string_view>,
		std::string, D>>>;

template <typename T>
option_value to_option_value(T&& v) {
	using D = std::decay_t<T>;
	using S = option_store_t<T>;
	static_assert(is_option_type_v<S>,
		"an option value is bool, size_t, string or option_list");
	if constexpr (std::is_integral_v<D> && !std::is_same_v<D, bool>) {
		if constexpr (std::is_signed_v<D>) { DBG(assert(v >= 0);) }
		return option_value(static_cast<std::size_t>(v));
	} else return option_value(S(std::forward<T>(v)));
}

inline std::string option_trim(std::string_view s) {
	std::size_t b = 0, e = s.size();
	while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
	while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
	return std::string(s.substr(b, e - b));
}

inline std::vector<std::string> option_split(std::string_view s, char sep) {
	std::vector<std::string> out;
	for (std::size_t start = 0;;) {
		std::size_t pos = s.find(sep, start);
		out.push_back(option_trim(s.substr(start, pos - start)));
		if (pos == std::string_view::npos) return out;
		start = pos + 1;
	}
}

/// The value of @p text, or nothing when the kind does not take it.
inline std::optional<option_value> parse_value(option_kind kind,
	std::string_view text)
{
	switch (kind) {
	case option_kind::flag: {
		std::string w(text);
		for (auto& c : w) c = static_cast<char>(
			std::tolower(static_cast<unsigned char>(c)));
		if (w == "true" || w == "on" || w == "1" || w == "yes"
			|| w == "y" || w == "t") return option_value(true);
		if (w == "false" || w == "off" || w == "0" || w == "no"
			|| w == "n" || w == "f") return option_value(false);
		return std::nullopt;
	}
	case option_kind::numeric: {
		std::size_t v = 0;
		auto r = std::from_chars(text.data(), text.data() + text.size(), v);
		if (text.empty() || r.ec != std::errc{}
			|| r.ptr != text.data() + text.size()) return std::nullopt;
		return option_value(v);
	}
	case option_kind::text: return option_value(std::string(text));
	case option_kind::list: {
		option_list out;
		for (auto& part : option_split(text, ','))
			if (!part.empty()) out.push_back(std::move(part));
		return option_value(std::move(out));
	}
	}
	return std::nullopt;
}

inline std::string format_value(const option_value& v) {
	switch (kind_of(v)) {
	case option_kind::flag: return std::get<bool>(v) ? "true" : "false";
	case option_kind::numeric:
		return std::to_string(std::get<std::size_t>(v));
	case option_kind::text: return std::get<std::string>(v);
	case option_kind::list: {
		std::string out;
		for (const auto& s : std::get<option_list>(v)) {
			if (!out.empty()) out += ", ";
			out += s;
		}
		return out;
	}
	}
	return {};
}

/// Parse @p text as a value of @p kind. A flag takes 1 t true on yes y and
/// 0 f false off no n in any case. A list is comma separated.
inline diagnostics::result<option_value> parse_option_value(option_kind kind,
	std::string_view text, std::string_view name = {})
{
	diagnostics::result<option_value> r;
	auto v = parse_value(kind, text);
	if (!v) return r.with_error(diagnostics::code::invalid_argument,
		parser_strings::messages::option_bad_value,
		{ { parser_strings::label::name, name },
		  { parser_strings::label::value, text } });
	return r.with_value(std::move(*v));
}

/// The text form of @p v, which parse_option_value() reads back.
inline diagnostics::result<std::string> format_option_value(
	const option_value& v)
{
	diagnostics::result<std::string> r;
	return r.with_value(format_value(v));
}

inline std::string option_env_name(std::string_view prefix,
	std::string_view name)
{
	std::string var(prefix);
	for (char c : name)
		var += c == '-' ? '_' : static_cast<char>(std::toupper(
			static_cast<unsigned char>(c)));
	return var;
}

/// The declarations. A declare() replaces the catalog as a whole.
struct options_catalog {
	std::vector<option_spec> specs;
	std::map<std::string, std::size_t, std::less<>> index;

	std::optional<std::size_t> find(std::string_view name) const {
		auto it = index.find(name);
		if (it == index.end()) return std::nullopt;
		return it->second;
	}
};

/// The shared values. A snapshot never changes after it is published.
struct options_snapshot {
	std::shared_ptr<const options_catalog> cat;
	/// One row per option, one cell per source id.
	std::vector<std::vector<std::optional<option_value>>> values;
};

/// The value of option @p i: the sources from high to low, the default of
/// the option.
inline const option_value& effective(const options_snapshot& s,
	std::size_t i)
{
	const auto& row = s.values[i];
	for (auto it = row.rbegin(); it != row.rend(); ++it)
		if (*it) return **it;
	return s.cat->specs[i].fallback;
}

template <typename T>
T typed(const option_value& v) {
	if (const T* p = std::get_if<T>(&v)) return *p;
	DBG(assert(false && "an option is read with another type");)
	return T{};
}

/// Store @p v at @p source. False when the cell holds it already.
inline bool set_cell(options_snapshot& s, std::size_t i, std::size_t source,
	option_value v)
{
	auto& row = s.values[i];
	if (row.size() <= source) row.resize(source + 1);
	auto& cell = row[source];
	if (cell && *cell == v) return false;
	cell = std::move(v);
	return true;
}

inline bool clear_cell(options_snapshot& s, std::size_t i, std::size_t source)
{
	auto& row = s.values[i];
	if (source >= row.size() || !row[source]) return false;
	row[source].reset();
	return true;
}

} // namespace detail

inline options_repository::options_repository()
	: snap_(std::make_unique<detail::options_snapshot>(
		detail::options_snapshot{
			std::make_shared<const detail::options_catalog>(), {} }))
{}

inline options_repository::~options_repository() = default;

inline const detail::options_snapshot& options_repository::current() const {
	return *snap_;
}

// @p f returns true when it changed a value. It changes nothing when it
// fails, so a failed write leaves the values as they were.
template <typename F>
void options_repository::modify(F&& f) {
	if (f(*snap_)) ++version_;
}

inline std::size_t options_repository::source(std::string_view name) {
	for (std::size_t i = 0; i < sources_.size(); ++i)
		if (sources_[i] == name) return i;
	sources_.emplace_back(name);
	return sources_.size() - 1;
}

inline diagnostics::result<void> options_repository::declare(
	const option_set& set)
{
	namespace ps = parser_strings;
	using diagnostics::code;
	diagnostics::result<void> r;
	modify([&](detail::options_snapshot& s) {
		auto cat = std::make_shared<detail::options_catalog>(*s.cat);
		for (const auto& o : set.options) {
			if (cat->find(o.name)) {
				r.error(code::invalid_argument,
					ps::messages::option_already_exists,
					{ { ps::label::name, o.name } });
				continue;
			}
			cat->index.emplace(o.name, cat->specs.size());
			cat->specs.push_back(o);
		}
		if (r.has_error()) return false;
		s.values.resize(cat->specs.size());
		s.cat = std::move(cat);
		return true;
	});
	return r;
}

template <typename T>
T options_repository::get(std::string_view name) const {
	static_assert(detail::is_option_type_v<T>,
		"an option value is bool, size_t, string or option_list");
	const detail::options_snapshot& s = current();
	const auto i = s.cat->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is read");)
		return T{};
	}
	return detail::typed<T>(detail::effective(s, *i));
}

template <typename T>
void options_repository::set(std::string_view name, T value,
	std::size_t source)
{
	option_value v = detail::to_option_value(std::move(value));
	modify([&](detail::options_snapshot& s) {
		DBG(assert(source < sources_.size() && "an unknown source");)
		const auto i = s.cat->find(name);
		if (!i) {
			DBG(assert(false && "an unknown option is set");)
			return false;
		}
		if (s.cat->specs[*i].fallback.index() != v.index()) {
			DBG(assert(false && "an option is set with another type");)
			return false;
		}
		return detail::set_cell(s, *i, source, std::move(v));
	});
}

inline void options_repository::clear(std::string_view name,
	std::size_t source)
{
	modify([&](detail::options_snapshot& s) {
		const auto i = s.cat->find(name);
		if (!i) {
			DBG(assert(false && "an unknown option is cleared");)
			return false;
		}
		return detail::clear_cell(s, *i, source);
	});
}

inline bool options_repository::is_set(std::string_view name,
	std::size_t source) const
{
	const detail::options_snapshot& s = current();
	const auto i = s.cat->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is read");)
		return false;
	}
	const auto& row = s.values[*i];
	return source < row.size() && row[source].has_value();
}

inline diagnostics::result<void> options_repository::set_text(
	std::string_view name, std::string_view text, std::size_t source)
{
	namespace ps = parser_strings;
	diagnostics::result<void> r;
	modify([&](detail::options_snapshot& s) {
		const auto i = s.cat->find(name);
		if (!i) {
			r.error(diagnostics::code::not_found,
				ps::messages::option_not_found,
				{ { ps::label::name, name } });
			return false;
		}
		auto v = r.merge_take(detail::parse_option_value(
			detail::kind_of(s.cat->specs[*i].fallback), text, name));
		if (!v) return false;
		return detail::set_cell(s, *i, source, std::move(*v));
	});
	return r;
}

inline diagnostics::result<std::string> options_repository::get_text(
	std::string_view name) const
{
	namespace ps = parser_strings;
	diagnostics::result<std::string> r;
	const detail::options_snapshot& s = current();
	const auto i = s.cat->find(name);
	if (!i) return r.with_error(diagnostics::code::not_found,
		ps::messages::option_not_found, { { ps::label::name, name } });
	auto text = r.merge_take(detail::format_option_value(
		detail::effective(s, *i)));
	if (!text) return r;
	return r.with_value(std::move(*text));
}

inline std::vector<std::string> options_repository::names() const {
	std::vector<std::string> out;
	for (const auto& o : current().cat->specs) out.push_back(o.name);
	return out;
}

inline diagnostics::result<void> options_repository::load_env(
	std::string_view prefix, std::size_t source)
{
	namespace ps = parser_strings;
	diagnostics::result<void> r;
	modify([&](detail::options_snapshot& s) {
		bool changed = false;
		for (std::size_t i = 0; i < s.cat->specs.size(); ++i) {
			const option_spec& o = s.cat->specs[i];
			if (!o.from_env) continue;
			const std::string var = detail::option_env_name(prefix,
				o.name);
			const char* text = std::getenv(var.c_str());
			if (!text) continue;
			auto v = detail::parse_value(detail::kind_of(o.fallback),
				text);
			if (!v) {
				r.warning(ps::messages::option_env_bad_value,
					{ { ps::label::name, var },
					  { ps::label::value, text } });
				continue;
			}
			changed = detail::set_cell(s, i, source, std::move(*v))
				|| changed;
		}
		return changed;
	});
	return r;
}

inline std::size_t options_repository::version() const {
	return version_;
}

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__
