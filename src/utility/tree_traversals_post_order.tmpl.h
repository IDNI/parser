// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "tree_traversals.tmpl.h"

namespace idni {

template <typename node>
post_order<node>::post_order(tref n) : root(n) {}

template <typename node>
post_order<node>::post_order(const htref& h) : root(h->get()) {}

template <typename node>
template <size_t slot>
tref post_order<node>::apply_unique(auto& f, auto& visit_subtree) {
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		tref res = traverse<slot>(root, f, visit_subtree);
		bintree<node>::gc_enabled = true;
		return res;
	}
	else return root;
}

template <typename node>
template <size_t slot>
tref post_order<node>::apply_unique(auto& f) {
	bintree<node>::gc_enabled = false;
	tref res = traverse<slot>(root, f, all);
	bintree<node>::gc_enabled = true;
	return res;
}

template <typename node>
void post_order<node>::search(auto& visit, auto& visit_subtree) {
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		const_traverse<false>(root, visit, visit_subtree);
		bintree<node>::gc_enabled = true;
	}
}

template <typename node>
void post_order<node>::search(auto& visit) {
	bintree<node>::gc_enabled = false;
	const_traverse<false>(root, visit, all);
	bintree<node>::gc_enabled = true;
}

template <typename node>
void post_order<node>::search_unique(auto& visit, auto& visit_subtree) {
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		const_traverse<true>(root, visit, visit_subtree);
		bintree<node>::gc_enabled = true;
	}
}

template <typename node>
void post_order<node>::search_unique(auto& visit) {
	bintree<node>::gc_enabled = false;
	const_traverse<true>(root, visit, all);
	bintree<node>::gc_enabled = true;
}

template <typename node>
template <size_t slot>
tref post_order<node>::traverse(tref n, auto& f, auto& visit_subtree) {
	if (n == nullptr) return nullptr;
	// Check cache first
	if constexpr (slot != 0) {
		const auto it = m.find(std::make_pair(n, slot));
		if (it != m.end()) return it->second;
	}
	subtree_unordered_map<node, tref> cache;
	std::vector<tref> stack;
	std::vector<size_t> upos;
	stack.push_back(n);
	upos.push_back(0);
#ifdef MEASURE_TRAVERSER_DEPTH
	inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH

	auto call = [](auto& cb, tref n) -> tref {
		tref nn = cb(n);
		if (nn == n) return n;
		if (nn == nullptr) return nullptr;
		const auto& c_tree = tree::get(nn);
		return bintree<node>::get(c_tree.value,
						c_tree.left_child(),
						tree::get(n).right_sibling());
	};

	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return stack[0];
		tref& c_node = stack[upos.back()];
		DBGT(std::cout << "\nnon-const loop begin: "
			<< tree::get(c_node).dump_to_str() << "\n";)
		DBGT(print_stack<node>(stack, c_node);)
		// Check cache first
		if constexpr (slot != 0) {
			const auto it = m.find(std::make_pair(c_node, slot));
			if (it != m.end()) {
				c_node = it->second;
				upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
		} else {
			const auto it = cache.find(c_node);
			if (it != cache.end()) {
				c_node = it->second;
				upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
		}
		// Check if node has children
		if (!tree::get(c_node).has_child()) {
			// Process node and move to next
			c_node = call(f, c_node);
			if (c_node == nullptr) return nullptr;
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
			continue;
		}
		// Get next child position
		size_t c_pos = (stack.size() - 1) - upos.back();
		tref c = tree::get(c_node).child(c_pos);
		DBGT(std::cout << "\tmove to a child: " << c << " \t"
			<< (stack.back() == c_node ? "LC" : "RS") << "\n";)
		// Are all children visited?
		if (c == nullptr) {
			// Check if children actually changed
			auto ch_range = tree::get(c_node).children();
			if (std::equal(stack.begin() + (upos.back() + 1),
				stack.end(), ch_range.begin(), ch_range.end()))
			{
				tref res = call(f, c_node);
				if (res == nullptr) return nullptr;
				if constexpr (slot != 0) m.emplace(
					std::make_pair(c_node, slot), res);
				else cache.emplace(c_node, res);
				c_node = res;
				// Pop children from stacks
				stack.erase(stack.end() - c_pos, stack.end());
				upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
			// Make new node if children are different
			tref res = tree::get(tree::get(c_node).value,
				&stack[upos.back() + 1],
				c_pos,
				tree::get(c_node).right_sibling());
			DBGT(std::cout << "\tnew node: " << tree::get(res).dump_to_str() << "\n";)
			if (res == nullptr) return nullptr;
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			res = call(f, res);
			if (res == nullptr) return nullptr;
			if constexpr (slot != 0)
				m.emplace(std::make_pair(c_node, slot), res);
			else cache.emplace(c_node, res);
			c_node = res;
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			stack.push_back(c);
			// c_node can become invalid due to push_back
			if (visit_subtree(c)) {
#ifdef MEASURE_TRAVERSER_DEPTH
				inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				upos.push_back(stack.size() - 1);
			}
		}
	}
}

template <typename node>
template<bool unique>
void post_order<node>::const_traverse(tref n, auto& visitor,
	auto& visit_subtree)
{
	if (n == nullptr) return;
	subtree_unordered_set<node> cache;
	trefs stack;
	std::vector<size_t> upos;
	auto get_parent = [&upos, &stack]() -> tref {
		return upos.size() < 2 ? nullptr : stack[upos[upos.size() - 2]];
	};
	// Call callback with parent if it is invocable with tref, tref
	// Otherwise, call it just with tref
	auto call = [](auto& cb, tref x, tref parent) -> bool {
		if constexpr (accepts_tref_tref<decltype(cb)>::value) {
			if constexpr (
				bool_accepts_tref_tref<decltype(cb)>::value)
							return cb(x, parent);
			else cb(x, parent);
		} else if constexpr (bool_accepts_tref<decltype(cb)>::value) {
			return cb(x);
		} else cb(x);
		return true;
	};
	stack.push_back(n);
	upos.push_back(0);
	if constexpr (unique) cache.emplace(n);
#ifdef MEASURE_TRAVERSER_DEPTH
	inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return;
		// Find first unprocessed position
		tref c_node = stack[upos.back()];
		const auto& c_tree = tree::get(c_node);
		// Check if node has children
		if (!c_tree.has_child()) {
			// Process node and move to next
			if (!call(visitor, c_node, get_parent())) return;
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
			continue;
		}
		// Get next child
		tref c = (stack.back() == c_node) ? c_tree.left_child()
				: tree::get(stack.back()).right_sibling();
		// Are all children visited?
		if (c == nullptr) {
			if (!call(visitor, c_node, get_parent())) return;
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			stack.push_back(c);
			// if unique, skip already visited nodes
			if constexpr (unique) if (cache.contains(c)) continue;
			// c_node can become invalid due to push_back
			if (call(visit_subtree, c, c_node)) {
#ifdef MEASURE_TRAVERSER_DEPTH
				inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				upos.push_back(stack.size() - 1);
			}
			if constexpr (unique) cache.emplace(c);
		}
	}
}

} // idni namespace
