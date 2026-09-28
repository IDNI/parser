// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// DSL compiler, node adapters and the matching engine.

#ifndef __IDNI__FORMAT__TREEMR__TREEMR_TMPL_H__
#define __IDNI__FORMAT__TREEMR__TREEMR_TMPL_H__

#include "treemr.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "treemr_parser.generated.h"
#include "utility/escapes.h"

namespace idni::treemr {

// Nonterminal ids of the DSL pattern grammar.
using parser_nt = ::treemr_parser_nonterminals::nonterminal;

// Parse-tree type produced by the generated DSL pattern parser.
using parser_tree = ::treemr_parser::tree;

// Traverser over the DSL parse tree.
using trv = parser_tree::traverser;

// Return the unique child of `parent` whose nonterminal id is `nt`,
// or nullptr if none exists.
inline tref child_by_nt(tref parent, parser_nt nt) {
	if (!parent) return nullptr;
	auto c = trv(parent) | (size_t)nt;
	return c ? c.value() : nullptr;
}

// Check whether `parent` has any direct child with the given NT id.
inline bool has_child_nt(tref parent, parser_nt nt) {
	return parent && (trv(parent) | (size_t)nt).has_value();
}

// Direct children of `n` with nonterminal id `target`. The compiler
// only ever walks the shaped tree, which already flattens a
// `x (sep x)*` repetition, so its children need no further descent.
inline trefs collect_flattened(tref n, parser_nt target) {
	if (!n) return {};
	return (trv(n) || (size_t)target).values();
}

// Collect terminal text under `n` from the DSL parse tree as UTF-8.
inline std::string collect_terminals(tref n) {
	if (!n) return {};
	return trv(n) | trv::terminals;
}

// Decode a node's char_lit or str_lit child into the literal text it
// stands for, expanding escape sequences.
// Returns std::nullopt when an escape sequence fails to decode.
inline std::optional<std::string> decode_terminal(tref terminal_node) {
	std::string out;
	if (!terminal_node) return out;

	tref cl = child_by_nt(terminal_node, parser_nt::char_lit);
	if (cl) {
		tref clc = child_by_nt(cl, parser_nt::char_lit_char);
		if (!clc) return out;
		tref uc = child_by_nt(clc, parser_nt::unescaped_c);
		if (uc) return collect_terminals(uc);
		tref ec = child_by_nt(clc, parser_nt::escaped_c);
		if (ec) {
			auto raw = collect_terminals(ec);
			auto dec = escapes::decode(raw, escapes::tgf_char);
			if (!dec.has_value()) return std::nullopt;
			return std::move(dec).value();
		}
		return out;
	}

	tref sl = child_by_nt(terminal_node, parser_nt::str_lit);
	if (sl) {
		// Walk the chain of str_lit_char children.
		const auto& slt = parser_tree::get(sl);
		for (tref c : slt.children()) {
			if (!c) continue;
			const auto& ct = parser_tree::get(c);
			const auto& sym = ct.value.first;
			if (!sym.nt() || sym.n()
				!= (size_t)parser_nt::str_lit_char) continue;
			tref us = child_by_nt(c, parser_nt::unescaped_s);
			if (us) { out += collect_terminals(us); continue; }
			tref es = child_by_nt(c, parser_nt::escaped_s);
			if (es) {
				auto raw = collect_terminals(es);
				auto dec = escapes::decode(raw,
					escapes::tgf_string);
				if (!dec.has_value()) return std::nullopt;
				out += std::move(dec).value();
				continue;
			}
		}
	}
	return out;
}

struct compiler {
	int         cap_counter = 0;
	std::string err;

	// Read a quantifier child off a slot node (simple_slot or edge_slot).
	// quantifier is @inline'd, so star/plus/opt sit directly on slot_node.
	quantifier read_quantifier(tref slot_node) {
		if (has_child_nt(slot_node, parser_nt::star))
			return quantifier::STAR;
		if (has_child_nt(slot_node, parser_nt::plus))
			return quantifier::PLUS;
		if (has_child_nt(slot_node, parser_nt::opt))
			return quantifier::OPT;
		return quantifier::NONE;
	}

	// Build a sibling_slot from a simple_slot parse node (no edge).
	// atom is @inline'd, so its alternatives sit directly on slot_node.
	sibling_slot compile_simple_slot(tref slot_node) {
		sibling_slot s;
		s.node = compile_atom(slot_node);
		s.q    = read_quantifier(slot_node);
		return s;
	}

	// Build a sibling_slot from an edge_slot parse node. The edge_slot
	// holds an atom (with optional quantifier), an edge, and an inner
	// slot_chain that is the rest of the enclosing sibling sequence.
	// atom, edge and sib_seq are all @inline'd, so their alternatives
	// sit directly on slot_node.
	sibling_slot compile_edge_slot(tref slot_node) {
		sibling_slot s;
		s.node = compile_atom(slot_node);
		if (s.node.leaf && err.empty()) err =
			"a leaf atom cannot have a child edge";
		bool has_edge = has_child_nt(slot_node, parser_nt::descendant_edge)
			|| has_child_nt(slot_node, parser_nt::direct_edge);
		if (has_edge) {
			s.node.edge = has_child_nt(slot_node,
					parser_nt::descendant_edge)
				? edge_type::DESCENDANT
				: edge_type::DIRECT;
			tref sub = child_by_nt(slot_node,
				parser_nt::slot_chain);
			s.node.sub.push_back(
				sub ? compile_sib_seq(sub) : sibling_seq{});
		}
		s.q = read_quantifier(slot_node);
		return s;
	}

