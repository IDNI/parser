// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Regression locks for the conjunction/negation retraction machinery.
//
// Covers two fixed bug classes (see
// .local/BUG.negation-conjunction-under-star.md for the full root-cause
// analysis and fix history):
//   1. Negation of a conjunction was not enforced under a Kleene star
//      (G1-G5): a disallowed char in the final star iteration leaked through
//      because cascade_uncomplete only tracked the last completer.
//   2. Over-retraction in ambiguous spans (G6/G7): a route provisionally
//      completed and killed late by a negated conjunct dragged down a
//      consumer that a surviving sibling route still supported.
// G8 locks the early-kill near-miss shape that was never broken.
//
// Convention: when a parser change breaks or fixes behavior captured here,
// add the failing input as a doctest::should_fail() case first, fix, then
// promote it to a normal assert (as was done for both classes above).

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"

#include <string>

using namespace std;
using namespace idni;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// Asserts: TGF compiles and `input` (given as std::string) parses successfully.
static void tgf_parses(const char* g_tgf, const string& input)
{
	nonterminals<char> nts;
	auto gr = tgf<char>::from_string(nts, string(g_tgf));
	if (!gr.has_value()) {
		FAIL_CHECK("TGF compile failed for grammar:\n" << g_tgf);
		return;
	}
	grammar<char> g = std::move(gr).value();
	if (g.size() == 0) {
		FAIL_CHECK("TGF produced empty grammar:\n" << g_tgf);
		return;
	}
	for (auto tree_path : {parse_tree_path::bintree_path,
			parse_tree_path::forest_path})
	for (bool gc : {false, true}) {
		parser<char> p(g);
		auto r = p.parse(input.data(), input.size(),
			{ .tree_path = tree_path, .enable_gc = gc, .gc_lag = 1 });
		if (!r.found) {
			string msg = r.parse_error.to_str(
				parser<char>::error::info_lvl::INFO_BASIC);
			FAIL_CHECK("expected input to parse (enable_gc="
				<< gc << "), got error: '" << msg << "'");
		}
	}
}

// Asserts: TGF compiles and `input` (given as std::string) is rejected.
static void tgf_rejects(const char* g_tgf, const string& input)
{
	nonterminals<char> nts;
	auto gr = tgf<char>::from_string(nts, string(g_tgf));
	if (!gr.has_value()) {
		FAIL_CHECK("TGF compile failed for grammar:\n" << g_tgf);
		return;
	}
	grammar<char> g = std::move(gr).value();
	if (g.size() == 0) {
		FAIL_CHECK("TGF produced empty grammar:\n" << g_tgf);
		return;
	}
	for (auto tree_path : {parse_tree_path::bintree_path,
			parse_tree_path::forest_path})
	for (bool gc : {false, true}) {
		parser<char> p(g);
		auto r = p.parse(input.data(), input.size(),
			{ .tree_path = tree_path, .enable_gc = gc, .gc_lag = 1 });
		if (r.found)
			FAIL_CHECK("expected input to be rejected (enable_gc="
				<< gc << "), but parse succeeded");
	}
}

// ---------------------------------------------------------------------------
// Grammar string constants
// ---------------------------------------------------------------------------

// G1: single use (no star) - the reference case showing the rule is correct
// in isolation.  See BUG.negation-conjunction-under-star.md §2.
static const char* G1 =
	"@use char class any, ascii, cntrl.\n"
	"start => unescaped.\n"
	"unescaped => any & ~('\"' | '\\\\' | cntrl & ascii & ~'\\x7F').\n";

// G2: negate a SINGLE literal under star - works correctly.
// See BUG.negation-conjunction-under-star.md §2 (table row `any & ~'a'`).
static const char* G2 =
	"@use char class any.\n"
	"start => x*.\n"
	"x => any & ~'a'.\n";

// G3: negate a DISJUNCTION under star - works correctly.
// See BUG.negation-conjunction-under-star.md §2 (table row `any & ~('a'|'b')`).
static const char* G3 =
	"@use char class any.\n"
	"start => x*.\n"
	"x => any & ~('a' | 'b').\n";

// G4: the JSON `unescaped` rule used under star.
// Multi-char inputs with a tab in the middle are rejected even today
// (the star cannot bridge the disallowed char between two allowed ones).
// A LONE disallowed char (e.g. bare TAB) incorrectly slips through - that
// is the bug captured in Group B below.
// See BUG.negation-conjunction-under-star.md §2 (table, last row).
static const char* G4 =
	"@use char class any, ascii, cntrl.\n"
	"start => unescaped*.\n"
	"unescaped => any & ~('\"' | '\\\\' | cntrl & ascii & ~'\\x7F').\n";

