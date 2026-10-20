// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TREE_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TREE_TMPL_H__

#include <bit>
#include <cstddef>
#include <vector>

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

//------------------------------------------------------------------------------
// traverser

inline tree::traverser::traverser() : has_value_(false) {}
inline tree::traverser::traverser(tref r) { set_values({ r }); }
inline tree::traverser::traverser(const tree& r) : traverser(r.get()) {}
inline tree::traverser::traverser(const htref& r)
	: traverser(r ? r->get() : nullptr) {}
inline tree::traverser::traverser(const trefs& n) { set_values(n); }

inline void tree::traverser::set_values(const trefs& n) {
	values_.clear();
	for (tref r : n) if (r != nullptr) values_.push_back(r);
	has_value_ = !values_.empty();
}

inline bool tree::traverser::has_value() const { return has_value_; }
inline tree::traverser::operator bool() const { return has_value(); }
inline tref tree::traverser::value() const { return values_.front(); }
inline const tree& tree::traverser::value_tree() const {
	return tree::get(values_.front());
}
inline const tree& tree::traverser::operator[](size_t n) const {
	return value_tree()[n];
}
inline const trefs& tree::traverser::values() const { return values_; }

inline std::vector<tree::traverser> tree::traverser::traversers() const {
	std::vector<traverser> tv;
	for (tref v : values_) tv.emplace_back(v);
	return tv;
}

inline std::vector<tree::traverser> tree::traverser::operator()() const {
	return traversers();
}

inline bool tree::traverser::is(size_t nt) const {
	return has_value() && value_tree().is(nt);
}
inline bool tree::traverser::empty() const { return !has_value(); }
inline size_t tree::traverser::size() const { return values_.size(); }

inline tree::traverser tree::traverser::operator|(size_t nt) const {
	if (!has_value()) return traverser();
	for (tref c : tree::get(value()).children())
		if (tree::get(c).is(nt)) return traverser(c);
	return traverser();
}

inline tree::traverser tree::traverser::operator||(size_t nt) const {
	trefs r;
	for (tref v : values())
		for (tref c : tree::get(v).children())
			if (tree::get(c).is(nt)) r.push_back(c);
	return traverser(r);
}

template <typename result_type>
result_type tree::traverser::operator|(const extractor<result_type>& e) const
{
	return e(*this);
}

template <typename result_type>
result_type tree::traverser::operator||(const extractor<result_type>& e)
	const
{
	return e(*this);
}

inline const tree::traverser::extractor<tref> tree::traverser::ref(
	[](const traverser& t) -> tref {
		return t ? t.value() : nullptr;
	});

inline const tree::traverser::extractor<trefs> tree::traverser::refs(
	[](const traverser& t) -> trefs { return t.values(); });

inline const tree::traverser::extractor<htref> tree::traverser::handle(
	[](const traverser& t) -> htref {
		return t ? tree::geth(t.value()) : htref();
	});

inline const tree::traverser::extractor<const tree&> tree::traverser::Tree(
	[](const traverser& t) -> const tree& { return t.value_tree(); });

inline const tree::traverser::extractor<tree::traverser>
	tree::traverser::dump(
		[](const traverser& t) -> traverser {
			if (!t) return t;
			t.value_tree().dump(std::cout) << "\n";
			return t;
		});

inline const tree::traverser::extractor<tree::type> tree::traverser::nt(
	[](const traverser& t) -> type {
		return t ? t.value_tree().get_type() : json_parser::nul;
	});

inline const tree::traverser::extractor<std::string>
	tree::traverser::string(
		[](const traverser& t) -> std::string {
			if (!t || !t.value_tree().is_string()) return {};
			return t.value_tree().get_string();
		});

inline const tree::traverser::extractor<double> tree::traverser::number(
	[](const traverser& t) -> double {
		if (!t || !t.value_tree().is_number()) return 0;
		return t.value_tree().get_number();
	});

inline const tree::traverser::extractor<bool> tree::traverser::boolean(
	[](const traverser& t) -> bool {
		return t && t.value_tree().is(json_parser::true_sym);
	});

inline const tree::traverser::extractor<tree::traverser>
	tree::traverser::only_child(
		[](const traverser& t) -> traverser {
			if (!t) return traverser();
			return traverser(t.value_tree().only_child());
		});

inline const tree::traverser::extractor<tree::traverser>
	tree::traverser::first(
		[](const traverser& t) -> traverser {
			if (!t) return traverser();
			return traverser(t.value_tree().first());
		});

inline const tree::traverser::extractor<tree::traverser>
	tree::traverser::second(
		[](const traverser& t) -> traverser {
			if (!t) return traverser();
			return traverser(t.value_tree().second());
		});

inline const tree::traverser::extractor<tree::traverser>
	tree::traverser::children(
		[](const traverser& t) -> traverser {
			trefs r;
			for (tref v : t.values())
				for (tref c : tree::get(v).children())
					r.push_back(c);
			return traverser(r);
		});

inline const tree::traverser::extractor<tref_range<node>>
	tree::traverser::children_range(
		[](const traverser& t) -> tref_range<node> {
			if (!t) return { nullptr };
			return t.value_tree().children();
		});

inline const tree::traverser::extractor<tree_range<tree>>
	tree::traverser::children_trees_range(
		[](const traverser& t) -> tree_range<tree> {
			if (!t) return { nullptr };
			return t.value_tree().children_trees();
		});

inline tree::traverser::extractor<tree::traverser> tree::traverser::member(
	std::string_view key)
{
	// the key is copied: the extractor can outlive the caller's view
	return extractor<traverser>([k = std::string(key)](const traverser& t)
		-> traverser
	{
		if (!t) return traverser();
		return traverser(t.value_tree().find(k));
	});
}

inline tree::traverser::extractor<tree::traverser> tree::traverser::f(
	const auto& fn)
{
	// the extractor can outlive the caller's callable
	return extractor<traverser>([fn](const traverser& t) -> traverser {
		trefs r;
		for (tref v : t.values())
			if (tref x = fn(v); x) r.push_back(x);
		return traverser(r);
	});
}

inline tree::traverser tree::operator()() const { return traverser(get()); }

//------------------------------------------------------------------------------
// free queries

inline bool is(tref n, size_t nt) {
	return n != nullptr && tree::get(n).is(nt);
}

inline std::function<bool(tref)> is(size_t nt) {
	return [nt](tref n) { return is(n, nt); };
}

inline bool is(tref n, std::initializer_list<size_t> nts) {
	for (size_t nt : nts) if (is(n, nt)) return true;
	return false;
}

inline std::function<bool(tref)> is(std::initializer_list<size_t> nts) {
	// an initializer_list does not own its elements
	return [v = std::vector<size_t>(nts)](tref n) {
		for (size_t nt : v) if (is(n, nt)) return true;
		return false;
	};
}

inline const std::string* string_member(tref object, std::string_view key) {
	if (object == nullptr) return nullptr;
	tref v = tree::get(object).find(key);
	if (v == nullptr || !tree::get(v).is_string()) return nullptr;
	return &tree::get(v).get_string();
}

} // namespace idni::format::json

#endif // __IDNI__PARSER__FORMAT__JSON_TREE_TMPL_H__
