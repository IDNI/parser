// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the treemr DSL compiler and matching engine: compile()
// accept/reject cases and IR assertions, then matcher<...> matching
// against real parse trees.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "parser.h"
#include "format/tgf/tgf.h"
#include "format/treemr/treemr.h"
#include "tgf/parser_gen.h"

using namespace std;
using namespace idni;
using namespace idni::treemr;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// Compile a DSL pattern; the test fixture only uses these for structural
// assertions on the resulting compiled_pattern.
diagnostics::result<compiled_pattern> compile_ok(string_view p) {
	auto r = treemr::compile(p);
	if (!r.has_value()) {
		FAIL_CHECK("expected DSL to compile: \"" << p << "\"");
	}
	return r;
}

// Build a parse tree from a TGF grammar + input string. Compiled fresh
// each call so each test owns its nonterminals/grammar lifetime, since
// the matcher captures `nts` by reference.
struct parsed {
	nonterminals<char>     nts;
	grammar<char>          g;
	parser<char>::result   r;
	tref                   root = nullptr;

	parsed(const char* tgf_src, const char* input)
		: nts{}
		, g{ [&]() {
			auto gr = tgf<char>::from_string(nts, string(tgf_src));
			REQUIRE(gr.has_value());
			return std::move(gr).value();
		}() }
		, r{ [&]() {
			parser<char> p(g);
			auto res = p.parse(input, strlen(input),
				{ .enable_gc = false, .gc_lag = 1 });
			REQUIRE(res.found);
			return res;
		}() }
	{
		root = r.get_shaped_tree2();
		REQUIRE(root != nullptr);
	}
};

// A tiny CSV-like grammar used by the matching tests.
const char* CSV_TGF =
	" @use char class digit. \n"
	" start  => csv. \n"
	" csv    => row. \n"
	" row    => cell (',' cell)*. \n"
	" cell   => digit+. \n";

// Detect whether the matcher's report carries a parse_error diagnostic.
template <typename T>
bool has_parse_error(const diagnostics::result<T>& r) {
	if (r.has_value()) return false;
	for (const auto& n : r.report().nodes())
		if (n.tag == diagnostics::code::parse_error) return true;
	return false;
}

// Pretty-printable terminals helper for trace messages (debug aid).
[[maybe_unused]] string terminals_of(tref n) {
	if (!n) return {};
	return parser<char>::tree::get(n).get_terminals();
}

// ===========================================================================
// A. DSL compile contract
// ===========================================================================

TEST_SUITE("treemr::compile - acceptance") {

	TEST_CASE("rule >> digit > '1'") {
		auto r = compile_ok("rule >> digit > '1'");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		CHECK(root.edge == edge_type::DESCENDANT);
	}

	TEST_CASE("rule > (^ digit+ term? $)") {
		// Both anchors bind to the capture's single alternative.
		auto r = compile_ok("rule > (^ digit+ term? $)");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		REQUIRE(root.edge == edge_type::DIRECT);
		REQUIRE(root.sub.front().slots.size() == 1);
		// The lone child of root.sub is the capture group; the anchored
		// seq is the capture's single alternative.
		const auto& cap_node = root.sub.front().slots[0].node;
		REQUIRE(cap_node.k == pattern_node::kind::CAPTURE);
		REQUIRE(cap_node.alternatives.size() == 1);
		const auto& body = cap_node.alternatives.front();
		CHECK(body.left_anchor);
		CHECK(body.right_anchor);
		REQUIRE(body.slots.size() == 2);
		CHECK(body.slots[0].q == quantifier::PLUS);
		CHECK(body.slots[1].q == quantifier::OPT);
	}

	TEST_CASE("rule > ^ digit+ term? $ - both anchors bind to the INNER seq") {
		// The edge slurps the tail, so both `^` and `$` land in the
		// sib_seq introduced by `>`, not the outer one.
		auto r = compile_ok("rule > ^ digit+ term? $");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		CHECK_FALSE(cp.top.right_anchor);      // outer holds neither
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		REQUIRE(root.edge == edge_type::DIRECT);
		CHECK(root.sub.front().left_anchor);           // inner holds the `^`
		CHECK(root.sub.front().right_anchor);          // inner holds the `$`
		REQUIRE(root.sub.front().slots.size() == 2);
	}

	TEST_CASE("rule > (digit | identifier) - alternation, 1 capture") {
		auto r = compile_ok("rule > (digit | identifier)");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		CHECK(cp.num_captures == 1);
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		REQUIRE(root.edge == edge_type::DIRECT);
		REQUIRE(root.sub.front().slots.size() == 1);
		const auto& cap_node = root.sub.front().slots[0].node;
		CHECK(cap_node.k == pattern_node::kind::CAPTURE);
		CHECK(cap_node.alternatives.size() == 2);
	}

	TEST_CASE("x > (^ a | ^ b) - `^` on every alternative anchors the "
		"enclosing sequence, so the pattern matches like x > ^ (a | b)")
	{
		// An anchor on every alternative of a group propagates to the
		// enclosing sequence, the same as a bare `^` in front of it.
		// Wrapped in an edge so the propagated anchor is not at the
		// top of the pattern, which has no parent for `^` to ask
		// about.
		auto r = compile_ok("x > (^ a | ^ b)");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		REQUIRE(root.edge == edge_type::DIRECT);
		const auto& inner = root.sub.front();
		CHECK(inner.left_anchor);
		REQUIRE(inner.slots.size() == 1);
		const auto& cap_node = inner.slots[0].node;
		CHECK(cap_node.k == pattern_node::kind::CAPTURE);
		REQUIRE(cap_node.alternatives.size() == 2);
		CHECK(cap_node.alternatives[0].left_anchor);
		CHECK(cap_node.alternatives[1].left_anchor);
	}

	TEST_CASE("rule > digit %* term - wildcard gap") {
		auto r = compile_ok("rule > digit %* term");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		REQUIRE(root.edge == edge_type::DIRECT);
		REQUIRE(root.sub.front().slots.size() == 3);
		const auto& mid = root.sub.front().slots[1].node;
		CHECK(mid.k == pattern_node::kind::WILDCARD);
		CHECK(root.sub.front().slots[1].q == quantifier::STAR);
	}

	TEST_CASE("% > rule - wildcard with direct edge") {
		auto r = compile_ok("% > rule");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		CHECK(root.k == pattern_node::kind::WILDCARD);
		CHECK(root.edge == edge_type::DIRECT);
	}

	TEST_CASE("row > cell ',' cell - edge slurps the whole tail") {
		auto r = compile_ok("row > cell ',' cell");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		// Edge-slurp: the entire tail belongs to row's child seq, so
		// the top sequence holds exactly one slot.
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		REQUIRE(root.edge == edge_type::DIRECT);
		CHECK(root.sub.front().slots.size() == 3);
	}

	TEST_CASE("(row > cell) ',' cell - parens scope the edge") {
		auto r = compile_ok("(row > cell) ',' cell");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		// Parenthesised - three top-level siblings, one capture group.
		CHECK(cp.top.slots.size() == 3);
		CHECK(cp.num_captures == 1);
	}

	TEST_CASE("(digit)+ - quantified capture") {
		auto r = compile_ok("(digit)+");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		CHECK(cp.top.slots[0].q == quantifier::PLUS);
		const auto& root = cp.top.slots[0].node;
		CHECK(root.k == pattern_node::kind::CAPTURE);
	}

	TEST_CASE("'\\'' - escaped quote char literal decodes") {
		auto r = compile_ok("'\\''");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		CHECK(root.k == pattern_node::kind::TERMINAL);
		CHECK(root.text == "'");
	}

	TEST_CASE("\"a\\\"b\" - escaped quote in string literal decodes") {
		auto r = compile_ok("\"a\\\"b\"");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		CHECK(root.k == pattern_node::kind::TERMINAL);
		CHECK(root.text == "a\"b");
	}

	TEST_CASE("'\"' - a raw double quote inside a char literal decodes") {
		auto r = compile_ok("'\"'");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		CHECK(root.k == pattern_node::kind::TERMINAL);
		CHECK(root.text == "\"");
	}

	TEST_CASE("\"'\" - a raw single quote inside a string literal decodes") {
		auto r = compile_ok("\"'\"");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		CHECK(root.k == pattern_node::kind::TERMINAL);
		CHECK(root.text == "'");
	}

	TEST_CASE("\"caf\xC3\xA9\" - a non-ASCII UTF-8 character in a string "
		"literal decodes") {
		// requires terminal_type = char32_t with a UTF-8 codec; a
		// terminal_type of char sees the raw 0xC3/0xA9 bytes as
		// unprintable and this pattern fails to compile.
		auto r = compile_ok("\"caf\xC3\xA9\"");
		REQUIRE(r.has_value());
		const auto& cp = *r;
		REQUIRE(cp.top.slots.size() == 1);
		const auto& root = cp.top.slots[0].node;
		CHECK(root.k == pattern_node::kind::TERMINAL);
		CHECK(root.text == "caf\xC3\xA9");
	}
}

TEST_SUITE("treemr::compile - rejection") {

	// Every row but the last is a DSL string that must fail to compile
	// with a parse_error diagnostic. The last row is the accepting
	// counter-example for the M11 empty-match rejections below it.

	struct compile_row {
		const char* label;
		const char* pattern;
		bool        expect_ok;
	};

	static const compile_row compile_rows[] = {
		{ ">> - edge without atom", ">>", false },
		{ "rule > - edge without inner seq", "rule >", false },
		{ "( - unclosed capture", "(", false },
		{ "() - empty alternation", "()", false },
		{ "rule >   - trailing whitespace after edge", "rule >   ",
			false },
		{ "'abc - unterminated char literal", "'abc", false },
		{ "'\\uD800' - undecodable escape errors, does not throw",
			"'\\uD800'", false },
		{ "x > b (^ a) - `^` inside a non-first group errors",
			"x > b (^ a)", false },
		// The grammar accepts a leaf atom with a child edge, so the
		// compiler can reject it with a clear message; a bad parse
		// can never reach this check.
		{ "a! > b - a leaf atom cannot carry a child edge",
			"a! > b", false },
		// ---- M19/M20. anchor placement inside a group.
		{ "(^) - an anchor-only group is a compile error",
			"(^)", false },
		{ "($) - an anchor-only group is a compile error",
			"($)", false },
		{ "(^ a | b) - `^` on only some alternatives errors",
			"(^ a | b)", false },
		{ "(a | b $) - `$` on only some alternatives errors",
			"(a | b $)", false },
		// ---- M11. a top-level pattern that can match empty is
		// rejected.
		{ "a* - a bare star slot can match empty", "a*", false },
		{ "%? - a bare optional slot can match empty", "%?", false },
		{ "(a*)* - a quantified capture over an empty-capable "
			"alternative can match empty", "(a*)*", false },
		// The leading `digit` slot cannot be empty, so the sequence
		// as a whole cannot match empty even though `%*` can.
		{ "digit %* term - a required slot keeps the pattern valid",
			"digit %* term", true },
	};

	TEST_CASE("compile - accept/reject table") {
		for (const auto& row : compile_rows) {
			SUBCASE(row.label) {
				auto r = treemr::compile(row.pattern);
				if (row.expect_ok) {
					REQUIRE(r.has_value());
				} else {
					CHECK_FALSE(r.has_value());
					CHECK(has_parse_error(r));
				}
			}
		}
	}
}

// ===========================================================================
// A2. parse_node_adapter<C,T> - node text converts to UTF-8 before compare
// ===========================================================================

