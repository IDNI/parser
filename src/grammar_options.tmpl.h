// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__GRAMMAR_OPTIONS_TMPL_H__
#define __IDNI__PARSER__GRAMMAR_OPTIONS_TMPL_H__

#include <algorithm>
#include <iterator>
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

/// The entries of @p v, or an error in @p rep when the value is no list.
inline const option_list* option_list_of(const option_value& v,
	diagnostics::report& rep)
{
	if (const auto* l = std::get_if<option_list>(&v)) return l;
	rep.error(diagnostics::code::type_error,
		parser_strings::messages::option_value_kind);
	return nullptr;
}

/// An error in @p rep that names the option and the entry it rejects.
inline void option_bad_entry(diagnostics::report& rep,
	std::string_view option, const std::string& entry)
{
	namespace ps = parser_strings;
	rep.error(diagnostics::code::invalid_argument,
		ps::messages::option_bad_value,
		{ { ps::label::name, option }, { ps::label::value, entry } });
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

inline constexpr std::string_view char_class_as = " as ";

template <typename C, typename T>
option_value char_classes_codec<C, T>::to_value(
	const char_class_fns<T>& cc) const
{
	option_list out;
	for (const auto& [id, fn] : cc.fns) {
		if (cc.is_derived(id)) continue;
		std::string name = to_std_string(nts->get(id));
		// the empty nonterminal holds the class that matches nothing
		if (name.empty()) continue;
		if (auto it = cc.renamed.find(id); it != cc.renamed.end())
			name = it->second + std::string(char_class_as) + name;
		out.push_back(std::move(name));
	}
	return out;
}

template <typename C, typename T>
diagnostics::result<char_class_fns<T>> char_classes_codec<C, T>::from_value(
	const option_value& v) const
{
	diagnostics::result<char_class_fns<T>> r;
	const option_list* entries = option_list_of(v, r.report());
	if (!entries) return r;
	std::vector<std::string> names;
	std::map<std::string, std::string> renames;
	for (const std::string& entry : *entries) {
		const size_t as = entry.find(char_class_as);
		if (as == std::string::npos) {
			names.push_back(entry);
			continue;
		}
		const std::string_view e(entry);
		std::string cc = option_trim(e.substr(0, as));
		std::string name = option_trim(
			e.substr(as + char_class_as.size()));
		if (cc.empty() || name.empty()
			|| !renames.emplace(cc, std::move(name)).second)
		{
			option_bad_entry(r.report(), "char-classes", entry);
			continue;
		}
		names.push_back(std::move(cc));
	}
	if (r.has_error()) return r;
	auto cc = predefined_char_classes<C, T>(names, renames, *nts,
		&r.report());
	if (r.has_error()) return r;
	return r.with_value(std::move(cc));
}

inline option_value highlights_codec::to_value(const pairs& hs) const {
	option_list out;
	for (const auto& [type, names] : hs)
		for (const auto& name : names) out.push_back(type + ": " + name);
	return out;
}

inline diagnostics::result<highlights_codec::pairs>
	highlights_codec::from_value(const option_value& v) const
{
	diagnostics::result<pairs> r;
	const option_list* entries = option_list_of(v, r.report());
	if (!entries) return r;
	pairs out;
	for (const std::string& entry : *entries) {
		// a pattern may hold a colon, a type never does
		const size_t colon = entry.find(':');
		std::string type = option_trim(
			std::string_view(entry).substr(0, colon));
		std::string name = colon == std::string::npos ? std::string()
			: option_trim(std::string_view(entry).substr(colon + 1));
		if (name.empty() || std::find(highlight_token_types.begin(),
			highlight_token_types.end(), type)
				== highlight_token_types.end())
		{
			option_bad_entry(r.report(), "highlight", entry);
			continue;
		}
		auto it = std::find_if(out.begin(), out.end(),
			[&type](const auto& p) { return p.first == type; });
		if (it == out.end()) {
			out.emplace_back(std::move(type), option_list{});
			it = std::prev(out.end());
		}
		it->second.push_back(std::move(name));
	}
	if (r.has_error()) return r;
	return r.with_value(std::move(out));
}

template <typename C>
option_value dynamic_codec<C>::to_value(const values& m) const {
	option_list out;
	for (const auto& [name, vals] : m) {
		const std::string head = to_std_string(name) + "=";
		if (vals.empty()) out.push_back(head);
		for (const auto& val : vals)
			out.push_back(head + to_std_string(val));
	}
	return out;
}

template <typename C>
diagnostics::result<typename dynamic_codec<C>::values>
	dynamic_codec<C>::from_value(const option_value& v) const
{
	diagnostics::result<values> r;
	const option_list* entries = option_list_of(v, r.report());
	if (!entries) return r;
	values out;
	for (const std::string& entry : *entries) {
		const size_t eq = entry.find('=');
		std::string name = option_trim(
			std::string_view(entry).substr(0, eq));
		if (eq == std::string::npos || name.empty()) {
			option_bad_entry(r.report(), "dynamic", entry);
			continue;
		}
		auto& vals = out[from_str<C>(name)];
		// `name=` declares the name and gives it no value
		if (eq + 1 == entry.size()) continue;
		auto val = from_str<C>(entry.substr(eq + 1));
		if (std::find(vals.begin(), vals.end(), val) == vals.end())
			vals.push_back(std::move(val));
	}
	if (r.has_error()) return r;
	return r.with_value(std::move(out));
}

inline option_value variants_codec::to_value(const profiles& m) const {
	option_list out;
	for (const auto& [name, guards] : m)
		for (const auto& guard : guards) out.push_back(name + "=" + guard);
	return out;
}

inline diagnostics::result<variants_codec::profiles>
	variants_codec::from_value(const option_value& v) const
{
	diagnostics::result<profiles> r;
	const option_list* entries = option_list_of(v, r.report());
	if (!entries) return r;
	profiles out;
	for (const std::string& entry : *entries) {
		const size_t eq = entry.find('=');
		std::string name = option_trim(
			std::string_view(entry).substr(0, eq));
		std::string guard = eq == std::string::npos ? std::string()
			: option_trim(std::string_view(entry).substr(eq + 1));
		if (name.empty() || guard.empty()) {
			option_bad_entry(r.report(), "variants", entry);
			continue;
		}
		out[name].insert(std::move(guard));
	}
	if (r.has_error()) return r;
	return r.with_value(std::move(out));
}

inline option_value variant_codec::to_value(const std::string& name) const {
	return name;
}

inline diagnostics::result<std::string> variant_codec::from_value(
	const option_value& v) const
{
	namespace ps = parser_strings;
	diagnostics::result<std::string> r;
	const std::string* name = option_text_of(v, r.report());
	if (!name) return r;
	if (!name->empty() && !variants->contains(*name))
		return r.with_error(diagnostics::code::not_found,
			ps::messages::unknown_variant,
			{ { ps::label::name, *name } });
	return r.with_value(*name);
}

} // namespace detail

} // namespace idni

#endif // __IDNI__PARSER__GRAMMAR_OPTIONS_TMPL_H__
