// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__GRAMMAR_OPTIONS_TMPL_H__
#define __IDNI__PARSER__GRAMMAR_OPTIONS_TMPL_H__

#include <optional>
#include <string_view>

#include "grammar_options.h"
#include "parser_strings.h"
#include "utility/characters.h"

namespace idni {

namespace detail {

/// The text of @p v, or an error in @p rep when the value is no text.
inline const std::string* option_text_of(const option_value& v,
	diagnostics::report& rep)
{
	if (const auto* s = std::get_if<std::string>(&v)) return s;
	rep.error(diagnostics::code::type_error,
		parser_strings::messages::option_value_kind);
	return nullptr;
}

// find() and never get(), so a name the grammar lacks adds no nonterminal
template <typename C, typename T>
std::optional<size_t> nonterminal_id(const nonterminals<C, T>& nts,
	const std::string& name, diagnostics::report& rep)
{
	if (auto id = nts.find(from_str<C>(name))) return id;
	rep.error(diagnostics::code::invalid_argument,
		parser_strings::messages::invalid_symbol,
		{ { parser_strings::label::name, name } });
	return std::nullopt;
}

template <typename C, typename T>
option_value nonterminal_ids_codec<C, T>::to_value(
	const std::set<size_t>& ids) const
{
	option_list out;
	for (size_t id : ids) out.push_back(to_std_string(nts->get(id)));
	return out;
}

template <typename C, typename T>
diagnostics::result<std::set<size_t>> nonterminal_ids_codec<C, T>::from_value(
	const option_value& v) const
{
	diagnostics::result<std::set<size_t>> r;
	const auto* names = std::get_if<option_list>(&v);
	if (!names) return r.with_error(diagnostics::code::type_error,
		parser_strings::messages::option_value_kind);
	std::set<size_t> out;
	bool ok = true;
	for (const auto& name : *names)
		if (auto id = nonterminal_id(*nts, name, r.report()))
			out.insert(*id);
		else ok = false;
	if (!ok) return r;
	return r.with_value(std::move(out));
}

template <typename C, typename T>
option_value nonterminal_paths_codec<C, T>::to_value(
	const std::set<std::vector<size_t>>& paths) const
{
	option_list out;
	for (const auto& path : paths) {
		std::string entry;
		for (size_t id : path) {
			if (!entry.empty()) entry += " > ";
			entry += to_std_string(nts->get(id));
		}
		out.push_back(std::move(entry));
	}
	return out;
}

template <typename C, typename T>
diagnostics::result<std::set<std::vector<size_t>>>
	nonterminal_paths_codec<C, T>::from_value(const option_value& v) const
{
	diagnostics::result<std::set<std::vector<size_t>>> r;
	const auto* entries = std::get_if<option_list>(&v);
	if (!entries) return r.with_error(diagnostics::code::type_error,
		parser_strings::messages::option_value_kind);
	std::set<std::vector<size_t>> out;
	bool ok = true;
	for (const std::string& entry : *entries) {
		std::vector<size_t> path;
		for (size_t start = 0;;) {
			const size_t gt = entry.find('>', start);
			const std::string name = option_trim(
				std::string_view(entry).substr(start, gt - start));
			if (auto id = nonterminal_id(*nts, name, r.report()))
				path.push_back(*id);
			else ok = false;
			if (gt == std::string::npos) break;
			start = gt + 1;
		}
		out.insert(std::move(path));
	}
	if (!ok) return r;
	return r.with_value(std::move(out));
}

inline option_value string_set_codec::to_value(
	const std::set<std::string>& s) const
{
	return option_list(s.begin(), s.end());
}

inline diagnostics::result<std::set<std::string>> string_set_codec::from_value(
	const option_value& v) const
{
	diagnostics::result<std::set<std::string>> r;
	const auto* list = std::get_if<option_list>(&v);
	if (!list) return r.with_error(diagnostics::code::type_error,
		parser_strings::messages::option_value_kind);
	return r.with_value(std::set<std::string>(list->begin(), list->end()));
}

template <typename C, typename T>
option_value start_codec<C, T>::to_value(const lit<C, T>& start) const {
	return to_std_string(nts->get(start.n()));
}

template <typename C, typename T>
diagnostics::result<lit<C, T>> start_codec<C, T>::from_value(
	const option_value& v) const
{
	diagnostics::result<lit<C, T>> r;
	const std::string* name = option_text_of(v, r.report());
	if (!name) return r;
	// the empty nonterminal holds the class that matches nothing
	if (name->empty())
		return r.with_error(diagnostics::code::invalid_argument,
			parser_strings::messages::invalid_symbol,
			{ { parser_strings::label::name, *name } });
	if (auto id = nonterminal_id(*nts, *name, r.report()))
		return r.with_value(lit<C, T>(*id, nts));
	return r;
}

} // namespace detail

} // namespace idni

#endif // __IDNI__PARSER__GRAMMAR_OPTIONS_TMPL_H__