TEST_SUITE("treemr - parse_node_adapter unit conversion") {

	TEST_CASE("is_nt matches a non-ASCII NT name for C = char32_t") {
		// The pattern text is always UTF-8; the tree's own NT name is a
		// std::u32string here, so is_nt_fn must convert it before the
		// byte-wise compare.
		nonterminals<char32_t> nts;
		size_t idx = nts.get(std::u32string(1, char32_t(0x00E9)));
		lit<char32_t, char32_t> l(idx, &nts);
		pnode_type<char32_t, char32_t> pn{ l, {0, 0} };
		tref node = lcrs_tree<pnode_type<char32_t, char32_t>>::get(pn);

		auto ad = parse_node_adapter<char32_t, char32_t>(nts);
		CHECK(ad.is_nt(node, "\xC3\xA9"));       // UTF-8 for U+00E9
		CHECK_FALSE(ad.is_nt(node, "e"));
		CHECK_FALSE(ad.is_nt(node, "\xC3\xA8")); // UTF-8 for U+00E8
	}
}

// ===========================================================================
// B. Matching against a real parse tree
// ===========================================================================

TEST_SUITE("treemr - matching") {

	using matcher_t = matcher<pnode_type<char, char>>;

	// Build a matcher from a DSL pattern against the parsed tree's nts.
	// The matcher captures `nts` by reference, so `tree_fx` must outlive
	// every call made against the returned matcher.
	static matcher_t build(const parsed& tree_fx, const char* pattern) {
		auto m = matcher_for<char, char>(
			tree_fx.nts, string_view{ pattern });
		REQUIRE(m.has_value());
		return std::move(m).value();
	}

	// ---- B1-B3, B5-B7, B9. one pattern against one parsed tree, ------
	// ---- expect a boolean search() and, optionally, a search_all() count

	struct match_row {
		const char* label;
		const char* tgf;
		const char* input;
		const char* pattern;
		bool        expect;
		long        expect_count = -1; // search_all().size(), -1 = don't check
	};

	static const match_row match_rows[] = {
		{ "B1: bare NT - row matches in '12,34'",
			CSV_TGF, "12,34", "row", true },
		{ "B1: bare NT - cell matches (multiple)",
			CSV_TGF, "12,34", "cell", true, 2 },
		{ "B1: nonexistent NT - no match",
			CSV_TGF, "12,34", "definitely_not_a_real_nt", false },

		{ "B2: row > cell ',' cell - edge-slurp matches '1,2'",
			CSV_TGF, "1,2", "row > cell ',' cell", true },
		{ "B2: row > cell ',' cell - no match on single-cell row",
			CSV_TGF, "1", "row > cell ',' cell", false },
		// Parens force both `^` and `$` to bind to the inner sib_seq.
		{ "B2: row > (^ cell ',' cell $) - both anchors inside the edge",
			CSV_TGF, "1,2", "row > (^ cell ',' cell $)", true },
		{ "B2: row > (^ cell ',' cell $) - a third cell breaks the anchor",
			CSV_TGF, "1,2,3", "row > (^ cell ',' cell $)", false },
		{ "B2: row > ',' - unanchored seq starts anywhere in the list",
			CSV_TGF, "1,2", "row > ','", true },
		{ "B2: row > ^ atom - left anchor pins to the first child (cell)",
			CSV_TGF, "1,2", "row > ^ cell", true },
		{ "B2: row > ^ atom - left anchor rejects a non-first child (',')",
			CSV_TGF, "1,2", "row > ^ ','", false },
		{ "B2: row > cell cell - slots match consecutive siblings",
			CSV_TGF, "1,2", "row > cell cell", false },
		// Right-anchor counterpart to the `^` case above: no prior test
		// pinned a bare `$` (without a paired `^`) to the last child.
		{ "B2: row > $ atom - right anchor pins to the last child (cell)",
			CSV_TGF, "1,2", "row > cell $", true },
		{ "B2: row > $ atom - right anchor rejects a non-last child (',')",
			CSV_TGF, "1,2", "row > ',' $", false },

		{ "B3: row >> digit - digit lives somewhere below row",
			CSV_TGF, "12,34", "row >> digit", true },
		{ "B3: row >> nonexistent - no match",
			CSV_TGF, "12,34", "row >> nonexistent_nt", false },
		{ "B3: row >> row - the atom's own node is not its descendant",
			CSV_TGF, "1,2", "row >> row", false },
		{ "B3: cell >> (cell ',') - descendant seq stays in the subtree",
			CSV_TGF, "1,2", "cell >> (cell ',')", false },
		{ "B3: csv >> ^ atom - anchors work under the descendant edge (cell)",
			CSV_TGF, "1,2", "csv >> ^ cell", true },
		{ "B3: csv >> ^ atom - anchors work under the descendant edge (',')",
			CSV_TGF, "1,2", "csv >> ^ ','", false },

		{ "B5: cell > digit+ - matches each cell",
			CSV_TGF, "12,34", "cell > digit+", true, 2 },
		{ "B5: cell > digit* - zero-or-more",
			CSV_TGF, "12,34", "cell > digit*", true },
		{ "B5: cell > digit? - one-or-zero",
			CSV_TGF, "12,34", "cell > digit?", true },
		{ "B5: cell > (^ digit+ $) - greedy backtracking succeeds",
			CSV_TGF, "12,34", "cell > (^ digit+ $)", true },
		{ "B5: cell > (^ %* digit $) - star backtracks to zero width",
			CSV_TGF, "5", "cell > (^ %* digit $)", true },

		{ "B6: row > %+ - one-or-more of anything",
			CSV_TGF, "1,2", "row > %+", true },
		{ "B6: cell > %* - zero-or-more of anything",
			CSV_TGF, "12", "cell > %*", true },
		{ "B6: % > digit - any node with a direct digit child",
			CSV_TGF, "1", "% > digit", true },

		{ "B7: row > cell ',' cell matches '1,2'",
			CSV_TGF, "1,2", "row > cell ',' cell", true },
		{ "B7: (row > cell) ',' cell does NOT match '1,2'",
			CSV_TGF, "1,2", "(row > cell) ',' cell", false },

		{ "B9: cell > '5' - terminal match by collected text (match)",
			CSV_TGF, "5", "cell > '5'", true },
		{ "B9: cell > '5' - terminal match by collected text (no match)",
			CSV_TGF, "9", "cell > '5'", false },
	};

	TEST_CASE("B1-B3,B5-B7,B9: pattern vs. tree, boolean match table") {
		for (const auto& row : match_rows) {
			SUBCASE(row.label) {
				parsed fx{ row.tgf, row.input };
				auto m = build(fx, row.pattern);
				CHECK(m.search(fx.root) == row.expect);
				if (row.expect_count >= 0) {
					auto sel = m.search_all(fx.root);
					CHECK((long)sel.size() == row.expect_count);
				}
			}
		}
	}

	// ---- B4. captures -------------------------------------------------

	TEST_CASE("B4: row > (cell) ',' (cell) - two captures, one match") {
		parsed fx{ CSV_TGF, "1,2" };
		auto m = build(fx, "row > (cell) ',' (cell)");
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() == 1);
		const auto& mt = sel[0];
		REQUIRE(mt.captures.size() == 2);
		// m[0] is the row node, m[1]/m[2] are the captured cells.
		CHECK(mt[0] == mt.root);
		CHECK(mt[1] != nullptr);
		CHECK(mt[2] != nullptr);
		// The captured nodes must be `cell` NT nodes.
		auto ad = parse_node_adapter<char, char>(fx.nts);
		CHECK(ad.is_nt(mt[1], "cell"));
		CHECK(ad.is_nt(mt[2], "cell"));
	}

	TEST_CASE("B4: (cell) - N matches against N cells") {
		parsed fx{ CSV_TGF, "1,2,3" };
		auto m = build(fx, "(cell)");
		auto sel = m.search_all(fx.root);
		// Three cells in "1,2,3"; each is captured by its own match.
		CHECK(sel.size() == 3);
		for (const auto& mt : sel) {
			REQUIRE(mt.captures.size() == 1);
			CHECK(mt.captures[0] == mt.root);
		}
	}

	TEST_CASE("B4: (digit | nonexistent_nt) - first alt captures digit") {
		parsed fx{ CSV_TGF, "12" };
		auto m = build(fx, "(digit | nonexistent_nt)");
		auto sel = m.search_all(fx.root);
		// Two digits; each match captures its digit node (first alt wins).
		CHECK(sel.size() == 2);
		for (const auto& mt : sel) {
			REQUIRE(mt.captures.size() == 1);
			CHECK(mt.captures[0] != nullptr);
		}
	}

	TEST_CASE("B4: row > (cell (',')) - nested groups number outer-first") {
		// Groups are numbered by their opening paren (regex
		// convention): capture 1 is the outer group (the cell),
		// capture 2 the inner one (the ',').
		parsed fx{ CSV_TGF, "1,2" };
		auto m = build(fx, "row > (cell (','))");
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() == 1);
		const auto& mt = sel[0];
		REQUIRE(mt.captures.size() == 2);
		auto ad = parse_node_adapter<char, char>(fx.nts);
		CHECK(ad.is_nt(mt[1], "cell"));
		CHECK(ad.is_terminal(mt[2], ","));
	}

	TEST_CASE("B4: outer sequence resumes after a multi-slot capture") {
		parsed fx{ CSV_TGF, "1,2" };
		CHECK(build(fx, "row > (cell ',') cell").search(fx.root));
		CHECK_FALSE(build(fx, "row > (cell ',') ','").search(fx.root));
	}

	TEST_CASE("B4: outer sequence resumes after a multi-slot alternative") {
		parsed fx{ CSV_TGF, "1,2" };
		CHECK(build(fx,
			"row > (nonexistent_nt | cell ',') cell").search(fx.root));
		CHECK_FALSE(build(fx,
			"row > (nonexistent_nt | cell ',') ','").search(fx.root));
	}

	// ---- B8. non-matching captures stay nullptr -----------------------

	TEST_CASE("B8: (digit | nonexistent_nt) - capture is the digit, not null") {
		parsed fx{ CSV_TGF, "5" };
		auto m = build(fx, "(digit | nonexistent_nt)");
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() >= 1);
		const auto& mt = sel[0];
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] != nullptr);
	}

	// ---- B10. zero-width captures under a quantifier don't loop -------

	TEST_CASE("B10: % (a*)* - a zero-width capture under a star matches, "
		"does not overflow the stack")
	{
		// "a" names no NT in CSV_TGF, so the inner `a*` always matches
		// zero-width; the outer `*` must not retry at the same position.
		// The leading `%` keeps the top sequence from matching empty.
		parsed fx{ CSV_TGF, "12,34" };
		auto m = build(fx, "% (a*)*");
		CHECK(m.search(fx.root));
	}

	TEST_CASE("B10: % (a?)+ - a zero-width capture under a plus matches, "
		"does not overflow the stack")
	{
		parsed fx{ CSV_TGF, "12,34" };
		auto m = build(fx, "% (a?)+");
		CHECK(m.search(fx.root));
	}

	TEST_CASE("B10: % (%*)* - a zero-width wildcard capture under a star "
		"matches, does not overflow the stack")
	{
		parsed fx{ CSV_TGF, "12,34" };
		auto m = build(fx, "% (%*)*");
		CHECK(m.search(fx.root));
	}

	// ---- B11. a zero-width capture records nullptr, not its anchor ----

	TEST_CASE("B11: x > (a*) b - zero matches of a* records nullptr") {
		const char* AB_TGF =
			" start => x.    \n"
			" x     => a* b. \n"
			" a     => 'a'.  \n"
			" b     => 'b'.  \n";
		parsed fx{ AB_TGF, "b" };
		auto m = build(fx, "x > (a*) b");
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() == 1);
		const auto& mt = sel[0];
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] == nullptr);
	}

	TEST_CASE("B11: x > (a+) b - capture records the first a") {
		const char* AB_TGF =
			" start => x.    \n"
			" x     => a* b. \n"
			" a     => 'a'.  \n"
			" b     => 'b'.  \n";
		parsed fx{ AB_TGF, "aab" };
		tref x_node = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(x_node != nullptr);
		tref first_a = lcrs_tree<pnode_type<char, char>>::get(
			x_node).first();
		REQUIRE(first_a != nullptr);

		auto m = build(fx, "x > (a+) b");
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() == 1);
		const auto& mt = sel[0];
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] == first_a);
	}

	// ---- M6. a multi-slot capture records only its first node ---------

	TEST_CASE("M6: (cell ',' cell) - a multi-slot capture records "
		"its first node")
	{
		parsed fx{ CSV_TGF, "1,2" };
		tref csv = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(csv != nullptr);
		tref row = lcrs_tree<pnode_type<char, char>>::get(csv).first();
		REQUIRE(row != nullptr);
		tref first_cell = lcrs_tree<pnode_type<char, char>>::get(
			row).first();
		REQUIRE(first_cell != nullptr);

		auto m = build(fx, "(cell ',' cell)");
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() == 1);
		const auto& mt = sel[0];
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] == first_cell);
	}

	// ---- B12. an optional group over a genuinely optional NT ----------
	//
	// G10's a_opt is mandatory in seq's production, so its node always
	// exists and the capture is never null. Here a_opt itself is
	// optional in seq's production, so the group can face a real
	// absence: no a_opt child at all.

	const char* OPT2_TGF =
		" start => seq.        \n"
		" seq   => a_opt? 'b'. \n"
		" a_opt => 'a'.        \n";

	TEST_CASE("B12: seq > (a_opt)? 'b' - the group records nullptr "
		"when a_opt is absent")
	{
		parsed fx{ OPT2_TGF, "b" };
		auto m = build(fx, "seq > (a_opt)? 'b'");
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() == 1);
		const auto& mt = sel[0];
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] == nullptr);
	}

	TEST_CASE("B12: seq > (a_opt)? 'b' - the group records a_opt "
		"when it is present")
	{
		parsed fx{ OPT2_TGF, "ab" };
		tref seq = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(seq != nullptr);
		tref a_opt_node = lcrs_tree<pnode_type<char, char>>::get(
			seq).first();
		REQUIRE(a_opt_node != nullptr);

		auto m = build(fx, "seq > (a_opt)? 'b'");
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() == 1);
		const auto& mt = sel[0];
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] == a_opt_node);
	}
}

