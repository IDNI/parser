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
#ifndef __IDNI__TREE_H__
#define __IDNI__TREE_H__

#include <vector>
#include <initializer_list>
#include <utility>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include <cassert>
#include <iterator>
#include "../defs.h"

namespace idni {

//------------------------------------------------------------------------------

// @brief tree node reference
using tref  = const intptr_t*;
// @brief vector of tree node reference handles
using trefs = std::vector<tref>;

// @brief range of tree node reference handles usable in range-based for loops
template <typename T>
struct tref_range {
	tref first;
	tref_range(tref first);
	struct iterator {
		using iterator_category = std::forward_iterator_tag;
		using value_type = tref;
		using difference_type = std::ptrdiff_t;
		using pointer = tref*;
		using reference = tref&;
        
		tref current;
		iterator(tref id);
		tref operator*() const;
		iterator& operator++();
		bool operator!=(const iterator& other) const;
		bool operator==(const iterator& other) const;
	};
	iterator begin() const;
	iterator end() const;
};

// @brief range of tree nodes usable in range-based for loops returning tree nodes
template <typename T>
struct tree_range {
	tref first;
	tree_range(tref first);
	struct iterator {
		using iterator_category = std::forward_iterator_tag;
		using value_type = const T&;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = const T&;
        
		tref current;
		iterator(tref id);
		const T& operator*() const;
		iterator& operator++();
		bool operator!=(const iterator& other) const;
		bool operator==(const iterator& other) const;
	};
	iterator begin() const;
	iterator end() const;
};

//------------------------------------------------------------------------------

template <typename T>
struct bintree;

// @brief tree handle wrapping a tref. htree::sp prevents tree from being gc-ed
struct htree {
	using sp = std::shared_ptr<htree>;
	using wp = std::weak_ptr<htree>;
	static const sp& null();
	inline tref get() const;
	inline bool operator==(const htree& r) const;
	inline bool operator< (const htree& r) const;
	//~htree();
private:
	tref hnd;
	explicit htree(tref id = nullptr);
	template <typename N> friend struct bintree;
};

//------------------------------------------------------------------------------

// common predicates / callbacks
inline static const auto all = [](const auto&) static { return true; };
inline static const auto none = [](const auto&) static { return false; };
inline static const auto identity = [](const auto& n) static { return n; };
inline static const auto do_nothing = [](const auto&) static {};

// helper type traits to check callback properties
template<typename Cb>
using bool_accepts_tref = std::is_invocable_r<bool, Cb, tref>;

template<typename Cb>
using bool_accepts_tref_tref = std::is_invocable_r<bool, Cb, tref, tref>;

template<typename Cb>
using accepts_tref_tref = std::is_invocable<Cb, tref, tref>;


//------------------------------------------------------------------------------
// rewriter types

namespace rewriter {

using rule = std::pair<htree::sp, htree::sp>;
using rules = std::vector<rule>;

using library = rules;
using builder = rule;

rules merge(const rules& rs1, const rules& rs2);

}

//------------------------------------------------------------------------------

/**
 * @brief Binary tree (each node has left and right child)
 * @tparam T The type of the tree node stored as a value
 */
template <typename T>
struct bintree {
	/**
	 * @brief Value of a node of node type T
	 */
	const T value;
	/**
	 * @brief Left child of the binary tree node
	 */
	const tref l;
	
	/**
	 * @brief Right child of the binary tree node
	 */
	const tref r;

	/**
	 * @brief Get nodes's tref id
	 * @return The tree node's tref
	 */
	tref get() const;

	/**
	 * @brief Get the tree node from its tref id
	 * @param id The tree node
	 * @return The tree node's id
	 */
	static const bintree& get(const tref id);

	/**
	 * @brief Get the tree node from handle
	 * @param h The tree node handle
	 * @return The tree node
	 */
	static const bintree& get(const htree::sp& h);

	/**
	 * @brief Get the tree node shared handle preventing it from gc
	 * @param id The tree node's ref
	 * @return The tree node's shared handle (preventing from gc)
	 */
	static const htree::sp geth(tref id);

