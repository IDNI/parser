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
		tref res = traverse<false, slot, true>(root, f, visit_subtree, up);
		return res;
	}

	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique(auto& f, auto& visit_subtree) {
	if (visit_subtree(root)) {
		tref res = traverse<false, slot, true>(root, f, visit_subtree,
								identity);
		return res;
	}
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique(auto& f) {
	tref res = traverse<false, slot, true>(root, f, all, identity);
	return res;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply(auto& f, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) {
		tref res = traverse<false, slot, false>(root, f, visit_subtree, up);
		return res;
	}
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply(auto& f) {
	tref res = traverse<false, slot, false>(root, f, all, identity);
	return res;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f, auto& visit_subtree,
	auto& up)
{
	if (visit_subtree(root)) {
		tref res = traverse<true, slot, true>(root, f, visit_subtree, up);
		return res;
	}
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f, auto& visit_subtree){
	if (visit_subtree(root)) {
		tref res = traverse<true, slot, true>(
					root, f, visit_subtree, identity);
		return res;
	}
	else return root;
}


template <typename node>
template<size_t slot>
tref pre_order<node>::apply_unique_until_change(auto& f) {
	tref res = traverse<true, slot, true>(root, f, all, identity);
	return res;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_until_change(auto& f, auto& visit_subtree,
	auto& up)
{
	if (visit_subtree(root)) {
		tref res = traverse<true, slot, false>(root, f, visit_subtree, up);
		return res;
	}
	else return root;
}

template <typename node>
template<size_t slot>
tref pre_order<node>::apply_until_change(auto& f) {
	tref res = traverse<true, slot, false>(root, f, all, identity);
	return res;
}

template <typename node>
void pre_order<node>::visit(auto& visit, auto& visit_subtree, auto& up,
	auto& between)
{
	if (visit_subtree(root)) {
		const_traverse<false, false>(root,
					visit, visit_subtree, up, between);
	}
}

template <typename node>
void pre_order<node>::visit(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) {
		const_traverse<false, false>(root,
					visit, visit_subtree, up, do_nothing);
	}
}

template <typename node>
void pre_order<node>::visit(auto& visit) {
	const_traverse<false, false>(root, visit, all, identity, do_nothing);
}

template <typename node>
void pre_order<node>::search(auto& visit, auto& visit_subtree, auto& up,
	auto& between)
{
	if (visit_subtree(root)) {
		const_traverse<true, false>(root,
					visit, visit_subtree, up, between);
	}
}

template <typename node>
void pre_order<node>::search(auto& visit, auto& visit_subtree, auto& up) {
	if (visit_subtree(root)) {
		const_traverse<true, false>(root,
					visit, visit_subtree, up, do_nothing);
	}
}

template <typename node>
void pre_order<node>::search(auto& visit) {
	const_traverse<true, false>(root, visit, all, identity, do_nothing);
}

template <typename node>
void pre_order<node>::visit_unique(auto& visit, auto& visit_subtree, auto& up)
{
	if (visit_subtree(root)) {
		const_traverse<false, true>(root,
					visit, visit_subtree, up, do_nothing);
	}
}

template <typename node>
void pre_order<node>::visit_unique(auto& visit) {
	const_traverse<false, true>(root, visit, all, identity, do_nothing);
}

template <typename node>
void pre_order<node>::search_unique(auto&visit, auto& visit_subtree, auto& up)
{
	if (visit_subtree(root)) {
		const_traverse<true, true>(root,
					visit, visit_subtree, up, do_nothing);
	}
}

template <typename node>
void pre_order<node>::search_unique(auto&visit) {
	const_traverse<true, true>(root, visit, all, identity, do_nothing);
}


template <typename node>
template<bool break_on_change, size_t slot, bool unique>
tref pre_order<node>::traverse(tref n, auto& f, auto& visit_subtree, auto& up)
{
	if (n == nullptr) return nullptr;
	scratch<node, traversal_buffers<subtree_memo<node>, build_frame>> s;
	auto& cache = s.buffers->cache;
	auto& stack = s.buffers->stack;
	auto& frames = s.buffers->positions;
	auto get_parent = [&frames]() -> tref {
		return frames.empty() ? nullptr : frames.back().node;
	};
	// What the memo holds for `n`, or null. The only place either memo is
	// read, once per node, where its frame is opened. While a frame is
	// open only its descendants are added, and none of those matches the
	// node above them.
	auto memoized = [&cache](tref n) -> tref {
		if constexpr (!unique) return (void) n, nullptr;
		else {
			TS(++tstats().cache_probes;)
			// Memo keys are subtree identities, which ignore the
			// right sibling, so the stored node may carry a
			// different one. Re-attach `n`'s sibling; that is a
			// compare when it already matches.
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
				return tree::get(found,
					tree::get(n).right_sibling());
			}
		}
	};

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
	// Opens a frame for `x`, unless the memo already holds its result, in
	// which case `x` becomes that result. `origin` is the parent's child
	// it was reached as.
	auto open = [&frames, &stack, &memoized](tref origin, tref& x) -> bool {
		const tref first = tree::get(x).left_child();
		// a leaf is never in the memo
		if (const tref hit = first == nullptr ? nullptr : memoized(x);
			hit != nullptr)
		{
			x = hit;
			return false;
		}
		TD(inc_depth();)
		TS(++tstats().frames_opened;)
		frames.push_back({ x, first, origin, stack.size(), false });
		return true;
	};

	// Apply f to the root. If the root is not descended into, its result
	// is the whole answer.
	tref r = call(f, n);
	if (r == nullptr) return nullptr;
	bool descended = false;
	if constexpr (break_on_change) {
		if (r == n) descended = open(nullptr, r);
		else r = call(up, r);
	}
	// If the transformed node should not be
	// visited, do not open a frame for it
	else if (visit_subtree(r)) descended = open(nullptr, r);
	else r = call(up, r);
	if (!descended) return r;

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
			// `up` runs with the frame popped, so that it sees
			// the parent
			frames.pop_back();
			TD(dec_depth();)
			res = call(up, res);
			if (res == nullptr) return nullptr;
			// Leaves are kept out of the memo, so that looking
			// one up can be skipped as a certain miss
			if constexpr (unique) if (has_children) {
				if constexpr (slot != 0) m.emplace(
					std::make_pair(finished, slot), res);
				else cache.insert(finished, res);
			}
			if (frames.empty()) return res;
			child_done(origin, res);
			continue;
		}
		// advance before descending: opening a frame can move
		// `frames`, which would leave `fr` dangling. The callbacks
		// below still run with the parent frame on top, so
		// get_parent() names the parent.
		TS(++tstats().sibling_steps;)
		fr.next_child = tree::get(c).right_sibling();
		if (!visit_subtree(c)) {
			child_done(c, c);
			continue;
		}
		// Apply f to the child, then descend into the result or
		// finish it here
		r = call(f, c);
		if (r == nullptr) return nullptr;
		bool into = false;
		if constexpr (break_on_change) {
			if (r == c) into = open(c, r);
			else r = call(up, r);
		}
		// If the transformed node should not be
		// visited, do not open a frame for it
		else if (visit_subtree(r)) into = open(c, r);
		else r = call(up, r);
		if (r == nullptr) return nullptr;
		if (!into) child_done(c, r);
	}
}