// G5: minimal reproduction - negate a conjunction under star.
// See BUG.negation-conjunction-under-star.md §2 (table row `any & ~(cntrl & ascii)`).
static const char* G5 =
	"@use char class any, ascii, cntrl.\n"
	"start => x*.\n"
	"x => any & ~(cntrl & ascii).\n";

// G6: star variant - ambiguous repeated element with two routes for 'a'.
// Route 'bad': bad => 'a' & ~k, where k => 'a' & ~nothing (conjunctive, late kill).
// Route 'good': good => 'a' & ~nope, where nope => 'q' (simple, always fails on 'a').
// The step-2 forward_deps completer-side edge retracts the consumer via the dying
// 'bad' route without checking whether the 'good' route still supports it.
// See .local/BUG.negation-conjunction-under-star.md §6.1 KNOWN LATENT RISK, step 3.
static const char* G6 =
	"@use char class any.\n"
	"start => e*.\n"
	"e => bad | good.\n"
	"bad => 'a' & ~k.\n"
	"good => 'a' & ~nope.\n"
	"k => 'a' & ~nothing.\n"
	"nope => 'q'.\n"
	"nothing => 'q'.\n";

// G7: no star - same late-kill shape under a plain two-symbol production.
// H can be Y (dying route: Y => 'a' & ~k, k conjunctive, late kill) or
// Y2 (surviving route: Y2 => 'a' & ~nope, nope => 'q').
// The step-2 completer edge retracts 'up' via the dying Y derivation even
// though the Y2 derivation of H still supports it.
// See .local/BUG.negation-conjunction-under-star.md §6.1 KNOWN LATENT RISK, step 3.
static const char* G7 =
	"@use char class any.\n"
	"start => up.\n"
	"up => H z.\n"
	"H => Y | Y2.\n"
	"Y => 'a' & ~k.\n"
	"Y2 => 'a' & ~nope.\n"
	"k => 'a' & ~nothing.\n"
	"nope => 'q'.\n"
	"nothing => 'q'.\n"
	"z => 'b'.\n";

// G8: early-kill near-miss - the dying route (Y => d_user => 'a' & ~kill,
// kill => 'a') fails BEFORE it propagates: kill completes early (non-conjunctive),
// so d_user is uncompleted before 'up' is ever derived via Y.
// The surviving route (Y2 => s_user => 'a' & ~nope, nope => 'q') is unaffected.
// 'ab' must parse today and must continue to parse after step 3.
static const char* G8 =
	"@use char class any.\n"
	"start => up.\n"
	"up => H z.\n"
	"H => Y | Y2.\n"
	"Y => d_user.\n"
	"Y2 => s_user.\n"
	"d_user => 'a' & ~kill.\n"
	"s_user => 'a' & ~nope.\n"
	"kill => 'a'.\n"
	"nope => 'q'.\n"
	"z => 'b'.\n";

// ---------------------------------------------------------------------------
// Group A - correct behavior, must always pass
// ---------------------------------------------------------------------------