// ===========================================================================
// C. match vs search_all
// ===========================================================================

TEST_SUITE("treemr - match vs search_all") {

	TEST_CASE("C1: match true at root, false elsewhere") {
		parsed fx{ CSV_TGF, "1,2" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "row" });
		REQUIRE(m.has_value());
		auto& matcher = *m;

		// `row` is below `csv` (which is below `start`), so the root
		// itself is not a `row` node and match should fail.
		CHECK_FALSE(matcher.match(fx.root));
		// Free search must still find it.
		CHECK(matcher.search(fx.root));
	}

	TEST_CASE("C2: match(m&) on a non-matching root leaves m empty") {
		parsed fx{ CSV_TGF, "1,2" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "nonexistent_nt" });
		REQUIRE(m.has_value());
		match_result mt;
		(*m).match(fx.root, mt);
		CHECK(mt.root == nullptr);
		CHECK_FALSE(static_cast<bool>(mt));
	}

	TEST_CASE("C3: search_all returns multiple overlapping candidates") {
		parsed fx{ CSV_TGF, "1,2,3" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "cell" });
		REQUIRE(m.has_value());
		auto sel = (*m).search_all(fx.root);
		// Three cells produce three matches.
		CHECK(sel.size() == 3);
	}
}

// ===========================================================================
// D. Error-path semantics (no exceptions)
// ===========================================================================

TEST_SUITE("treemr - error path") {

	TEST_CASE("D1: matcher_for with bad DSL returns error result") {
		nonterminals<char> nts;
		auto m = matcher_for<char, char>(
			nts, string_view{ "bad >>" });
		CHECK_FALSE(m.has_value());
		// Bad DSL must surface a parse_error node in the report.
		bool saw_parse_error = false;
		for (const auto& n : m.report().nodes())
			if (n.tag == diagnostics::code::parse_error)
				saw_parse_error = true;
		CHECK(saw_parse_error);
	}

	// D2 (no catch blocks in test code) is enforced by inspection of this
	// file - none of the cases above use try/catch around matcher calls.
}

// ===========================================================================
// E. Known v1 limitations
// ===========================================================================

TEST_SUITE("treemr - v1 limitations (locked-in current behaviour)") {

	TEST_CASE("E2: (row > cell)+ - quantified capture wrapping an edge") {
		// A quantified capture wrapping an edge compiles and matches;
		// each match keeps exactly one capture.
		parsed fx{ CSV_TGF, "1,2" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "(row > cell)+" });
		REQUIRE(m.has_value());
		auto sel = (*m).search_all(fx.root);
		for (const auto& mt : sel) {
			REQUIRE(mt.captures.size() == 1);
		}
	}
}

// ===========================================================================
// F. ambig_mode - __AMB__ transparency
// ===========================================================================
//
// AMBIG_TGF derives 'x' two ways under A. With auto_disambiguate off the
// shaped tree keeps both: start > __AMB__ > [A > B > x, A > C > x].

struct ambig_parsed {
	nonterminals<char>     nts;
	grammar<char>          g;
	parser<char>::result   r;
	tref                   root = nullptr;

	ambig_parsed(const char* tgf_src, const char* input)
		: nts{}
		, g{ [&]() {
			auto gr = tgf<char>::from_string(nts, string(tgf_src));
			REQUIRE(gr.has_value());
			auto built = std::move(gr).value();
			built.opt.auto_disambiguate = false;
			return built;
		}() }
		, r{ [&]() {
			parser<char> p(g);
			auto res = p.parse(input, strlen(input),
				{ .enable_gc = false, .gc_lag = 1 });
			REQUIRE(res.found);
			return res;
		}() }
	{
		root = r.get_shaped_tree2();
		REQUIRE(root != nullptr);
		REQUIRE(r.is_ambiguous());
	}
};

const char* AMBIG_TGF =
	" start => A. \n"
	" A     => B | C. \n"
	" B     => 'x'. \n"
	" C     => 'x'. \n";

// Return true iff some node anywhere under `n` is `__AMB__` (via adapter).
bool has_amb_anywhere(tref n, const nonterminals<char>& nts) {
	auto ad = parse_node_adapter<char, char>(nts);
	bool found = false;
	auto visit = [&](tref d) -> bool {
		if (found) return false;
		if (ad.is_nt(d, "__AMB__")) { found = true; return false; }
		return true;
	};
	pre_order<pnode_type<char, char>>(n).search(visit);
	return found;
}