	// `^`/`$` are simple_slot alternatives: a left_anchor must sit at
	// index 0, a right_anchor at the last index (edge_slot included);
	// neither is emitted as a pattern_node.
	// sib_seq is @inline'd, so `chain` is the slot_chain node itself.
	sibling_seq compile_sib_seq(tref chain) {
		sibling_seq seq;
		if (!chain) return seq;

		tref simples   = child_by_nt(chain, parser_nt::simple_slots);
		tref tail_edge = child_by_nt(chain, parser_nt::edge_slot);

		trefs simple_nodes = simples
			? collect_flattened(simples, parser_nt::simple_slot)
			: trefs{};
		size_t total = simple_nodes.size() + (tail_edge ? 1 : 0);

		for (size_t i = 0; i < simple_nodes.size(); ++i) {
			tref sn = simple_nodes[i];
			if (has_child_nt(sn, parser_nt::left_anchor)) {
				if (i == 0) seq.left_anchor = true;
				else if (err.empty()) err =
					"'^' must be the first slot of its "
					"sequence";
			} else if (has_child_nt(sn, parser_nt::right_anchor)) {
				if (i + 1 == total) seq.right_anchor = true;
				else if (err.empty()) err =
					"'$' must be the last slot of its "
					"sequence";
			} else seq.slots.push_back(compile_simple_slot(sn));
		}
		if (tail_edge) seq.slots.push_back(compile_edge_slot(tail_edge));

		// A group anchors its sequence only when every alternative
		// carries the anchor. `^` propagates from a first-slot group,
		// `$` from a last-slot group; a mixed group is a compile error.
		for (size_t i = 0; i < seq.slots.size(); ++i) {
			const pattern_node& f = seq.slots[i].node;
			if (f.k != pattern_node::kind::CAPTURE) continue;
			// An anchor-only group, such as `(^)`, can never
			// capture, so it would only shift later capture numbers.
			bool anchor_only = false;
			for (const auto& alt : f.alternatives)
				if (alt.slots.empty()) { anchor_only = true; break; }
			if (anchor_only) {
				if (err.empty()) err =
					"an anchor-only group has no meaning";
				continue;
			}
			size_t n_left = 0, n_right = 0;
			for (const auto& alt : f.alternatives) {
				if (alt.left_anchor) ++n_left;
				if (alt.right_anchor) ++n_right;
			}
			if (n_left > 0) {
				if (n_left != f.alternatives.size()) {
					if (err.empty()) err =
						"'^' must be on every "
						"alternative of its group, or "
						"on none";
				} else if (i != 0) {
					if (err.empty()) err =
						"'^' inside a group is only "
						"meaningful when the group is "
						"first in its sequence";
				} else seq.left_anchor = true;
			}
			if (n_right > 0) {
				if (n_right != f.alternatives.size()) {
					if (err.empty()) err =
						"'$' must be on every "
						"alternative of its group, or "
						"on none";
				} else if (i + 1 != seq.slots.size()) {
					if (err.empty()) err =
						"'$' inside a group is only "
						"meaningful when the group is "
						"last in its sequence";
				} else seq.right_anchor = true;
			}
		}
		return seq;
	}

