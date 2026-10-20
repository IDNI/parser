// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_TYPES_TMPL_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_TYPES_TMPL_H__

#include <algorithm>
#include <cctype>
#include <charconv>
#include <system_error>

#include "options_types.h"
#include "characters.h"
#include "escapes.h"
#include "../parser_strings.h"

namespace idni {

inline option_kind kind_of(const option_value& v) {
	return static_cast<option_kind>(v.index());
}

inline option_kind kind_of(const option_spec& s) {
	return kind_of(s.fallback);
}

namespace detail {

inline std::string option_trim(std::string_view s) {
	std::size_t b = 0, e = s.size();
	while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
	while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
	return std::string(s.substr(b, e - b));
}

inline std::size_t option_quote_end(std::string_view s, std::size_t i) {
	for (++i; i < s.size(); ++i) {
		if (s[i] == '\\') ++i;
		else if (s[i] == '"') return i + 1;
	}
	return std::string_view::npos;
}

inline std::optional<std::string> option_unquote(std::string_view quoted,
	diagnostics::report& rep)
{
	auto dec = escapes::decode(quoted.substr(1, quoted.size() - 2),
		escapes::tgf_string);
	if (!dec.has_value()) {
		rep.append(std::move(dec.report()));
		return std::nullopt;
	}
	return std::move(dec).value();
}

inline std::optional<option_list> option_list_entries(std::string_view s,
	diagnostics::report& rep)
{
	auto is_space = [](char c) {
		return std::isspace(static_cast<unsigned char>(c)) != 0;
	};
	option_list out;
	for (std::size_t i = 0;;) {
		while (i < s.size() && is_space(s[i])) ++i;
		std::size_t comma;
		if (i < s.size() && s[i] == '"') {
			const std::size_t end = option_quote_end(s, i);
			if (end == std::string_view::npos) return std::nullopt;
			auto entry = option_unquote(s.substr(i, end - i), rep);
			if (!entry) return std::nullopt;
			out.push_back(std::move(*entry));
			for (comma = end; comma < s.size() && is_space(s[comma]);)
				++comma;
			if (comma < s.size() && s[comma] != ',') return std::nullopt;
		} else {
			comma = std::min(s.find(',', i), s.size());
			auto part = option_trim(s.substr(i, comma - i));
			if (!part.empty()) out.push_back(std::move(part));
		}
		if (comma >= s.size()) return out;
		i = comma + 1;
	}
}

inline diagnostics::result<option_value> parse_value(option_kind kind,
	std::string_view text, std::string_view name)
{
	namespace ps = parser_strings;
	diagnostics::result<option_value> r;
	const std::string trimmed = option_trim(text);
	switch (kind) {
	case option_kind::flag: {
		const std::string w = ascii_to_lower(trimmed);
		if (w == "true" || w == "on" || w == "1" || w == "yes"
			|| w == "y" || w == "t") return r.with_value(option_value(true));
		if (w == "false" || w == "off" || w == "0" || w == "no"
			|| w == "n" || w == "f") return r.with_value(option_value(false));
		break;
	}
	case option_kind::numeric: {
		std::size_t v = 0;
		const char* end = trimmed.data() + trimmed.size();
		auto res = std::from_chars(trimmed.data(), end, v);
		if (!trimmed.empty() && res.ec == std::errc{} && res.ptr == end)
			return r.with_value(option_value(v));
		break;
	}
	case option_kind::text:
		if (trimmed.empty() || trimmed[0] != '"')
			return r.with_value(option_value(trimmed));
		if (option_quote_end(trimmed, 0) == trimmed.size())
			if (auto v = option_unquote(trimmed, r.report()))
				return r.with_value(option_value(std::move(*v)));
		break;
	case option_kind::list:
		if (auto v = option_list_entries(trimmed, r.report()))
			return r.with_value(option_value(std::move(*v)));
		break;
	}
	r.error(diagnostics::code::invalid_argument,
		ps::messages::option_bad_value,
		{ { ps::label::name, name }, { ps::label::value, text } });
	return r;
}

inline bool option_reads_bare(std::string_view v, bool in_list) {
	if (v.empty()) return !in_list;
	if (in_list && v.find(',') != std::string_view::npos) return false;
	for (char c : v) {
		const auto u = static_cast<unsigned char>(c);
		if (c == '"' || c == '\\' || u < 0x20 || u == 0x7F) return false;
	}
	return !std::isspace(static_cast<unsigned char>(v.front()))
		&& !std::isspace(static_cast<unsigned char>(v.back()));
}

inline std::string option_quote(std::string_view v, bool in_list) {
	if (option_reads_bare(v, in_list)) return std::string(v);
	return "\"" + escapes::encode(v, escapes::tgf_string) + "\"";
}

inline std::string format_value(const option_value& v) {
	switch (kind_of(v)) {
	case option_kind::flag: return std::get<bool>(v) ? "true" : "false";
	case option_kind::numeric:
		return std::to_string(std::get<std::size_t>(v));
	case option_kind::text:
		return option_quote(std::get<std::string>(v), false);
	case option_kind::list: {
		std::string out;
		for (const auto& s : std::get<option_list>(v)) {
			if (!out.empty()) out += ", ";
			out += option_quote(s, true);
		}
		return out;
	}
	}
	return {};
}

} // namespace detail

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__OPTIONS_TYPES_TMPL_H__
