// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__AST_JSON_H__
#define __IDNI__PARSER__FORMAT__AST_JSON_H__

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../json/json.h"
#include "../../utility/characters.h"

namespace idni::format::ast_json {

/// The AST JSON object of one parser tree node. A nonterminal carries its
/// number as id. A terminal leaf carries "" as its symbol and its
/// characters as text. Null children are skipped. The walk uses no
/// recursion, so a deep parse tree converts. @p tree_t is the parser's
/// tree type.
template <typename tree_t>
tref node_to_tree(tref n) {
	using jt = json::tree;
	using pnode_t = std::remove_cvref_t<decltype(tree_t::get(n).value)>;
	// equal parser subtrees are visited once and share one JSON object
	subtree_unordered_map<pnode_t, tref> m;
	auto is_kept = [](tref c, tref parent = nullptr) {
		return parent == nullptr
			|| !tree_t::get(c).value.first.is_null();
	};
	auto build = [&m](tref p) {
		const auto& t = tree_t::get(p);
		const auto& l = t.value.first;
		trefs f{ jt::build_pair("symbol",
			jt::build_string(l.nt() ? l.to_std_string() : "")) };
		if (l.nt()) f.push_back(jt::build_pair("id",
			jt::build_number(static_cast<double>(l.n()))));
		const auto& [b, e] = t.value.second;
		f.push_back(jt::build_pair("range", jt::build_array({
			jt::build_number(static_cast<double>(b)),
			jt::build_number(static_cast<double>(e)) })));
		if (!l.nt()) f.push_back(jt::build_pair("text",
			jt::build_string(to_std_string(l.t()))));
		else {
			trefs ch;
			for (tref c : t.children())
				if (!tree_t::get(c).value.first.is_null())
					ch.push_back(m.find(c)->second);
			if (!ch.empty()) f.push_back(jt::build_pair("children",
				jt::build_array(ch)));
		}
		m.emplace(p, jt::build_object(f));
	};
	post_order<pnode_t>(n).search_unique(build, is_kept);
	return m.find(n)->second;
}

/// The AST JSON document of a parser tree. @p start is the start symbol
/// name and @p input is the parsed text.
template <typename tree_t>
tref to_tree(tref root, std::string_view start, std::string_view input) {
	using jt = json::tree;
	return jt::build_object({
		jt::build_pair("format", jt::build_string("ast")),
		jt::build_pair("version", jt::build_string("1")),
		jt::build_pair("start", jt::build_string(start)),
		jt::build_pair("input", jt::build_string(input)),
		jt::build_pair("ast", node_to_tree<tree_t>(root)) });
}

/// Convert one parser tree node into an AST JSON object. A nonterminal
/// carries its number as id. A terminal leaf carries "" as its symbol
/// and its characters as text. A caller skips a null node, so this never
/// sees one. @p tree_t is the parser's tree type.
template <typename tree_t>
json::value node_to_value(tref n) {
	const auto& t = tree_t::get(n);
	const auto& l = t.value.first;
	auto v = json::value::object();
	v.set("symbol", json::value::string(l.nt() ? l.to_std_string() : ""));
	if (l.nt()) v.set("id", json::value::number(
		static_cast<double>(l.n())));
	auto range = json::value::array();
	range.push_back(json::value::number(
		static_cast<double>(t.value.second[0])));
	range.push_back(json::value::number(
		static_cast<double>(t.value.second[1])));
	v.set("range", std::move(range));
	if (!l.nt()) {
		v.set("text", json::value::string(to_std_string(l.t())));
		return v;
	}
	auto ch = json::value::array();
	for (tref c : t.children())
		if (!tree_t::get(c).value.first.is_null())
			ch.push_back(node_to_value<tree_t>(c));
	if (ch.size()) v.set("children", std::move(ch));
	return v;
}

/// Convert a parser tree into an AST JSON document. @p start is the start
/// symbol name and @p input is the parsed text.
template <typename tree_t>
json::value to_value(tref root, std::string_view start,
	std::string_view input)
{
	auto v = json::value::object();
	v.set("format", json::value::string("ast"))
	 .set("version", json::value::string("1"))
	 .set("start", json::value::string(std::string(start)))
	 .set("input", json::value::string(std::string(input)))
	 .set("ast", node_to_value<tree_t>(root));
	return v;
}

} // namespace idni::format::ast_json

#endif // __IDNI__PARSER__FORMAT__AST_JSON_H__