	/**
	 * @brief Create a new tree node from value and left and right children
	 * @param v The value
	 * @param l The left child
	 * @param r The right child
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, tref l, tref r);

	/**
	 * @brief Dump the tree (including all its nodes) to stdout
	 */
	static void dump();

	/**
	 * @brief Garbage collect tree nodes
	 */
	static void gc();

	/**
	 * @brief Print the tree to an ostream.
	 * @param os The ostream to print to.
	 * @param l The indentation level.
	 * @return The ostream.
	 */
	std::ostream& print(std::ostream& os, size_t l = 0) const;

	bool operator<(const bintree& o) const;
	bool operator==(const bintree& o) const;
protected:
	/**
	 * @brief Constructor for a new tree node from value and left and right children
	 * @param _value The value
	 * @param _l The left child
	 * @param _r The right child
	 */
	bintree(const T& _value, tref _l = nullptr, tref _r = nullptr);

	/**
	 * @brief Get the string representation of the node (for debugging)
	 * @return The string representation of the node.
	 */
	const std::string str() const;

	/**
	 * @brief Map of tree nodes to their handles
	 */
	static std::map<const bintree, htree::wp> M;
};

template <typename T> struct pre_order;
template <typename T> struct post_order;

template <typename T>
struct hash_tref {
	size_t operator()(tref r) const;
};

template <typename T>
struct subtree_equality {
	bool operator()(tref a, tref b) const;
};

template <typename T, typename PT>
struct subtree_pair_equality {
	bool operator()(const std::pair<tref, PT>& a,
			const std::pair<tref, PT>& b) const;
};

template <typename T>
using subtree_set = std::set<tref, subtree_equality<T>>;

template <typename T, typename PT>
using subtree_map = std::map<tref, PT, subtree_equality<T>>;

template <typename T>
using subtree_unordered_set = std::unordered_set<tref,
				hash_tref<T>, subtree_equality<T>>;

template <typename T, typename PT>
using subtree_unordered_map = std::unordered_map<tref, PT,
				hash_tref<T>, subtree_pair_equality<T, PT>>;

/**
 * @brief Left child right sibling tree
 * @tparam T The type of the tree node value
 */
template <typename T>
struct lcrs_tree : public bintree<T> {
	friend post_order<T>;
	friend pre_order<T>;

	bool operator==(const lcrs_tree<T>& other) const;
	bool operator<(const lcrs_tree<T>& other) const;
	static bool subtree_equals(tref a, tref b);
	static bool subtree_less(tref a, tref b);

	/**
	 * @brief Get node's tref
	 * @return The tree node reference
	 */
	tref get() const;

	/**
	 * @brief Get the tree node from its tref id
	 * @param id The tree node's tref id
	 * @return The tree node's id
	 */
	static const lcrs_tree<T>& get(tref id);

	/**
	 * @brief Get the tree node from its handle
	 * @param h The tree node handle
	 * @return The tree node
	 */
	static const lcrs_tree<T>& get(const htree::sp& h);

	/**
	 * @brief Get the tree node shared handle
	 * @param h The tree node handle
	 * @return The tree node shared handle
	 */
	static const htree::sp geth(tref id);

	// creation with tref childs

	/**
	 * @brief Creates new tree node from value and children and right sibling without triggering hooks
	 * @param v The value
	 * @param ch The children pointer / array
	 * @param len The number of children
	 * @param r The right sibling
	 * @return The new tree node's tref id
	 */
	static tref get_raw(const T& v, const tref* ch = nullptr,
					size_t len = 0, tref r = nullptr);

