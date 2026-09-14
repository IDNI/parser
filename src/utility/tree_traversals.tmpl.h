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

// Buffers for one traversal, lent from a pool so that later traversals reuse
// the capacity earlier ones built up.
template <typename cache_t, typename position_t>
struct traversal_buffers {
	trefs stack;
	std::vector<position_t> positions;
	cache_t cache;

	void clear() { stack.clear(), positions.clear(), cache.clear(); }
};

// How many traversals of this node type are running on this thread. A
// callback may start another traversal, so each depth gets buffers of its
// own; counting per type keeps a nested traversal of another type from being
// taken for a nested one of this type, whose collection is already suspended.
template <typename node>
size_t& traversal_depth() {
	static thread_local size_t depth = 0;
	return depth;
}

// Lends a traversal its buffers and returns them cleared. The pool is a deque
// so that growing it leaves references held by running traversals valid.
//
// Garbage collection is suspended for the duration: a traversal creates
// interned nodes that nothing refers to yet, and a collection would sweep
// them. Only the outermost traversal sets the flag, so nested ones stay
// covered until it ends. The stores are relaxed, matching the loads in
// bintree::gc(); the intern map's mutex is what excludes gc from node
// creation.
template <typename node, typename buffers_t>
struct scratch {
	scratch() {
		static thread_local std::deque<buffers_t> pool;
		const size_t depth = traversal_depth<node>();
		while (pool.size() <= depth) pool.emplace_back();
		buffers = &pool[depth];
		outermost = depth == 0;
		// raised only once nothing above it can throw, so a failed
		// allocation cannot leave the count raised for good
		++traversal_depth<node>();
		if (outermost) bintree<node>::gc_enabled.store(false,
					std::memory_order_relaxed);
	}
	~scratch() {
		if (outermost) bintree<node>::gc_enabled.store(true,
					std::memory_order_relaxed);
		buffers->clear(), --traversal_depth<node>();
	}

	scratch(const scratch&) = delete;
	scratch& operator=(const scratch&) = delete;

	buffers_t* buffers;
	bool outermost;
};

// Value type of a table that stores keys alone.
struct no_value {};

/**
 * @brief Open addressed table of subtrees, used for a traversal's caches.
 *
 * Storage is a single flat array. Every slot records the generation it was
 * written in, so clearing the table is one increment and the array is reused:
 * nothing is allocated per entry and nothing freed per clear.
 *
 * Keys are compared by subtree identity, with the same hash and equality as
 * the subtree_unordered_* containers.
 */
template <typename node, typename value_t>
class subtree_table {
public:
	bool empty() const { return count_ == 0; }

	void clear() {
		count_ = 0;
		// on wrap, stale slots would read as live, so blank them
		if (++generation_ == 0) {
			slots_.assign(slots_.size(), slot{});
			generation_ = 1;
		}
	}

	/// @return the value stored for `key`, or a default constructed one
	/// when it holds none. Callers store no default constructed value, so
	/// that is how a miss reads. By value, because a later insert can
	/// move the storage a pointer would refer to.
	value_t find(tref key) const {
		const size_t at = slot_of(key);
		return at == slots_.size() ? value_t{} : slots_[at].value;
	}

	bool contains(tref key) const { return slot_of(key) != slots_.size(); }

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

	/// index of the slot holding `key`, or slots_.size() when none does
	size_t slot_of(tref key) const {
		if (slots_.empty()) return 0;  // which is also slots_.size()
		for (size_t i = index_of(key); ; i = (i + 1) & mask_) {
			const slot& s = slots_[i];
			if (s.generation != generation_) return slots_.size();
			if (subtree_equality<node>{}(s.key, key)) return i;
		}
	}

	void grow() {
		const size_t wanted = slots_.empty() ? 16 : slots_.size() * 2;
		std::vector<slot> live;
		live.reserve(count_);
		for (const slot& s : slots_)
			if (s.generation == generation_) live.push_back(s);
		slots_.assign(wanted, slot{});
		mask_ = wanted - 1;
		// the new array holds no stale slots, so generations restart
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

// One open node of a read-only traversal: the node, which its children are
// given as their parent, and the next child to descend into.
struct walk_frame {
	tref node;
	tref next_child;
};

// State of a read-only traversal: the nodes it has open, and the subtrees
// already seen when it visits each of them only once.
template <typename cache_t>
struct walk_buffers {
	std::vector<walk_frame> frames;
	cache_t cache;

	void clear() { frames.clear(), cache.clear(); }
};

// One open node of a rewriting traversal.
struct build_frame {
	tref node;
	tref next_child;      // null once every child has been visited
	tref origin;          // the parent's child this node was reached as,
			      // which the parent compares its result against
	size_t buffer_start;  // where this node's buffered children begin
	bool dirty;           // has a child finished as a different node?
};

// Counters for the work a traversal does, off by default. The counts are
// deterministic, so they measure work independently of timing.
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

// Traversal depth accounting, off by default. TD() wraps each point where a
// frame is opened or closed.
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