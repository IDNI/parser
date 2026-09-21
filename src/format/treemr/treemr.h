// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__FORMAT__TREEMR__TREEMR_H__
#define __IDNI__FORMAT__TREEMR__TREEMR_H__

#include <concepts>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "utility/tree.h"            // tref, trefs, lcrs_tree, pre_order
#include "utility/diagnostics.h"     // idni::diagnostics::result
#include "parser.h"                  // nonterminals, pnode_type

namespace idni::treemr {

//------------------------------------------------------------------------------
// IR (plain recursive value type - std::vector<T> accepts an incomplete T,
// so no index-based arena is needed to break the pattern_node/sibling_seq
// cycle).
//------------------------------------------------------------------------------

enum class quantifier : uint8_t { NONE, STAR, PLUS, OPT };
enum class edge_type  : uint8_t { NONE, DIRECT, DESCENDANT };

struct sibling_seq;

// A tree-node pattern. `sub` holds at most one child sequence (edge !=
// NONE); `alternatives` holds one or more capture alternatives. Kept as
// two members so each arity is explicit at the call site.
struct pattern_node {
	enum class kind : uint8_t {
		NT,        // bare name - match an NT by name
		WILDCARD,  // %
		TERMINAL,  // 'c' / "str"
		CAPTURE,   // ( a | b | ... ) - capture group, 1+ alternatives
	};

	kind                      k    = kind::WILDCARD;
	std::string               text;            // NT name or TERMINAL text
	edge_type                 edge = edge_type::NONE;
	std::vector<sibling_seq>  sub;             // edge != NONE: 1 element
	std::vector<sibling_seq>  alternatives;    // CAPTURE: 1+ alternatives
	int                       capture_idx = -1;
	bool                      leaf = false;    // '!' - the matched node must have no children
};

// One slot of a sibling sequence: a node pattern together with its
// quantifier.
struct sibling_slot {
	pattern_node node;
	quantifier   q = quantifier::NONE;
};

// A sibling sequence with optional anchors.
struct sibling_seq {
	bool                      left_anchor  = false;
	bool                      right_anchor = false;
	std::vector<sibling_slot> slots;
};

// Compiled pattern: top-level sibling sequence + capture count.
struct compiled_pattern {
	sibling_seq top;
	int         num_captures = 0;
	bool        root_anchored = false;    // '/' - the matched root must be the tree root
};

//------------------------------------------------------------------------------
// ambig_mode - how the matcher treats `__AMB__` ambiguity wrappers
//------------------------------------------------------------------------------
// FORBID refuses a match whose subtree holds any __AMB__ node. ANY,
// UNIQUE and ALL quantify over the alternatives a pattern crosses:
// ANY needs one, UNIQUE needs exactly one, ALL needs every one.
enum class ambig_mode : uint8_t { FORBID, ANY, UNIQUE, ALL };

//------------------------------------------------------------------------------
// match_result - single match result: root + captures
//------------------------------------------------------------------------------
// root == nullptr iff no match. operator[](0) is root; operator[](n) is
// captures[n - 1], 1-based. An unmatched optional group stores nullptr.
struct match_result {
	tref  root = nullptr;
	trefs captures;

	tref operator[](size_t i) const {
		return i == 0 ? root : captures.at(i - 1);
	}
	size_t size() const { return root ? 1 + captures.size() : 0; }
	explicit operator bool() const { return root != nullptr; }
};

//------------------------------------------------------------------------------
// capture_state - implementation detail
//------------------------------------------------------------------------------
// Mutable capture slots with an undo log, used internally while
// backtracking.
struct capture_state;

//------------------------------------------------------------------------------
// replace_fn - callback to build a replacement subtree from a match
//------------------------------------------------------------------------------
// Receives the full match: mt.root is the matched node, mt.captures[i]
// is the i-th capture group (nullptr if unmatched). Returning mt.root
// means no change here; returning nullptr deletes the matched node.
using replace_fn = std::function<tref(const match_result&)>;

// Predicate used by replace_if/replace_until to gate descent into a
// subtree, and by the forwarding methods on lcrs_tree to name a query.
using query_fn = std::function<bool(tref)>;

//------------------------------------------------------------------------------
// compile() - DSL string -> compiled_pattern (declaration only)
//------------------------------------------------------------------------------

// Parse the DSL string and build a tree-type-independent compiled_pattern.
// On syntax error the result carries a diagnostics::code::parse_error.
// Not thread safe: the DSL parser is a process-wide singleton.
diagnostics::result<compiled_pattern> compile(std::string_view pattern);

//------------------------------------------------------------------------------
// node_adapter - runtime, type-erased node operations
//------------------------------------------------------------------------------

// The matcher's two node-level queries, held as std::function so the
// engine is not a template over the adapter type. is_terminal_fn may be
// left empty; a terminal pattern then fails to match instead of crashing.
struct node_adapter {
	std::function<bool(tref, std::string_view)> is_nt_fn;
	std::function<bool(tref, std::string_view)> is_terminal_fn;
	// Builds a node named `nt` with the given children, for an ALL-mode ambiguity capture.
	// `parse_node_adapter` always sets it. A hand-built adapter may leave it empty.
	std::function<tref(std::string_view nt, const trefs& children)> make_node_fn;