	// atom and terminal are @inline'd, so name/wildcard/char_lit/str_lit/
	// capture sit directly on n, alongside leaf_anchor when present.
	pattern_node compile_atom(tref n) {
		pattern_node out;
		if (!n) { out.k = pattern_node::kind::WILDCARD; return out; }
		out.leaf = has_child_nt(n, parser_nt::leaf_anchor);
		tref name_n = child_by_nt(n, parser_nt::name);
		if (name_n) {
			out.k = pattern_node::kind::NT;
			out.text = collect_terminals(name_n);
			return out;
		}
		tref wild = child_by_nt(n, parser_nt::wildcard);
		if (wild) {
			out.k = pattern_node::kind::WILDCARD;
			return out;
		}
		bool is_terminal = has_child_nt(n, parser_nt::char_lit)
			|| has_child_nt(n, parser_nt::str_lit);
		if (is_terminal) {
			out.k = pattern_node::kind::TERMINAL;
			auto txt = decode_terminal(n);
			if (txt) out.text = std::move(*txt);
			else if (err.empty()) err = "invalid escape sequence"
				" in terminal literal";
			return out;
		}
		tref cap = child_by_nt(n, parser_nt::capture);
		if (cap) {
			tref alt_seq = child_by_nt(cap,
				parser_nt::alt_seq);
			out.k = pattern_node::kind::CAPTURE;
			// Number the group at its opening paren, before the body
			// compiles, so nested groups follow regex convention:
			// outer first, then inner left-to-right.
			out.capture_idx = cap_counter++;
			// sib_seq is @inline'd, so alt_seq's alternatives are
			// slot_chain nodes directly.
			for (tref s : collect_flattened(alt_seq,
				parser_nt::slot_chain))
				out.alternatives.push_back(compile_sib_seq(s));
			return out;
		}
		// Unknown atom - fall back to wildcard.
		out.k = pattern_node::kind::WILDCARD;
		return out;
	}
};

// True if `seq` can match against zero siblings.
inline bool seq_can_match_empty(const sibling_seq& seq);

// True if a slot can contribute zero width to its sequence: a `*`/`?`
// quantifier always can, and a capture group can when every one of its
// alternatives can.
inline bool slot_can_match_empty(const sibling_slot& slot) {
	if (slot.q == quantifier::STAR || slot.q == quantifier::OPT)
		return true;
	if (slot.node.k != pattern_node::kind::CAPTURE) return false;
	if (slot.node.alternatives.empty()) return false;
	for (const auto& alt : slot.node.alternatives)
		if (!seq_can_match_empty(alt)) return false;
	return true;
}

inline bool seq_can_match_empty(const sibling_seq& seq) {
	for (const auto& slot : seq.slots)
		if (!slot_can_match_empty(slot)) return false;
	return true;
}

inline diagnostics::result<compiled_pattern> compile(std::string_view pattern) {
	diagnostics::result<compiled_pattern> R;

	auto& p = ::treemr_parser::instance();
	auto pr = p.parse(pattern.data(), pattern.size());
	if (!pr.found) {
		R.error(diagnostics::code::parse_error,
			"invalid treemr pattern");
		return R;
	}
	if (pr.is_ambiguous()) {
		R.error(diagnostics::code::internal_error,
			"treemr compile: ambiguous DSL parse");
		return R;
	}
	tref root = pr.get_shaped_tree2();
	if (!root) {
		R.error(diagnostics::code::internal_error,
			"treemr compile: parser returned a null root");
		return R;
	}

	// pattern and sib_seq are @inline'd, so the root's children are an
	// optional root_anchor followed by the top-level slot_chain directly.
	tref top_chain = child_by_nt(root, parser_nt::slot_chain);
	if (!top_chain) {
		R.error(diagnostics::code::internal_error,
			"treemr compile: missing top slot_chain");
		return R;
	}

	compiler c;
	compiled_pattern cp;
	cp.top           = c.compile_sib_seq(top_chain);
	cp.num_captures  = c.cap_counter;
	cp.root_anchored = has_child_nt(root, parser_nt::root_anchor);
	// A left-continuous sibling list cannot be walked backwards in this
	// tree representation, and the top of a pattern has no parent to ask,
	// so '^' is not computable there; '$' stays legal since a right
	// sibling is always reachable. cp.top.left_anchor catches both a bare
	// top-level '^' and one propagated from a first-slot group.
	if (c.err.empty() && cp.top.left_anchor)
		c.err = "a left anchor has no meaning at the top of a pattern";
	if (c.err.empty() && seq_can_match_empty(cp.top))
		c.err = "pattern can match empty";
	if (!c.err.empty()) {
		R.error(diagnostics::code::parse_error, c.err);
		return R;
	}
	R.emplace(std::move(cp));
	return R;
}

//------------------------------------------------------------------------------
// node_adapter
//------------------------------------------------------------------------------

inline bool node_adapter::is_nt(tref n, std::string_view s) const {
	return is_nt_fn ? is_nt_fn(n, s) : false;
}

inline bool node_adapter::is_terminal(tref n, std::string_view s) const {
	return is_terminal_fn ? is_terminal_fn(n, s) : false;
}

inline std::optional<std::string> node_adapter::terminal_leaf_text(
	tref n) const
{
	return terminal_leaf_text_fn ? terminal_leaf_text_fn(n) : std::nullopt;
}

//------------------------------------------------------------------------------
// parse_node_adapter<C,T>
//------------------------------------------------------------------------------

template <typename C, typename T>
node_adapter parse_node_adapter(const nonterminals<C, T>& nts) {
	node_adapter a;
	a.is_nt_fn = [&nts](tref n, std::string_view name) -> bool {
		if (!n) return false;
		using tree_t = typename ::idni::parser<C, T>::tree;
		const auto& tr = tree_t::get(n);
		const auto& sym = tr.value.first;
		if (!sym.nt()) return false;
		const auto& nm = nts.get(sym.n());
		// The pattern text is always UTF-8; nm is basic_string<C>, so
		// only C == char needs no conversion (nm is already bytes).
		if constexpr (std::is_same_v<C, char>) {
			if (nm.size() != name.size()) return false;
			return std::equal(nm.begin(), nm.end(), name.begin());
		} else {
			auto u8 = idni::to_std_string(nm);
			if (u8.size() != name.size()) return false;
			return std::equal(u8.begin(), u8.end(), name.begin());
		}
	};
	a.is_terminal_fn = [](tref n, std::string_view text) -> bool {
		if (!n) return false;
		using tree_t = typename ::idni::parser<C, T>::tree;
		const auto& tr = tree_t::get(n);
		auto got = tr.get_terminals();
		if (got.size() != text.size()) return false;
		return std::equal(got.begin(), got.end(), text.begin());
	};
	a.terminal_leaf_text_fn = [](tref n) -> std::optional<std::string> {
		if (!n) return std::nullopt;
		using tree_t = typename ::idni::parser<C, T>::tree;
		const auto& tr = tree_t::get(n);
		if (tr.is_nt() || tr.has_child()) return std::nullopt;
		return tr.get_terminals();
	};
	a.make_node_fn = [&nts](std::string_view name, const trefs& children)
		-> tref
	{
		using tree_t = typename ::idni::parser<C, T>::tree;
		using pnode  = pnode_type<C, T>;
		auto want = idni::from_str<C>(std::string(name));
		for (size_t i = 0; i < nts.size(); ++i)
			if (nts.get(i) == want)
				return tree_t::get(
					pnode(lit<C, T>(i, &nts), {0, 0}),
					children);
		return nullptr;
	};
	return a;
}

//------------------------------------------------------------------------------
// capture_state - mutable capture slots with an undo log (trail)
//------------------------------------------------------------------------------
// Backtracking matchers checkpoint with mark() and discard speculative capture
// writes with rollback(), so a failed branch costs O(writes-undone) rather than
// copying the whole capture vector at every step.
struct capture_state {
	std::vector<tref>                 v;         // current capture slots
	std::vector<tref>                 ends;      // exclusive end, parallel to v
	std::vector<std::pair<size_t, tref>> trail;      // (index, previous v)
	std::vector<std::pair<size_t, tref>> end_trail;  // (index, previous end)