TEST_SUITE("treemr - ambig_mode") {

	TEST_CASE("F1: __AMB__ is present in the parse forest (sanity)") {
		// Every other F-test rests on this fixture producing a real
		// ambiguous parse forest.
		ambig_parsed fx{ AMBIG_TGF, "x" };
		CHECK(has_amb_anywhere(fx.root, fx.nts));
	}

	// F2-F7, F9, F10 all run one pattern against the same __AMB__
	// fixture in one ambig_mode, either through search() or match().
	// `also_default` also checks the no-mode-arg overload behaves like
	// FORBID, per the documented default.

	struct amb_row {
		const char* label;
		const char* pattern;
		ambig_mode  mode;
		bool        expect;
		bool        use_root     = false;
		bool        also_default = false;
	};

	static const amb_row amb_rows[] = {
		// Both `A` nodes sit directly under __AMB__, so the only way
		// to try "A > %" against either is to fork through the
		// wrapper, which FORBID never does. Default arg exercises
		// the same path.
		{ "F2: FORBID rejects a pattern that can only match by "
			"crossing __AMB__",
			"A > %", ambig_mode::FORBID, false, false, true },

		// `A > %` matches an `A` whose first child is anything; under
		// __AMB__ both B-alt and C-alt are `A`-wrappers with a single
		// child, so ANY succeeds on the first one tried.
		{ "F3: ANY succeeds when at least one alternative matches (A > %)",
			"A > %", ambig_mode::ANY, true },
		// `A > B` exercises one-alt-wins-the-other-doesn't: only the B
		// alternative has B as its child, but ANY only requires some
		// alternative to match.
		{ "F3: ANY succeeds when at least one alternative matches (A > B)",
			"A > B", ambig_mode::ANY, true },

		// `A > B` matches the B-alt but not the C-alt; ALL must reject.
		{ "F4: ALL fails when only one alternative satisfies the pattern",
			"A > B", ambig_mode::ALL, false },

		// `A > %` is satisfied by every alt-A wrapper (each has a single
		// child - either B or C - and `%` matches either).
		{ "F5: ALL succeeds when every alternative matches",
			"A > %", ambig_mode::ALL, true },

		// Belt-and-braces check that match's default argument
		// matches the contract documented on the public API.
		{ "F6: match with default arg is FORBID-mode (rejects)",
			"start > A", ambig_mode::FORBID, false, true, true },
		{ "F6: match with default arg is FORBID-mode (ANY accepts)",
			"start > A", ambig_mode::ANY, true, true },

		// Tree: start > __AMB__ > [A > B > x, A > C > x]. `B` is a
		// direct child of __AMB__; ANY finds it as a match root.
		{ "F7: ANY finds a match root strictly below an alternative",
			"B", ambig_mode::ANY, true },

		{ "F9: descendant edge crosses __AMB__ under ANY and ALL (ANY)",
			"start >> B", ambig_mode::ANY, true },
		{ "F9: descendant edge crosses __AMB__ under ANY and ALL (ALL)",
			"start >> B", ambig_mode::ALL, true },

		// A direct edge must not turn into a descendant edge that
		// reaches into the other alternative under ALL.
		{ "F10: ALL still fails when only one alternative satisfies "
			"a direct edge",
			"A > B", ambig_mode::ALL, false },
	};

	TEST_CASE("F2-F7,F9,F10: ambig_mode search/match table") {
		ambig_parsed fx{ AMBIG_TGF, "x" };
		for (const auto& row : amb_rows) {
			SUBCASE(row.label) {
				auto m = matcher_for<char, char>(
					fx.nts, string_view{ row.pattern });
				REQUIRE(m.has_value());
				if (row.use_root) {
					if (row.also_default)
						CHECK((*m).match(fx.root) == row.expect);
					CHECK((*m).match(fx.root, row.mode)
						== row.expect);
				} else {
					if (row.also_default)
						CHECK((*m).search(fx.root) == row.expect);
					CHECK((*m).search(fx.root, row.mode) == row.expect);
				}
			}
		}
	}

	TEST_CASE("F8: search_all finds exactly one match root below an alternative") {
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "B" });
		REQUIRE(m.has_value());
		auto sel = (*m).search_all(fx.root, ambig_mode::ANY);
		CHECK(sel.size() == 1);
	}

	TEST_CASE("F11: FORBID rejects a match attempted directly at an "
		"__AMB__ node, since that requires crossing it")
	{
		ambig_parsed fx{ AMBIG_TGF, "x" };
		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(amb != nullptr);
		auto ad = parse_node_adapter<char, char>(fx.nts);
		REQUIRE(ad.is_nt(amb, "__AMB__"));

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "%" });
		REQUIRE(m.has_value());
		CHECK_FALSE((*m).match(amb, ambig_mode::FORBID));
		CHECK((*m).match(amb, ambig_mode::ANY));
	}

	TEST_CASE("F12: FORBID matches inside a branch that never reaches "
		"the tree's __AMB__ node")
	{
		// fx.root: start > __AMB__ > [A > B > x, A > C > x]. `B`'s
		// own subtree never touches __AMB__, so FORBID finds it.
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "B" });
		REQUIRE(m.has_value());
		CHECK((*m).search(fx.root, ambig_mode::FORBID));
		auto sel = (*m).search_all(fx.root, ambig_mode::FORBID);
		CHECK(sel.size() == 1);
	}

	// ---- F13-F15. FORBID scans the whole match-root subtree -----------

	TEST_CASE("F13: FORBID refuses a plain match at the tree's own root, "
		"since __AMB__ lives in its subtree, although the pattern "
		"does not descend into it")
	{
		// fx.root: start > __AMB__ > [A > B > x, A > C > x]. The
		// pattern "start" matches start itself without descending,
		// yet its own subtree holds __AMB__.
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "start" });
		REQUIRE(m.has_value());
		CHECK_FALSE((*m).match(fx.root, ambig_mode::FORBID));
		CHECK((*m).match(fx.root, ambig_mode::ANY));
	}

	TEST_CASE("F14: FORBID allows the same plain match at the tree's own "
		"root when no __AMB__ node exists anywhere in the tree")
	{
		const char* CHAIN_TGF =
			" start => A.   \n"
			" A     => B.   \n"
			" B     => 'x'. \n";
		parsed fx{ CHAIN_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "start" });
		REQUIRE(m.has_value());
		CHECK((*m).match(fx.root, ambig_mode::FORBID));
	}

	TEST_CASE("F15: FORBID matches in a branch that holds no __AMB__, "
		"although a sibling branch under the same root does, since "
		"the scan is rooted at the candidate, not at the whole tree")
	{
		const char* AMB_SIBLING_TGF =
			" start => Amb Clean. \n"
			" Amb   => B | C.     \n"
			" B     => 'x'.       \n"
			" C     => 'x'.       \n"
			" Clean => 'y'.       \n";
		ambig_parsed fx{ AMB_SIBLING_TGF, "xy" };
		auto ad = parse_node_adapter<char, char>(fx.nts);

		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(amb != nullptr);
		REQUIRE(ad.is_nt(amb, "__AMB__"));
		tref clean = lcrs_tree<pnode_type<char, char>>::get(
			amb).right_sibling();
		REQUIRE(clean != nullptr);
		REQUIRE(ad.is_nt(clean, "Clean"));

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "Clean" });
		REQUIRE(m.has_value());
		CHECK((*m).search(fx.root, ambig_mode::FORBID));
		auto sel = (*m).search_all(fx.root, ambig_mode::FORBID);
		REQUIRE(sel.size() == 1);
		CHECK(sel[0].root == clean);
	}

	// ---- U1-U4. UNIQUE mode -------------------------------------------

	struct unique_row {
		const char* label;
		const char* pattern;
		bool        expect;
	};

	static const unique_row unique_rows[] = {
		// Only the B-alternative satisfies "A > B"; exactly one hit.
		{ "U1: UNIQUE matches when exactly one alternative satisfies "
			"the pattern (A > B)", "A > B", true },
		{ "U1: UNIQUE matches when exactly one alternative satisfies "
			"the pattern (A > C)", "A > C", true },
		// Both alternatives satisfy "A > %"; not unique.
		{ "U2: UNIQUE does not match when every alternative satisfies "
			"the pattern", "A > %", false },
		// Neither alternative has a "D" child.
		{ "U3: UNIQUE does not match when no alternative satisfies "
			"the pattern", "A > D", false },
	};

	TEST_CASE("U1-U3: UNIQUE mode match table") {
		ambig_parsed fx{ AMBIG_TGF, "x" };
		for (const auto& row : unique_rows) {
			SUBCASE(row.label) {
				auto m = matcher_for<char, char>(
					fx.nts, string_view{ row.pattern });
				REQUIRE(m.has_value());
				CHECK((*m).search(fx.root, ambig_mode::UNIQUE)
					== row.expect);
			}
		}
	}

	TEST_CASE("U4: UNIQUE matches an ordinary pattern when no ambiguity "
		"is in play")
	{
		const char* CHAIN_TGF =
			" start => A.   \n"
			" A     => B.   \n"
			" B     => 'x'. \n";
		parsed fx{ CHAIN_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "start > A" });
		REQUIRE(m.has_value());
		CHECK((*m).match(fx.root, ambig_mode::UNIQUE));
	}

	// ---- M25. ALL merges a capture across every alternative -----------

	TEST_CASE("M25: A > (%) under ALL synthesizes an __AMB__ node holding "
		"both alternatives' captures")
	{
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto ad = parse_node_adapter<char, char>(fx.nts);
		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(amb != nullptr);
		REQUIRE(ad.is_nt(amb, "__AMB__"));

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "A > (%)" });
		REQUIRE(m.has_value());
		match_result mt;
		(*m).match(amb, mt, ambig_mode::ALL);
		REQUIRE((bool)mt);
		REQUIRE(mt.captures.size() == 1);
		tref cap = mt.captures[0];
		REQUIRE(cap != nullptr);
		CHECK(ad.is_nt(cap, "__AMB__"));

		tref c1 = lcrs_tree<pnode_type<char, char>>::get(cap).first();
		REQUIRE(c1 != nullptr);
		tref c2 = lcrs_tree<pnode_type<char, char>>::get(
			c1).right_sibling();
		REQUIRE(c2 != nullptr);
		// Exactly two children, one B and one C, order not guaranteed.
		CHECK(lcrs_tree<pnode_type<char, char>>::get(
			c2).right_sibling() == nullptr);
		bool c1_is_b = ad.is_nt(c1, "B"), c1_is_c = ad.is_nt(c1, "C");
		bool c2_is_b = ad.is_nt(c2, "B"), c2_is_c = ad.is_nt(c2, "C");
		CHECK(((c1_is_b && c2_is_c) || (c1_is_c && c2_is_b)));
	}

	TEST_CASE("M25: A > % > (%) under ALL keeps a capture that is the "
		"same node in every alternative, without synthesizing a node")
	{
		// Both alternatives derive 'x' from the same input span, so
		// the parser interns one shared leaf node for it.
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto ad = parse_node_adapter<char, char>(fx.nts);
		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(amb != nullptr);
		tref alt1 = lcrs_tree<pnode_type<char, char>>::get(
			amb).first();
		REQUIRE(alt1 != nullptr);
		tref alt2 = lcrs_tree<pnode_type<char, char>>::get(
			alt1).right_sibling();
		REQUIRE(alt2 != nullptr);
		tref leaf1 = lcrs_tree<pnode_type<char, char>>::get(
			lcrs_tree<pnode_type<char, char>>::get(
				alt1).first()).first();
		tref leaf2 = lcrs_tree<pnode_type<char, char>>::get(
			lcrs_tree<pnode_type<char, char>>::get(
				alt2).first()).first();
		REQUIRE(leaf1 != nullptr);
		REQUIRE(leaf1 == leaf2);

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "A > % > (%)" });
		REQUIRE(m.has_value());
		match_result mt;
		(*m).match(amb, mt, ambig_mode::ALL);
		REQUIRE((bool)mt);
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] == leaf1);
		CHECK_FALSE(ad.is_nt(mt.captures[0], "__AMB__"));
	}

	TEST_CASE("M25: A > (nonexistent_nt)? % under ALL keeps a capture "
		"that is nullptr in every alternative")
	{
		ambig_parsed fx{ AMBIG_TGF, "x" };
		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(amb != nullptr);

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "A > (nonexistent_nt)? %" });
		REQUIRE(m.has_value());
		match_result mt;
		(*m).match(amb, mt, ambig_mode::ALL);
		REQUIRE((bool)mt);
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] == nullptr);
	}

	TEST_CASE("M25: A > (%) under ANY still reports a single "
		"alternative's capture, never a synthesized node")
	{
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto ad = parse_node_adapter<char, char>(fx.nts);
		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(amb != nullptr);

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "A > (%)" });
		REQUIRE(m.has_value());
		match_result mt;
		(*m).match(amb, mt, ambig_mode::ANY);
		REQUIRE((bool)mt);
		REQUIRE(mt.captures.size() == 1);
		tref cap = mt.captures[0];
		REQUIRE(cap != nullptr);
		CHECK_FALSE(ad.is_nt(cap, "__AMB__"));
		CHECK((ad.is_nt(cap, "B") || ad.is_nt(cap, "C")));
	}

	TEST_CASE("M25: without make_node_fn, ALL falls back to the first "
		"alternative's capture instead of synthesizing a node")
	{
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto base = parse_node_adapter<char, char>(fx.nts);
		node_adapter no_make;
		no_make.is_nt_fn = base.is_nt_fn;
		no_make.is_terminal_fn = base.is_terminal_fn;
		// make_node_fn left empty on purpose.

		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(amb != nullptr);

		auto pat = treemr::compile("A > (%)");
		REQUIRE(pat.has_value());
		matcher<pnode_type<char, char>> m(*pat, no_make);

		match_result mt;
		m.match(amb, mt, ambig_mode::ALL);
		REQUIRE((bool)mt);
		REQUIRE(mt.captures.size() == 1);
		tref cap = mt.captures[0];
		REQUIRE(cap != nullptr);
		CHECK_FALSE(base.is_nt(cap, "__AMB__"));
		CHECK((base.is_nt(cap, "B") || base.is_nt(cap, "C")));
	}
}

// ===========================================================================
// G. replace - tree rewriting via callback
// ===========================================================================

const char* REPLACE_TGF =
	" @use char class digit. \n"
	" start => list. \n"
	" list  => expr (',' expr)*. \n"
	" expr  => add | sub | int. \n"
	" add   => expr '+' expr. \n"
	" sub   => expr '-' expr. \n"
	" int   => digit+. \n";

// Helper: create a pnode for an NT, using the nts from a parsed fixture.
// The location is {0,0} (dummy) - suitable for building replacement trees
// where only the structural NT identity matters for matching.
pnode_type<char, char> make_nt_pnode(
	nonterminals<char>& nts, const char* name)
{
	size_t idx = nts.get(std::string(name));
	lit<char, char> l(idx, &nts);
	return { l, {0, 0} };
}

// Helper: create a pnode for a terminal char.
pnode_type<char, char> make_t_pnode(char ch)
{
	lit<char, char> l(ch);
	return { l, {0, 0} };
}

// Build a terminal leaf node.
tref make_t_leaf(char ch)
{
	return lcrs_tree<pnode_type<char, char>>::get(make_t_pnode(ch));
}

// Build an NT node with children.
tref make_nt_node(nonterminals<char>& nts,
	const char* name, const trefs& children)
{
	return lcrs_tree<pnode_type<char, char>>::get(
		make_nt_pnode(nts, name), children);
}

// Count how many nodes under `n` match a given NT name.
size_t count_nt(tref n, const char* name,
	const nonterminals<char>& nts)
{
	auto ad = parse_node_adapter<char, char>(nts);
	size_t count = 0;
	auto visit = [&](tref d) -> bool {
		if (ad.is_nt(d, name)) ++count;
		return true;
	};
	pre_order<pnode_type<char, char>>(n).search(visit);
	return count;
}

