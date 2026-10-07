// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__PARSER_OPTIONS_H__
#define __IDNI__PARSER__PARSER_OPTIONS_H__

#include <cstddef>
#include <string>

#include "utility/options.h"

namespace idni {

template <typename C, typename T> struct nonterminals;
template <typename C, typename T> class parser;

/// The options of a parse. The defaults are those of parse_options.
// parser.h includes this file after it defines the two DEFAULT_ macros.
inline const option_set parser_option_set = { .options = {
	{ "gc", "parse", bool(DEFAULT_ENABLE_GC),
		"enable garbage collection" },
	{ "gc-lag", "parse", size_t(DEFAULT_GC_LAG),
		"garbage collection lag" },
	{ "max-length", "parse", size_t(0),
		"maximum input length (0 = unlimited)" },
	{ "measure", "parse", false, "measure parse time and counters" },
	{ "measure-each-pos", "parse", false,
		"measure parse time at each position" },
	{ "measure-forest", "parse", false, "measure forest building time" },
	{ "measure-preprocess", "parse", false,
		"measure forest preprocess time" },
	{ "debug", "parse", false, "print debug diagnostics" },
	{ "error-verbosity", "parse", std::string("detailed"),
		"parse error verbosity: basic, detailed, root-cause" },
	{ "tree-path", "parse", std::string("bintree"),
		"parse tree path: bintree or forest" },
	{ "start", "parse", std::string(), "starting literal" },
} };

namespace detail {

/// A nonterminal id, as its name in the option. SIZE_MAX is the empty
/// text.
template <typename C, typename T>
struct nonterminal_id_codec {
	const nonterminals<C, T>* nts;
	option_value to_value(const size_t& id) const;
	diagnostics::result<size_t> from_value(const option_value& v) const;
};

/// The parse error verbosity, as basic, detailed or root-cause.
template <typename C, typename T>
struct error_verbosity_codec {
	using level = typename parser<C, T>::error::info_lvl;
	option_value to_value(const level& l) const;
	diagnostics::result<level> from_value(const option_value& v) const;
};

/// The parse tree path, as bintree or forest.
struct tree_path_codec {
	option_value to_value(const parse_tree_path& p) const;
	diagnostics::result<parse_tree_path> from_value(
		const option_value& v) const;
};

} // namespace detail

} // namespace idni

#include "parser_options.tmpl.h"

#endif // __IDNI__PARSER__PARSER_OPTIONS_H__