template <typename node>
template<bool search, bool unique>
void pre_order<node>::const_traverse(tref n, auto& visitor,
	auto& visit_subtree, auto& up, auto& between)
{
	if (n == nullptr) return;
	scratch<node, walk_buffers<subtree_seen<node>>> s;
	auto& cache = s.buffers->cache;
	auto& frames = s.buffers->frames;
	// Call callback with parent if it is invocable with tref, tref
	// Otherwise, call it just with tref
	// `name` only labels the log line, so it stays a plain pointer
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
	// visit the root; it is descended into only if the visitor says so
	if (!call(visit_subtree, n, nullptr, "visit_subtree"))
		return;
	bool ret = !call(visitor, n, nullptr, "visitor0");
	if constexpr (search) { if (ret) return; }
	if constexpr (unique) cache.insert(n);
	if (ret) return;
	TD(inc_depth();)
	// The node being descended and the next of its children to visit are
	// kept in locals; `frames` is touched only on a descent or a return,
	// so a child that is not descended into costs no push.
	tref current = n;
	tref cursor = tree::get(n).left_child();
	while (true) {
		DBGT(std::cout << "-- non-const loop begin: "
			<< tree::get(current).value << "\n";)
		// All children done; a leaf reaches this on its first turn.
		// The frame below gives both this node's parent and where to
		// carry on.
		if (cursor == nullptr) {
			const tref parent = frames.empty() ? nullptr
						: frames.back().node;
			call(up, current, parent, "up");
			if (tree::get(current).has_right_sibling())
				call(between, current, parent, "between");
			TD(dec_depth();)
			if (frames.empty()) return;
			const walk_frame below = frames.back();
			frames.pop_back();
			current = below.node;
			cursor = below.next_child;
			continue;
		}
		const tref c = cursor;
		TS(++tstats().sibling_steps;)
		cursor = tree::get(c).right_sibling();
		DBGT(std::cout << "-- move to a child: " << c << " "
			<< tree::get(c).value << "\n\n";)
		if constexpr (unique) {
			TS(++tstats().cache_probes;)
			if (cache.contains(c)) {
				TS(++tstats().cache_hits;)
				continue;
			}
		}
		if (!call(visit_subtree, c, current, "visit_subtree")) continue;
		ret = !call(visitor, c, current, "visitor");
		if constexpr (search) { if (ret) return; }
		// Seen only once admitted: a node this predicate turns away
		// here may be admitted elsewhere, and has to stay reachable
		// there.
		if constexpr (unique) cache.insert(c);
		if (ret) continue;
		TD(inc_depth();)
		TS(++tstats().frames_opened;)
		frames.push_back({ current, cursor });
		current = c;
		cursor = tree::get(c).left_child();
	}
}

} // idni namespace
