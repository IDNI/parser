// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TREE_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TREE_TMPL_H__

#include <bit>
#include <cstddef>

#include "json.h"

namespace idni::format::json {

//------------------------------------------------------------------------------
// node

inline node::node(type nt, uint64_t data, std::string str)
	: nt(nt), data(data), str(std::move(str)), hash(hashit()) {}

inline const std::string& node::name(size_t nt) {
	return json_parser_data::nts().get(nt);
}

inline const std::string& node::name() const { return name(nt); }

inline size_t node::get_nt() const { return static_cast<size_t>(nt); }

inline uint64_t node::hashit() const {
	std::uint64_t seed = 0;
	hash_combine(seed, static_cast<size_t>(nt), data, str);
	return seed;
}

inline std::weak_ordering node::operator<=>(const node& that) const {
	if (auto c = hash <=> that.hash; c != 0) return c;
	if (auto c = nt <=> that.nt; c != 0) return c;
	if (auto c = data <=> that.data; c != 0) return c;
	return str.compare(that.str) <=> 0;
}

inline bool node::operator==(const node& that) const {
	return hash == that.hash && nt == that.nt && data == that.data
		&& str == that.str;
}

//------------------------------------------------------------------------------
// direct tree API

inline tref tree::get() const { return base_t::get(); }

inline const tree& tree::get(const tref id) {
	return static_cast<const tree&>(base_t::get(id));
}

inline const tree& tree::get(const htref& h) {
	return static_cast<const tree&>(base_t::get(h));
}

inline htref tree::geth(tref id) { return base_t::geth(id); }

inline size_t tree::m_size() { return bintree<node>::node_count(); }

inline tref tree::get(const node& v) { return base_t::get(v); }

inline tref tree::get(const node& v, tref ch) { return base_t::get(v, ch); }

inline tref tree::get(const node& v, tref ch1, tref ch2) {
	return base_t::get(v, ch1, ch2);
}

inline tref tree::get(const node& v, const trefs& ch, tref r) {
	return base_t::get(v, ch, r);
}

inline tref tree::get(type nt) { return get(node(nt)); }

inline tref tree::get(type nt, const trefs& ch, tref r) {
	return get(node(nt), ch, r);
}

//------------------------------------------------------------------------------
// children

inline tref_range<node> tree::children() const { return base_t::children(); }

inline tree_range<tree> tree::children_trees() const {
	return tree_range<tree>(this->l);
}

inline const tree& tree::operator[](size_t n) const { return child_tree(n); }

inline const tree& tree::child_tree(size_t n) const {
	return get(base_t::child(n));
}

inline const tree& tree::first_tree() const { return child_tree(0); }

inline const tree& tree::second_tree() const { return child_tree(1); }

inline const tree& tree::only_child_tree() const {
	return get(base_t::only_child());
}

//------------------------------------------------------------------------------
// fast access helpers

inline bool tree::is(size_t nt) const { return value.get_nt() == nt; }
inline bool tree::is_null()   const { return is(json_parser::null_sym); }
inline bool tree::is_bool()   const {
	return is(json_parser::true_sym) || is(json_parser::false_sym);
}
inline bool tree::is_number() const { return is(json_parser::number); }
inline bool tree::is_string() const { return is(json_parser::str); }
inline bool tree::is_array()  const { return is(json_parser::arr); }
inline bool tree::is_object() const { return is(json_parser::object); }

inline tree::type tree::get_type() const { return value.nt; }

inline bool tree::get_bool() const {
	DBG(assert(is_bool());)
	return is(json_parser::true_sym);
}

inline double tree::get_number() const {
	DBG(assert(is_number());)
	return std::bit_cast<double>(value.data);
}

inline const std::string& tree::get_string() const {
	DBG(assert(is_string() || is(json_parser::object_pair));)
	return value.str;
}

inline const tree& tree::value_tree() const {
	DBG(assert(is(json_parser::object_pair));)
	return only_child_tree();
}

inline tref tree::find(std::string_view key) const {
	if (!is_object()) return nullptr;
	for (const tree& p : children_trees())
		if (p.value.str == key) return p.first();
	return nullptr;
}

} // namespace idni::format::json

#endif // __IDNI__PARSER__FORMAT__JSON_TREE_TMPL_H__
