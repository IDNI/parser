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

namespace idni {

// #define LOG_TRAVERSALS_ENABLED

#ifdef LOG_TRAVERSALS_ENABLED
#define DBGT(x) x
#else
#define DBGT(x)
#endif // LOG_TRAVERSALS_ENABLED

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

struct traverser_cache_equality {
	bool operator() (tref l, tref r) const {
		return l == r;
	}
};

struct traverser_pair_cache_equality {
	using p = std::pair<tref, size_t>;
	bool operator() (const p& l, const p& r) const {
		return l == r;
	}
};

template <typename node>
post_order<node>::post_order(tref n) : root(n) {}

template <typename node>
post_order<node>::post_order(const htree::sp& h) : root(h->get()) {}

template <typename node>
template <size_t slot>
tref post_order<node>::apply_unique(auto& f, auto& visit_subtree) {
	if (visit_subtree(root))
		return traverse<slot>(root, f, visit_subtree);
	else return root;
}

template <typename node>
template <size_t slot>
tref post_order<node>::apply_unique(auto& f) {
	return traverse<slot>(root, f, all);
}

template <typename node>
void post_order<node>::search(auto& visit, auto& visit_subtree) {
	if (visit_subtree(root))
		const_traverse<false>(root, visit, visit_subtree);
}

template <typename node>
void post_order<node>::search(auto& visit) {
	const_traverse<false>(root, visit, all);
}

template <typename node>
void post_order<node>::search_unique(auto& visit, auto& visit_subtree) {
	if (visit_subtree(root))
		const_traverse<true>(root, visit, visit_subtree);
}

template <typename node>
void post_order<node>::search_unique(auto& visit) {
	const_traverse<true>(root, visit, all);
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
	// std::unordered_map<tref, tref, std::hash<tref>,
	// 	traverser_cache_equality> cache;
	std::unordered_map<tref, tref> cache;
	std::vector<tref> stack;
	std::vector<size_t> upos;
	stack.push_back(n);
	upos.push_back(0);
#ifdef MEASURE_TRAVERSER_DEPTH
	inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH

#ifdef LOG_TRAVERSALS_ENABLED
	auto pst = [&stack](tref cn) {
		std::cout << "\tstack: ";
		for (tref n : stack) {
			std::cout << "\n\t\t";
			std::stringstream ss;
			const auto& c_tree = tree::get(n);
			ss << c_tree.value << ":";
			while (ss.tellp() < 16) ss << " ";
			ss << (n == cn ? "[" : " ") << n << (n == cn ? "]" : " ");
			if (c_tree.l) ss << " __ " << c_tree.l;
			if (c_tree.r) ss << " >> " << c_tree.r;
			std::cout << ss.str();
		}
		std::cout << "\n";
	};
#endif //LOG_TRAVERSALS_ENABLED

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
		DBGT(pst(c_node);)
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

		// Get next child
		tref c = (stack.back() == c_node)
				? tree::get(c_node).left_child()
				: tree::get(stack.back()).right_sibling();
		DBGT(std::cout << "\tmove to a child: " << c << " \t"
			<< (stack.back() == c_node ? "LC" : "RS") << "\n";)
		// Are all children visited?
		if (c == nullptr) {
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
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
				stack.size() - upos.back() - 1,
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
	subtree_set<node> cache; // TODO subtree_unordered_set
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

template <typename node>
pre_order<node>::pre_order(tref n) : root(n) {}

template <typename node>
pre_order<node>::pre_order(const htree::sp& h) : root(h->get()) {}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		return traverse<false, slot, true>(root, f, visit_subtree, up);
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique(auto& f, auto& visit_subtree) {
	if (visit_subtree(root))
		return traverse<false, slot, true>(root, f, visit_subtree,
								identity);
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique(auto& f) {
	return traverse<false, slot, true>(root, f, all, identity);
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		return traverse<false, slot, false>(root, f, visit_subtree, up);
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply(auto& f) {
	return traverse<false, slot, false>(root, f, all, identity);
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f, auto& visit_subtree,
	auto& up)
{
	if (visit_subtree(root))
		return traverse<true, slot, true>(root, f, visit_subtree, up);
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f, auto& visit_subtree){
	if (visit_subtree(root))
		return traverse<true, slot, true>(
					root, f, visit_subtree, identity);
	else return root;
}


template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f) {
	return traverse<true, slot, true>(root, f, all, identity);
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_until_change(auto& f, auto& visit_subtree,
	auto& up)
{
	if (visit_subtree(root))
		return traverse<true, slot, false>(root, f, visit_subtree, up);
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_until_change(auto& f) {
	return traverse<true, slot, false>(root, f, all, identity);
}

template <typename node>
void pre_order<node>::visit(auto& visit, auto& visit_subtree, auto& up,
	auto& between)
{
	if (visit_subtree(root)) const_traverse<false, false>(root,
					visit, visit_subtree, up, between);
}

template <typename node>
void pre_order<node>::visit(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) const_traverse<false, false>(root,
					visit, visit_subtree, up, do_nothing);
}

template <typename node>
void pre_order<node>::visit(auto& visit) {
	const_traverse<false, false>(root, visit, all, identity, do_nothing);
}

template <typename node>
void pre_order<node>::search(auto& visit, auto& visit_subtree, auto& up,
	auto& between)
{
	if (visit_subtree(root)) const_traverse<true, false>(root,
					visit, visit_subtree, up, between);
}

template <typename node>
void pre_order<node>::search(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) const_traverse<true, false>(root,
					visit, visit_subtree, up, do_nothing);
}

template <typename node>
void pre_order<node>::search(auto& visit) {
	const_traverse<true, false>(root, visit, all, identity, do_nothing);
}

template <typename node>
void pre_order<node>::visit_unique(auto& visit, auto& visit_subtree, auto& up)
{
	if (visit_subtree(root)) const_traverse<false, true>(root,
					visit, visit_subtree, up, do_nothing);
}

template <typename node>
void pre_order<node>::visit_unique(auto& visit) {
	const_traverse<false, true>(root, visit, all, identity, do_nothing);
}

template <typename node>
void pre_order<node>::search_unique(auto&visit, auto& visit_subtree, auto& up)
{
	if (visit_subtree(root)) const_traverse<true, true>(root,
					visit, visit_subtree, up, do_nothing);
}

template <typename node>
void pre_order<node>::search_unique(auto&visit) {
	const_traverse<true, true>(root, visit, all, identity, do_nothing);
}

#ifdef MEASURE_TRAVERSER_DEPTH
template <typename node>
std::pair<size_t, size_t> pre_order<node>::get_tree_depth_and_size() {
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

template <typename node>
template<bool break_on_change, size_t slot, bool unique>
tref pre_order<node>::traverse(tref n, auto& f, auto& visit_subtree, auto& up)
{
	if (n == nullptr) return nullptr;
	// std::unordered_map<node_t, node_t, std::hash<node>,
	// 	traverser_cache_equality<node>> cache;
	std::unordered_map<tref, tref> cache;
	trefs stack;
	std::vector<size_t> upos;
	// Apply f and save on stack
	tref r = f(n);
	if (r == nullptr) return nullptr;
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
	stack.emplace_back(r);

#ifdef LOG_TRAVERSALS_ENABLED
	auto pst = [&stack](tref cn) {
		std::cout << "\tstack: ";
		for (tref n : stack) {
			std::cout << "\n\t\t";
			std::stringstream ss;
			const auto& c_tree = tree::get(n);
			ss << c_tree.value << ":";
			while (ss.tellp() < 16) ss << " ";
			ss << (n == cn ? "[" : " ") << n << (n == cn ? "]" : " ");
			if (c_tree.l) ss << " __ " << c_tree.l;
			if (c_tree.r) ss << " >> " << c_tree.r;
			std::cout << ss.str();
		}
		std::cout << "\n";
	};
#endif //LOG_TRAVERSALS_ENABLED

	auto call = [](auto& cb, tref n) -> tref {
		tref nn = cb(n);
		if (nn == n) return n;
		if (nn == nullptr) return nullptr;
		return tree::get(nn, tree::get(n).right_sibling());
	};

	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return stack[0];
		// Find first unprocessed position
		tref& c_node = stack[upos.back()];
		DBGT(std::cout << "\nnon-const loop begin: "
			<< tree::get(c_node).dump_to_str() << "\n";)
		DBGT(pst(c_node);)
		// Check cache first
		// If we want to visit all nodes, deactivate caching/memory
		if constexpr (unique) {
			if constexpr (slot != 0) {
				const auto it = m.find(
						std::make_pair(c_node, slot));
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
		// // Check if node has children
		if (!tree::get(c_node).has_child()) {
			// Call up and move to next
			c_node = call(up, c_node);
			if (c_node == nullptr) return nullptr;
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
			continue;
		}
		tref c = (stack.back() == c_node)
				? tree::get(c_node).left_child()
				: tree::get(stack.back()).right_sibling();
		DBGT(std::cout << "\tmove to a child: " << c << " \t"
			<< (stack.back() == c_node ? "LC" : "RS") << "\n";)
		// Are all children visited?
		if (c == nullptr) {
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
			// Check if children actually changed
			auto ch_range = tree::get(c_node).children();
			if (std::equal(stack.begin() + (upos.back() + 1),
				stack.end(), ch_range.begin(), ch_range.end()))
			{
				// Call up
				auto res = call(up, c_node);
				if (res == nullptr) return nullptr;
				if constexpr (unique) {
					if constexpr (slot != 0) m.emplace(
						std::make_pair(c_node, slot),
						res);
					else cache.emplace(c_node, res);
				}
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
				stack.size() - upos.back() - 1,
				tree::get(c_node).right_sibling());
			DBGT(std::cout << "\tnew node: " << tree::get(res).dump_to_str() << "\n";)
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			if (res == nullptr) return nullptr;
			// Call up
			res = call(up, res);
			if (res == nullptr) return nullptr;
			if constexpr (unique) {
				if constexpr (slot != 0)
					m.emplace(std::make_pair(c_node, slot),
									res);
				else cache.emplace(c_node, res);
			}
			c_node = res;
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			if (visit_subtree(c)) {
				// Apply f and save on stack
				r = call(f, c);
				if (r == nullptr) return nullptr;
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
				stack.emplace_back(r);
			}
			else stack.push_back(c);
		}
	}
}

template <typename node>
template<bool search, bool unique>
void pre_order<node>::const_traverse(tref n, auto& visitor,
	auto& visit_subtree, auto& up, auto& between)
{
	if (n == nullptr) return;
	subtree_set<node> cache; // TODO subtree_unordered_set
	std::vector<tref> stack;
	std::vector<size_t> upos;
	auto get_parent = [&upos, &stack]() -> tref {
		return upos.empty() ? nullptr : stack[upos.back()];
	};
	// Call callback with parent if it is invocable with tref, tref
	// Otherwise, call it just with tref
	auto call = [](auto& cb, tref x, tref parent) -> bool {
		if constexpr (accepts_tref_tref<decltype(cb)>::value) {
			if constexpr (
				bool_accepts_tref_tref<decltype(cb)>::value)
					return cb(x, parent);
			else cb(x, parent);
		} else
			if constexpr (bool_accepts_tref<decltype(cb)>::value) {
				return cb(x);
			} else cb(x);
		return true;
	};
	// visit n and save on stack
	bool ret = !call(visitor, n, nullptr);
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
		tref c_node = stack[upos.back()];
		if (c_node == nullptr) return;
		const auto& c_tree = tree::get(c_node);
		if (!c_tree.has_child()) { // If node has no children
			// Call up and move to next
			upos.pop_back();
			call(up, c_node, get_parent());
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
			// Call up (get parent from stack[upos[upos.size() - 2]])
			call(up, c_node, upos.size() < 2 ? nullptr
					: stack[upos[upos.size() - 2]]);
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			upos.pop_back();
			// Node is finished. Call between if has right sibling
			if (c_tree.has_right_sibling())
				call(between, c_node, get_parent());
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			stack.push_back(c);
			if constexpr (unique) if (cache.contains(c)) continue;
			if (call(visit_subtree, c, c_node)) {
				// visit c and save on stack
				ret = !call(visitor, c, c_node);
				if constexpr (search) { if (ret) return; }
				if (!ret) {
#ifdef MEASURE_TRAVERSER_DEPTH
					inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
					upos.push_back(stack.size() - 1);
				}
				// // Node is finished. Call between if has right sibling
				// if (tree::get(c).has_right_sibling())
				// 	call(between, c, get_parent());
			}
			if constexpr (unique) cache.emplace(c);
		}
	}
}

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
