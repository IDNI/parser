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
		tref res = traverse<slot>(root, f, visit_subtree);
		return res;
	}
	else return root;
}

template <typename node>
template <size_t slot>
tref post_order<node>::apply_unique(auto& f) {
	tref res = traverse<slot>(root, f, all);
	return res;
}

template <typename node>
void post_order<node>::search(auto& visit, auto& visit_subtree) {
	if (visit_subtree(root)) {
		const_traverse<false>(root, visit, visit_subtree);
	}
}

template <typename node>
void post_order<node>::search(auto& visit) {
	const_traverse<false>(root, visit, all);
}

template <typename node>
void post_order<node>::search_unique(auto& visit, auto& visit_subtree) {
	if (visit_subtree(root)) {
		const_traverse<true>(root, visit, visit_subtree);
	}
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
		if (it != m.end())
			return tree::get(it->second,
					tree::get(n).right_sibling());
	}
	scratch<node, traversal_buffers<subtree_unordered_map<node, tref>, frame>> s;
	auto& cache = s.buffers->cache;
	auto& stack = s.buffers->stack;
	auto& frames = s.buffers->positions;
	stack.push_back(n);
	frames.push_back({ 0, tree::get(n).left_child(), false });
	TD(inc_depth();)

	// Writes a finished node back into the slot its frame occupied and
	// tells the parent whether it changed. Called after the frame is
	// popped, so `frames.back()` is the parent. Every close goes through
	// here, so no path can forget to report a change upwards.
	auto finish = [&stack, &frames](size_t pos, tref res) {
		tref& dest = stack[pos];
		if (res != dest) {
			dest = res;
			if (!frames.empty()) frames.back().dirty = true;
		}
	};

	// What the memo holds for `n`, or null. A node is only looked up once,
	// so this is the single place that reads either memo.
	auto memoized = [&cache](tref n) -> tref {
		TS(++tstats().cache_probes;)
	// A memo entry is keyed by subtree identity, which ignores the right
	// sibling, so the node it hands back may carry a different one than
	// the node being looked up. Re-attach the caller's sibling; this
	// costs a compare when it already matches.
		if constexpr (slot != 0) {
			const auto it = m.find(std::make_pair(n, slot));
			if (it == m.end()) return nullptr;
			TS(++tstats().cache_hits;)
			return tree::get(it->second,
					tree::get(n).right_sibling());
		} else {
			const auto it = cache.find(n);
			if (it == cache.end()) return nullptr;
			TS(++tstats().cache_hits;)
			return tree::get(it->second,
					tree::get(n).right_sibling());
		}
	};

	auto call = [](auto& cb, tref n) -> tref {
		tref nn = cb(n);
		if (nn == n) return n;
		if (nn == nullptr) return nullptr;
		// the result takes the sibling the original had; this form
		// skips the intern lookup when it already has it
		return tree::get(nn, tree::get(n).right_sibling());
	};

	while (true) {
		// If no unprocessed position exists, we are done
		if (frames.empty()) return stack[0];
		frame& fr = frames.back();
		tref& c_node = stack[fr.pos];
		DBGT(std::cout << "\nnon-const loop begin: "
			<< tree::get(c_node).dump_to_str() << "\n";)
		DBGT(print_stack<node>(stack, c_node);)
		// Check if node has children
		if (!tree::get(c_node).has_child()) {
			// Process node and move to next
			const tref res = call(f, c_node);
			if (res == nullptr) return nullptr;
			const size_t pos = fr.pos;
			frames.pop_back();
			finish(pos, res);
			TD(dec_depth();)
			continue;
		}
		const tref c = fr.next_child;
		DBGT(std::cout << "\tmove to a child: " << c << " \t"
			<< (stack.back() == c_node ? "LC" : "RS") << "\n";)
		// Are all children visited?
		if (c == nullptr) {
			// Get child position
			const size_t c_pos = (stack.size() - 1) - fr.pos;
			// No child changed, so the node stands as it is
			if (!fr.dirty) {
				const tref key = c_node;
				tref res = call(f, key);
				if (res == nullptr) return nullptr;
				if constexpr (slot != 0) m.emplace(
					std::make_pair(key, slot), res);
				else cache.emplace(key, res);
				// Pop children from stacks
				stack.erase(stack.end() - c_pos, stack.end());
				const size_t pos = fr.pos;
				frames.pop_back();
				finish(pos, res);
				TD(dec_depth();)
				continue;
			}
			// Make new node if children are different
			TS(++tstats().rebuilds;)
			const tref key = c_node;
			tref res = tree::get(tree::get(c_node).value,
				&stack[fr.pos + 1],
				c_pos,
				tree::get(c_node).right_sibling());
			DBGT(std::cout << "\tnew node: " << tree::get(res).dump_to_str() << "\n";)
			if (res == nullptr) return nullptr;
			// Pop children from stacks
			stack.erase(stack.end() - c_pos, stack.end());
			res = call(f, res);
			if (res == nullptr) return nullptr;
			if constexpr (slot != 0)
				m.emplace(std::make_pair(key, slot), res);
			else cache.emplace(key, res);
			const size_t pos = fr.pos;
			frames.pop_back();
			finish(pos, res);
			TD(dec_depth();)
		} else {
			// Advance this frame before touching either vector:
			// pushing can reallocate, which would leave `fr` and
			// `c_node` dangling
			TS(++tstats().sibling_steps;)
			fr.next_child = tree::get(c).right_sibling();
			// Add next child
			stack.push_back(c);
			if (visit_subtree(c)) {
				// The memo is consulted here, once, rather
				// than on every turn around the loop: while a
				// frame is open only its own descendants are
				// added to the memo, and a descendant is
				// never equal to the node it sits under, so a
				// later look could not find anything this one
				// did not. A subtree that is not visited is
				// not looked up either, as before.
				// A leaf is never put in the memo: entries are
				// only made where a node's children finish.
				// Subtree identity includes the child list,
				// so a leaf can never match a memoized node
				// either - looking one up is always a miss,
				// and leaves are about half of a binary tree.
				const tref first = tree::get(c).left_child();
				const tref hit = first == nullptr ? nullptr
							: memoized(c);
				if (hit != nullptr) {
					stack.back() = hit;
					if (hit != c) fr.dirty = true;
				} else {
					TD(inc_depth();)
					TS(++tstats().frames_opened;)
					frames.push_back({ stack.size() - 1,
							first, false });
				}
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
	scratch<node, walk_buffers<subtree_unordered_set<node>>> s;
	auto& cache = s.buffers->cache;
	auto& frames = s.buffers->frames;
	// The node being worked on and its cursor stay in locals rather than
	// being read back out of `frames` each turn. Stepping to the next
	// sibling through a reference into the vector puts a store and a
	// dependent reload on the loop's critical path, which costs more than
	// the sibling walking it replaces; `frames` is touched only when the
	// traversal actually descends or returns.
	tref current = n;
	tref cursor = tree::get(n).left_child();

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
	if constexpr (unique) cache.insert(n);
	TD(inc_depth();)
	while (true) {
		// Every child visited - a node with none arrives here at once
		if (cursor == nullptr) {
			if (frames.empty()) {
				// the root: nothing above it to return to
				call(visitor, current, nullptr);
				return;
			}
			// one read of the frame below serves as this node's
			// parent and as where to carry on
			const walk_frame below = frames.back();
			frames.pop_back();
			if (!call(visitor, current, below.node)) return;
			current = below.node;
			cursor = below.next_child;
			TD(dec_depth();)
			continue;
		}
		const tref c = cursor;
		TS(++tstats().sibling_steps;)
		cursor = tree::get(c).right_sibling();
		// if unique, skip already visited nodes
		if constexpr (unique) {
			TS(++tstats().cache_probes;)
			if (cache.contains(c)) {
				TS(++tstats().cache_hits;)
				continue;
			}
		}
		if (call(visit_subtree, c, current)) {
			TD(inc_depth();)
			TS(++tstats().frames_opened;)
			frames.push_back({ current, cursor });
			current = c;
			cursor = tree::get(c).left_child();
		}
		if constexpr (unique) cache.insert(c);
	}
}

} // idni namespace