TEST_SUITE("treemr - replace") {

	using matcher_r = matcher<pnode_type<char, char>>;

	// ---- G1. swap children (add -> sub) ----------------------------

	TEST_CASE("G1: swap - add > (expr) '+' (expr) -> sub(expr2, '-', expr1)") {
		parsed fx{ REPLACE_TGF, "1+2" };
		auto ad = parse_node_adapter<char, char>(fx.nts);

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "add > (expr) '+' (expr)" });
		REQUIRE(m.has_value());

		// Sanity: the pattern matches the tree
		CHECK((*m).search(fx.root));

		// fn: build sub(expr2, '-', expr1)
		auto fn = [&](const match_result& mt) -> tref {
			REQUIRE(mt.captures.size() >= 2);
			tref e1 = mt.captures[0];
			tref e2 = mt.captures[1];
			tref minus = make_t_leaf('-');
			return make_nt_node(fx.nts, "sub", { e2, minus, e1 });
		};

		tref result = (*m).replace(fx.root, fn);
		REQUIRE(result != nullptr);
		REQUIRE(result != fx.root);

		// Traverse: start > list > expr > sub
		CHECK(ad.is_nt(result, "start"));
		tref list = lcrs_tree<pnode_type<char, char>>::get(
			result).first();
		REQUIRE(list != nullptr);
		CHECK(ad.is_nt(list, "list"));
		tref expr = lcrs_tree<pnode_type<char, char>>::get(
			list).first();
		REQUIRE(expr != nullptr);
		CHECK(ad.is_nt(expr, "expr"));
		tref sub = lcrs_tree<pnode_type<char, char>>::get(
			expr).first();
		REQUIRE(sub != nullptr);
		CHECK(ad.is_nt(sub, "sub"));

		// sub's children: expr(from "2"), '-', expr(from "1")
		const auto& sub_tr = lcrs_tree<pnode_type<char, char>>::get(
			sub);
		auto ch = sub_tr.children();
		REQUIRE(std::distance(ch.begin(), ch.end()) == 3);
		trefs sub_children(ch.begin(), ch.end());
		// Captured nodes are exprs wrapping ints
		CHECK(ad.is_nt(sub_children[0], "expr"));
		CHECK(ad.is_terminal(sub_children[1], "-"));
		CHECK(ad.is_nt(sub_children[2], "expr"));
	}

	// ---- G2. no match - pointer identity ---------------------------

	TEST_CASE("G2: no-match returns root unchanged (pointer-equal)") {
		parsed fx{ REPLACE_TGF, "1+2" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "nonexistent_nt" });
		REQUIRE(m.has_value());

		bool called = false;
		auto fn = [&](const match_result&) -> tref {
			called = true;
			return nullptr;
		};
		tref result = (*m).replace(fx.root, fn);
		CHECK_FALSE(called);
		CHECK(result == fx.root);
	}

	// ---- G3. fn returns the matched node - no-op -------------------

	TEST_CASE("G3: fn returning the matched node is a no-op") {
		parsed fx{ REPLACE_TGF, "1+2" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "add > (expr) '+' (expr)" });
		REQUIRE(m.has_value());

		bool called = false;
		auto fn = [&](const match_result& mt) -> tref {
			called = true;
			return mt.root;
		};
		tref result = (*m).replace(fx.root, fn);
		CHECK(called);
		// The matched add subtree is unchanged, so the overall root
		// should be structurally equal after the no-op pass.
		CHECK(lcrs_tree<pnode_type<char, char>>::subtree_equals(
			result, fx.root));
	}

	// ---- G4. root itself is the match ------------------------------

	TEST_CASE("G4: root-matches - entire tree is replaced") {
		parsed fx{ REPLACE_TGF, "1" };
		auto ad = parse_node_adapter<char, char>(fx.nts);

		// Pattern matches `list > expr` at root. No captures.
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "list > expr" });
		REQUIRE(m.has_value());

		// Sanity: pattern matches
		CHECK((*m).search(fx.root));

		auto fn = [&](const match_result& mt) -> tref {
			(void)mt; // no captures in this pattern
			// Build a new list wrapping an add(int, '+', int)
			tref plus = make_t_leaf('+');
			tref one = make_nt_node(fx.nts, "int", {});
			tref inner = make_nt_node(fx.nts, "add", { one, plus, one });
			return make_nt_node(fx.nts, "list", { inner });
		};

		tref result = (*m).replace(fx.root, fn);
		REQUIRE(result != nullptr);
		REQUIRE(result != fx.root);

		// Verify: result is `start > list > add > int '+' int`
		CHECK(ad.is_nt(result, "start"));
		tref list = lcrs_tree<pnode_type<char, char>>::get(
			result).first();
		REQUIRE(list != nullptr);
		CHECK(ad.is_nt(list, "list"));
		tref c0 = lcrs_tree<pnode_type<char, char>>::get(list).first();
		REQUIRE(c0 != nullptr);
		CHECK(ad.is_nt(c0, "add"));
	}

	// ---- G5, G6. add->sub swap over disjoint and nested matches ----
	// Same pattern, same fn, run post-order; only the input tree shape
	// (disjoint pair vs. nested pair) differs between the two rows.

	struct swap_row {
		const char* label;
		const char* input;
		size_t      adds_before;
		int         expect_call_count;
		size_t      adds_after;
		size_t      subs_after;
	};

	static const swap_row swap_rows[] = {
		{ "G5: multiple disjoint matches in one pass",
			"1+2,3+4", 2, 2, 0, 2 },
		// "1+2+3" has two nested add nodes. Post-order ensures both
		// fire in a single replace() call.
		{ "G6: nested - both adds fire in one pass",
			"1+2+3", 2, 2, 0, 2 },
	};

	TEST_CASE("G5,G6: add > (expr) '+' (expr) -> sub(expr2,'-',expr1)") {
		for (const auto& row : swap_rows) {
			SUBCASE(row.label) {
				parsed fx{ REPLACE_TGF, row.input };
				REQUIRE(count_nt(fx.root, "add", fx.nts)
					== row.adds_before);

				auto m = matcher_for<char, char>(
					fx.nts,
					string_view{ "add > (expr) '+' (expr)" });
				REQUIRE(m.has_value());
				CHECK((*m).search(fx.root));

				int call_count = 0;
				auto fn = [&](const match_result& mt) -> tref {
					++call_count;
					REQUIRE(mt.captures.size() >= 2);
					tref e1 = mt.captures[0];
					tref e2 = mt.captures[1];
					tref minus = make_t_leaf('-');
					return make_nt_node(fx.nts, "sub",
						{ e2, minus, e1 });
				};

				tref result = (*m).replace(fx.root, fn);
				REQUIRE(result != nullptr);
				CHECK(call_count == row.expect_call_count);
				CHECK(count_nt(result, "add", fx.nts)
					== row.adds_after);
				CHECK(count_nt(result, "sub", fx.nts)
					== row.subs_after);
			}
		}
	}

	// ---- G7. replace_fixpoint - convergence -----------------------

	TEST_CASE("G7: replace_fixpoint runs until stable") {
		// Convert all adds to subs. After one pass, no adds remain
		// so the second pass finds nothing -> fixpoint.
		parsed fx{ REPLACE_TGF, "1+2,3+4" };
		size_t adds_before = count_nt(fx.root, "add", fx.nts);
		REQUIRE(adds_before == 2);

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "add > (expr) '+' (expr)" });
		REQUIRE(m.has_value());

		// Sanity: pattern matches
		CHECK((*m).search(fx.root));

		int call_count = 0;
		auto fn = [&](const match_result& mt) -> tref {
			++call_count;
			REQUIRE(mt.captures.size() >= 2);
			tref e1 = mt.captures[0];
			tref e2 = mt.captures[1];
			tref minus = make_t_leaf('-');
			return make_nt_node(fx.nts, "sub", { e2, minus, e1 });
		};

		auto fp = (*m).replace_fixpoint(fx.root, fn);
		REQUIRE(fp.has_value());
		tref result = fp.value();
		// First pass replaces 2 adds. Second pass finds 0 adds
		// and returns the same tree -> fixpoint stops.
		CHECK(call_count == 2);

		// All adds gone
		size_t adds_after = count_nt(result, "add", fx.nts);
		CHECK(adds_after == 0);

		// Second fixpoint call on result: returns same tree
		auto fp2 = (*m).replace_fixpoint(result, fn);
		REQUIRE(fp2.has_value());
		CHECK(fp2.value() == result);
	}

	// ---- G8. max_iters bound -----------------------------------------

	TEST_CASE("G8: max_iters reached before a fixed point is an error") {
		// fn wraps the captured int in a fresh expr each time. The
		// first pass builds a genuinely new (synthetic) node; the
		// second pass re-synthesizes that same node and reaches a
		// real fixed point, so this rule needs two passes to settle.
		// max_iters=1 is not enough.
		parsed fx{ REPLACE_TGF, "42" };

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "expr > (int)" });
		REQUIRE(m.has_value());

		// Sanity: pattern matches
		CHECK((*m).search(fx.root));

		int call_count = 0;
		auto fn = [&](const match_result& mt) -> tref {
			++call_count;
			REQUIRE(mt.captures.size() >= 1);
			tref i = mt.captures[0];
			if (!i) return mt.root;
			// Wrap the int in a new expr node; a second pass over
			// this exact shape re-synthesizes the identical node.
			return make_nt_node(fx.nts, "expr", { i });
		};

		// Use ANY mode so FORBID doesn't skip.
		auto fp = (*m).replace_fixpoint(fx.root, fn,
			ambig_mode::ANY, 1);
		CHECK(call_count == 1);
		CHECK_FALSE(fp.has_value());
		bool saw_out_of_range = false;
		for (const auto& n : fp.report().nodes())
			if (n.tag == diagnostics::code::out_of_range)
				saw_out_of_range = true;
		CHECK(saw_out_of_range);

		// With a high enough max_iters (or 0, unbounded), the same
		// rule reaches its real fixed point after two passes.
		call_count = 0;
		auto fp2 = (*m).replace_fixpoint(fx.root, fn, ambig_mode::ANY);
		CHECK(call_count == 2);
		REQUIRE(fp2.has_value());
		CHECK(fp2.value() != nullptr);
	}

	// ---- G9. ambiguity modes on replace ---------------------------

	TEST_CASE("G9: replace skips a node whose parent is __AMB__, in "
		"every mode") {
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "A > B" });
		REQUIRE(m.has_value());

		// FORBID: replace tries "A > B" at every node whose own
		// parent is not __AMB__. Both A nodes sit directly under
		// __AMB__, so neither is tried; fn never runs.
		int uniq_calls = 0;
		auto fn_uniq = [&](const match_result& mt) -> tref {
			++uniq_calls;
			return mt.root;
		};
		tref r_uniq = (*m).replace(fx.root, fn_uniq,
			ambig_mode::FORBID);
		CHECK(uniq_calls == 0);
		CHECK(r_uniq == fx.root); // fn returned the matched node (no-op)

		// ANY: the same skip applies to the A nodes, but not to
		// __AMB__ itself, whose own parent is start. It matches by
		// forking into the B-alternative.
		int any_calls = 0;
		auto fn_any = [&](const match_result& mt) -> tref {
			++any_calls;
			return mt.root;
		};
		tref r_any = (*m).replace(fx.root, fn_any,
			ambig_mode::ANY);
		CHECK(any_calls == 1);
		CHECK(r_any == fx.root); // fn returned the matched node (no-op)
	}

	// ---- Q1. replace() rewriting inside one __AMB__ alternative ----

	TEST_CASE("Q1: replace rewrites inside one __AMB__ alternative under "
		"ANY, keeps the wrapper and leaves the other alternative intact")
	{
		// Tree: start > __AMB__ > [A > B > 'x', A > C > 'x']. Rewrite
		// the C node of the C-alternative into a D node, same child.
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto ad = parse_node_adapter<char, char>(fx.nts);

		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(amb != nullptr);
		REQUIRE(ad.is_nt(amb, "__AMB__"));
		// The forest may store the B- and C-alternatives in either
		// order under __AMB__, so identify each by its own child's
		// NT name rather than by position.
		tref amb_child1 = lcrs_tree<pnode_type<char, char>>::get(
			amb).first();
		REQUIRE(amb_child1 != nullptr);
		tref amb_child2 = lcrs_tree<pnode_type<char, char>>::get(
			amb_child1).right_sibling();
		REQUIRE(amb_child2 != nullptr);
		REQUIRE(ad.is_nt(amb_child1, "A"));
		REQUIRE(ad.is_nt(amb_child2, "A"));
		bool child1_is_b = ad.is_nt(lcrs_tree<pnode_type<char, char>>::get(
			amb_child1).first(), "B");
		tref alt_b = child1_is_b ? amb_child1 : amb_child2;
		tref alt_c = child1_is_b ? amb_child2 : amb_child1;
		REQUIRE(ad.is_nt(lcrs_tree<pnode_type<char, char>>::get(
			alt_b).first(), "B"));
		REQUIRE(ad.is_nt(lcrs_tree<pnode_type<char, char>>::get(
			alt_c).first(), "C"));

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "C" });
		REQUIRE(m.has_value());

		auto fn = [&](const match_result& mt) -> tref {
			tref c_child = lcrs_tree<pnode_type<char, char>>::get(
				mt.root).first();
			return make_nt_node(fx.nts, "D", { c_child });
		};

		tref result = (*m).replace(fx.root, fn, ambig_mode::ANY);
		REQUIRE(result != nullptr);

		CHECK(ad.is_nt(result, "start"));
		tref r_amb = lcrs_tree<pnode_type<char, char>>::get(
			result).first();
		REQUIRE(r_amb != nullptr);
		CHECK(ad.is_nt(r_amb, "__AMB__"));

		tref r_child1 = lcrs_tree<pnode_type<char, char>>::get(
			r_amb).first();
		REQUIRE(r_child1 != nullptr);
		tref r_child2 = lcrs_tree<pnode_type<char, char>>::get(
			r_child1).right_sibling();
		REQUIRE(r_child2 != nullptr);
		// Exactly two alternatives remain under __AMB__.
		CHECK(lcrs_tree<pnode_type<char, char>>::get(
			r_child2).right_sibling() == nullptr);
		CHECK(ad.is_nt(r_child1, "A"));
		CHECK(ad.is_nt(r_child2, "A"));

		bool r_child1_is_b = ad.is_nt(lcrs_tree<pnode_type<char, char>>::get(
			r_child1).first(), "B");
		tref r_alt_b = r_child1_is_b ? r_child1 : r_child2;
		tref r_alt_d = r_child1_is_b ? r_child2 : r_child1;

		// Q3: the untouched B-alternative keeps its original content.
		// Its tref itself can still change: an LCRS node embeds its
		// right-sibling link, so replacing the C-alternative (its right
		// sibling) forces a fresh node even though B's own subtree, on
		// the left, did not change. Compare content, not identity.
		CHECK(lcrs_tree<pnode_type<char, char>>::subtree_equals(
			r_alt_b, alt_b));
		tref r_b = lcrs_tree<pnode_type<char, char>>::get(
			r_alt_b).first();
		REQUIRE(r_b != nullptr);
		CHECK(ad.is_nt(r_b, "B"));

		// The C-alternative now wraps D instead of C, same 'x' leaf.
		tref r_d = lcrs_tree<pnode_type<char, char>>::get(
			r_alt_d).first();
		REQUIRE(r_d != nullptr);
		CHECK(ad.is_nt(r_d, "D"));
		tref r_d_child = lcrs_tree<pnode_type<char, char>>::get(
			r_d).first();
		REQUIRE(r_d_child != nullptr);
		CHECK(ad.is_terminal(r_d_child, "x"));
	}

	// ---- Q2. replace() under FORBID in an unambiguous branch ------

	TEST_CASE("Q2: replace rewrites a node in an unambiguous branch "
		"under FORBID, although the tree holds __AMB__ elsewhere")
	{
		// fx.root: start > __AMB__ > [A > B > x, A > C > x]. `B`'s own
		// subtree never touches __AMB__, so FORBID rewrites it.
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto ad = parse_node_adapter<char, char>(fx.nts);
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "B" });
		REQUIRE(m.has_value());

		int calls = 0;
		auto fn = [&](const match_result& mt) -> tref {
			++calls;
			return make_nt_node(fx.nts, "D", {
				lcrs_tree<pnode_type<char, char>>::get(
					mt.root).first() });
		};
		tref result = (*m).replace(fx.root, fn, ambig_mode::FORBID);
		CHECK(calls == 1);
		REQUIRE(result != nullptr);

		tref r_amb = lcrs_tree<pnode_type<char, char>>::get(
			result).first();
		REQUIRE(r_amb != nullptr);
		CHECK(ad.is_nt(r_amb, "__AMB__"));
		tref r_child1 = lcrs_tree<pnode_type<char, char>>::get(
			r_amb).first();
		REQUIRE(r_child1 != nullptr);
		tref r_child2 = lcrs_tree<pnode_type<char, char>>::get(
			r_child1).right_sibling();
		REQUIRE(r_child2 != nullptr);

		bool child1_has_d = ad.is_nt(lcrs_tree<pnode_type<char, char>>::get(
			r_child1).first(), "D");
		tref d_side = child1_has_d ? r_child1 : r_child2;
		tref c_side = child1_has_d ? r_child2 : r_child1;
		CHECK(ad.is_nt(lcrs_tree<pnode_type<char, char>>::get(
			d_side).first(), "D"));
		CHECK(ad.is_nt(lcrs_tree<pnode_type<char, char>>::get(
			c_side).first(), "C"));
	}

	// ---- Q4,Q5. replace() honors the parent-under-__AMB__ rule -----

	TEST_CASE("Q4: replace never passes an A node as a match root, "
		"when that A sits directly under __AMB__, under FORBID or ANY")
	{
		// fx.root: start > __AMB__ > [A > B > x, A > C > x]. Each A
		// sits directly under __AMB__, so replace never treats
		// either as a match root, in every mode.
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto ad = parse_node_adapter<char, char>(fx.nts);
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "A" });
		REQUIRE(m.has_value());

		std::vector<tref> roots;
		auto fn = [&](const match_result& mt) -> tref {
			roots.push_back(mt.root);
			return mt.root;
		};

		// FORBID also forbids the __AMB__ node's own match attempt,
		// which forks into an alternative, so fn never runs at all.
		tref r_forbid = (*m).replace(fx.root, fn, ambig_mode::FORBID);
		CHECK(roots.empty());
		CHECK(r_forbid == fx.root);

		// ANY lets the __AMB__ node itself match by forking into the
		// B-alternative, but the match root stays the __AMB__ node,
		// never the A node underneath it.
		roots.clear();
		tref r_any = (*m).replace(fx.root, fn, ambig_mode::ANY);
		for (tref r : roots) CHECK_FALSE(ad.is_nt(r, "A"));
		CHECK(r_any == fx.root);
	}

	TEST_CASE("Q5: replace calls fn for a B node whose parent is A, "
		"not __AMB__, under FORBID and ANY")
	{
		// fx.root: start > __AMB__ > [A > B > x, A > C > x]. B's
		// parent is A, so replace calls fn there in every mode.
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "B" });
		REQUIRE(m.has_value());

		int calls = 0;
		auto fn = [&](const match_result& mt) -> tref {
			++calls;
			return mt.root;
		};

		tref r_forbid = (*m).replace(fx.root, fn, ambig_mode::FORBID);
		CHECK(calls == 1);
		CHECK(r_forbid == fx.root);

		calls = 0;
		tref r_any = (*m).replace(fx.root, fn, ambig_mode::ANY);
		CHECK(calls == 1);
		CHECK(r_any == fx.root);
	}

	// ---- Q6,Q7. replace() honors the FORBID subtree rule -----------

	TEST_CASE("Q6: replace refuses a match at the tree's own root under "
		"FORBID, since __AMB__ lives below it although the pattern "
		"does not descend into it")
	{
		ambig_parsed fx{ AMBIG_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "start" });
		REQUIRE(m.has_value());

		int calls = 0;
		auto fn = [&](const match_result& mt) -> tref {
			++calls;
			return mt.root;
		};

		tref r_forbid = (*m).replace(fx.root, fn, ambig_mode::FORBID);
		CHECK(calls == 0);
		CHECK(r_forbid == fx.root);

		calls = 0;
		tref r_any = (*m).replace(fx.root, fn, ambig_mode::ANY);
		CHECK(calls == 1);
		CHECK(r_any == fx.root);
	}

	TEST_CASE("Q7: replace matches at the tree's own root under FORBID "
		"when no __AMB__ node exists anywhere in the tree")
	{
		const char* CHAIN_TGF =
			" start => A.   \n"
			" A     => B.   \n"
			" B     => 'x'. \n";
		parsed fx{ CHAIN_TGF, "x" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "start" });
		REQUIRE(m.has_value());

		int calls = 0;
		auto fn = [&](const match_result& mt) -> tref {
			++calls;
			return mt.root;
		};
		tref result = (*m).replace(fx.root, fn, ambig_mode::FORBID);
		CHECK(calls == 1);
		CHECK(result == fx.root);
	}

	// ---- M21. an injected __AMB__ node blocks FORBID above it ------

	TEST_CASE("M21: replace stops calling fn on an ancestor once a "
		"callback injects an __AMB__ node below it, under FORBID mode")
	{
		// Unambiguous tree: start > A > B > 'x'.
		const char* CHAIN_TGF =
			" start => A.   \n"
			" A     => B.   \n"
			" B     => 'x'. \n";
		parsed fx{ CHAIN_TGF, "x" };
		auto ad = parse_node_adapter<char, char>(fx.nts);

		tref a_node = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(a_node != nullptr);
		REQUIRE(ad.is_nt(a_node, "A"));
		tref b_node = lcrs_tree<pnode_type<char, char>>::get(
			a_node).first();
		REQUIRE(b_node != nullptr);
		REQUIRE(ad.is_nt(b_node, "B"));
		tref x_leaf = lcrs_tree<pnode_type<char, char>>::get(
			b_node).first();
		REQUIRE(x_leaf != nullptr);

		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "%" });
		REQUIRE(m.has_value());

		// A stand-in __AMB__ subtree; its own content does not matter,
		// only that the adapter reports its NT name as "__AMB__".
		tref amb_stub = make_nt_node(fx.nts, "__AMB__",
			{ make_t_leaf('x') });

		std::vector<tref> calls;
		auto fn = [&](const match_result& mt) -> tref {
			calls.push_back(mt.root);
			if (mt.root == b_node) return amb_stub;
			return mt.root;
		};

		tref result = (*m).replace(fx.root, fn, ambig_mode::FORBID);
		REQUIRE(result != nullptr);

		// FORBID scans each candidate's own subtree. Once the callback
		// plants an __AMB__ node at B, every ancestor's subtree holds
		// one, so fn stops running above B, although replace still
		// rebuilds the tree with the stub in place.
		REQUIRE(calls.size() == 2);
		CHECK(calls[0] == x_leaf);
		CHECK(calls[1] == b_node);

		CHECK(ad.is_nt(result, "start"));
		tref r_a = lcrs_tree<pnode_type<char, char>>::get(
			result).first();
		REQUIRE(r_a != nullptr);
		CHECK(ad.is_nt(r_a, "A"));
		tref r_b_replacement = lcrs_tree<pnode_type<char, char>>::get(
			r_a).first();
		REQUIRE(r_b_replacement != nullptr);
		CHECK(r_b_replacement == amb_stub);
	}

	// ---- G10. captures with nullptr slot (optional group) ----------

	TEST_CASE("G10: optional capture - nullptr slot tolerated") {
		// Parse "b" (no 'a'): the optional 'a' inside a_opt's own
		// production doesn't match.
		const char* OPT_TGF =
			" start => seq.   \n"
			" seq   => a_opt 'b'. \n"
			" a_opt => 'a'?.  \n";

		parsed fx{ OPT_TGF, "b" }; // no 'a'
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "seq > (a_opt)? 'b'" });
		REQUIRE(m.has_value());

		// Sanity: pattern matches
		CHECK((*m).search(fx.root));

		bool called = false;
		bool cap_is_null = false;
		auto fn = [&](const match_result& mt) -> tref {
			called = true;
			cap_is_null = (mt.captures.size() == 0
				|| mt.captures[0] == nullptr);
			return mt.root;
		};

		tref result = (*m).replace(fx.root, fn);
		CHECK(called);
		CHECK(result == fx.root); // no-op since fn returned the matched node
		// a_opt is itself mandatory in seq's production (only its own
		// 'a' is optional), so the node exists and the capture is
		// never null here.
		CHECK(cap_is_null == false);
	}

	// ---- M9. one callback call per distinct subtree -----------------

	TEST_CASE("M9: replace calls fn once for two structurally equal "
		"subtrees")
	{
		nonterminals<char> nts;
		tref plus = make_t_leaf('+');
		tref one  = make_nt_node(nts, "int", {});
		auto build_add = [&]() {
			return make_nt_node(nts, "add", {
				make_nt_node(nts, "expr", { one }),
				plus,
				make_nt_node(nts, "expr", { one }) });
		};
		tref add_a = build_add();
		tref add_b = build_add();
		// Hash-consing: identical structure yields the same node.
		REQUIRE(add_a == add_b);

		tref list = make_nt_node(nts, "list", {
			make_nt_node(nts, "expr", { add_a }),
			make_nt_node(nts, "expr", { add_b }) });
		tref root = make_nt_node(nts, "start", { list });

		auto m = matcher_for<char, char>(
			nts, string_view{ "add > (expr) '+' (expr)" });
		REQUIRE(m.has_value());

		int call_count = 0;
		auto fn = [&](const match_result& mt) -> tref {
			++call_count;
			REQUIRE(mt.captures.size() >= 2);
			tref e1 = mt.captures[0];
			tref e2 = mt.captures[1];
			tref minus = make_t_leaf('-');
			return make_nt_node(nts, "sub", { e2, minus, e1 });
		};

		tref result = (*m).replace(root, fn);
		CHECK(call_count == 1);

		size_t subs_after = count_nt(result, "sub", nts);
		CHECK(subs_after == 2);
	}

	// ---- M4. replace() stays linear in the number of visited nodes -

	TEST_CASE("M4: replace() visits a depth-200 chain in linear "
		"is_nt calls")
	{
		// A 200-level "chain > chain > ... > 'a'" tree. FORBID mode's
		// __AMB__ check costs a small multiple of depth in is_nt
		// calls, not O(depth^2).
		nonterminals<char> nts;
		constexpr int depth = 200;
		tref cur = make_t_leaf('a');
		for (int i = 0; i < depth; ++i)
			cur = make_nt_node(nts, "chain", { cur });

		auto base = parse_node_adapter<char, char>(nts);
		int is_nt_calls = 0;
		node_adapter counting;
		counting.is_nt_fn = [&](tref n, std::string_view s) -> bool {
			++is_nt_calls;
			return base.is_nt(n, s);
		};

		auto pat = treemr::compile("chain");
		REQUIRE(pat.has_value());
		matcher<pnode_type<char, char>> m(*pat, counting);

		auto fn = [](const match_result& mt) -> tref { return mt.root; };
		tref result = m.replace(cur, fn, ambig_mode::FORBID);
		CHECK(result == cur); // fn always returns the matched node: no-op

		// A quadratic rescan would need on the order of depth^2 / 2
		// (about 20000) calls for depth == 200; a linear pass stays a
		// small multiple of depth.
		CHECK(is_nt_calls < depth * 10);
	}
}

