// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__GRAMMAR_OPTIONS_H__
#define __IDNI__PARSER__GRAMMAR_OPTIONS_H__

#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "utility/options.h"

namespace idni {

template <typename C, typename T> struct nonterminals;

/// The options of a grammar. The directive names are the option names.
inline const option_set grammar_option_set = { .options = {
	{ "auto-disambiguate", "grammar", true,
		"enables auto-disambiguation", {} },
	{ "nodisambig-list", "grammar", option_list{},
		"nonterminals whose ambiguity is kept", {} },
	{ "trim-terminals", "grammar", false, "trim all terminals", {} },
	{ "inline-char-classes", "grammar", false,
		"inline character classes", {} },
	{ "derive-char-classes", "grammar", true,
		"scan rules that match one character as character classes", {} },
	{ "trim", "grammar", option_list{},
		"nonterminals to trim from the tree", {} },
	{ "trim-children", "grammar", option_list{},
		"nonterminals whose children are trimmed", {} },
	{ "trim-children-terminals", "grammar", option_list{},
		"nonterminals whose children and terminals are trimmed", {} },
	{ "dont-trim-terminals-of", "grammar", option_list{},
		"nonterminals whose terminals are kept", {} },
	{ "inline", "grammar", option_list{},
		"tree paths to inline, each written as a > b", {} },
	{ "enabled-productions", "grammar", option_list{},
		"comma-separated enabled productions", {} },
} };

namespace detail {

/// The id of the nonterminal @p name, or an error in @p rep when the
/// grammar has no nonterminal of that name.
template <typename C, typename T>
std::optional<size_t> nonterminal_id(const nonterminals<C, T>& nts,
	const std::string& name, diagnostics::report& rep);

/// A set of nonterminal ids, as names in the option.
template <typename C, typename T>
struct nonterminal_ids_codec {
	const nonterminals<C, T>* nts;
	option_value to_value(const std::set<size_t>& ids) const;
	diagnostics::result<std::set<size_t>> from_value(
		const option_value& v) const;
};

/// A set of id paths, as entries `a > b` in the option.
template <typename C, typename T>
struct nonterminal_paths_codec {
	const nonterminals<C, T>* nts;
	option_value to_value(const std::set<std::vector<size_t>>& paths) const;
	diagnostics::result<std::set<std::vector<size_t>>> from_value(
		const option_value& v) const;
};

/// A set of strings, as a list.
struct string_set_codec {
	option_value to_value(const std::set<std::string>& s) const;
	diagnostics::result<std::set<std::string>> from_value(
		const option_value& v) const;
};

} // namespace detail

} // namespace idni

#include "grammar_options.tmpl.h"

#endif // __IDNI__PARSER__GRAMMAR_OPTIONS_H__
