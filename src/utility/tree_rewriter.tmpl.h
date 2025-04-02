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

#include "tree.h"

namespace idni::rewriter {

// former rewriter predicate API
// ---------------------------------------------------------------------

template <typename tree_t, typename wrapped_t, typename predicate_t>
post_order_traverser<tree_t, wrapped_t, predicate_t>::post_order_traverser(wrapped_t& wrapped,
	predicate_t& query) : wrapped(wrapped), query(query) {}

template <typename tree_t, typename wrapped_t, typename predicate_t>
tref post_order_traverser<tree_t, wrapped_t, predicate_t>::operator()(tref n) {
	// we kept track of the visited nodes to avoid visiting the same node
	// twice. However, we do not need to keep track of the root node, since
	// it is the one we start from and we will always be visited.
	std::set<tref> visited;
	// if the root node matches the query predicate, we traverse it, otherwise
		// we return the result of apply the wrapped transform to the node.
	return query(n) ? traverse(n, visited) : wrapped(n);
}

template <typename tree_t, typename wrapped_t, typename predicate_t>
tref post_order_traverser<tree_t, wrapped_t, predicate_t>::traverse(
	tref n, std::set<tref>& visited)
{
	// we traverse the children of the node in post-order, i.e. we visit
	// the children first and then the node itself.
	for (tref c : tree_t::get(n).children())
		// we skip already visited nodes and nodes that do not match the
		// query predicate if it is present.
		if (!visited.contains(c) && query(c)) {
			if (traverse(c, visited) == nullptr) return nullptr;
			// we assume we have no cycles, i.e. there is no way we could
			// visit the same node again down the tree.
			// thus we can safely add the node to the visited set after
			// visiting it.
			visited.insert(c);
		}
	// finally we apply the wrapped visitor to the node if it is present.
	return wrapped(n);
}

template <typename predicate_t>
select_top_predicate<predicate_t>::select_top_predicate(predicate_t& query,
	trefs& selected) : query(query), selected(selected) {}

template <typename predicate_t>
bool select_top_predicate<predicate_t>::operator()(tref n) {
	if (!query(n)) return true;
	// we return false to avoid visiting the children of the node
	// since we are only interested in the top nodes.
	if (std::find(selected.begin(), selected.end(), n)
			== selected.end()) selected.push_back(n);
	return false;
}


} // idni::rewriter namespace