// ===========================================================================
// H. anchors - decided semantics for the root anchor '/' and '$', and
// group propagation over alternation
// ===========================================================================

TEST_SUITE("treemr - anchors") {

	TEST_CASE("H1: /nt matches when the tree root is that nt, "
		"fails for the same nt deeper in the tree")
	{
		parsed fx{ CSV_TGF, "1,2" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "/row" });
		REQUIRE(m.has_value());

		tref csv = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first();
		REQUIRE(csv != nullptr);
		tref row = lcrs_tree<pnode_type<char, char>>::get(csv).first();
		REQUIRE(row != nullptr);

		// `row` sits below the tree's actual root (`start`), so a
		// search from the true root never counts it as a match root.
		CHECK((*m).search_all(fx.root).empty());
		// `row` matches when it is itself the search root.
		CHECK((*m).match(row));
		CHECK((*m).search_all(row).size() == 1);
	}

	TEST_CASE("H2: /nt and /nt $ accept the same tree") {
		parsed fx{ CSV_TGF, "1,2" };
		auto m1 = matcher_for<char, char>(
			fx.nts, string_view{ "/start" });
		auto m2 = matcher_for<char, char>(
			fx.nts, string_view{ "/start $" });
		REQUIRE(m1.has_value());
		REQUIRE(m2.has_value());

		// The tree root has no right sibling, so `$` adds no
		// constraint here: both patterns accept the same tree.
		CHECK((*m1).match(fx.root));
		CHECK((*m2).match(fx.root));
	}

	TEST_CASE("H3: '^' at the top of a pattern is a compile error, "
		"whether written directly or propagated from a first-slot group")
	{
		// Both forms would set the top sequence's left_anchor; the
		// top of a pattern has no parent for '^' to ask about.
		auto r1 = treemr::compile("(^ start | ^ row)");
		CHECK_FALSE(r1.has_value());
		CHECK(has_parse_error(r1));

		auto r2 = treemr::compile("^ (start | row)");
		CHECK_FALSE(r2.has_value());
		CHECK(has_parse_error(r2));
	}

	TEST_CASE("H4: an anchor on only some alternatives of a group is "
		"a compile error")
	{
		auto r1 = treemr::compile("(^ a | b)");
		CHECK_FALSE(r1.has_value());
		CHECK(has_parse_error(r1));

		auto r2 = treemr::compile("(a | b $)");
		CHECK_FALSE(r2.has_value());
		CHECK(has_parse_error(r2));
	}

	TEST_CASE("H5: an anchor-only group is a compile error") {
		// `(^)`/`($)` would allocate a capture index that can never
		// record a node, silently shifting every later capture.
		auto r1 = treemr::compile("(^)");
		CHECK_FALSE(r1.has_value());
		CHECK(has_parse_error(r1));

		auto r2 = treemr::compile("($)");
		CHECK_FALSE(r2.has_value());
		CHECK(has_parse_error(r2));
	}

	TEST_CASE("H6: a capture alternative with no slots matches zero "
		"width and does not consume a sibling")
	{
		// The DSL cannot produce this IR directly: an anchor-only
		// group is a compile error (H5). Build the compiled_pattern
		// by hand to pin the CAPTURE case of match_node.
		parsed fx{ CSV_TGF, "1,2" };

		compiled_pattern cp;
		cp.num_captures = 1;

		sibling_seq empty_alt; // slots.empty(): zero-width alternative

		pattern_node cap;
		cap.k = pattern_node::kind::CAPTURE;
		cap.capture_idx = 0;
		cap.alternatives.push_back(empty_alt);
		sibling_slot cap_slot;
		cap_slot.node = cap;

		pattern_node cell_nt;
		cell_nt.k = pattern_node::kind::NT;
		cell_nt.text = "cell";
		sibling_slot cell_slot;
		cell_slot.node = cell_nt;

		sibling_seq inner;
		inner.slots.push_back(cap_slot);
		inner.slots.push_back(cell_slot);

		pattern_node row_atom;
		row_atom.k = pattern_node::kind::NT;
		row_atom.text = "row";
		row_atom.edge = edge_type::DIRECT;
		row_atom.sub.push_back(inner);
		sibling_slot row_slot;
		row_slot.node = row_atom;

		cp.top.slots.push_back(row_slot);

		matcher<pnode_type<char, char>> m(cp,
			parse_node_adapter<char, char>(fx.nts));

		// If the empty alternative advanced to the next sibling, the
		// following `cell` slot would have to match ',' and fail.
		CHECK(m.search(fx.root));
		auto sel = m.search_all(fx.root);
		REQUIRE(sel.size() == 1);
		REQUIRE(sel[0].captures.size() == 1);
		CHECK(sel[0].captures[0] == nullptr);
	}

	TEST_CASE("H7: an alternative of the root __AMB__ wrapper still "
		"counts as the search root for a root anchor")
	{
		ambig_parsed fx{ AMBIG_TGF, "x" };
		tref amb = lcrs_tree<pnode_type<char, char>>::get(
			fx.root).first(); // start's only child: __AMB__
		REQUIRE(amb != nullptr);
		auto ad = parse_node_adapter<char, char>(fx.nts);
		REQUIRE(ad.is_nt(amb, "__AMB__"));

		// Searched from the __AMB__ node itself, each alternative (an
		// `A` node) inherits the wrapper's root-ness.
		auto m_a = matcher_for<char, char>(
			fx.nts, string_view{ "/A" });
		REQUIRE(m_a.has_value());
		CHECK((*m_a).match(amb, ambig_mode::ANY));
		CHECK((*m_a).match(amb, ambig_mode::ALL));

		// Neither alternative is itself a `B` node (each is an `A`
		// wrapping a `B` or a `C`), so the anchor still discriminates.
		auto m_b = matcher_for<char, char>(
			fx.nts, string_view{ "/B" });
		REQUIRE(m_b.has_value());
		CHECK_FALSE((*m_b).match(amb, ambig_mode::ANY));
	}
}