	explicit capture_state(int n)
		: v(static_cast<size_t>(n < 0 ? 0 : n), nullptr)
		, ends(static_cast<size_t>(n < 0 ? 0 : n), nullptr) {}

	// Checkpoint: the trail length to roll back to.
	size_t mark() const { return trail.size(); }

	// Record-then-write a capture slot and its end so both can be undone.
	void set(size_t idx, tref n, tref end) {
		trail.emplace_back(idx, v[idx]);
		end_trail.emplace_back(idx, ends[idx]);
		v[idx] = n;
		ends[idx] = end;
	}

	// Undo all capture writes recorded since checkpoint `m`.
	void rollback(size_t m) {
		while (trail.size() > m) {
			v[trail.back().first] = trail.back().second;
			trail.pop_back();
		}
		while (end_trail.size() > m) {
			ends[end_trail.back().first] = end_trail.back().second;
			end_trail.pop_back();
		}
	}

	// Reuse across candidates: clear writes and the trail.
	void reset() {
		for (auto& slot : v) slot = nullptr;
		for (auto& slot : ends) slot = nullptr;
		trail.clear();
		end_trail.clear();
	}
};

//------------------------------------------------------------------------------
// matcher<NodeT>
//------------------------------------------------------------------------------

template <typename NodeT>
matcher<NodeT>::matcher(compiled_pattern pat, node_adapter adapter)
	: pat_(std::move(pat)), adapter_(std::move(adapter))
{}

// --- ambig helpers -----------------------------------------------------------

template <typename NodeT>
bool matcher<NodeT>::is_amb(tref n) const {
	return n && adapter_.is_nt(n, "__AMB__");
}

// True if the subtree rooted at `n` holds any __AMB__ node.
template <typename NodeT>
bool matcher<NodeT>::subtree_has_amb(tref n,
	std::unordered_map<tref, bool>& seen) const
{
	if (!n) return false;
	auto it = seen.find(n);
	if (it != seen.end()) return it->second;
	bool r = is_amb(n);
	for (tref c = lcrs_tree<NodeT>::get(n).first(); c && !r;
		c = lcrs_tree<NodeT>::get(c).right_sibling())
		if (subtree_has_amb(c, seen)) r = true;
	seen.emplace(n, r);
	return r;
}

// Applies `pred` to each alternative under an `__AMB__` wrapper. FORBID
// rejects; ANY needs one match; UNIQUE needs exactly one; ALL needs
// every alternative to match, merging each capture across them.
template <typename NodeT>
template <typename Pred>
bool matcher<NodeT>::match_amb_alts(tref n, capture_state& caps,
	ambig_mode mode, Pred&& pred) const
{
	if (mode == ambig_mode::FORBID) return false;
	tref alt = lcrs_tree<NodeT>::get(n).first();
	if (!alt) return false;
	const size_t entry = caps.mark();
	if (mode == ambig_mode::ANY) {
		for (; alt; alt = lcrs_tree<NodeT>::get(alt).right_sibling()) {
			if (pred(alt)) return true;
			caps.rollback(entry);
		}
		return false;
	}
	if (mode == ambig_mode::UNIQUE) {
		int count = 0;
		trefs winner, winner_ends;
		for (; alt; alt = lcrs_tree<NodeT>::get(alt).right_sibling()) {
			if (pred(alt)) {
				if (++count == 1) {
					winner = caps.v;
					winner_ends = caps.ends;
				}
				caps.rollback(entry);
				if (count > 1) return false;
			} else caps.rollback(entry);
		}
		if (count != 1) return false;
		for (size_t idx = 0; idx < caps.v.size(); ++idx)
			caps.set(idx, winner[idx], winner_ends[idx]);
		return true;
	}
	// ALL - every alternative must match. Each alternative's captures
	// are recorded, then rolled back so the next alternative starts
	// from the same baseline; a single failure rolls back to entry.
	std::vector<trefs> per_alt, per_alt_ends;
	for (; alt; alt = lcrs_tree<NodeT>::get(alt).right_sibling()) {
		if (!pred(alt)) { caps.rollback(entry); return false; }
		per_alt.push_back(caps.v);
		per_alt_ends.push_back(caps.ends);
		caps.rollback(entry);
	}
	// Merge: a capture that is the same node (or nullptr) in every
	// alternative stays that node; otherwise it becomes a synthesized
	// __AMB__ node over the distinct values, in alternative order.
	// Written via caps.set() so the merge survives a later rollback.
	for (size_t idx = 0; idx < caps.v.size(); ++idx) {
		if (!adapter_.make_node_fn) {
			caps.set(idx, per_alt.front()[idx],
				per_alt_ends.front()[idx]);
			continue;
		}
		trefs distinct;
		tref distinct_end = nullptr;
		for (size_t a = 0; a < per_alt.size(); ++a) {
			tref v = per_alt[a][idx];
			if (v && std::find(distinct.begin(), distinct.end(), v)
				== distinct.end()) {
				distinct.push_back(v);
				if (distinct.size() == 1)
					distinct_end = per_alt_ends[a][idx];
			}
		}
		if (distinct.size() <= 1) {
			caps.set(idx, distinct.empty() ? nullptr
				: distinct.front(), distinct_end);
			continue;
		}
		// Pin the end to the node's own right sibling, immune to hash-consing reuse.
		tref synth = adapter_.make_node_fn("__AMB__", distinct);
		caps.set(idx, synth,
			synth ? lcrs_tree<NodeT>::get(synth).right_sibling() : nullptr);
	}
	return true;
}

// --- match_node --------------------------------------------------------------

template <typename NodeT>
bool matcher<NodeT>::match_node(tref n,
	const pattern_node& p, capture_state& caps,
	ambig_mode mode, tref& next_sib) const
{
	if (!n) return false;
	const size_t entry = caps.mark();

	// An ambiguity wrapper occupies one slot in its sibling list.
	if (is_amb(n)) {
		bool matched = match_amb_alts(n, caps, mode, [&](tref alt) {
			tref ignored = nullptr;
			return match_node(alt, p, caps, mode, ignored);
		});
		if (matched)
			next_sib = lcrs_tree<NodeT>::get(n).right_sibling();
		return matched;
	}

	bool atom_ok = false;
	bool zero_width = false;
	tref matched_next = lcrs_tree<NodeT>::get(n).right_sibling();

	switch (p.k) {
	case pattern_node::kind::WILDCARD:
		atom_ok = true;
		break;
	case pattern_node::kind::NT:
		atom_ok = adapter_.is_nt(n, p.text);
		break;
	case pattern_node::kind::TERMINAL: {
		atom_ok = adapter_.is_terminal(n, p.text);
		// A run of childless terminal siblings can also spell the text.
		if (!atom_ok) {
			std::string acc;
			size_t run_len = 0;
			tref cur = n;
			while (cur) {
				auto txt = adapter_.terminal_leaf_text(cur);
				if (!txt || acc.size() + txt->size() > p.text.size())
					break;
				acc += *txt;
				++run_len;
				cur = lcrs_tree<NodeT>::get(cur).right_sibling();
				if (acc.size() >= p.text.size()) break;
			}
			if (run_len >= 2 && acc == p.text) {
				atom_ok = true;
				matched_next = cur;
			}
		}
		break;
	}
	case pattern_node::kind::CAPTURE: {
		// A capture is transparent. Its alternative can consume more
		// than one sibling, so preserve the position after its body.
		if (p.alternatives.empty()) atom_ok = true;
		for (const auto& alt : p.alternatives) {
			size_t m = caps.mark();
			// An alternative with no slots matches zero width at
			// `n` itself; it does not advance to the next sibling.
			tref alt_next = n;
			bool ok = alt.slots.empty()
				? true
				: match_slots(alt, 0, n, caps, mode,
					[&](tref s) { alt_next = s; return true; });
			if (ok) {
				atom_ok = true;
				zero_width = alt_next == n;
				matched_next = alt_next;
				break;
			}
			caps.rollback(m);
		}
		// A zero-width match has no node to point at: record
		// nullptr instead of the group's anchor position.
		if (atom_ok && p.capture_idx >= 0
			&& (int)caps.v.size() > p.capture_idx)
			caps.set(static_cast<size_t>(p.capture_idx),
				zero_width ? nullptr : n,
				zero_width ? nullptr : matched_next);
		break;
	}
	}

	if (!atom_ok) { caps.rollback(entry); return false; }

	// A zero-width capture has no node for '!' to test, so it fails.
	if (p.leaf && (zero_width || lcrs_tree<NodeT>::get(n).has_child())) {
		caps.rollback(entry);
		return false;
	}

	// Edge constraint on the matched node (orthogonal to atom kind).
	if (p.edge == edge_type::NONE) {
		next_sib = matched_next;
		return true;
	}

	if (p.edge == edge_type::DIRECT) {
		tref first_child = lcrs_tree<NodeT>::get(n).first();
		if (!match_seq(first_child, p.sub.front(), caps, mode)) {
			caps.rollback(entry);
			return false;
		}
	} else { // DESCENDANT
		if (!match_descendant(n, p.sub.front(), caps, mode)) {
			caps.rollback(entry);
			return false;
		}
	}

	next_sib = matched_next;
	return true;
}

// --- match_seq + match_slots ---------------------------------------------

template <typename NodeT>
bool matcher<NodeT>::match_seq(tref sib, const sibling_seq& seq,
	capture_state& caps, ambig_mode mode) const
{
	// A left-anchored sequence must start at `sib` (the first sibling
	// of the list). An unanchored one may start at any right sibling.
	tref start = sib;
	while (true) {
		size_t m = caps.mark();
		if (match_slots(seq, 0, start, caps, mode,
			[](tref) { return true; }))
			return true;
		caps.rollback(m);
		if (seq.left_anchor || !start) return false;
		start = lcrs_tree<NodeT>::get(start).right_sibling();
	}
}

template <typename NodeT>
template <typename Cont>
bool matcher<NodeT>::match_slots(const sibling_seq& seq,
	size_t slot_idx, tref sib, capture_state& caps, ambig_mode mode,
	Cont&& k) const
{
	// All slots consumed: the right anchor is checked here, then the
	// continuation takes over at the resulting position.
	if (slot_idx == seq.slots.size()) {
		if (seq.right_anchor && sib != nullptr) return false;
		return k(sib);
	}

	const sibling_slot& slot     = seq.slots[slot_idx];
	const pattern_node& node_pat = slot.node;
	int  min_rem = (slot.q == quantifier::NONE
			|| slot.q == quantifier::PLUS) ? 1 : 0;
	bool unbnd   = (slot.q == quantifier::STAR
			|| slot.q == quantifier::PLUS);

	// Greedy: consume one more sibling for this slot before advancing to
	// the next slot. A step that consumes nothing must not repeat at the
	// same position - it falls through to advancing instead.
	auto rep = [&](auto& self, tref s, int m_rem) -> bool {
		if (!s) return false;
		size_t m = caps.mark();
		tref matched_next = nullptr;
		if (match_node(s, node_pat, caps, mode, matched_next)) {
			int next_min = (m_rem > 0) ? m_rem - 1 : 0;
			if (unbnd && matched_next != s
				&& self(self, matched_next, next_min))
				return true;
			if (next_min == 0 && match_slots(seq, slot_idx + 1,
				matched_next, caps, mode, k))
				return true;
		}
		caps.rollback(m);
		return false;
	};
	if (rep(rep, sib, min_rem)) return true;

	// Slots match consecutive siblings: a failed slot is skipped at the
	// entry position (legal once the minimum is met), never slid past.
	return min_rem == 0
		&& match_slots(seq, slot_idx + 1, sib, caps, mode, k);
}

// --- match_descendant --------------------------------------------------------

template <typename NodeT>
bool matcher<NodeT>::match_descendant(tref n,
	const sibling_seq& seq, capture_state& caps,
	ambig_mode mode) const
{
	if (!n) return false;
	bool found = false;
	// Tries `seq` against the child list of every node in the subtree
	// (including `n`), so a match lies strictly below `n`. Anchors
	// still pin `^`/`$` to the ends of that child list.
	auto visit = [&](tref parent) -> bool {
		if (found) return false; // terminate
		if (is_amb(parent)) return true; // alts are not siblings
		tref first_child = lcrs_tree<NodeT>::get(parent).first();
		if (!first_child) return true;
		size_t m = caps.mark();
		if (match_seq(first_child, seq, caps, mode)) {
			found = true;
			return false; // terminate traversal, keep captures
		}
		caps.rollback(m);
		return true; // continue
	};
	pre_order<NodeT>(n).search(visit);
	return found;
}

// --- try_at_root -------------------------------------------------------------

template <typename NodeT>
bool matcher<NodeT>::try_at_root(tref n,
	capture_state& caps, ambig_mode mode, bool at_root) const
{
	// An `__AMB__` wrapper is not itself a match candidate: fork into
	// its alternatives, each inheriting the wrapper's own root-ness so
	// a top-level `^` still matches inside an ambiguous root.
	if (is_amb(n))
		return match_amb_alts(n, caps, mode, [&](tref alt) {
			return try_at_root(alt, caps, mode, at_root);
		});
	if (pat_.root_anchored && !at_root) return false;
	return match_slots(pat_.top, 0, n, caps, mode,
		[](tref) { return true; });
}

// --- match / search / search_all -----------------------------------------

template <typename NodeT>
bool matcher<NodeT>::match(tref root, match_result& m, ambig_mode mode) const
{
	m = match_result();
	if (!root) return false;
	capture_state caps(pat_.num_captures);
	std::unordered_map<tref, bool> amb_seen;
	if (try_at_root(root, caps, mode, true)
		&& !(mode == ambig_mode::FORBID
			&& subtree_has_amb(root, amb_seen))) {
		m.root = root;
		m.captures = std::move(caps.v);
		m.capture_ends = std::move(caps.ends);
		return true;
	}
	return false;
}

template <typename NodeT>
bool matcher<NodeT>::match(tref root, ambig_mode mode) const {
	match_result m;
	return match(root, m, mode);
}

template <typename NodeT>
bool matcher<NodeT>::search(tref root, match_result& m, ambig_mode mode) const
{
	m = match_result();
	if (!root) return false;
	// Root-anchored: only the root itself can ever match, so test just
	// that node instead of walking the tree and failing everywhere else.
	if (pat_.root_anchored) return match(root, m, mode);

	std::unordered_map<tref, bool> amb_seen;
	// ANY/ALL also visit nodes below an __AMB__ alternative.
	auto visit = [&](tref candidate, tref parent) -> bool {
		if (m.root) return false;
		if (is_amb(parent)) return true;
		capture_state caps(pat_.num_captures);
		if (try_at_root(candidate, caps, mode, candidate == root)
			&& !(mode == ambig_mode::FORBID
				&& subtree_has_amb(candidate, amb_seen))) {
			m.root = candidate;
			m.captures = std::move(caps.v);
			m.capture_ends = std::move(caps.ends);
			return false;
		}
		return true;
	};
	pre_order<NodeT>(root).search(visit);
	return static_cast<bool>(m);
}

template <typename NodeT>
bool matcher<NodeT>::search(tref root, ambig_mode mode) const {
	match_result m;
	return search(root, m, mode);
}

template <typename NodeT>
auto matcher<NodeT>::search_all(tref root,
	ambig_mode mode) const -> std::vector<match_result>
{
	std::vector<match_result> matches;
	if (!root) return matches;
	if (pat_.root_anchored) {
		match_result m;
		if (match(root, m, mode)) matches.push_back(std::move(m));
		return matches;
	}
	std::unordered_map<tref, bool> amb_seen;
	// ANY/ALL also visit nodes below an __AMB__ alternative.
	auto visit = [&](tref candidate, tref parent) -> bool {
		if (!is_amb(parent)) {
			capture_state caps(pat_.num_captures);
			if (try_at_root(candidate, caps, mode,
				candidate == root)
				&& !(mode == ambig_mode::FORBID
					&& subtree_has_amb(candidate, amb_seen))) {
				match_result m;
				m.root = candidate;
				m.captures = std::move(caps.v);
				m.capture_ends = std::move(caps.ends);
				matches.push_back(std::move(m));
			}
		}
		return true;
	};
	pre_order<NodeT>(root).search(visit);
	return matches;
}

// --- replace / replace_if / replace_until / trim / trim_top --------------

template <typename NodeT>
tref matcher<NodeT>::replace_at_root_only(tref root, const replace_fn& fn,
	ambig_mode mode) const
{
	match_result m;
	if (match(root, m, mode)) return fn(m);
	return root;
}

// Post-order rebuild: children are visited (and possibly deleted) before
// their parent, so a parent always tests its already-final children.
template <typename NodeT>
tref matcher<NodeT>::replace_walk(tref n, tref parent, bool at_root,
	const replace_fn& fn, ambig_mode mode, const query_fn& should_descend,
	std::unordered_map<tref, bool>& amb_seen,
	std::unordered_map<tref, std::pair<tref, bool>>& memo,
	bool& deleted) const
{
	deleted = false;
	if (!n) return nullptr;
	if (should_descend && !should_descend(n)) return n;

	auto cached = memo.find(n);
	if (cached != memo.end()) {
		deleted = cached->second.second;
		return cached->second.first;
	}

	const auto& nt = lcrs_tree<NodeT>::get(n);
	trefs new_children;
	bool changed = false;
	for (tref c = nt.first(); c;
		c = lcrs_tree<NodeT>::get(c).right_sibling()) {
		bool child_deleted = false;
		tref nc = replace_walk(c, n, false, fn, mode, should_descend,
			amb_seen, memo, child_deleted);
		if (child_deleted) { changed = true; continue; }
		if (nc != c) changed = true;
		new_children.push_back(nc);
	}
	tref rebuilt = changed
		? lcrs_tree<NodeT>::get(nt.value, new_children) : n;

	tref result = rebuilt;
	// A node whose own parent is an __AMB__ wrapper is never a match
	// candidate on its own; the wrapper itself forks into it instead.
	if (!is_amb(parent)) {
		capture_state caps(pat_.num_captures);
		if (try_at_root(rebuilt, caps, mode, at_root)
			&& !(mode == ambig_mode::FORBID
				&& subtree_has_amb(rebuilt, amb_seen))) {
			match_result mt;
			mt.root = rebuilt;
			mt.captures = std::move(caps.v);
			mt.capture_ends = std::move(caps.ends);
			tref r = fn(mt);
			if (!r) { deleted = true; result = nullptr; }
			else result = r;
		}
	}
	memo.emplace(n, std::make_pair(result, deleted));
	return result;
}

template <typename NodeT>
tref matcher<NodeT>::replace(tref root, replace_fn fn,
	ambig_mode mode) const
{
	if (!root || !fn) return root;
	if (pat_.root_anchored) return replace_at_root_only(root, fn, mode);

	std::unordered_map<tref, bool> amb_seen;
	std::unordered_map<tref, std::pair<tref, bool>> memo;
	bool deleted = false;
	tref result = replace_walk(root, nullptr, true, fn, mode, query_fn(),
		amb_seen, memo, deleted);
	return deleted ? nullptr : result;
}

template <typename NodeT>
tref matcher<NodeT>::replace_if(tref root, replace_fn fn, query_fn query,
	ambig_mode mode) const
{
	if (!root || !fn || !query) return root;
	if (pat_.root_anchored)
		return query(root) ? replace_at_root_only(root, fn, mode) : root;

	std::unordered_map<tref, bool> amb_seen;
	std::unordered_map<tref, std::pair<tref, bool>> memo;
	bool deleted = false;
	tref result = replace_walk(root, nullptr, true, fn, mode, query,
		amb_seen, memo, deleted);
	return deleted ? nullptr : result;
}

template <typename NodeT>
tref matcher<NodeT>::replace_until(tref root, replace_fn fn, query_fn query,
	ambig_mode mode) const
{
	if (!root || !fn || !query) return root;
	query_fn until_gate = [&query](tref n) { return !query(n); };
	if (pat_.root_anchored)
		return until_gate(root)
			? replace_at_root_only(root, fn, mode) : root;

	std::unordered_map<tref, bool> amb_seen;
	std::unordered_map<tref, std::pair<tref, bool>> memo;
	bool deleted = false;
	tref result = replace_walk(root, nullptr, true, fn, mode, until_gate,
		amb_seen, memo, deleted);
	return deleted ? nullptr : result;
}

template <typename NodeT>
tref matcher<NodeT>::trim(tref root, ambig_mode mode) const {
	replace_fn always_delete = [](const match_result&) -> tref {
		return nullptr;
	};
	return replace(root, always_delete, mode);
}

// Top-down: a match is deleted without descending into it, so a nested
// match under a deleted ancestor is never independently tested.
template <typename NodeT>
tref matcher<NodeT>::trim_top_walk(tref n, tref parent, bool at_root,
	ambig_mode mode, std::unordered_map<tref, bool>& amb_seen,
	bool& deleted) const
{
	deleted = false;
	if (!n) return nullptr;
	if (!is_amb(parent)) {
		capture_state caps(pat_.num_captures);
		if (try_at_root(n, caps, mode, at_root)
			&& !(mode == ambig_mode::FORBID
				&& subtree_has_amb(n, amb_seen))) {
			deleted = true;
			return nullptr;
		}
	}
	const auto& nt = lcrs_tree<NodeT>::get(n);
	trefs new_children;
	bool changed = false;
	for (tref c = nt.first(); c;
		c = lcrs_tree<NodeT>::get(c).right_sibling()) {
		bool child_deleted = false;
		tref nc = trim_top_walk(c, n, false, mode, amb_seen,
			child_deleted);
		if (child_deleted) { changed = true; continue; }
		if (nc != c) changed = true;
		new_children.push_back(nc);
	}
	return changed ? lcrs_tree<NodeT>::get(nt.value, new_children) : n;
}

template <typename NodeT>
tref matcher<NodeT>::trim_top(tref root, ambig_mode mode) const {
	if (!root) return root;
	if (pat_.root_anchored) {
		replace_fn always_delete = [](const match_result&) -> tref {
			return nullptr;
		};
		return replace_at_root_only(root, always_delete, mode);
	}
	std::unordered_map<tref, bool> amb_seen;
	bool deleted = false;
	tref result = trim_top_walk(root, nullptr, true, mode, amb_seen,
		deleted);
	return deleted ? nullptr : result;
}

// --- replace_fixpoint --------------------------------------------------------

template <typename NodeT>
diagnostics::result<tref> matcher<NodeT>::replace_fixpoint(tref root,
	replace_fn fn, ambig_mode mode, size_t max_iters) const
{
	diagnostics::result<tref> R;
	tref cur = root;
	for (size_t i = 0; max_iters == 0 || i < max_iters; ++i) {
		tref next = replace(cur, fn, mode);
		if (next == cur) { R.emplace(cur); return R; }
		cur = next;
	}
	// A rule set that never settles is a real failure, not a silent
	// truncation: the iteration count needed exceeded the allowed range.
	R.error(diagnostics::code::out_of_range,
		"replace_fixpoint: pattern did not reach a fixed point");
	return R;
}

//------------------------------------------------------------------------------
// Factories
//------------------------------------------------------------------------------

template <typename C, typename T>
auto matcher_for(const nonterminals<C, T>& nts, compiled_pattern pat)
	-> matcher<pnode_type<C, T>>
{
	return matcher<pnode_type<C, T>>(std::move(pat),
		parse_node_adapter<C, T>(nts));
}

template <typename C, typename T>
auto matcher_for(const nonterminals<C, T>& nts, std::string_view pattern)
	-> diagnostics::result<matcher<pnode_type<C, T>>>
{
	using matcher_t = matcher<pnode_type<C, T>>;
	diagnostics::result<matcher_t> R;
	auto cr = compile(pattern);
	if (!cr.has_value()) {
		R.report().append(std::move(cr).report());
		return R;
	}
	R.emplace(matcher_for<C, T>(nts, std::move(cr).value()));
	return R;
}

template <nt_source P>
auto matcher_for(const P& p, compiled_pattern pat)
	-> matcher<pnode_type<typename P::char_type, typename P::terminal_type>>
{
	return matcher_for(p.get_grammar().get_nts(), std::move(pat));
}

template <nt_source P>
auto matcher_for(const P& p, std::string_view pattern)
	-> diagnostics::result<matcher<pnode_type<typename P::char_type,
		typename P::terminal_type>>>
{
	return matcher_for(p.get_grammar().get_nts(), pattern);
}

} // namespace idni::treemr

#endif // __IDNI__FORMAT__TREEMR__TREEMR_TMPL_H__