	/**
	 * @brief Creates new tree node from value and children and right sibling
	 * @param v The value
	 * @param ch The children pointer / array
	 * @param len The number of children
	 * @param r The right sibling
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, const tref* ch, size_t len,
		tref r = nullptr);

	/**
	 * @brief Creates new tree node from value and children and right sibling
	 * @param v The value
	 * @param ch The children vector
	 * @param r The right sibling
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, const trefs& ch, tref r = nullptr);

	/**
	 * @brief Creates new tree node from value and children
	 * @param v The value
	 * @param ch The children trefs in an initializer list
	 * @param r The right sibling
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, const std::initializer_list<tref>& ch,
		tref r = nullptr);

	/**
	 * @brief Creates new tree leaf node from value
	 * @param v The value
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v);

	/**
	 * @brief Creates new tree node from value and tref of a child
	 * @param v The node
	 * @param ch The child's tref
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, tref ch);

	/**
	 * @brief Creates new tree node from value and two children trefs
	 * @param v The node
	 * @param ch1 The first child
	 * @param ch2 The second child
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, tref ch1, tref ch2);

	/**
	 * @brief Creates a new tree node from node n and sets right sibling
	 * @param n The node we want to copy and set right sibling for
	 * @param r The right sibling
	 * @return The new tree node's tref id
	 */
	static tref get(tref n, tref r);

	// creation with node childs

	/**
	 * @brief Creates new tree node from value and childdren array / pointer
	 * @param v The node
	 * @param ch Children pointer / array
	 * @param len The number of children
	 * @param r The right sibling
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, const T* ch, size_t len, tref r = nullptr);

	/**
	 * @brief Creates new tree node from value and children vector of nodes
	 * @param v The node
	 * @param ch Children nodes vector
	 * @param r The right sibling
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, const std::vector<T>& children,
		tref r = nullptr);

	/**
	 * @brief Creates new tree node from value and children tref initializer list
	 * @param v The node
	 * @param ch Children tref initializer list
	 * @param r The right sibling	
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, const std::initializer_list<T>& children,
		tref r = nullptr);

	/**
	 * @brief Creates new tree node from value and child node
	 * @param v The node
	 * @param ch The child node
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, const T& ch);

	/**
	 * @brief Creates new tree node from value and two child nodes
	 * @param v The node
	 * @param ch1 The first child node
	 * @param ch2 The second child node
	 * @return The new tree node's tref id
	 */
	static tref get(const T& v, const T& ch1, const T& ch2); 

	/**
	 * @brief Check if the node has a right sibling
	 * @return True if the node has a right sibling, false otherwise
	 */
	bool has_right_sibling() const;

	/**
	 * @brief Check if the node has a child
	 * @return True if the node has a child, false otherwise
	 */
	bool has_child() const;

	/**
	 * @brief Get the left sibling of the node
	 * @return The left sibling of the node
	 */
	tref right_sibling() const;

	/**
	 * @brief Get the right sibling tree of the node without safety checks!
	 * @return The right sibling tree of the node
	 */
	const lcrs_tree<T>& right_sibling_tree() const;

	/**
	 * @brief Get the child of the node
	 * @return The child of the node
	 */
	tref left_child() const;

	/**
	 * @brief Get the number of children
	 * @return The number of children
	 */
	size_t children_size() const;

	/**
	 * @brief Return children trefs or calculate size
	 * @param ch Data pointer to fill with children trefs or nullptr to get size
	 * @param len Reference to the number of children to populate or to be populated with size
	 * @return The new tree node's tref id
	 * Fills caller allocated child array upto len size with child ids.
	 * If child is nullptr, then just calculates number of children in len.
	 * Caller's responsibility to free allocated memory.
	 */
	bool get_kary_children(tref* ch, size_t& len) const;
	bool get_children(tref* ch, size_t& len) const;

	/**
	 * @brief Get the children trefs
	 * @return The children trefs
	 */
	trefs get_children() const;

	/**
	 * @brief Get the children tref range usable in range-based for loops
	 * @return tref_range with iterator
	 */
	tref_range<T> children() const;

	/**
	 * @brief Get the children tree nodes range usable in range-based for loops
	 * @return tree_range with iterator
	 */
	tree_range<lcrs_tree<T>> children_trees() const;

	/**
	 * @brief Get the child node's tref id
	 * @param n The index of the child
	 * @return The child node's tref id
	 */
	tref child(size_t n) const;

	/**
	 * @brief Get the first child node's tref id
	 * @return The first child node's tref id
	 */
	tref first()  const;

	/**
	 * @brief Get the second child node's tref id
	 * @return The second child node's tref id
	 */
	tref second() const;

