// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "tree_traversals.tmpl.h"

namespace idni {

template <typename node>
pre_order<node>::pre_order(tref n) : root(n) {}

template <typename node>
pre_order<node>::pre_order(const htref& h) : root(h->get()) {}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		tref res = traverse<false, slot, true>(root, f, visit_subtree, up);
		bintree<node>::gc_enabled = true;
		return res;
	}

	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique(auto& f, auto& visit_subtree) {
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		tref res = traverse<false, slot, true>(root, f, visit_subtree,
								identity);
		bintree<node>::gc_enabled = true;
		return res;
	}
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique(auto& f) {
	bintree<node>::gc_enabled = false;
	tref res = traverse<false, slot, true>(root, f, all, identity);
	bintree<node>::gc_enabled = true;
	return res;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		tref res = traverse<false, slot, false>(root, f, visit_subtree, up);
		bintree<node>::gc_enabled = true;
		return res;
	}
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply(auto& f) {
	bintree<node>::gc_enabled = false;
	tref res = traverse<false, slot, false>(root, f, all, identity);
	bintree<node>::gc_enabled = true;
	return res;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f, auto& visit_subtree,
	auto& up)
{
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		tref res = traverse<true, slot, true>(root, f, visit_subtree, up);
		bintree<node>::gc_enabled = true;
		return res;
	}
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f, auto& visit_subtree){
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		tref res = traverse<true, slot, true>(
					root, f, visit_subtree, identity);
		bintree<node>::gc_enabled = true;
		return res;
	}
	else return root;
}


template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f) {
	bintree<node>::gc_enabled = false;
	tref res = traverse<true, slot, true>(root, f, all, identity);
	bintree<node>::gc_enabled = true;
	return res;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_until_change(auto& f, auto& visit_subtree,
	auto& up)
{
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		tref res = traverse<true, slot, false>(root, f, visit_subtree, up);
		bintree<node>::gc_enabled = true;
		return res;
	}
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_until_change(auto& f) {
	bintree<node>::gc_enabled = false;
	tref res = traverse<true, slot, false>(root, f, all, identity);
	bintree<node>::gc_enabled = true;
	return res;
}

template <typename node>
void pre_order<node>::visit(auto& visit, auto& visit_subtree, auto& up,
	auto& between)
{
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		const_traverse<false, false>(root,
					visit, visit_subtree, up, between);
		bintree<node>::gc_enabled = true;
	}
}

template <typename node>
void pre_order<node>::visit(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		const_traverse<false, false>(root,
					visit, visit_subtree, up, do_nothing);
		bintree<node>::gc_enabled = true;
	}
}

template <typename node>
void pre_order<node>::visit(auto& visit) {
	bintree<node>::gc_enabled = false;
	const_traverse<false, false>(root, visit, all, identity, do_nothing);
	bintree<node>::gc_enabled = true;
}

template <typename node>
void pre_order<node>::search(auto& visit, auto& visit_subtree, auto& up,
	auto& between)
{
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		const_traverse<true, false>(root,
					visit, visit_subtree, up, between);
		bintree<node>::gc_enabled = true;
	}
}

template <typename node>
void pre_order<node>::search(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		const_traverse<true, false>(root,
					visit, visit_subtree, up, do_nothing);
		bintree<node>::gc_enabled = true;
	}
}

template <typename node>
void pre_order<node>::search(auto& visit) {
	bintree<node>::gc_enabled = false;
	const_traverse<true, false>(root, visit, all, identity, do_nothing);
	bintree<node>::gc_enabled = true;
}

template <typename node>
void pre_order<node>::visit_unique(auto& visit, auto& visit_subtree, auto& up)
{
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		const_traverse<false, true>(root,
					visit, visit_subtree, up, do_nothing);
		bintree<node>::gc_enabled = true;
	}
}

template <typename node>
void pre_order<node>::visit_unique(auto& visit) {
	bintree<node>::gc_enabled = false;
	const_traverse<false, true>(root, visit, all, identity, do_nothing);
	bintree<node>::gc_enabled = true;
}

template <typename node>
void pre_order<node>::search_unique(auto&visit, auto& visit_subtree, auto& up)
{
	if (visit_subtree(root)) {
		bintree<node>::gc_enabled = false;
		const_traverse<true, true>(root,
					visit, visit_subtree, up, do_nothing);
		bintree<node>::gc_enabled = true;
	}
}

template <typename node>
void pre_order<node>::search_unique(auto&visit) {
	bintree<node>::gc_enabled = false;
	const_traverse<true, true>(root, visit, all, identity, do_nothing);
	bintree<node>::gc_enabled = true;
}


