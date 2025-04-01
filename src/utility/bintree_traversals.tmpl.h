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

static const auto all = [](tref) static { return true; };
static const auto none = [](tref) static { return false; };
static const auto identity = [](tref n) static { return n; };
static const auto do_nothing = [](tref) static {};

// Helper type traits to check callback properties
template<typename Cb>
using bool_accepts_tref = std::is_invocable_r<bool, Cb, tref>;

template<typename Cb>
using bool_accepts_tref_tref = std::is_invocable_r<bool, Cb, tref, tref>;

template<typename Cb>
using accepts_tref_tref = std::is_invocable<Cb, tref, tref>;


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
	if (n == nullptr) return nullptr;
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
	std::unordered_map<tref, typename tref_range<node_t>::iterator> ch_its;
	stack.push_back(n);
	upos.push_back(0);
#ifdef MEASURE_TRAVERSER_DEPTH
	inc_depth();
#endif //MEASURE_TRAVERSER_DEPTH
	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return stack[0];
		tref c_node = stack[upos.back()];
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
		const auto& c_tree = tree::get(c_node);
		// Check if node has children
		if (ch_its.find(c_node) == ch_its.end()) {
			if (ch_its.emplace(c_node, c_tree.children().begin())
				.first == nullptr)
			{
				// Process node and move to next
				c_node = f(c_node);
				if (c_node == nullptr) return nullptr;
				upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
		}
		// Get child iterator
		auto& it = ch_its.at(c_node);
		// Are all children visited?
		if (*it == nullptr) {
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
			// Check if children actually changed
			auto ch_range = c_tree.children();
			if (std::equal(stack.begin() + (upos.back() + 1), stack.end(),
				ch_range.begin(), ch_range.end())) {
				tref res = f(c_node);
				if (res == nullptr) return nullptr;
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
			tref res = tree::get(c_tree.value,
				&stack[upos.back() + 1], stack.end(),
				stack.size() - upos.back() - 1);
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			if (res == nullptr) return nullptr;
			res = f(res);
			if (res == nullptr) return nullptr;
			if constexpr (slot != 0) m.emplace(std::make_pair(c_node, slot), res);
			else cache.emplace(c_node, res);
			c_node = std::move(res);
			upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			tref c = *it; ++it;
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
void post_order<node_t>::const_traverse(tref n, auto& visitor,
	auto& visit_subtree)
{
	if (n == nullptr) return;
	// std::unordered_set<node_t, std::hash<node_t>,
	// 	traverser_cache_equality<node_t>> cache;
	tref_cache_set cache;
	trefs stack;
	std::vector<size_t> upos;
	std::unordered_map<tref, typename tref_range<node_t>::iterator> ch_its;
	std::unordered_map<tref, tref> parent;
	auto get_parent = [&parent, &upos, &stack](tref n) -> tref {
		auto it = parent.find(n);
		return it == parent.end() ? nullptr : it->second;
	};
	// Call callback with parent if it is invocable with tref, tref
	// Otherwise, call it just with tref
	auto call = [&get_parent](auto& cb, tref x) -> bool {
		if constexpr (accepts_tref_tref<decltype(cb)>::value) {
			if constexpr (bool_accepts_tref_tref<decltype(cb)>::value)
				return cb(x, get_parent(x));
			else cb(x, get_parent(x));
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
		if (ch_its.find(c_node) == ch_its.end()) {
			if (ch_its.emplace(c_node, c_tree.children().begin())
				.first == nullptr)
			{
				// Process node and move to next
				if (!call(visitor, c_node)) return;
				upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
		}
		// Get child iterator
		auto& it = ch_its.at(c_node);
		// First check if node has children
		// Are all children visited?
		if (*it == nullptr) {
			if (!call(visitor, c_node)) return;
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			upos.pop_back();
			ch_its.erase(c_node);
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			tref c = *it; ++it;
			stack.push_back(c);
			// Remember parent if is required by any of the callbacks
			if constexpr (std::is_invocable_v<decltype(visitor), tref, tref>
				|| std::is_invocable_v<decltype(visit_subtree), tref, tref>)
				parent.emplace(c, c_node);
			if constexpr (unique) if (cache.contains(c)) continue;
			// c_node can become invalid due to push_back
			if (call(visit_subtree, c)) {
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
void pre_order<node_t>::visit(auto& visit, auto& visit_subtree, auto& up, auto& between) {
	if (visit_subtree(root))
		const_traverse<false, false>(root, visit, visit_subtree, up, between);
}

template<typename node_t>
void pre_order<node_t>::visit(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		const_traverse<false, false>(root, visit, visit_subtree,
						up, do_nothing);
}

template<typename node_t>
void pre_order<node_t>::visit(auto& visit) {
	const_traverse<false, false>(root, visit, all, identity,
						do_nothing);
}

template<typename node_t>
void pre_order<node_t>::search(auto& visit, auto& visit_subtree, auto& up, auto& between) {
	if (visit_subtree(root))
		const_traverse<true, false>(root, visit, visit_subtree,
						up, between);
}

template<typename node_t>
void pre_order<node_t>::search(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root))
		const_traverse<true, false>(root, visit, visit_subtree,
						up, do_nothing);
}

template<typename node_t>
void pre_order<node_t>::search(auto& visit) {
	const_traverse<true, false>(root, visit, all, identity,	do_nothing);
}

template<typename node_t>
void pre_order<node_t>::visit_unique (auto& visit, auto& visit_subtree, auto& up) {
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
	if (n == nullptr) return nullptr;
	// std::unordered_map<node_t, node_t, std::hash<node_t>,
	// 	traverser_cache_equality<node_t>> cache;
	tref_cache_map cache;
	trefs stack;
	std::vector<size_t> upos;
	std::unordered_map<tref, typename tref_range<node_t>::iterator> ch_its;
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
	stack.emplace_back(std::move(r));
	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return stack[0];
		// Find first unprocessed position
		tref c_node = stack[upos.back()];
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
		const auto& c_tree = tree::get(c_node);
		// // Check if node has children
		if (ch_its.find(c_node) == ch_its.end()) {
			if (ch_its.emplace(c_node, c_tree.children().begin())
				.first == nullptr)
			{
				// Call up and move to next
				c_node = up(c_node);
				if (c_node == nullptr) return nullptr;
				upos.pop_back();
#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
		}
		auto& it = ch_its.at(c_node);
		// Are all children visited?
		if (*it == nullptr) {
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
			// Check if children actually changed
			auto ch_range = c_tree.children();
			if (std::equal(stack.begin() + (upos.back() + 1), stack.end(),
				ch_range.begin(), ch_range.end())) {
				// Call up
				tref res = up(c_node);
				if (res == nullptr) return nullptr;
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
			tref res = tree::get(c_tree.value,
				&stack[upos.back() + 1], stack.end(),
				stack.size() - upos.back() - 1);
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			if (res == nullptr) return nullptr;
			// Call up
			res = up(res);
			if (res == nullptr) return nullptr;
			if constexpr (unique) {
				if constexpr (slot != 0) m.emplace(std::make_pair(c_node, slot), res);
				else cache.emplace(c_node, res);
			}
			c_node = std::move(res);
			upos.pop_back();
			ch_its.erase(c_node);
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			tref c = *it; ++it;
			if (visit_subtree(c)) {
				// Apply f and save on stack
				r = f(c);
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
				stack.emplace_back(std::move(r));
			}
			else stack.push_back(c);
		}
	}
}

template<typename node_t>
template<bool search, bool unique>
void pre_order<node_t>::const_traverse(tref n, auto& visitor,
	auto& visit_subtree, auto& up, auto& between)
{
	if (n == nullptr) return;
	// std::unordered_set<node_t, std::hash<node_t>,
	// 	traverser_cache_equality<node_t>> cache;
	std::unordered_set<tref> cache;
	std::vector<tref> stack;
	std::vector<size_t> upos;
	std::unordered_map<tref, typename tref_range<node_t>::iterator> ch_its;
	std::unordered_map<tref, tref> parent;
	auto get_parent = [&parent, &upos, &stack](tref n) -> tref {
		auto it = parent.find(n);
		tref p = (it == parent.end() ? nullptr : it->second);
		// tree::get(n).print(std::cerr << "get_parent of: ") << std::endl;
		// std::cerr << "parent of " << n << " is " << p << std::endl;
		return p;
	};
	// Call callback with parent if it is invocable with tref, tref
	// Otherwise, call it just with tref
	auto call = [&get_parent](auto& cb, tref x) -> bool {
		if constexpr (accepts_tref_tref<decltype(cb)>::value) {
			if constexpr (
				bool_accepts_tref_tref<decltype(cb)>::value)
					return cb(x, get_parent(x));
			else cb(x, get_parent(x));
		} else
			if constexpr (bool_accepts_tref<decltype(cb)>::value) {
				return cb(x);
			} else cb(x);
		return true;
	};
	// visit n and save on stack
	bool ret = !call(visitor, n);
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
		const auto& c_tree = lcrs_tree<node_t>::get(c_node);
		if (ch_its.find(c_node) == ch_its.end()) {
			// Get child iterator
			if (ch_its.emplace(c_node, c_tree.children().begin())
				.first == nullptr)
			{ // If node has no children
				// Call up and move to next
				call(up, c_node);
				upos.pop_back();
	#ifdef MEASURE_TRAVERSER_DEPTH
				dec_depth();
	#endif //MEASURE_TRAVERSER_DEPTH
				continue;
			}
		}
		// Get child iterator
		auto& it = ch_its.at(c_node);
		// Are all children visited?
		if (*it == nullptr) {
			// Call up
			call(up, c_node);
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			upos.pop_back();
			ch_its.erase(c_node);
			if (c_tree.has_right_sibling())
				call(between, c_node);
#ifdef MEASURE_TRAVERSER_DEPTH
			dec_depth();
#endif //MEASURE_TRAVERSER_DEPTH
		} else {
			// Add next child
			tref c = *it; ++it;
			stack.push_back(c);
			// Remember parent if is required by any of the callbacks
			if constexpr (std::is_invocable_v<decltype(visitor), tref, tref>
				|| std::is_invocable_v<decltype(visit_subtree), tref, tref>
				|| std::is_invocable_v<decltype(up), tref, tref>
				|| std::is_invocable_v<decltype(between), tref, tref>)
					parent.emplace(c, c_node);
			if constexpr (unique)
				if (cache.contains(c)) continue;
			if (call(visit_subtree, c)) {
				// visit c and save on stack
				ret = !call(visitor, c);
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