// ===========================================================================
// I. leaf anchor ('!') - the matched node must have no children
// ===========================================================================

TEST_SUITE("treemr - leaf anchor ('!')") {

	TEST_CASE("I1: name! matches a childless NT node, fails on one "
		"with children")
	{
		nonterminals<char> nts;
		tref leaf_x   = make_nt_node(nts, "x", {});
		tref branch_x = make_nt_node(nts, "x", { make_t_leaf('a') });

		auto pat = treemr::compile("x!");
		REQUIRE(pat.has_value());
		matcher<pnode_type<char, char>> m(*pat,
			parse_node_adapter<char, char>(nts));

		CHECK(m.match(leaf_x));
		CHECK_FALSE(m.match(branch_x));
	}

	TEST_CASE("I2: %! matches any childless node, fails on one "
		"with children")
	{
		nonterminals<char> nts;
		tref leaf_leaf = make_t_leaf('z');
		tref branch    = make_nt_node(nts, "branch", { leaf_leaf });

		auto pat = treemr::compile("%!");
		REQUIRE(pat.has_value());
		matcher<pnode_type<char, char>> m(*pat,
			parse_node_adapter<char, char>(nts));

		CHECK(m.match(leaf_leaf));
		CHECK_FALSE(m.match(branch));
	}

	TEST_CASE("I3: 'a'! matches a childless terminal leaf, fails on a "
		"node whose collected text is the same but which has children")
	{
		nonterminals<char> nts;
		tref leaf_a = make_t_leaf('a');
		tref wrap_a = make_nt_node(nts, "wrap", { leaf_a });

		auto pat = treemr::compile("'a'!");
		REQUIRE(pat.has_value());
		matcher<pnode_type<char, char>> m(*pat,
			parse_node_adapter<char, char>(nts));

		CHECK(m.match(leaf_a));
		CHECK_FALSE(m.match(wrap_a));
	}

	TEST_CASE("I4: (x)! - the leaf anchor tests the node the capture "
		"matched, not what its alternative descends into")
	{
		nonterminals<char> nts;
		tref leaf_x   = make_nt_node(nts, "x", {});
		tref branch_x = make_nt_node(nts, "x", { make_t_leaf('a') });

		auto pat = treemr::compile("(x)!");
		REQUIRE(pat.has_value());
		matcher<pnode_type<char, char>> m(*pat,
			parse_node_adapter<char, char>(nts));

		match_result mt;
		CHECK(m.match(leaf_x, mt));
		REQUIRE(mt.captures.size() == 1);
		CHECK(mt.captures[0] == leaf_x);

		CHECK_FALSE(m.match(branch_x));
	}
}

// ===========================================================================
// J. trim vs. trim_top
// ===========================================================================