	bool is_nt(tref n, std::string_view s) const;
	bool is_terminal(tref n, std::string_view s) const;
};

// Build a node_adapter for an idni::parser<C,T> parse tree
// (lcrs_tree<pnode_type<C,T>>), bound to `nts` by reference.
template <typename C = char, typename T = C>
node_adapter parse_node_adapter(const nonterminals<C, T>& nts);

//------------------------------------------------------------------------------
// matcher<NodeT>
//------------------------------------------------------------------------------

template <typename NodeT>
struct matcher {
	matcher(compiled_pattern pat, node_adapter adapter);

	// Default cap on replace_fixpoint's iteration count; 0 requests no cap.
	static constexpr size_t default_max_fixpoint_iters = 1000;

	// Anchored, like std::regex_match: `root` itself must be the match root.
	bool  match(tref root, ambig_mode mode = ambig_mode::FORBID) const;
	bool  match(tref root, match_result& m,
		ambig_mode mode = ambig_mode::FORBID) const;

	// Free-form, like std::regex_search: the pattern may match anywhere
	// in the subtree rooted at `root`.
	bool  search(tref root, ambig_mode mode = ambig_mode::FORBID) const;
	bool  search(tref root, match_result& m,
		ambig_mode mode = ambig_mode::FORBID) const;
	std::vector<match_result> search_all(tref root,
		ambig_mode mode = ambig_mode::FORBID) const;

	// One post-order pass: every node that matches as a root is replaced
	// by fn(mt), children before parents, so a parent sees its already-
	// final replacement children. The original tree is untouched.
	tref replace(tref root, replace_fn fn,
		ambig_mode mode = ambig_mode::FORBID) const;

	// Like replace(), but skips (leaves untouched) a subtree whose root
	// does not satisfy `query`.
	tref replace_if(tref root, replace_fn fn, query_fn query,
		ambig_mode mode = ambig_mode::FORBID) const;

	// Like replace(), but skips (leaves untouched) a subtree whose root
	// does satisfy `query`.
	tref replace_until(tref root, replace_fn fn, query_fn query,
		ambig_mode mode = ambig_mode::FORBID) const;

	// replace() with a callback that always deletes: removes every match,
	// at any depth, in one post-order pass.
	tref trim(tref root, ambig_mode mode = ambig_mode::FORBID) const;

	// Deletes only a match with no matching ancestor: the traversal does
	// not descend into a match, so a nested match is removed along with it
	// rather than tested on its own.
	tref trim_top(tref root, ambig_mode mode = ambig_mode::FORBID) const;

	// Repeat replace() until a fixed point (no node matches). max_iters
	// guards a rule whose replacement re-matches the pattern; 0 requests
	// no cap. Reaching the cap without a fixed point is an error.
	diagnostics::result<tref> replace_fixpoint(tref root, replace_fn fn,
		ambig_mode mode = ambig_mode::FORBID,
		size_t max_iters = default_max_fixpoint_iters) const;

private:
	compiled_pattern pat_;
	node_adapter     adapter_;

	// True if `n` is an `__AMB__` wrapper as reported by the adapter.
	bool is_amb(tref n) const;

	// Applies `pred` per `mode`: FORBID rejects, ANY needs one hit,
	// UNIQUE needs exactly one, ALL needs every one with captures
	// merged across alternatives.
	template <typename Pred>
	bool match_amb_alts(tref n, capture_state& caps,
		ambig_mode mode, Pred&& pred) const;