template <typename node>
template<bool break_on_change, size_t slot, bool unique>
tref pre_order<node>::traverse(tref n, auto& f, auto& visit_subtree, auto& up)
{
	if (n == nullptr) return nullptr;
	subtree_unordered_map<node, tref> cache;
	trefs stack;
	std::vector<frame> frames;
	auto get_parent = [&frames, &stack]() -> tref {
		return frames.empty() ? nullptr : stack[frames.back().pos];
	};
	auto call = [&get_parent](auto& cb, tref n) -> tref {
		tref nn;
		if constexpr (accepts_tref_tref<decltype(cb)>::value)
				nn = cb(n, get_parent());
		else nn = cb(n);
		if (nn == n) return n;
		if (nn == nullptr) {
			DBGT(std::cerr << " call returned nullptr\n";)
			return nullptr;
		}
		nn = tree::get(nn, tree::get(n).right_sibling());
		DBGT(std::cout << "\tcall returned: " << tree::get(nn).dump_to_str() << "\n";)
		return nn;
	};
	// Apply f and save on stack
	tref r = call(f, n);
	if (r == nullptr) return nullptr;
	if constexpr (break_on_change) {
		if (r == n) {
			TD(inc_depth();)
			TS(++tstats().frames_opened;)
			frames.push_back({ 0, tree::get(r).left_child() });
		} else {
			r = call(up, r);
		}
	}
	// If the transformed node should not be
	// visited, do not open a frame for it
	else if (visit_subtree(r)) {
		TD(inc_depth();)
		TS(++tstats().frames_opened;)
		frames.push_back({ 0, tree::get(r).left_child() });
	} else {
		r = call(up, r);
		if (r == nullptr) return nullptr;
	}
	stack.emplace_back(r);
	while (true) {
		// If no unprocessed position exists, we are done
		if (frames.empty()) return stack[0];
		// Find first unprocessed position
		frame& fr = frames.back();
		tref& c_node = stack[fr.pos];
		DBGT(std::cout << "\nnon-const loop begin: "
			<< tree::get(c_node).dump_to_str() << "\n";)
		DBGT(print_stack<node>(stack, c_node);)
		// Check cache first
		// If we want to visit all nodes, deactivate caching/memory
		if constexpr (unique) {
			if constexpr (slot != 0) {
				TS(++tstats().cache_probes;)
				const auto it = m.find(
						std::make_pair(c_node, slot));
				if (it != m.end()) {
					TS(++tstats().cache_hits;)
					c_node = it->second;
					frames.pop_back();
					TD(dec_depth();)
					continue;
				}
			} else {
				TS(++tstats().cache_probes;)
				const auto it = cache.find(c_node);
				if (it != cache.end()) {
					TS(++tstats().cache_hits;)
					c_node = it->second;
					frames.pop_back();
					TD(dec_depth();)
					continue;
				}
			}
		}
		// // Check if node has children
		if (!tree::get(c_node).has_child()) {
			// Call up and move to next
			frames.pop_back();
			c_node = call(up, c_node);
			if (c_node == nullptr) return nullptr;
			TD(dec_depth();)
			continue;
		}
		const tref c = fr.next_child;
		DBGT(std::cout << "\tmove to a child: " << c << " \t"
			<< (stack.back() == c_node ? "LC" : "RS") << "\n";)
		// Are all children visited?
		if (c == nullptr) {
			// `fr` does not survive the pops below, so take what
			// is still needed from it first
			const size_t pos = fr.pos;
			const size_t c_pos = (stack.size() - 1) - pos;
			// Check if children actually changed
			auto ch_range = tree::get(c_node).children();
			TS(tstats().children_compared += c_pos;)
			if (std::equal(stack.begin() + (pos + 1),
				stack.end(), ch_range.begin(), ch_range.end()))
			{
				// Call up
				frames.pop_back();
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
				TD(dec_depth();)
				continue;
			}
			// Make new node if children are different
			TS(++tstats().rebuilds;)
			tref res = tree::get(tree::get(c_node).value,
				&stack[pos + 1],
				c_pos,
				tree::get(c_node).right_sibling());
			DBGT(std::cout << "\tnew node: " << tree::get(res).dump_to_str() << "\n";)
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			if (res == nullptr) return nullptr;
			// Call up
			frames.pop_back();
			res = call(up, res);
			if (res == nullptr) return nullptr;
			if constexpr (unique) {
				if constexpr (slot != 0)
					m.emplace(std::make_pair(c_node, slot),
									res);
				else cache.emplace(c_node, res);
			}
			c_node = res;
			TD(dec_depth();)
		} else {
			// Advance this frame before anything can reallocate
			// either vector, which would leave `fr` and `c_node`
			// dangling. The callbacks below still see the parent
			// frame on top, so get_parent() stays correct.
			TS(++tstats().sibling_steps;)
			fr.next_child = tree::get(c).right_sibling();
			// Add next child
			if (visit_subtree(c)) {
				// Apply f and save on stack
				r = call(f, c);
				if (r == nullptr) return nullptr;
				if constexpr (break_on_change) {
					if (r == c) {
						TD(inc_depth();)
						TS(++tstats().frames_opened;)
						frames.push_back({ stack.size(),
						  tree::get(r).left_child() });
					} else {
						r = call(up, r);
					}
				}
				// If the transformed node should not be
				// visited, do not open a frame for it
				else if (visit_subtree(r)) {
					TD(inc_depth();)
					TS(++tstats().frames_opened;)
					frames.push_back({ stack.size(),
						tree::get(r).left_child() });
				} else {
					r = call(up, r);
					if (r == nullptr) return nullptr;
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
	subtree_unordered_set<node> cache;
	std::vector<tref> stack;
	std::vector<size_t> upos;
	auto get_parent = [&upos, &stack]() -> tref {
		return upos.empty() ? nullptr : stack[upos.back()];
	};
	// Call callback with parent if it is invocable with tref, tref
	// Otherwise, call it just with tref
	// `name` only labels the log line, so it stays a plain pointer: a
	// std::string parameter would construct and destroy a temporary at
	// every call site on every node, including when logging is off
	auto call = [](auto& cb, tref x, tref parent,
		[[maybe_unused]] const char* name) -> bool
	{
		DBGT(std::cerr << "!! calling " << name << "("
			<< tree::get(x).value << ")";)
		auto lg = [&](bool ret) -> bool {
			DBGT(std::cout << " " << (ret?"true":"false") << "\n";)
			return ret;
		};
		if constexpr (accepts_tref_tref<decltype(cb)>::value) {
			if constexpr (bool_accepts_tref_tref<decltype(cb)>
									::value)
				return lg(cb(x, parent));
			else cb(x, parent);
		} else
			if constexpr (bool_accepts_tref<decltype(cb)>::value) {
				return lg(cb(x));
			} else cb(x);
		DBGT(std::cerr << " N/A\n";)
		return true;
	};
	// visit n and save on stack
	if (!call(visit_subtree, n, nullptr, "visit_subtree"))
		return;
	bool ret = !call(visitor, n, nullptr, "visitor0");
	if constexpr (search) { if (ret) return; }
	stack.push_back(n);
	if (!ret) {
		TD(inc_depth();)
		upos.push_back(0);
	}
	if constexpr (unique) cache.insert(n);
	while (true) {
		// If no unprocessed position exists, we are done
		if (upos.empty()) return;
		// Find first unprocessed position
		tref c_node = stack[upos.back()];
		DBGT(std::cout << "-- non-const loop begin: "
			<< tree::get(c_node).value << "\n";)
		DBGT(print_stack<node>(stack, c_node);)
		const auto& c_tree = tree::get(c_node);
		if (!c_tree.has_child()) { // If node has no children
			// Call up and move to next
			upos.pop_back();
			call(up, c_node, get_parent(), "up1");
			if (c_tree.has_right_sibling())
				call(between, c_node, get_parent(), "between");
			TD(dec_depth();)
			continue;
		}
		// Get next child
		TS(++tstats().sibling_steps;)
		tref c = (stack.back() == c_node) ? c_tree.left_child()
				: tree::get(stack.back()).right_sibling();
		DBGT(std::cout << "-- move to a child: " << c;)
		DBGT(if (c) std::cout << tree::get(c).value;)
		DBGT(std::cout << " " << (stack.back() == c_node
						? "LC" : "RS") << "\n\n";)
		// Are all children visited?
		if (c == nullptr) {
			// Get child position
			size_t c_pos = (stack.size() - 1) - upos.back();
			upos.pop_back();
			// Call up
			call(up, c_node, get_parent(), "up2");

			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			// Node is finished. Call between if has right sibling
			if (c_tree.has_right_sibling())
				call(between, c_node, get_parent(), "between");
			TD(dec_depth();)
		} else {
			// Add next child
			stack.push_back(c);
			if constexpr (unique) {
				TS(++tstats().cache_probes;)
				if (cache.contains(c)) {
					TS(++tstats().cache_hits;)
					continue;
				}
			}
			if (call(visit_subtree, c, c_node, "visit_subtree")) {
				// visit c and save on stack
				ret = !call(visitor, c, c_node, "visitor");
				if constexpr (search) { if (ret) return; }
				if (!ret) {
					TD(inc_depth();)
					TS(++tstats().frames_opened;)
					upos.push_back(stack.size() - 1);
				}
			}
			if constexpr (unique) cache.insert(c);
		}
	}
}

} // idni namespace