	/**
	 * @brief Get the third child node's tref id
	 * @return The third child node's tref id
	 */
	tref third()  const;

	/**
	 * @brief Get the only child's tref id. Return nullptr if not only
	 * @return The only child node's tref id or nullptr
	 */
	tref only_child() const;

	/**
	 * @brief Get the child tree node from its index without checks for safety!
	 * @param n The index of the child
	 * @return The child tree node
	 */
	const lcrs_tree<T>& child_tree(size_t n) const;

	/**
	 * @brief Get the child tree node from its index without checks for safety! (alias for child_tree(n))
	 * @param n The index of the child
	 * @return The child tree node
	 */
	const lcrs_tree<T>& operator[](size_t n) const;

	/**
	 * @brief Get the first child tree node without checks for safety!
	 * @param id The child node's tref id
	 * @return The child tree node
	 */
	const lcrs_tree<T>& first_tree() const;

	/**
	 * @brief Get the second child tree node without checks for safety!
	 * @param id The child node's tref id
	 * @return The child tree node
	 */
	const lcrs_tree<T>& second_tree() const;

	/**
	 * @brief Get the third child tree node without checks for safety!
	 * @param id The child node's tref id
	 * @return The child tree node
	 */
	const lcrs_tree<T>& third_tree()  const;

	/**
	 * @brief Get the only child's tree node without checks for safety!
	 * @return The only child tree node
	 */
	const lcrs_tree<T>& only_child_tree() const;

	/**
	 * @brief Print the tree to an ostream
	 * @param os The ostream to print to
	 * @param l The indentation level
	 * @return The ostream
	 */
	std::ostream& print(std::ostream& os, size_t l = 0) const;

	/**
	 * @brief Print the tree to a string
	 * @param l The indentation level
	 * @return The string
	 */
	std::string print_to_str(size_t l = 0) const;

	/**
	 * @brief Print the tree to an ostream in line
	 * @param os The ostream to print to
	 * @param open The opening string
	 * @param close The closing string
	 * @param sep The separator string
	 * @return The ostream
	 */
	std::ostream& print_in_line(std::ostream& os,
		std::string open = " { ", std::string close = " }",
		std::string sep = ", ") const;

	/**
	 * @brief Print the tree to a string in line
	 * @param open The opening string
	 * @param close The closing string
	 * @param sep The separator string
	 * @return The string
	 */
	std::string print_in_line_to_str(
		std::string open = " { ", std::string close = " }",
		std::string sep = ", ") const;

	/**
	 * @brief Dump the node to an ostream
	 * @param os The ostream to print to
	 * @param n The tree node to dump
	 * @param subtree with node's subtree
	 * @return The ostream
	 */
	static std::ostream& dump(std::ostream& os, tref n, bool subtree = true);

	/**
	 * @brief Dump the node to a string
	 * @param n The tree node to dump
	 * @param subtree with node's subtree
	 * @return The string
	 */
	static std::string dump_to_str(tref n, bool subtree = true);

	/**
	 * @brief Dump the node to an ostream
	 * @param os The ostream to print to
	 * @param subtree with node's subtree
	 * @return The ostream
	 */
	std::ostream& dump(std::ostream& os, bool subtree = true) const;

	/**
	 * @brief Dump the node to astring
	 * @param subtree with node's subtree
	 * @return The string
	 */
	std::string dump_to_str(bool subtree = true) const;

	/**
	 * @brief Dump the node to std::cout
	 * @param subtree with node's subtree
	 * @return The node itself
	 * This method can be used to simply add ".dump()" anywhere 
	 * to lcrs_tree<T> instance for simple debugging. 
	 * Example:
	 *         auto t = lcrs_tree<int>::get(0);
	 *         if (t.value == 1) ...
	 * Dumping the node:
	 *         if (t.dump().value == 1) ...
	 */
	const lcrs_tree<T>& dump(bool subtree = true) const;

	// hooks

	using hook_function
		= std::function<tref(const T&, const tref*, size_t, tref)>;
	inline static hook_function hook = nullptr;
	inline static void set_hook(hook_function h);
	inline static void reset_hook();
	inline static bool is_hooked();

