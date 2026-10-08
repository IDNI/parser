// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__GRAMMAR_OPTIONS_H__
#define __IDNI__PARSER__GRAMMAR_OPTIONS_H__

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "utility/options.h"

namespace idni {

template <typename C, typename T> struct nonterminals;
template <typename C, typename T> struct lit;
template <typename T> struct char_class_fns;

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
	{ "start", "grammar", std::string("start"), "starting literal", {} },
	{ "char-classes", "grammar", option_list{},
		"character classes of the grammar, an entry is a class or "
		"class as name", {} },
	{ "highlight-auto", "grammar", false,
		"highlight by name and content heuristics", {} },
	{ "highlight", "grammar", option_list{},
		"highlight types, each entry written as type: nonterminal", {} },
	{ "dynamic", "grammar", option_list{},
		"default values for a @dynamic nonterminal, each entry written "
		"as name=value", {} },
} };

/// @highlight token type legend, in index order.
constexpr std::array<std::string_view, 8> highlight_token_types = {
	"keyword", "operator", "string", "comment",
	"number", "type", "variable", "delimiter",
};

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

/// The start literal, as the name of its nonterminal.
template <typename C, typename T>
struct start_codec {
	const nonterminals<C, T>* nts;
	option_value to_value(const lit<C, T>& start) const;
	diagnostics::result<lit<C, T>> from_value(const option_value& v) const;
};

/// The predefined classes, as entries `digit` or `digit as d`. A derived
/// class is no entry.
template <typename C, typename T>
struct char_classes_codec {
	// not const, because a write adds the names of the classes
	nonterminals<C, T>* nts;
	option_value to_value(const char_class_fns<T>& cc) const;
	diagnostics::result<char_class_fns<T>> from_value(
		const option_value& v) const;
};

/// The highlight pairs, as entries `type: name`, one per name.
struct highlights_codec {
	using pairs = std::vector<std::pair<std::string,
		std::vector<std::string>>>;
	option_value to_value(const pairs& hs) const;
	diagnostics::result<pairs> from_value(const option_value& v) const;
};

/// The values of the @dynamic nonterminals, as entries `name=value`, one
/// per value. `name=` is a name with no value.
template <typename C>
struct dynamic_codec {
	using values = std::map<std::basic_string<C>,
		std::vector<std::basic_string<C>>>;
	option_value to_value(const values& m) const;
	diagnostics::result<values> from_value(const option_value& v) const;
};

} // namespace detail

} // namespace idni

#include "grammar_options.tmpl.h"

#endif // __IDNI__PARSER__GRAMMAR_OPTIONS_H__