TEST_SUITE("treemr - trim and trim_top") {

	TEST_CASE("J1: trim deletes every match, including one nested "
		"inside another match, in a single pass")
	{
		// "1+2+3" nests one `add` inside another. trim's post-order
		// pass deletes the inner `add` first, then finds the
		// rebuilt outer `add` still matches and deletes it too.
		parsed fx{ REPLACE_TGF, "1+2+3" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "add" }).value();
		tref result = m.trim(fx.root);
		CHECK(count_nt(result, "add", fx.nts) == 0);
	}

	TEST_CASE("J2: trim and trim_top diverge when a match is nested "
		"inside another match")
	{
		// Two `target` nodes, the outer wrapping the inner. `target!`
		// only matches a childless `target`; the inner one already
		// is childless, the outer one is not, until the inner one
		// is removed.
		nonterminals<char> nts;
		auto ad = parse_node_adapter<char, char>(nts);
		tref inner = make_nt_node(nts, "target", {});
		tref outer = make_nt_node(nts, "target", { inner });
		tref root  = make_nt_node(nts, "root", { outer });

		auto pat = treemr::compile("target!");
		REQUIRE(pat.has_value());
		matcher<pnode_type<char, char>> m(*pat, ad);

		// trim (post-order): the inner match is removed first,
		// which leaves the outer `target` childless too, so it now
		// matches and is removed in the same pass. Both are gone.
		tref trimmed = m.trim(root);
		CHECK(ad.is_nt(trimmed, "root"));
		CHECK(lcrs_tree<pnode_type<char, char>>::get(
			trimmed).first() == nullptr);

		// trim_top (top-down): the outer `target` is tested before
		// its child is touched, so it still has a child and does
		// not match. Only the inner match is removed; the traversal
		// never descends into a match, so the outer node is left
		// behind with its match removed.
		tref trimmed_top = m.trim_top(root);
		CHECK(ad.is_nt(trimmed_top, "root"));
		tref remaining = lcrs_tree<pnode_type<char, char>>::get(
			trimmed_top).first();
		REQUIRE(remaining != nullptr);
		CHECK(ad.is_nt(remaining, "target"));
		CHECK(lcrs_tree<pnode_type<char, char>>::get(
			remaining).first() == nullptr);
	}
}

// ===========================================================================
// K. replace_if / replace_until - gating descent with a query
// ===========================================================================

TEST_SUITE("treemr - replace_if and replace_until") {

	TEST_CASE("K1: replace_if skips a subtree that fails query, "
		"replace_until skips a subtree that satisfies it")
	{
		parsed fx{ CSV_TGF, "12,34" };
		auto ad = parse_node_adapter<char, char>(fx.nts);
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "digit" }).value();

		auto is_first_cell = [&](tref n) {
			return ad.is_nt(n, "cell") && terminals_of(n) == "12";
		};
		auto fn = [&](const match_result&) -> tref {
			return make_nt_node(fx.nts, "X", {});
		};

		query_fn descend_unless_first_cell =
			[&](tref n) { return !is_first_cell(n); };
		tref r_if = m.replace_if(fx.root, fn, descend_unless_first_cell);
		CHECK(count_nt(r_if, "digit", fx.nts) == 2); // first cell untouched
		CHECK(count_nt(r_if, "X", fx.nts) == 2);     // second cell rewritten

		query_fn skip_first_cell = is_first_cell;
		tref r_until = m.replace_until(fx.root, fn, skip_first_cell);
		CHECK(count_nt(r_until, "digit", fx.nts) == 2);
		CHECK(count_nt(r_until, "X", fx.nts) == 2);
	}
}

// ===========================================================================
// L. replace_fn contract - nullptr deletes, the matched node is a no-op
// ===========================================================================

TEST_SUITE("treemr - replace_fn contract") {

	TEST_CASE("L1: fn returning nullptr deletes the matched node") {
		parsed fx{ REPLACE_TGF, "1+2" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "add > (expr) '+' (expr)" }).value();
		auto fn = [](const match_result&) -> tref { return nullptr; };
		tref result = m.replace(fx.root, fn);
		CHECK(count_nt(result, "add", fx.nts) == 0);
	}
}

// ===========================================================================
// M. nt_source overloads - matcher_for from a parser object directly
// ===========================================================================
//
// idni::parser<C,T> declares char_type, terminal_type and a const
// get_grammar(), so it satisfies nt_source on its own, with no generated
// parser struct needed.

// Like `parsed`, but keeps the `parser<char>` object itself alive: the
// nt_source overloads read nts through it, not through a bare table.
struct parser_obj_fixture {
	nonterminals<char>     nts;
	grammar<char>          g;
	parser<char>           p;
	parser<char>::result   r;
	tref                   root = nullptr;

	parser_obj_fixture(const char* tgf_src, const char* input)
		: nts{}
		, g{ [&]() {
			auto gr = tgf<char>::from_string(nts, string(tgf_src));
			REQUIRE(gr.has_value());
			return std::move(gr).value();
		}() }
		, p{ g }
		, r{ [&]() {
			auto res = p.parse(input, strlen(input),
				{ .enable_gc = false, .gc_lag = 1 });
			REQUIRE(res.found);
			return res;
		}() }
	{
		root = r.get_shaped_tree2();
		REQUIRE(root != nullptr);
	}
};

TEST_SUITE("treemr - nt_source overloads") {

	TEST_CASE("M1: matcher_for(parser_object, pattern) finds what "
		"matcher_for(nts, pattern) finds")
	{
		parser_obj_fixture fx{ CSV_TGF, "12,34" };
		auto m_nts = matcher_for<char, char>(
			fx.nts, string_view{ "cell" }).value();
		auto m_p = matcher_for(fx.p, string_view{ "cell" }).value();

		CHECK(m_p.search_all(fx.root).size() ==
			m_nts.search_all(fx.root).size());
		CHECK(m_p.search_all(fx.root).size() == 2);
	}

	TEST_CASE("M2: matcher_for(parser_object, compiled_pattern) matches, "
		"using a pattern from compile()")
	{
		parser_obj_fixture fx{ CSV_TGF, "12,34" };
		auto cp = compile_ok("cell");
		auto m = matcher_for(fx.p, std::move(cp).value());

		CHECK(m.search_all(fx.root).size() == 2);
	}

	TEST_CASE("M3: a bad pattern through the parser-object overload "
		"returns a failed result, and does not crash")
	{
		parser_obj_fixture fx{ CSV_TGF, "12,34" };
		auto m = matcher_for(fx.p, string_view{ "bad >>" });

		CHECK_FALSE(m.has_value());
		CHECK(has_parse_error(m));
	}

	TEST_CASE("M4: idni::parser<char> satisfies nt_source") {
		static_assert(nt_source<parser<char>>);
		CHECK(true);
	}
}

// ===========================================================================
// N. parser_gen treemr option - --treemr toggles the emitted matcher()
// member on the generated parser
// ===========================================================================

// A grammar just big enough to drive generate_parser_cpp_from_string.
const char* TREEMR_OPT_TGF =
	" start => 'a'. \n";

// generate_parser_cpp_from_string takes the grammar as an in-memory string,
// so the case needs no .tgf fixture file; the emitted header still lands in
// a temp file, since parser_gen.h has no in-memory output sink, so this
// reads it back the way tests/test_tgf.cpp's generator case does.
string generate_parser_text(const char* tgf_src, parser_gen_options gopt) {
	namespace fs = std::filesystem;
	fs::path dir = fs::temp_directory_path();
	gopt.output_dir = dir.string() + "/";
	if (gopt.output.empty())
		gopt.output = "treemr_opt_test.generated.h";
	if (gopt.name.empty())
		gopt.name = "treemr_opt_test_parser";
	fs::path out = dir / gopt.output;
	auto gr = generate_parser_cpp_from_string<char>(
		"treemr_opt_test.tgf", string(tgf_src), gopt);
	REQUIRE(gr.has_value());
	ifstream in(out);
	stringstream ss; ss << in.rdbuf();
	in.close();
	fs::remove(out);
	return ss.str();
}

TEST_SUITE("treemr - parser_gen treemr option") {

	TEST_CASE("N1: treemr = false emits neither the treemr include nor "
		"the matcher() member")
	{
		parser_gen_options gopt;
		gopt.treemr = false;
		string text = generate_parser_text(TREEMR_OPT_TGF, gopt);
		CHECK(text.find("format/treemr/treemr.h") == string::npos);
		CHECK(text.find("matcher(") == string::npos);
	}

	TEST_CASE("N2: treemr = true emits both the treemr include and the "
		"matcher() member")
	{
		parser_gen_options gopt;
		gopt.treemr = true;
		string text = generate_parser_text(TREEMR_OPT_TGF, gopt);
		CHECK(text.find("format/treemr/treemr.h") != string::npos);
		CHECK(text.find("matcher(") != string::npos);
	}

	TEST_CASE("N3: a freshly constructed parser_gen_options defaults "
		"treemr to false, so a caller that sets nothing emits no "
		"matcher() member")
	{
		parser_gen_options gopt;
		CHECK_FALSE(gopt.treemr);
		string text = generate_parser_text(TREEMR_OPT_TGF, gopt);
		CHECK(text.find("format/treemr/treemr.h") == string::npos);
		CHECK(text.find("matcher(") == string::npos);
	}
}

// ===========================================================================
// O. lcrs_tree forwarding - match/search/search_all/replace*/trim* reach
// the matcher via the tree_matcher-constrained overloads
// ===========================================================================

TEST_SUITE("treemr - tree forwarding methods") {

	TEST_CASE("O1: t.match/t.search/t.search_all reach the matcher") {
		parsed fx{ CSV_TGF, "12,34" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "cell" }).value();
		const auto& t = parser<char>::tree::get(fx.root);

		CHECK_FALSE(t.match(m));    // the tree root is "start", not "cell"
		CHECK(t.search(m));
		CHECK(t.search_all(m).size() == 2);
	}

	TEST_CASE("O2: t.replace/t.replace_if/t.replace_until reach "
		"the matcher")
	{
		parsed fx{ REPLACE_TGF, "1+2" };
		auto m = matcher_for<char, char>(
			fx.nts, string_view{ "add > (expr) '+' (expr)" }).value();
		const auto& t = parser<char>::tree::get(fx.root);

		auto fn = [&](const match_result& mt) -> tref {
			tref minus = make_t_leaf('-');
			return make_nt_node(fx.nts, "sub",
				{ mt.captures[1], minus, mt.captures[0] });
		};
		query_fn always_descend = [](tref) { return true; };
		query_fn never_skip     = [](tref) { return false; };

		CHECK(count_nt(t.replace(m, fn), "sub", fx.nts) == 1);
		CHECK(count_nt(t.replace_if(m, fn, always_descend),
			"sub", fx.nts) == 1);
		CHECK(count_nt(t.replace_until(m, fn, never_skip),
			"sub", fx.nts) == 1);
	}

	TEST_CASE("O3: t.trim_top(m) resolves to the matcher overload, "
		"not the older predicate overload")
	{
		// The unconstrained lcrs_tree::trim_top(const auto& query)
		// also exists; a `matcher` argument must resolve to the
		// tree_matcher-constrained overload instead, since a bare
		// matcher has no operator() for the predicate form to call.
		nonterminals<char> nts;
		auto ad = parse_node_adapter<char, char>(nts);
		tref inner = make_nt_node(nts, "target", {});
		tref outer = make_nt_node(nts, "target", { inner });
		tref root  = make_nt_node(nts, "root", { outer });

		auto pat = treemr::compile("target!");
		REQUIRE(pat.has_value());
		matcher<pnode_type<char, char>> m(*pat, ad);

		const auto& t = lcrs_tree<pnode_type<char, char>>::get(root);
		tref trimmed = t.trim(m);
		CHECK(lcrs_tree<pnode_type<char, char>>::get(
			trimmed).first() == nullptr);

		tref trimmed_top = t.trim_top(m);
		tref remaining = lcrs_tree<pnode_type<char, char>>::get(
			trimmed_top).first();
		REQUIRE(remaining != nullptr);
		CHECK(ad.is_nt(remaining, "target"));
	}
}