	// rewriter API available directly on the tree

	// delete all top nodes that satisfy a predicate.
	tref trim_top(const auto& query) const;

	// select all top nodes that satisfy a predicate and return them.
	trefs select_top(const auto& query) const;

	// select all subnodes that satisfy a predicate according to the extractor and return them.
	trefs select_subnodes(const auto& query, const auto& extractor) const;

	// select all nodes that satisfy a predicate and return them.
	trefs select_all(const auto& query) const;

	// Select all nodes in input that satisfy query ignoring subtrees under a node satisfying until
	trefs select_all_until(const auto& query, const auto& until) const;

	// Select top nodes in input that satisfy query ignoring subtrees under a node satisfying until
	trefs select_top_until(const auto& query, const auto& until) const;

	// find the first node that satisfy a predicate and return it.
	tref find_top(const auto& query) const;

	// Select single top node in input that satisfy query ignoring subtrees under a node satisfying until
	tref find_top_until(const auto& query, const auto& until) const;

	// find the first node that satisfy a predicate and return it.
	tref find_bottom(const auto& query) const;

	tref replace(const std::map<tref, tref>& changes) const;
	tref replace(const subtree_map<T, tref>& changes) const;
	tref replace(tref what, tref with) const;

	// Replace nodes in n according to changes while skipping subtrees that don't satisfy query
	tref replace_if(const subtree_map<T, tref>& changes, const auto& query) const;

	// Replace nodes in n according to changes while skipping subtrees that satisfy query
	tref replace_until(const subtree_map<T, tref>& changes, const auto& query) const;

	// TODO (LOW) consider adding a similar function for replace_node...

	// apply a rule to a tree using the predicate to pattern_matcher.
	tref apply_rule(const rewriter::rule& r, const auto& is_capture) const;

	// apply a rule to a tree using the predicate to pattern_matcher and skipping
	// unnecessary subtrees
	tref apply_if(const rewriter::rule& r, const auto& is_capture,
						const auto& predicate) const;

	// apply a substitution to a rule according to a given matcher, this method is
	// use internaly by apply and apply with skip.
	tref apply(tref s, const auto& matcher) const;
};

template <typename node>
std::ostream& dump(std::ostream& os, const std::map<tref, tref>& m,
	bool subtree = true);

template <typename node>
std::ostream& dump(std::ostream& os, const subtree_map<node, tref>& m,
	bool subtree = true);

template <typename node>
std::string dump_to_str(const std::map<tref, tref>& m, bool subtree = true);


template <typename node>
std::string dump_to_str(const subtree_map<node, tref>& m,
	bool subtree = true);

// helper to get value from a cache
template <typename node>
tref get_cached(tref n, const std::map<tref, tref>& cache);

// helper to get value from a subtree cached map (using subtree_equality)
template <typename node>
tref get_cached(tref n, const subtree_map<node, tref>& cache);

// helper to get a value from a cache using subtree_equality
template <typename node>
tref get_cached_subtree(tref n, const std::map<tref, tref>& cache);

template <typename node>
bool is_cached_subtree(tref n, const std::unordered_set<tref>& cache);

//------------------------------------------------------------------------------
// post_order and pre_order traversals
// - can be used with lcrs_tree, tree and their descendants
// - does not work with bintree

// TODO handle traversals and gc

/**
 * @brief Struct for tree traversals in post order
 * @tparam node Tree node type
 */
template <typename node>
struct post_order {
	using tree = lcrs_tree<node>;

	explicit post_order(tref n);
	explicit post_order(const htree::sp& h);

	/**
	 * @brief Apply f in post order to root according to visit_subtree.
	 * If f is applied to a node, its children are already transformed by f
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. Must not have side effects due to memorization
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @return The tree obtained after applying f to root
	 */
	template <size_t slot = 0>
	tref apply_unique(auto& f, auto& visit_subtree);

	/**
	 * @brief Apply f in post order to root.
	 * If f is applied to a node, its children are already transformed by f
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. Must not have side effects due to memorization
	 * @return The tree obtained after applying f to root
	 */
	template <size_t slot = 0>
	tref apply_unique(auto& f);

