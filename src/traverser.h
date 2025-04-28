// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#ifndef __IDNI__PARSER__TRAVERSER_H__
#define __IDNI__PARSER__TRAVERSER_H__
#include <ranges>
#include "parser.h"
#include "rewriting.h"

namespace idni {

template <typename node_variant_t, typename parser_t>
struct traverser;

template <typename node_variant_t, typename parser_t,
	typename result_t = traverser<node_variant_t, parser_t>>
struct extractor {
	using traverser_t = traverser<node_variant_t, parser_t>;
	using extractor_function = std::function<result_t(const traverser_t&)>;
	extractor(const extractor_function& e) : e(e) {}
	result_t operator()(const traverser_t& t) const { return e(t); }
	extractor_function e;
};

template <typename node_variant_t, typename parser_t>
using filter = extractor<node_variant_t, parser_t>;

template <typename node_variant_t, typename parser_t>
const auto children_extractor = extractor<node_variant_t, parser_t>(
	[](const traverser<node_variant_t, parser_t>& t) {
		using node_t = idni::rewriter::sp_node<node_variant_t>;
		if (!t.has_value())
			return traverser<node_variant_t, parser_t>();
		std::vector<node_t> nv;
		for (const auto& n : t.values())
			for (const auto& c : n->child) nv.push_back(c);
		return traverser<node_variant_t, parser_t>(nv);
	});

template <typename node_variant_t, typename parser_t>
const auto only_child_extractor = extractor<node_variant_t, parser_t>(
	[](const traverser<node_variant_t, parser_t>& t) {
		using node_t = idni::rewriter::sp_node<node_variant_t>;
		if (!t.has_value())
			return traverser<node_variant_t, parser_t>();
		std::vector<node_t> nv;
		for (const auto& n : t.values())
			if (n->child.size() == 1)
				nv.push_back(n->child.front());
		return traverser<node_variant_t, parser_t>(nv);
	});

template <typename node_variant_t, typename parser_t>
const auto terminal_extractor = extractor<node_variant_t, parser_t,std::string>(
	[](const traverser<node_variant_t, parser_t>& t) {
		using node_t = idni::rewriter::sp_node<node_variant_t>;
		using symbol_t = typename parser_t::symbol_type;
		if (!t.has_value()) return std::string();
		std::vector<char> nv;
		std::function<void(const node_t&)> extract_terminal;
		extract_terminal = [&nv, &extract_terminal](const node_t& n) {
			if (std::holds_alternative<symbol_t>(n->value)) {
				auto& l = std::get<symbol_t>(n->value);
				if (!l.nt() && !l.is_null())
					nv.push_back(l.t());
			}
			for (const auto& c : n->child) extract_terminal(c);
		};
		for (const auto& v : t.values()) extract_terminal(v);
		return std::string(nv.begin(), nv.end());
	});

template <typename node_variant_t, typename parser_t>
const auto nonterminal_extractor = extractor<node_variant_t, parser_t,
						typename parser_t::nonterminal>(
	[](const traverser<node_variant_t, parser_t>& t) {
		using symbol_t = typename parser_t::symbol_type;
		using nonterminal_t = typename parser_t::nonterminal;
		static nonterminal_t none = static_cast<nonterminal_t>(0);
		if (!t.has_value()) return none;
		if (t.values().size() > 1) {
			std::cout << "cannot use nonterminal_extractor on "
							"multiple nodes.\n";
			return none;
		}
		if (std::holds_alternative<symbol_t>(t.value()->value)) {
			auto& l = std::get<symbol_t>(t.value()->value);
			if (l.nt()) return static_cast<nonterminal_t>(l.n());
		}
		return none;
	});

/**
 * @brief Traverser for traversing a subtree and extracting information from it.
 * 
 * Traverser object can be created from a rewriter tree node and provides API
 * for traversing its subtree and extract information from it.
 * 
 * @tparam node_variant_t variant of the rewriter tree node.
 * @tparam parser_t parser type.
 */
template <typename node_variant_t, typename parser_t>
struct traverser {
	using node_t        = idni::rewriter::sp_node<node_variant_t>;
	using symbol_t      = typename parser_t::symbol_type;
	using nonterminal_t = typename parser_t::nonterminal;
	using extractor_t   = extractor<node_variant_t, parser_t>;
	traverser() : has_value_(false) {}
	traverser(const node_t& n) : values_({n}) {}
	traverser(const std::vector<node_t>& n)
		: has_value_(n.size()), values_(n) {}
	/// Returns true if it contains (points to) at least one node.
	bool has_value() const { return has_value_; }
	/// Returns the first node from all it is pointing to.
	const node_t& value() const { return values_.front(); }
	/// Returns all nodes traverser is pointing to.
	const std::vector<node_t>& values() const { return values_; }
	/// Returns a vector of traversers for each node it points to.
	std::vector<traverser<node_variant_t, parser_t>> traversers() const {
		std::vector<traverser<node_variant_t, parser_t>> tv;
		for (const auto& v : values_) tv.emplace_back(v);
		return tv;
	}
	/// Returns a vector of traversers for each node it points to.
	std::vector<traverser<node_variant_t, parser_t>> operator()() const {
		return traversers();
	}
	/// Helper methods to check if a node is a nonterminal or if a node is a concrete nonterminal.
	bool is_non_terminal_node(const node_t& n) const {
		if (!std::holds_alternative<symbol_t>(n->value))
			std::cout << "is_non_terminal_node: not a symbol_t.\n";
		//std::cout << "n = " << std::get<symbol_t>(n->value) << std::endl;
		return std::holds_alternative<symbol_t>(n->value)
			&& std::get<symbol_t>(n->value).nt();
	}
	/// Helper methods to check if a node is a nonterminal or if a node is a concrete nonterminal.
	bool is_non_terminal_node() const {
		if (!has_value_) return false;
		if (values_.size() > 1) {
			std::cout << "cannot use is_non_terminal_node on "
						"multiple nodes.\n";
			return false;
		}
		return is_non_terminal_node(values_.front());
	}
	/// Helper methods to check if a node is a nonterminal or if a node is a concrete nonterminal.
	bool is_non_terminal(const node_t& n, const nonterminal_t& nt) const {
		//size_t i_nt = nt;
		return is_non_terminal_node(n)
			&& std::get<symbol_t>(n->value).n() == nt;
	}
	/// Helper methods to check if a node is a nonterminal or if a node is a concrete nonterminal.
	bool is_non_terminal(const nonterminal_t& nt) const {
		return is_non_terminal_node()
			&& is_non_terminal(values_.front(), nt);
	}
	traverser operator|(const nonterminal_t& nt) const {
		if (!has_value_) return *this;
		if (values_.size() > 1) {
			std::cout << "cannot use operator| on multiple nodes. "
						"use operator|| instead.\n";
			return *this;
		}
		auto v = values_[0]->child
			| std::ranges::views::filter([nt,this](const node_t& n){
				return is_non_terminal(n, nt); })
			| std::ranges::views::take(1);
		return v.empty() ? traverser() : traverser(v.front());
	}
	traverser operator||(const nonterminal_t& nt) const {
		if (!has_value_) return traverser();
		std::vector<node_t> nv;
		//std::cout << "values_.size() = " << values_.size() << std::endl;
		for (const auto& v : values_) {
			//std::cout << "v->child.size() = " << v->child.size() << std::endl;
			for (const auto& n : v->child)
				if (is_non_terminal(n, nt)) nv.push_back(n);
		}
		//std::cout << "nv.size() = " << nv.size() << std::endl;
		//for (const auto& n : nv) std::cout << n << std::endl;
		return nv.empty() ? traverser() : traverser(nv);
	}
	template <typename result_t>
	result_t operator|(
		const extractor<node_variant_t, parser_t, result_t>& e) const
	{
		if (!has_value_) return result_t();
		if (values_.size() > 1) {
			std::cout << "cannot use operator| on multiple nodes. "
						"use operator|| instead.\n";
			return result_t();
		}
		return e(*this);
	}
	template <typename result_t>
	result_t operator||(
		const extractor<node_variant_t, parser_t, result_t>& e) const
	{
		if (!has_value_) return result_t();
		return e(*this);
	}
	static constexpr const extractor<node_variant_t, parser_t>&
		get_children_extractor()
	{
		return children_extractor<node_variant_t, parser_t>;
	}
	/**
	 * Returns an extractor usable to traverse to a single child if a single
	 * child exists.
	 */
	static constexpr const extractor<node_variant_t, parser_t>&
		get_only_child_extractor()
	{
		return only_child_extractor<node_variant_t, parser_t>;
	}
	/**
	 * Returns an extractor usable to extract a string containing terminals
	 * collected from subtrees of nodes traverser is pointing to.
	 */
	static constexpr const extractor<node_variant_t, parser_t, std::string>&
		get_terminal_extractor()
	{
		return terminal_extractor<node_variant_t, parser_t>;
	}
	/**
	 * Returns an extractor usable to extract nonterminal enum type from a node
	 * traverser is pointing to.
	 */
	static constexpr const extractor<node_variant_t, parser_t,
		typename parser_t::nonterminal>& get_nonterminal_extractor()
	{
		return nonterminal_extractor<node_variant_t, parser_t>;
	}
private:
	bool has_value_ = true;
	std::vector<node_t> values_{};
};

} // idni namespace
#endif // __IDNI__PARSER__TRAVERSER_H__
