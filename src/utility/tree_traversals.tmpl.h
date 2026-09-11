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

	void clear() { stack.clear(), positions.clear(), cache.clear(); }
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

// Marks a table that stores keys only, so that a set costs no more per slot
// than the key it holds.
struct no_value {};

/**
 * @brief Open addressed table of subtrees, for the caches a traversal keeps.
 *
 * std::unordered_map allocates a node per insert and frees every one of them
 * on clear. The traversals are dominated by short calls - a rewrite rule
 * application walks a handful of nodes - and for those, that allocation is
 * more work than the traversal itself. Here the storage is one flat array
 * whose slots carry the generation they were written in, so clearing is an
 * increment: nothing is allocated, nothing is freed, and the capacity earned
 * by earlier traversals is kept.
 *
 * Keys are compared by subtree identity, as elsewhere: the same hash and
 * equality the std:: containers were given.
 */
template <typename node, typename value_t>
class subtree_table {
public:
	bool empty() const { return count_ == 0; }

	void clear() {
		count_ = 0;
		// A wrap would make stale slots look live again, so on the
		// one occasion it happens the table is blanked instead.
		if (++generation_ == 0) {
			slots_.assign(slots_.size(), slot{});
			generation_ = 1;
		}
	}

	/// @return the stored value for `key`, or nullptr if it has none
	const value_t* find(tref key) const {
		if (slots_.empty()) return nullptr;
		for (size_t i = index_of(key); ; i = (i + 1) & mask_) {
			const slot& s = slots_[i];
			if (s.generation != generation_) return nullptr;
			if (subtree_equality<node>{}(s.key, key))
				return &s.value;
		}
	}

	bool contains(tref key) const { return find(key) != nullptr; }

	/// Stores `value` under `key`, replacing any value already there.
	void insert(tref key, value_t value = {}) {
		if ((count_ + 1) * 2 > slots_.size()) grow();
		for (size_t i = index_of(key); ; i = (i + 1) & mask_) {
			slot& s = slots_[i];
			if (s.generation != generation_) {
				s.generation = generation_, s.key = key,
					s.value = value;
				++count_;
				return;
			}
			if (subtree_equality<node>{}(s.key, key))
				return (void) (s.value = value);
		}
	}

private:
	struct slot {
		std::uint32_t generation = 0;
		tref key = nullptr;
		[[no_unique_address]] value_t value{};
	};

	size_t index_of(tref key) const {
		return hash_lcrs_tref<node>{}(key) & mask_;
	}

	void grow() {
		const size_t wanted = slots_.empty() ? 16 : slots_.size() * 2;
		std::vector<slot> live;
		live.reserve(count_);
		for (const slot& s : slots_)
			if (s.generation == generation_) live.push_back(s);
		slots_.assign(wanted, slot{});
		mask_ = wanted - 1;
		// a fresh array has no stale slots, so the generation can
		// start over and every live entry is written back under it
		generation_ = 1, count_ = 0;
		for (const slot& s : live) insert(s.key, s.value);
	}

	std::vector<slot> slots_;
	size_t mask_ = 0;
	size_t count_ = 0;
	std::uint32_t generation_ = 1;
};

template <typename node>
using subtree_memo = subtree_table<node, tref>;

template <typename node>
using subtree_seen = subtree_table<node, no_value>;

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

	void clear() { frames.clear(), cache.clear(); }
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