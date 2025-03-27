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

#include "bintree.h"

namespace idni {

static const auto all = [](const auto&) static { return true; };
static const auto none = [](const auto&) static { return false; };
static const auto identity = [](const auto& n) static { return n; };

#ifdef MEASURE_TRAVERSER_DEPTH
static size_t depth = 0;
static size_t max_depth = 0;

static void inc_depth() {
	++depth;
	static bool printed = false;
	if (depth > max_depth) {
		max_depth = depth;
		if (max_depth % 5000 != 0) printed = false;
		else if (!printed) {
			std::cout << "max_depth: " << max_depth << "\n";
			printed = true;
		}
	}
}

static void dec_depth() {
	--depth;
}
#endif // MEASURE_TRAVERSER_DEPTH

// template <typename node_t>
// struct traverser_cache_equality {
// 	bool operator() (const node_t& l, const node_t& r) const {
// 		return l == r;
// 	}
// };

// template <typename node_t>
// struct traverser_pair_cache_equality {
// 	using p = std::pair<node_t, size_t>;
// 	bool operator() (const p& l, const p& r) const {
// 		return l == r;
// 	}
// };

template <typename node_t>
post_order<node_t>::post_order(tref n) : root(n) {}

template<typename node_t>
post_order<node_t>::post_order(const htree::sp& h) : root(h->get()) {}

template <typename node_t>
template <size_t slot>
tref post_order<node_t>::apply_unique (auto& f, auto& visit_subtree) {
	if (visit_subtree(root))
		return traverse<slot>(root, f, visit_subtree);
	else return root;
}

template <typename node_t>
template <size_t slot>
tref post_order<node_t>::apply_unique (auto& f) {
	return traverse<slot>(root, f, all);
}

template <typename node_t>
void post_order<node_t>::search(auto& visit, auto& visit_subtree) {
	if (visit_subtree(root))
		const_traverse<false>(root, visit, visit_subtree);
}

template <typename node_t>
void post_order<node_t>::search(auto& visit) {
	const_traverse<false>(root, visit, all);
}

template <typename node_t>
void post_order<node_t>::search_unique(auto& visit, auto& visit_subtree) {
	if (visit_subtree(root))
		const_traverse<true>(root, visit, visit_subtree);
}

template <typename node_t>
void post_order<node_t>::search_unique(auto& visit) {
	const_traverse<true>(root, visit, all);
}

template <typename node_t>
template <size_t slot>
tref post_order<node_t>::traverse(tref n, auto& f, auto& visit_subtree) {
	// Check cache first
	if constexpr (slot != 0) {
		const auto it = m.find(std::make_pair(n, slot));
		if (it != m.end()) return it->second;
	}
	// std::unordered_map<node_t, node_t, std::hash<node_t>,
	// 	traverser_cache_equality<node_t>> cache;
	tref_cache_map cache;
	std::vector<tref> stack;
	std::vector<size_t> upos;
	stack.push_back(n);
	upos.push_back(0);
#ifdef MEASURE_TRAVERSER_DEPTH
	inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return stack[0];
		auto& c_node = stack[upos.back()];
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
		auto c_tree = lcrs_tree<node_t>::get(c_node);
		auto c_children = c_tree.get_children_trefs();
		if (c_children.empty()) {
			// Process node and move to next
			c_node = f(c_node);
			if (c_node == error_node<tref>::value)
				return error_node<tref>::value;
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
			continue;
		}
		// Get child position
		size_t c_pos = (stack.size() - 1) - upos.back();
		// Are all children visited?
		if (c_pos == c_children.size()) {
			// Check if children actually changed
			if (std::equal(stack.begin() + (upos.back() + 1), stack.end(),
				c_children.begin(), c_children.end())) {
				auto res = f(c_node);
				if (res == error_node<tref>::value)
					return error_node<tref>::value;
				if constexpr (slot != 0) m.emplace(
					std::make_pair(c_node, slot), res);
				else cache.emplace(c_node, res);
				c_node = std::move(res);
				// Pop children from stacks
				stack.erase(stack.end() - c_pos, stack.end());
				upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
			// Make new node if children are different
			trefs children;
			for (size_t i = upos.back() + 1; i < stack.size(); ++i)
				children.emplace_back(std::move(stack[i]));
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			// Apply wrapped and mark processed
			tref new_node = lcrs_tree<node_t>::get(c_tree.value, children);
			if (new_node == error_node<tref>::value)
				return error_node<tref>::value;
			auto res = f(new_node);
			if (res == error_node<tref>::value)
				return error_node<tref>::value;
			if constexpr (slot != 0) m.emplace(std::make_pair(c_node, slot), res);
			else cache.emplace(c_node, res);
			c_node = std::move(res);
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			const auto& c = c_children[c_pos];
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

template <typename node_t>
template<bool unique>
void post_order<node_t>::const_traverse(tref n, auto& visit, auto& visit_subtree) {
	if (n == nullptr) return;
	// std::unordered_set<node_t, std::hash<node_t>,
	// 	traverser_cache_equality<node_t>> cache;
	tref_cache_set cache;
	trefs stack;
	std::vector<size_t> upos;
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
		auto& c_node = stack[upos.back()];
		auto c_tree = lcrs_tree<node_t>::get(c_node);
		auto c_children = c_tree.get_children();
		// First check if node has children
		if (c_children.empty()) {
			// Process node and move to next
			if (!visit(c_node)) return;
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
			continue;
		}
		// Get child position
		size_t c_pos = (stack.size() - 1) - upos.back();
		// Are all children visited?
		if (c_pos == c_children.size()) {
			if (!visit(c_node)) return;
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			const auto& c = c_children[c_pos];
			stack.push_back(c);
			if constexpr (unique) {
				if (cache.contains(c))
					continue;
			}
			// c_node can become invalid due to push_back
			if (visit_subtree(c)) {
#ifdef MEASURE_TRAVERSER_DEPTH
				inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				upos.push_back(stack.size() - 1);
			}
			if constexpr (unique) cache.emplace(c);
		}
	}
}

template<typename node_t>
pre_order<node_t>::pre_order(tref n) : root(n) {}

template<typename node_t>
pre_order<node_t>::pre_order(const htree::sp& h) : root(h->get()) {}

template<typename node_t>
template<size_t slot>
tref pre_order<node_t>::apply_unique(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		return traverse<false, slot, true>(root, f, visit_subtree, up);
	else return root;
}

template<typename node_t>
template<size_t slot>
tref pre_order<node_t>::apply_unique(auto& f) {
	return traverse<false, slot, true>(root, f, all, identity);
}

template<typename node_t>
template<size_t slot>
tref pre_order<node_t>::apply(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		return traverse<false, slot, false>(root, f, visit_subtree, up);
	else return root;
}

template<typename node_t>
template<size_t slot>
tref pre_order<node_t>::apply_unique_until_change(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		return traverse<true, slot, true>(root, f, visit_subtree, up);
	else return root;
}

template<typename node_t>
template<size_t slot>
tref pre_order<node_t>::apply_unique_until_change(auto& f) {
	return traverse<true, slot, true>(root, f, all, identity);
}

template<typename node_t>
template<size_t slot>
tref pre_order<node_t>::apply_until_change(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		return traverse<true, slot, false>(root, f, visit_subtree, up);
	else return root;
}

template<typename node_t>
template<size_t slot>
tref pre_order<node_t>::apply_until_change(auto& f) {
	return traverse<true, slot, false>(root, f, all, identity);
}

template<typename node_t>
void pre_order<node_t>::visit(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		const_traverse<false, false>(root, visit, visit_subtree, up);
}

template<typename node_t>
void pre_order<node_t>::visit(auto& visit) {
	const_traverse<false, false>(root, visit, all, identity);
}

template<typename node_t>
void pre_order<node_t>::search(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		const_traverse<true, false>(root, visit, visit_subtree, up);
}

template<typename node_t>
void pre_order<node_t>::search(auto& visit) {
	const_traverse<true, false>(root, visit, all, identity);
}

template<typename node_t>
void pre_order<node_t>::visit_unique (auto&visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		const_traverse<false, true>(root, visit, visit_subtree, up);
}

template<typename node_t>
void pre_order<node_t>::visit_unique (auto&visit) {
	const_traverse<false, true>(root, visit, all, identity);
}

template<typename node_t>
void pre_order<node_t>::search_unique (auto&visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		const_traverse<true, true>(root, visit, visit_subtree, up);
}

template<typename node_t>
void pre_order<node_t>::search_unique (auto&visit) {
	const_traverse<true, true>(root, visit, all, identity);
}

#ifdef MEASURE_TRAVERSER_DEPTH
template<typename node_t>
std::pair<size_t, size_t> pre_order<node_t>::get_tree_depth_and_size () {
	size_t c_depth = depth;
	size_t t_max_depth = depth;
	size_t size;
	auto find_max_depth = [&t_max_depth, &size] (const auto&){
		++size;
		if (depth > t_max_depth)
			t_max_depth = depth;
		return true;
	};
	visit(find_max_depth);
	return {t_max_depth - c_depth, size};
}
#endif //MEASURE_TRAVERSER_DEPTH

template<typename node_t>
template<bool break_on_change, size_t slot, bool unique>
tref pre_order<node_t>::traverse(tref n, auto& f, auto& visit_subtree, auto& up) {
	// std::unordered_map<node_t, node_t, std::hash<node_t>,
	// 	traverser_cache_equality<node_t>> cache;
	tref_cache_map cache;
	trefs stack;
	std::vector<size_t> upos;
	// Apply f and save on stack
	tref r = f(n);
	if (r == error_node<tref>::value)
		return error_node<tref>::value;
	if constexpr (break_on_change) {
		if (r == n) {
#ifdef MEASURE_TRAVERSER_DEPTH
			inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
			upos.push_back(0);
		}
	}
	else {
#ifdef MEASURE_TRAVERSER_DEPTH
		inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		upos.push_back(0);
	}
	stack.emplace_back(std::move(r));
	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return stack[0];
		// Find first unprocessed position
		auto& c_node = stack[upos.back()];
		// Check cache first
		// If we want to visit all nodes, deactivate caching/memory
		if constexpr (unique) {
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
		}
		auto c_tree = lcrs_tree<node_t>::get(c_node);
		auto c_children = c_tree.get_children_trefs();
		// Check if node has children
		if (c_children.empty()) {
			// Call up and move to next
			c_node = up(c_node);
			if (c_node == error_node<tref>::value)
				return error_node<tref>::value;
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
			continue;
		}
		// Get child position
		size_t c_pos = (stack.size() - 1) - upos.back();
		// Are all children visited?
		if (c_pos == c_children.size()) {
			// Check if children actually changed
			if (std::equal(stack.begin() + (upos.back() + 1), stack.end(),
				c_children.begin(), c_children.end())) {
				// Call up
				auto res = up(c_node);
				if (res == error_node<tref>::value)
					return error_node<tref>::value;
				if constexpr (unique) {
					if constexpr (slot != 0) m.emplace(std::make_pair(c_node, slot), res);
					else cache.emplace(c_node, res);
				}
				c_node = std::move(res);
				// Pop children from stacks
				stack.erase(stack.end() - c_pos, stack.end());
				upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
			// Make new node if children are different
			trefs children;
			for (size_t i = upos.back() + 1; i < stack.size(); ++i)
				children.emplace_back(std::move(stack[i]));
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			// make new node
			auto res = lcrs_tree<node_t>::get(c_tree.value, children);
			if (res == error_node<tref>::value)
				return error_node<tref>::value;
			// Call up
			res = up(res);
			if (res == error_node<tref>::value)
				return error_node<tref>::value;
			if constexpr (unique) {
				if constexpr (slot != 0) m.emplace(std::make_pair(c_node, slot), res);
				else cache.emplace(c_node, res);
			}
			c_node = std::move(res);
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			const auto& c = c_children[c_pos];
			if (visit_subtree(c)) {
				// Apply f and save on stack
				r = f(c);
				if (r == error_node<tref>::value)
					return error_node<tref>::value;
				if constexpr (break_on_change) {
					if (r == c) {
#ifdef MEASURE_TRAVERSER_DEPTH
						inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
						upos.push_back(stack.size());
					}
				}
				else {
#ifdef MEASURE_TRAVERSER_DEPTH
					inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
					upos.push_back(stack.size());
				}
				stack.emplace_back(std::move(r));
			}
			else stack.push_back(c);
		}
	}
}

template<typename node_t>
template<bool search, bool unique>
void pre_order<node_t>::const_traverse(tref n, auto& visit, auto& visit_subtree, auto& up) {
	// std::unordered_set<node_t, std::hash<node_t>,
	// 	traverser_cache_equality<node_t>> cache;
	std::unordered_set<tref> cache;
	std::vector<tref> stack;
	std::vector<size_t> upos;
	// visit n and save on stack
	bool ret = !visit(n);
	if constexpr (search) { if (ret) return; }
	stack.push_back(n);
	if (!ret) {
#ifdef MEASURE_TRAVERSER_DEPTH
		inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		upos.push_back(0);
	}
	if constexpr (unique) cache.emplace(n);
	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return;
		// Find first unprocessed position
		auto& c_node = stack[upos.back()];
		auto c_tree = lcrs_tree<node_t>::get(c_node);
		auto c_children = c_tree.get_children_trefs();
		// First check if node has children
		if (c_children.empty()) {
			// Call up and move to next
			up(c_node);
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
			continue;
		}
		// Get child position
		size_t c_pos = (stack.size() - 1) - upos.back();
		// Are all children visited?
		if (c_pos == c_children.size()) {
			// Call up
			up(c_node);
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			const auto& c = c_children[c_pos];
			stack.push_back(c);
			if constexpr (unique)
				if (cache.contains(c)) continue;
			if (visit_subtree(c)) {
				// visit c and save on stack
				ret = !visit(c);
				if constexpr (search) { if (ret) return; }
				if (!ret) {
#ifdef MEASURE_TRAVERSER_DEPTH
					inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
					upos.push_back(stack.size() - 1);
				}
			}
			if constexpr (unique) cache.emplace(n);
		}
	}
}

} // idni namespace
