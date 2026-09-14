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
	scratch<node, traversal_buffers<subtree_memo<node>, build_frame>> s;
	auto& cache = s.buffers->cache;
	auto& stack = s.buffers->stack;
	auto& frames = s.buffers->positions;
	frames.push_back({ n, tree::get(n).left_child(), nullptr, 0, false });
	TD(inc_depth();)

	// Records what a child finished as. Nothing is buffered until a child
	// changes; that first change buffers the children before it, and
	// every child after it.
	auto child_done = [&stack, &frames](tref original, tref res) {
		build_frame& fr = frames.back();
		if (!fr.dirty) {
			if (res == original) return;
			fr.dirty = true;
			for (tref c = tree::get(fr.node).left_child();
					c != original;
					c = tree::get(c).right_sibling())
				stack.push_back(c);
		}
		stack.push_back(res);
	};

	// What the memo holds for `n`, or null. The only place either memo is
	// read.
	auto memoized = [&cache](tref n) -> tref {
		TS(++tstats().cache_probes;)
		// Memo keys are subtree identities, which ignore the right
		// sibling, so the stored node may carry a different one.
		// Re-attach `n`'s sibling; a compare when it already matches.
		if constexpr (slot != 0) {
			const auto it = m.find(std::make_pair(n, slot));
			if (it == m.end()) return nullptr;
			TS(++tstats().cache_hits;)
			return tree::get(it->second,
					tree::get(n).right_sibling());
		} else {
			const tref found = cache.find(n);
			if (found == nullptr) return nullptr;
			TS(++tstats().cache_hits;)
			return tree::get(found, tree::get(n).right_sibling());
		}
	};

	auto call = [](auto& cb, tref n) -> tref {
		tref nn = cb(n);
		if (nn == n) return n;
		if (nn == nullptr) return nullptr;
		// give the result the sibling `n` had
		return tree::get(nn, tree::get(n).right_sibling());
	};

	while (true) {
		build_frame& fr = frames.back();
		const tref c = fr.next_child;
		DBGT(std::cout << "\nnon-const loop begin: "
			<< tree::get(fr.node).dump_to_str() << "\n";)
		// All children done; a leaf reaches this on its first turn
		if (c == nullptr) {
			const tref finished = fr.node;
			const tref origin = fr.origin;
			const size_t start = fr.buffer_start;
			const bool has_children =
				tree::get(finished).left_child() != nullptr;
			tref res = finished;
			if (fr.dirty) {
				// Rebuild from the buffered children
				TS(++tstats().rebuilds;)
				res = tree::get(tree::get(finished).value,
					&stack[start], stack.size() - start,
					tree::get(finished).right_sibling());
				stack.resize(start);
				if (res == nullptr) return nullptr;
			}
			res = call(f, res);
			if (res == nullptr) return nullptr;
			// Leaves are kept out of the memo, so that looking
			// one up can be skipped as a certain miss
			if (has_children) {
				if constexpr (slot != 0) m.emplace(
					std::make_pair(finished, slot), res);
				else cache.insert(finished, res);
			}
			frames.pop_back();
			TD(dec_depth();)
			if (frames.empty()) return res;
			child_done(origin, res);
			continue;
		}
		// advance before descending: opening a frame can move
		// `frames`, which would leave `fr` dangling
		TS(++tstats().sibling_steps;)
		fr.next_child = tree::get(c).right_sibling();
		if (!visit_subtree(c)) {
			child_done(c, c);
			continue;
		}
		// The memo is read once per node: while a frame is open only
		// its descendants are added, and none of those matches the
		// node above them.
		const tref first = tree::get(c).left_child();
		const tref hit = first == nullptr ? nullptr : memoized(c);
		if (hit != nullptr) {
			child_done(c, hit);
			continue;
		}
		TD(inc_depth();)
		TS(++tstats().frames_opened;)
		frames.push_back({ c, first, c, stack.size(), false });
	}
}

template <typename node>
template<bool unique>
void post_order<node>::const_traverse(tref n, auto& visitor,
	auto& visit_subtree)
{
	if (n == nullptr) return;
	scratch<node, walk_buffers<subtree_seen<node>>> s;
	auto& cache = s.buffers->cache;
	auto& frames = s.buffers->frames;
	// The current node and its cursor are kept in locals; `frames` is
	// touched only on a descent or a return.
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
		// All children done; a leaf reaches this on its first turn
		if (cursor == nullptr) {
			if (frames.empty()) {
				// the root: nothing above it to return to
				call(visitor, current, nullptr);
				return;
			}
			// the frame below gives both this node's parent and
			// where to carry on
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
