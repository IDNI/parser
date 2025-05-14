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

#include "tree_traversals.tmpl.h"

namespace idni {

template <typename node>
morris_post_order<node>::morris_post_order(tref n) : root(n) {}

template <typename node>
void morris_post_order<node>::search(auto &visit) {
	traverse(root, visit);
}

template<typename node>
void morris_post_order<node>::traverse(tref &root, auto &f) {
	if (!root) return;
	tref dummy = bintree<node>::get({}, root, nullptr);
	tref curr = dummy;
	while (curr) {
		const auto& curr_node = bintree<node>::get(curr);
		if (!curr_node.l) {
			curr = curr_node.r;
		} else {
			tref pred = curr_node.l;
			while (true) {
				const auto& pred_node =
						bintree<node>::get(pred);
				if (!pred_node.r || pred_node.r == curr) break;
				pred = pred_node.r;
			}
			auto& pred_r = bintree<node>::get(pred).r;
			if (!pred_r) {
				const_cast<tref&>(pred_r) = curr;
				curr = curr_node.l;
			} else {
				const_cast<tref&>(pred_r)  = nullptr;
				traverse_right_list(curr_node.l, f);
				curr = curr_node.r;
			}
		}
	}
}
template <typename node>
void morris_post_order<node>::traverse_right_list(tref from, auto &f) {
	if (!from) return;
	tref prev = nullptr;
	tref curr = from, next;
	// Reverse right pointers
	while (curr) {
		const auto& curr_node = bintree<node>::get(curr);
		next = curr_node.r;
		const_cast<tref&>(curr_node.r)  = prev;
		prev = curr;
		curr = next;
	}
	// Process in reverse while restoring right pointers
	curr = prev;
	prev = nullptr;
	while (curr) {
		const auto& curr_node = bintree<node>::get(curr);
		next = curr_node.r;
		const_cast<tref&>(curr_node.r) = prev;
		f(curr);
		prev = curr;
		curr = next;
	}
}

} // idni namespace
