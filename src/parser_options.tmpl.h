// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__PARSER_OPTIONS_TMPL_H__
#define __IDNI__PARSER__PARSER_OPTIONS_TMPL_H__

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "parser_options.h"
#include "grammar_options.h"
#include "parser_strings.h"

namespace idni {

namespace detail {

/// The value that the text @p v names in @p names, or an error that
/// names the text.
template <typename E, size_t N>
diagnostics::result<E> option_enum_of(
	const std::array<std::pair<std::string_view, E>, N>& names,
	const option_value& v, std::string_view option)
{
	namespace ps = parser_strings;
	diagnostics::result<E> r;
	const std::string* text = option_text_of(v, r.report());
	if (!text) return r;
	for (const auto& [name, e] : names)
		if (name == *text) return r.with_value(e);
	return r.with_error(diagnostics::code::invalid_argument,
		ps::messages::option_bad_value,
		{ { ps::label::name, option }, { ps::label::value, *text } });
}

template <typename E, size_t N>
std::string option_enum_name(
	const std::array<std::pair<std::string_view, E>, N>& names, E e)
{
	for (const auto& [name, x] : names)
		if (x == e) return std::string(name);
	return {};
}

template <typename C, typename T>
auto error_verbosity_names() {
	using level = typename error_verbosity_codec<C, T>::level;
	return std::array<std::pair<std::string_view, level>, 3>{ {
		{ "basic", level::INFO_BASIC },
		{ "detailed", level::INFO_DETAILED },
		{ "root-cause", level::INFO_ROOT_CAUSE },
	} };
}

template <typename C, typename T>
option_value error_verbosity_codec<C, T>::to_value(const level& l) const {
	return option_enum_name(error_verbosity_names<C, T>(), l);
}

template <typename C, typename T>
diagnostics::result<typename error_verbosity_codec<C, T>::level>
	error_verbosity_codec<C, T>::from_value(const option_value& v) const
{
	return option_enum_of(error_verbosity_names<C, T>(), v,
		"error-verbosity");
}

inline constexpr std::array<std::pair<std::string_view, parse_tree_path>, 2>
	tree_path_names{ {
		{ "bintree", parse_tree_path::bintree_path },
		{ "forest", parse_tree_path::forest_path },
	} };

inline option_value tree_path_codec::to_value(const parse_tree_path& p) const
{
	return option_enum_name(tree_path_names, p);
}

inline diagnostics::result<parse_tree_path> tree_path_codec::from_value(
	const option_value& v) const
{
	return option_enum_of(tree_path_names, v, "tree-path");
}

} // namespace detail

} // namespace idni

#endif // __IDNI__PARSER__PARSER_OPTIONS_TMPL_H__