	// Single-node predicate. On failure rolls `caps` back to its entry
	// checkpoint, so capture writes survive only on success.
	bool match_node(tref n, const pattern_node& p,
		capture_state& caps, ambig_mode mode, tref& next_sib) const;

	// Sequence entry point honoring the left anchor: an unanchored
	// sequence may start at `sib` or any of its right siblings.
	bool match_seq(tref sib, const sibling_seq& seq,
		capture_state& caps, ambig_mode mode) const;

	// Matches seq.slots[slot_idx..] against consecutive siblings from
	// `sib`, then calls the continuation `k(next_sib)`. A step that
	// consumes nothing does not repeat itself at the same position.
	template <typename Cont>
	bool match_slots(const sibling_seq& seq, size_t slot_idx, tref sib,
		capture_state& caps, ambig_mode mode, Cont&& k) const;

	// "anywhere strictly below `n`" search: tries `seq` against the
	// child list of every node in the subtree rooted at `n`.
	bool match_descendant(tref n, const sibling_seq& seq,
		capture_state& caps, ambig_mode mode) const;

	// Attempts the pattern with `n` as the match root. `at_root` says
	// whether `n` counts as the root of the tree being searched, which
	// decides a root-anchored ('/') pattern.
	bool try_at_root(tref n, capture_state& caps,
		ambig_mode mode, bool at_root) const;

	// Root-anchored patterns can only ever match the tree's root, so
	// every replace variant tests just that node instead of walking the
	// tree and failing at every other candidate.
	tref replace_at_root_only(tref root, const replace_fn& fn,
		ambig_mode mode) const;

	// Shared post-order rewrite behind replace/replace_if/replace_until/
	// trim. `should_descend` gates whether a subtree is entered at all; a
	// false answer leaves that whole subtree untouched. `deleted` reports
	// whether `n` itself was removed (fn returned nullptr for it). `memo`
	// caches by node identity, so two hash-consed occurrences of the same
	// subtree call `fn` once and share its result.
	tref replace_walk(tref n, tref parent, bool at_root,
		const replace_fn& fn, ambig_mode mode,
		const query_fn& should_descend,
		std::unordered_map<tref, bool>& amb_seen,
		std::unordered_map<tref, std::pair<tref, bool>>& memo,
		bool& deleted) const;

	// Top-down counterpart behind trim_top: stops descending once a match
	// is found, so a match nested under a deleted ancestor is never
	// independently tested.
	tref trim_top_walk(tref n, tref parent, bool at_root, ambig_mode mode,
		std::unordered_map<tref, bool>& amb_seen, bool& deleted) const;

	// True if the subtree rooted at `n` holds any __AMB__ node.
	bool subtree_has_amb(tref n, std::unordered_map<tref, bool>& seen) const;
};

//------------------------------------------------------------------------------
// nt_source - anything that can name its own nonterminals
//------------------------------------------------------------------------------

// An object that carries a parse tree's character types and its own
// symbol table. Every generated `<x>_parser` struct satisfies it.
template <typename P>
concept nt_source = requires (const P& p) {
	typename P::char_type;
	typename P::terminal_type;
	{ p.get_grammar().get_nts() } -> std::convertible_to<
		const nonterminals<typename P::char_type,
			typename P::terminal_type>&>;
};

//------------------------------------------------------------------------------
// Factory function declarations
//------------------------------------------------------------------------------

// Parse tree matcher from an already-compiled pattern. Infallible.
template <typename C, typename T>
auto matcher_for(const nonterminals<C, T>& nts, compiled_pattern pat)
	-> matcher<pnode_type<C, T>>;

// Parse tree matcher from a DSL string. Compile errors flow through
// diagnostics::result - no exceptions thrown.
template <typename C, typename T>
auto matcher_for(const nonterminals<C, T>& nts, std::string_view pattern)
	-> diagnostics::result<matcher<pnode_type<C, T>>>;

// Same as above, taking any object that can name its own nonterminals
// (a generated `<x>_parser` instance) instead of a bare symbol table.
template <nt_source P>
auto matcher_for(const P& p, compiled_pattern pat)
	-> matcher<pnode_type<typename P::char_type, typename P::terminal_type>>;

template <nt_source P>
auto matcher_for(const P& p, std::string_view pattern)
	-> diagnostics::result<matcher<pnode_type<typename P::char_type,
		typename P::terminal_type>>>;

} // namespace idni::treemr

#include "treemr.tmpl.h"

#endif // __IDNI__FORMAT__TREEMR__TREEMR_H__
