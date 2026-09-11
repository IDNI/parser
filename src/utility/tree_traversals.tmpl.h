// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TREE_TRAVERSALS_TMPL_H__
#define __IDNI__PARSER__TREE_TRAVERSALS_TMPL_H__

#include <deque>

#include "tree.h"

namespace idni {

// #define LOG_TRAVERSALS_ENABLED

#ifdef LOG_TRAVERSALS_ENABLED

#define DBGT(x) x

template <typename node>
void print_stack(const std::vector<tref>& stack, tref current) {
	std::cout << "\tstack: ";
	for (tref n : stack) {
		std::cout << "\n\t\t";
		std::stringstream ss;
		const auto& c_tree = lcrs_tree<node>::get(n);
		ss << c_tree.value << ":";
		while (ss.tellp() < 16) ss << " ";
		ss      << (n == current ? "[" : " ") << n
			<< (n == current ? "]" : " ");
		if (c_tree.l) ss << " __ " << c_tree.l;
		if (c_tree.r) ss << " >> " << c_tree.r;
		std::cout << ss.str();
	}
	std::cout << "\n";
};

#else
#	define DBGT(x)
#endif // LOG_TRAVERSALS_ENABLED

// The buffers one traversal needs. They are pooled rather than built per
// call: the dominant workload is a great many tiny traversals - a rewrite
// rule application walks a handful of nodes - and for those, allocating a
// stack and a hash table costs more than the walk itself. Reusing them keeps
// the capacity earned by earlier traversals, so the steady state allocates
// nothing at all.
template <typename cache_t, typename position_t>
struct traversal_buffers {
	trefs stack;
	std::vector<position_t> positions;
	cache_t cache;

	void clear() {
		stack.clear(), positions.clear();
		// clearing a hash table memsets its whole bucket array, so a
		// cache that was never touched - every traversal that does
		// not memoize has one - must not be made to pay for the
		// buckets some earlier, larger traversal left behind
		if (!cache.empty()) cache.clear();
	}
};

// Nesting depth of the traversals running on this thread. A callback may
// start another traversal, so buffers are handed out per depth instead of
// from a single shared set.
inline size_t& traversal_depth() {
	static thread_local size_t depth = 0;
	return depth;
}

// Holds one traversal's buffers for as long as it runs, and returns them
// cleared. A deque is the store because growing it never invalidates a
// reference a running traversal is already holding.
//
// It also suspends garbage collection, because a traversal mints interned
// nodes that nothing roots yet and a collection partway through would sweep
// them. Only the outermost traversal touches the flag: a nested one restoring
// it on the way out would expose the nodes its caller is still building. The
// stores are relaxed to match the loads in bintree::gc() - the flag is a
// policy hint, and what actually keeps gc out of node creation is the intern
// map's mutex.
template <typename node, typename buffers_t>
struct scratch {
	scratch() : outermost(traversal_depth()++ == 0) {
		static thread_local std::deque<buffers_t> pool;
		const size_t depth = traversal_depth() - 1;
		while (pool.size() <= depth) pool.emplace_back();
		buffers = &pool[depth];
		if (outermost) bintree<node>::gc_enabled.store(false,
					std::memory_order_relaxed);
	}
	~scratch() {
		if (outermost) bintree<node>::gc_enabled.store(true,
					std::memory_order_relaxed);
		buffers->clear(), --traversal_depth();
	}

	scratch(const scratch&) = delete;
	scratch& operator=(const scratch&) = delete;

	buffers_t* buffers;
	const bool outermost;
};

// One entry per node a read-only traversal is still working through: the node
// itself, so a child can be told who its parent is, and how far along its
// children the traversal has got.
struct walk_frame {
	tref node;
	tref next_child;
};

// What a read-only traversal needs. It never rebuilds a node, so unlike a
// rewriting traversal it has no use for a buffer of finished children: the
// frames it has open, and the subtrees already seen when it visits each only
// once, are the whole of its state.
template <typename cache_t>
struct walk_buffers {
	std::vector<walk_frame> frames;
	cache_t cache;

	void clear() {
		frames.clear();
		// clearing a hash table memsets its whole bucket array, so a
		// cache that was never touched must not pay for the buckets
		// some earlier, larger traversal left behind
		if (!cache.empty()) cache.clear();
	}
};

// One entry per node whose children a rewriting traversal is still working
// through. `next_child` walks the node's own sibling chain rather than being
// read back out of the traversal's stack: a memoized result written into the
// stack can carry a different right sibling than the child it replaced, so
// the stack is not a safe place to take the chain from. Walking it also costs
// one link per child, where locating the n-th child costs n.
// `dirty` records whether any child came back different from the one it
// replaced. That is exactly the question the node's rebuild asks, so tracking
// it as children are finished answers it for free, where comparing the
// finished children against the original chain costs one comparison per child.
struct frame {
	size_t pos;       // where in the traversal's stack this node sits
	tref next_child;  // null once every child has been visited
	bool dirty;       // did any child below this node change?
};

// Traversal work counters. Off by default. They are deterministic and machine
// independent, which is what makes them useful: whether a change actually
// removed work is a question a wall clock cannot settle once the difference is
// smaller than the machine's noise.
#ifdef TAU_PARSER_TRAVERSAL_STATS

struct traversal_stats {
	size_t frames_opened = 0;     // nodes descended into
	size_t sibling_steps = 0;     // links followed to find the next child
	size_t cache_probes = 0;      // memo and unique set lookups
	size_t cache_hits = 0;
	size_t children_compared = 0; // children examined by the change check
	size_t rebuilds = 0;          // nodes rebuilt from a changed child list

	void reset() { *this = traversal_stats{}; }
};

inline traversal_stats& tstats() {
	static traversal_stats s;
	return s;
}

#	define TS(x) x

#else
#	define TS(x)
#endif // TAU_PARSER_TRAVERSAL_STATS

// Traversal depth accounting. Off by default: the traversals call TD() at
// every point where a frame is opened or closed, which is too many places to
// spell as #ifdef blocks without burying the loop they are measuring.
#ifdef MEASURE_TRAVERSER_DEPTH

#	define TD(x) x

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

#else
#	define TD(x)
#endif // MEASURE_TRAVERSER_DEPTH

} // idni namespace

#include "tree_traversals_pre_order.tmpl.h"
#include "tree_traversals_post_order.tmpl.h"
#include "tree_traversals_morris.tmpl.h"

#endif // __IDNI__PARSER__TREE_TRAVERSALS_TMPL_H__