	/**
	 * @brief Call visit in post order on the nodes of root according to visit_subtree.
	 * If visit returns false on a node, the traversal is terminated
	 * @param visit Function to call on each node
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 */
	void search(auto& visit, auto& visit_subtree);

	/**
	 * @brief Call visit in post order on the nodes of root.
	 * If visit returns false on a node, the traversal is terminated
	 * @param visit Function to call on each node
	 */
	void search(auto& visit);

	/**
	 * @brief Call visit in post order on each node of root according to visit_subtree.
	 * Equal nodes are not visited twice.
	 * If visit returns false on a node, the traversal is terminated
	 * @param visit Function to call on each node
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 */
	void search_unique(auto& visit, auto& visit_subtree);

	/**
	 * @brief Call visit in post order on each node of root.
	 * Equal nodes are not visited twice.
	 * If visit returns false on a node, the traversal is terminated
	 * @param visit Function to call on each node
	 */
	void search_unique(auto& visit);

private:
	tref root;
	// inline static std::unordered_map<std::pair<node_t, size_t>, node_t,
	// 	std::hash<std::pair<node_t, size_t>>,
	// 	traverser_pair_cache_equality<node>> m;
	inline static std::unordered_map<std::pair<tref, size_t>, tref> m;

	template <size_t slot>
	tref traverse(tref n, auto& f, auto& visit_subtree);

	template<bool unique>
	void const_traverse(tref n, auto& visit, auto& visit_subtree);
};

template <typename node>
struct morris_post_order
{
	morris_post_order(tref root);
	void search(auto &visit);
private:
	tref root;
	void traverse(tref &n, auto &f);
	void traverse_right_list(tref from, auto &f);
};



/**
 * @brief Struct for tree traversals in pre order
 * @tparam node Tree node type
 */
template<typename node>
struct pre_order {
	using tree = lcrs_tree<node>;

	explicit pre_order(tref n);
	explicit pre_order(const htree::sp& h);

	/**
	 * @brief Apply f in pre order to root according to visit_subtree.
	 * If f is applied to a node, the traversal will continue with the children of the transformed node
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. The function can have side effects
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function to apply to processed node in post order
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply(auto& f, auto& visit_subtree, auto& up);

	/**
	 * @brief Apply f in pre order to root.
	 * If f is applied to a node, the traversal will continue with the children of the transformed node
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. The function can have side effects
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply(auto& f);

	/**
	 * @brief Apply f in pre order to root according to visit_subtree.
	 * If f is applied to a node resulting in a change, its children are not traversed
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. The function can have side effects
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function to apply to processed node in post order
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply_until_change(auto& f, auto& visit_subtree, auto& up);

	/**
	 * @brief Apply f in pre order to root while revisiting already visited nodes.
	 * If f is applied to a node resulting in a change, its children are not traversed
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. The function can have side effects
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply_until_change(auto& f);

	/**
	 * @brief Apply f in pre order to root according to visit_subtree.
	 * If f is applied to a node, the traversal will continue with the children of the transformed node
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. Must not have side effects due to memorization
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function to apply to processed node in post order
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply_unique(auto& f, auto& visit_subtree, auto& up);

	/**
	 * @brief Apply f in pre order to root according to visit_subtree.
	 * If f is applied to a node, the traversal will continue with the children of the transformed node
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. Must not have side effects due to memorization
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply_unique(auto& f, auto& visit_subtree);

	/**
	 * @brief Apply f in pre order to root.
	 * If f is applied to a node, the traversal will continue with the children of the transformed node
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. Must not have side effects due to memorization
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply_unique(auto& f);

	/**
	 * @brief Apply f in pre order to root according to visit_subtree.
	 * If f is applied to a node resulting in a change, its children are not traversed
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. Must not have side effects due to memorization
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function to apply to processed node in post order
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply_unique_until_change(auto& f, auto& visit_subtree, auto& up);

	/**
	 * @brief Apply f in pre order to root according to visit_subtree.
	 * If f is applied to a node resulting in a change, its children are not traversed
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. Must not have side effects due to memorization
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply_unique_until_change(auto& f, auto& visit_subtree);

	/**
	 * @brief Apply f in pre order to root.
	 * If f is applied to a node resulting in a change, its children are not traversed
	 * @tparam slot Memory slot to use for memorization, disabled by default
	 * @param f Function to apply on each node. Must not have side effects due to memorization
	 * @return The tree obtained after applying f to root
	 */
	template<size_t slot = 0>
	tref apply_unique_until_change(auto& f);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * If visit returns false on a node, the traversal terminates
	 * @param visit The function called on nodes
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function called on visited nodes in post order
	 * @param between Function called between children of a node
	 */
	void search(auto& visit, auto& visit_subtree, auto& up, auto& between);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * If visit returns false on a node, the traversal terminates
	 * @param visit The function called on nodes
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function called on visited nodes in post order
	 */
	void search(auto& visit, auto& visit_subtree, auto& up);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * If visit returns false on a node, the traversal terminates
	 * @param visit The function called on nodes
	 */
	void search(auto& visit);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * Equal nodes are not visited twice.
	 * If visit returns false on a node, the traversal terminates
	 * @param visit The function called on nodes
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function called on visited nodes in post order
	 */
	void search_unique(auto& visit, auto& visit_subtree, auto& up);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * Equal nodes are not visited twice.
	 * If visit returns false on a node, the traversal terminates
	 * @param visit The function called on nodes
	 */
	void search_unique(auto& visit);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * If visit returns false on a node, its children are not visited
	 * @param visit The function called on nodes
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function called on visited nodes in post order
	 * @param between Function called between children of a node
	 */
	void visit(auto& visit, auto& visit_subtree, auto& up, auto& between);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * If visit returns false on a node, its children are not visited
	 * @param visit The function called on nodes
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function called on visited nodes in post order
	 */
	void visit(auto& visit, auto& visit_subtree, auto& up);