TEST_SUITE("negation under repetition: regression locks") {

// --- G1: single use (no star), correct in isolation ---

TEST_CASE("G1 single-use: accepts plain letter 'a'") {
	tgf_parses(G1, "a");
}

TEST_CASE("G1 single-use: accepts DEL (0x7F) as valid unescaped char") {
	tgf_parses(G1, string(1, '\x7F'));
}

TEST_CASE("G1 single-use: rejects TAB (0x09, C0 control)") {
	tgf_rejects(G1, string(1, '\x09'));
}

TEST_CASE("G1 single-use: rejects SOH (0x01, C0 control)") {
	tgf_rejects(G1, string(1, '\x01'));
}

TEST_CASE("G1 single-use: rejects quote '\"'") {
	tgf_rejects(G1, "\"");
}

TEST_CASE("G1 single-use: rejects backslash '\\\\'") {
	tgf_rejects(G1, "\\");
}

// --- G2: negate single literal under star ---

TEST_CASE("G2 star, ~single-literal: accepts 'b' (not 'a')") {
	tgf_parses(G2, "b");
}

TEST_CASE("G2 star, ~single-literal: rejects lone 'a'") {
	tgf_rejects(G2, "a");
}

TEST_CASE("G2 star, ~single-literal: rejects 'cac' (contains 'a')") {
	tgf_rejects(G2, "cac");
}

// --- G3: negate disjunction under star ---

TEST_CASE("G3 star, ~disjunction: accepts 'c' (not 'a' or 'b')") {
	tgf_parses(G3, "c");
}

TEST_CASE("G3 star, ~disjunction: rejects lone 'a'") {
	tgf_rejects(G3, "a");
}

TEST_CASE("G3 star, ~disjunction: rejects 'cac' (contains 'a')") {
	tgf_rejects(G3, "cac");
}

// --- G4: multi-char with embedded disallowed char - rejected even today ---

TEST_CASE("G4 unescaped*: rejects 'a' TAB 'b' (tab between valid chars)") {
	// The star cannot bridge the TAB between two valid unescaped chars.
	tgf_rejects(G4, string("a\x09""b"));
}

TEST_CASE("G4 unescaped*: accepts 'ab' (two valid chars)") {
	tgf_parses(G4, "ab");
}

TEST_CASE("G4 unescaped*: accepts 'a' DEL 'b' (DEL is valid unescaped)") {
	tgf_parses(G4, string("a\x7F""b"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Former Group B - promoted to regression locks after the cascade fix.
//
// These cases previously required doctest::should_fail() because the parser
// incorrectly accepted disallowed chars under a Kleene star when the negated
// operand was a conjunction (forward_deps leak - see
// .local/BUG.negation-conjunction-under-star.md §4). The fix (forward_deps +
// retract_item in cascade_uncomplete) closed the leak; these are now normal
// regression locks inside the "negation under repetition: regression locks"
// suite.
//
// Ref: .local/BUG.negation-conjunction-under-star.md §2, §3, §4, §6.1
// ---------------------------------------------------------------------------

TEST_SUITE("negation under repetition: regression locks") {

// G4, lone TAB: rejected by `unescaped*` now that the cascade is fixed.
TEST_CASE("G4 unescaped*: lone TAB (0x09) must be rejected")
{
	// A single C0 control char under the Kleene star was incorrectly accepted
	// when the negated operand was a conjunction (cntrl & ascii & ~'\x7F').
	// Fixed by recording predecessor->derived edges (forward_deps) and
	// following them in retract_item / cascade_uncomplete.
	// See .local/BUG.negation-conjunction-under-star.md §6.1.
	tgf_rejects(G4, string(1, '\x09'));
}

// G4, trailing TAB: "a\x09" - the TAB at the tail position is now correctly
// rejected; same cascade fix as the lone-TAB case.
TEST_CASE("G4 unescaped*: trailing TAB 'a' TAB must be rejected")
{
	// The negation-conjunction cascade leak previously allowed the lone
	// disallowed char at the tail position to slip through. Now correctly
	// retracted via forward_deps.
	// See .local/BUG.negation-conjunction-under-star.md §6.1.
	tgf_rejects(G4, string("a\x09"));
}

// G5, lone SOH: `x => any & ~(cntrl & ascii)` under star now correctly rejects
// SOH after the forward_deps fix.
TEST_CASE("G5 star, ~conjunction: lone SOH (0x01) must be rejected")
{
	// The negated conjunction (~(cntrl & ascii)) was not enforced for a lone
	// disallowed char under Kleene star. Fixed by the forward_deps cascade.
	// See .local/BUG.negation-conjunction-under-star.md §6.1.
	tgf_rejects(G5, string(1, '\x01'));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Sanity locks for G6/G7/G8 - correct today, must remain correct.
// ---------------------------------------------------------------------------

TEST_SUITE("negation under repetition: regression locks") {

TEST_CASE("G6 e*: 'q' must be rejected (matches nothing in e)") {
	tgf_rejects(G6, "q");
}

TEST_CASE("G7 plain: 'b' must be rejected (missing H)") {
	tgf_rejects(G7, "b");
}

TEST_CASE("G7 plain: 'a' must be rejected (missing z)") {
	tgf_rejects(G7, "a");
}

// G8 early-kill near-miss: kill => 'a' is non-conjunctive, so d_user fails
// before 'up' is derived via Y. The surviving Y2 route must still parse 'ab'.
TEST_CASE("G8 early-kill near-miss: 'ab' parses via Y2") {
	tgf_parses(G8, "ab");
}

TEST_CASE("G8 early-kill near-miss: 'b' must be rejected (missing H)") {
	tgf_rejects(G8, "b");
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Ambiguity survival locks - fixed by step 3a (exact completion_count via
// counted_completions + removal of the unguarded completer-side forward_deps
// edge). See .local/BUG.negation-conjunction-under-star.md.
//
// These inputs are VALID: an ambiguous span has a route that provisionally
// completes and is killed late by a negated conjunct (its target is itself
// conjunctive), and a sibling route that survives. The step-2 MVP wrongly
// rejected them by retracting the consumer through the dying route; with the
// exact span count the cascade now stops while a sibling still supports it.
// ---------------------------------------------------------------------------

TEST_SUITE("negation under repetition: regression locks") {

TEST_CASE("G6 e*: 'a' parses via good (dying sibling must not kill it)") {
	tgf_parses(G6, "a");
}

TEST_CASE("G6 e*: 'aa' parses via good good") {
	tgf_parses(G6, "aa");
}

TEST_CASE("G7 plain: 'ab' parses via Y2 z (dying Y must not kill consumer)") {
	tgf_parses(G7, "ab");
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Grammar constants G9-G12: extended coverage for the negation/retraction
// machinery.  All four shapes were listed as TODO in the step-1 entry of
// .local/BUG.negation-conjunction-under-star.md §7.
// ---------------------------------------------------------------------------

// G9: `+` repetition - same `unescaped` predicate as G4 but desugared via
// one-or-more (requires at least one element).  The Kleene-star leak from the
// original bug §4.2 applied equally to `+`; this locks the correct behaviour
// after the cascade fix.
static const char* G9 =
	"@use char class any, ascii, cntrl.\n"
	"start => unescaped+.\n"
	"unescaped => any & ~('\"' | '\\\\' | cntrl & ascii & ~'\\x7F').\n";

// G10: optional (nullable) tail AFTER a negated-conjunction symbol - the §4.2
// "non-last completer" shape without any star.  The nullable optional `['!']`
// plays the role of the null-producing tail; retraction must propagate past it.
static const char* G10 =
	"@use char class any, ascii, cntrl.\n"
	"start => unescaped ['!'].\n"
	"unescaped => any & ~(cntrl & ascii).\n";

// G11: nested negation `~(alpha & ~vowel)` under star - matches anything that
// is NOT a non-vowel letter (i.e. accepts vowels, digits, punctuation but
// rejects consonants).  Tests that nested negation inside a conjunction is
// correctly retracted for the final star iteration.
static const char* G11 =
	"@use char class any, alpha.\n"
	"start => x*.\n"
	"x => any & ~(alpha & ~vowel).\n"
	"vowel => 'a' | 'e' | 'i' | 'o' | 'u'.\n";

// G12: negated conjunction at a NON-LAST symbol in a plain two-symbol body
// (no repetition, no optional) - the §4.2 "non-last completer" base shape.
// Both `a` and `b` use `any & ~(cntrl & ascii)`.  Retraction must be
// enforced for the FIRST symbol even though a second symbol follows.
static const char* G12 =
	"@use char class any, ascii, cntrl.\n"
	"start => a b.\n"
	"a => any & ~(cntrl & ascii).\n"
	"b => any & ~(cntrl & ascii).\n";

// ---------------------------------------------------------------------------
// G9: `+` repetition (§4.2 - same cascade shape as `*` but different desug.)
// ---------------------------------------------------------------------------

TEST_SUITE("negation under repetition: regression locks") {

TEST_CASE("G9 unescaped+: rejects empty string (+ requires at least one)") {
	tgf_rejects(G9, "");
}

TEST_CASE("G9 unescaped+: accepts single valid char 'a'") {
	tgf_parses(G9, "a");
}

TEST_CASE("G9 unescaped+: accepts two valid chars 'ab'") {
	tgf_parses(G9, "ab");
}

TEST_CASE("G9 unescaped+: accepts DEL (0x7F) as valid unescaped char") {
	// DEL is excluded from cntrl & ascii & ~'\x7F' - it must pass through.
	tgf_parses(G9, string(1, '\x7F'));
}

TEST_CASE("G9 unescaped+: lone TAB (0x09) must be rejected") {
	// TAB is a C0 control (cntrl & ascii & ~'\x7F'): must be rejected.
	// The * analogue (G4) leaked pre-fix; the + desugaring must not either.
	tgf_rejects(G9, string(1, '\x09'));
}

TEST_CASE("G9 unescaped+: trailing TAB 'a' TAB must be rejected") {
	// Disallowed char in the final + iteration - the leak shape from §4.2.
	tgf_rejects(G9, string("a\x09"));
}

TEST_CASE("G9 unescaped+: interior TAB 'a' TAB 'b' must be rejected") {
	// Disallowed char between valid chars - correctly rejected even pre-fix.
	tgf_rejects(G9, string("a\x09""b"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// G10: optional (nullable) tail after negated-conjunction - §4.2 no-star form
// ---------------------------------------------------------------------------

TEST_SUITE("negation under repetition: regression locks") {

TEST_CASE("G10 optional tail: accepts 'a' (optional empty)") {
	// Plain valid unescaped char with the optional '!' absent.
	tgf_parses(G10, "a");
}

TEST_CASE("G10 optional tail: accepts \"a!\" (optional present)") {
	// Valid char followed by the optional '!' suffix.
	tgf_parses(G10, "a!");
}

TEST_CASE("G10 optional tail: accepts '!' alone ('!' is a valid unescaped char)") {
	// '!' is not a control char so unescaped matches it; optional absent.
	tgf_parses(G10, "!");
}

TEST_CASE("G10 optional tail: lone SOH (0x01) must be rejected") {
	// SOH is cntrl & ascii: retraction must survive even with the nullable tail.
	tgf_rejects(G10, string(1, '\x01'));
}

TEST_CASE("G10 optional tail: SOH followed by '!' must be rejected") {
	// Control char then optional present - retraction must not be evaded by '!'.
	tgf_rejects(G10, string("\x01") + "!");
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// G11: nested negation ~(alpha & ~vowel) under star
// ---------------------------------------------------------------------------

TEST_SUITE("negation under repetition: regression locks") {

TEST_CASE("G11 nested ~(alpha & ~vowel)*: accepts vowel 'a'") {
	// 'a' is alpha & vowel, so ~(alpha & ~vowel) is satisfied.
	tgf_parses(G11, "a");
}

TEST_CASE("G11 nested ~(alpha & ~vowel)*: accepts vowel + digit 'e5'") {
	// 'e' is a vowel; '5' is not alpha so ~(alpha & ~vowel) is satisfied.
	tgf_parses(G11, "e5");
}

TEST_CASE("G11 nested ~(alpha & ~vowel)*: accepts digit '5'") {
	// '5' is not alpha at all, so the negated conjunction is trivially satisfied.
	tgf_parses(G11, "5");
}

TEST_CASE("G11 nested ~(alpha & ~vowel)*: rejects consonant 'b'") {
	// 'b' is alpha & ~vowel, so ~(alpha & ~vowel) fails.
	tgf_rejects(G11, "b");
}

TEST_CASE("G11 nested ~(alpha & ~vowel)*: rejects 'ab' (contains consonant)") {
	// 'b' at position 1 makes the second star iteration fail.
	tgf_rejects(G11, "ab");
}

TEST_CASE("G11 nested ~(alpha & ~vowel)*: rejects '5b' (trailing consonant)") {
	// Disallowed char in the FINAL star iteration - the late-position leak shape.
	tgf_rejects(G11, "5b");
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// G12: negated conjunction at a NON-LAST symbol, plain two-symbol body (§4.2)
// ---------------------------------------------------------------------------

TEST_SUITE("negation under repetition: regression locks") {

TEST_CASE("G12 two-symbol body: accepts 'xy' (both valid)") {
	tgf_parses(G12, "xy");
}

TEST_CASE("G12 two-symbol body: rejects 'x' SOH (control in last position)") {
	// 'b' position is a control - must be rejected.
	tgf_rejects(G12, string("x\x01"));
}

TEST_CASE("G12 two-symbol body: rejects SOH 'y' (control in FIRST position)") {
	// 'a' position is a control - non-last symbol retraction must fire (§4.2).
	tgf_rejects(G12, string("\x01y"));
}

TEST_CASE("G12 two-symbol body: rejects TAB TAB (both control)") {
	tgf_rejects(G12, string("\x09\x09"));
}

TEST_CASE("G12 two-symbol body: rejects 'x' (too short - one char)") {
	tgf_rejects(G12, "x");
}

TEST_CASE("G12 two-symbol body: rejects 'xyz' (too long - three chars)") {
	tgf_rejects(G12, "xyz");
}

} // TEST_SUITE