	/**
	 * @brief Call visit in pre order on the nodes of root.
	 * If visit returns false on a node, its children are not visited
	 * @param visit The function called on nodes
	 */
	void visit(auto& visit);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * Equal nodes are not visited twice.
	 * If visit returns false on a node, its children are not visited
	 * @param visit The function called on nodes
	 * @param visit_subtree If a node does not satisfy visit_subtree, children are not visited
	 * @param up Function called on visited nodes in post order
	 */
	void visit_unique(auto& visit, auto& visit_subtree, auto& up);

	/**
	 * @brief Call visit in pre order on the nodes of root according to visit_subtree.
	 * Equal nodes are not visited twice.
	 * If visit returns false on a node, its children are not visited
	 * @param visit The function called on nodes
	 */
	void visit_unique(auto& visit);

#ifdef MEASURE_TRAVERSER_DEPTH
	std::pair<size_t, size_t> get_tree_depth_and_size();
#endif //MEASURE_TRAVERSER_DEPTH

private:
	tref root;
	// inline static std::unordered_map<std::pair<node_t, size_t>, node_t,
	// 		std::hash<std::pair<node_t, size_t>>,
	// 		traverser_pair_cache_equality<node>> m;
	inline static std::unordered_map<std::pair<tref, size_t>, tref> m;

	template<bool break_on_change, size_t slot, bool unique>
	tref traverse(tref n, auto& f, auto& visit_subtree, auto& up);

	template<bool search, bool unique>
	void const_traverse(tref n, auto& visit, auto& visit_subtree,
						auto& up, auto& between);
};

namespace rewriter {

// a environment is a map from captures to tree nodes, it is used
// to keep track of the captures that have been unified and their
// corresponding tree nodes.
template <typename node>
using environment = subtree_map<node, tref>;

} // rewriter namespace

} // idni namespace

//------------------------------------------------------------------------------
// include rewriter types and predicates

#include "tree_rewriter.inc.h"

//------------------------------------------------------------------------------


#include "tree.tmpl.h"
#include "tree_traversals.tmpl.h"
#include "tree_rewriter.tmpl.h"
#endif // __IDNI__TREE_H__
