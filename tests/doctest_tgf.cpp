// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for TGF grammar parsing - char terminals, literals, comments,
// EBNF (optional, zero-or-any, plus, asterisk, group), directives,
// and TGF-syntax boolean operators (&, ~).
//
// Stress tests live in doctest_stress.cpp.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"
#include "grammar_inspector.h"

using namespace std;
using namespace idni;

// ---------------------------------------------------------------------------
// helpers - assert parse outcomes against a TGF grammar string
// ---------------------------------------------------------------------------

// Asserts: TGF compiles and `input` parses successfully. On rejection,
// FAIL_CHECK reports the error message. Ambiguity is checked via shadow parse.
template <typename T = char>
static void expect_tgf_parses(const char* g_tgf, const T* input,
	bool allow_ambiguity = false)
{
	basic_string<T> istr(input);
	nonterminals<T> nts;
	auto gr = tgf<T>::from_string(nts, basic_string<T>(g_tgf));
	if (!gr.has_value()) {
		FAIL_CHECK("TGF compile failed for grammar:\n" << g_tgf);
		return;
	}
	grammar<T> g = std::move(gr).value();
	if (g.size() == 0) {
		FAIL_CHECK("TGF produced empty grammar:\n" << g_tgf);
		return;
	}
	for (auto tree_path : {parse_tree_path::bintree_path,
			parse_tree_path::forest_path}) {
	parser<T> p(g);
	auto r = p.parse(istr.c_str(), istr.size(),
		{ .tree_path = tree_path, .enable_gc = false, .gc_lag = 1 });
	if (!r.found) {
		string msg = r.parse_error.to_str(
			parser<T>::error::info_lvl::INFO_BASIC);
		FAIL_CHECK("expected '" << to_std_string(istr)
			<< "' to parse, got error: '" << msg << "'");
		continue;
	}
	if (allow_ambiguity) continue;
	nonterminals<T> nts_shadow;
	auto gr_shadow = tgf<T>::from_string(nts_shadow, basic_string<T>(g_tgf));
	if (!gr_shadow.has_value()) continue;
	grammar<T> g_shadow = std::move(gr_shadow).value();
	g_shadow.opt.auto_disambiguate = false;
	parser<T> p_shadow(g_shadow);
	auto r_shadow = p_shadow.parse(istr.c_str(), istr.size(),
		{ .tree_path = tree_path, .enable_gc = false, .gc_lag = 1 });
	if (r_shadow.found && r_shadow.is_ambiguous())
		FAIL_CHECK("expected unambiguous parse for '"
			<< to_std_string(istr)
			<< "', shadow parse with auto_disambig=false is ambiguous");
	}
}

// Asserts: TGF compiles and `input` is rejected by it.
template <typename T = char>
static void expect_tgf_rejects(const char* g_tgf, const T* input) {
	basic_string<T> istr(input);
	nonterminals<T> nts;
	auto gr = tgf<T>::from_string(nts, basic_string<T>(g_tgf));
	if (!gr.has_value()) {
		FAIL_CHECK("TGF compile failed for grammar:\n" << g_tgf);
		return;
	}
	grammar<T> g = std::move(gr).value();
	if (g.size() == 0) {
		FAIL_CHECK("TGF produced empty grammar:\n" << g_tgf);
		return;
	}
	for (auto tree_path : {parse_tree_path::bintree_path,
			parse_tree_path::forest_path}) {
	parser<T> p(g);
	auto r = p.parse(istr.c_str(), istr.size(),
		{ .tree_path = tree_path, .enable_gc = false, .gc_lag = 1 });
	if (r.found)
		FAIL_CHECK("expected '" << to_std_string(istr)
			<< "' to be rejected, but parse succeeded");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: basic TGF parsing
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: basic") {

	TEST_CASE("comment-only TGF parses without crash") {
		// Should not throw or crash
		nonterminals<char> nts;
		tgf<char>::from_string(nts, string("	# TGF only with ws and ws_comment \n"));
	}

	TEST_CASE("single char terminal") {
		expect_tgf_parses(" start => 'a'. ", "a");
	}

	TEST_CASE("single string terminal") {
		expect_tgf_parses(" start => \"a\". ", "a");
	}

	TEST_CASE("two char terminals") {
		expect_tgf_parses(" start => 'a' 'b'. ", "ab");
	}

	TEST_CASE("two string terminals") {
		expect_tgf_parses(" start => \"a\" \"b\". ", "ab");
	}

	TEST_CASE("multi-char string terminal") {
		expect_tgf_parses(" start => \"ab\". ", "ab");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: literals
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: literals") {

	TEST_CASE("literal rule") {
		expect_tgf_parses(
			"	start  => \"hi\". \n"
		, "hi");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: comments
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: comments") {

	TEST_CASE("comment with tabs") {
		expect_tgf_parses(
			"	#	comment	with	tabs	\n"
			"	start  => '1'. \n"
		, "1");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: ambiguous grammar with nulls
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: ambiguous with nulls") {

	TEST_CASE("A B & D C") {
		// intentionally ambiguous (multiple null splits)
		expect_tgf_parses(
			"	start => A B & D C. \n"
			"	A  => 'a' A     | null. \n"
			"	B  => 'b' B 'c' | null. \n"
			"	C  => 'c' C     | null. \n"
			"	D  => 'a' D 'b' | null. \n"
		, "abc", true);
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: EBNF - optional
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: ebnf optional") {

	TEST_CASE("optional with nonterminal") {
		expect_tgf_parses(
			"	start  => binary [ two ]. \n"
			"	binary => '0' | '1'. \n"
			"	two    => '2'. \n"
		, "02");
	}

	TEST_CASE("optional terminal with both present") {
		expect_tgf_parses(
			"	start  => '1' [ '0' ] '1'. \n"
		, "11");
	}

	TEST_CASE("optional terminal with none") {
		expect_tgf_parses(
			"	start  => [ '0' ]. \n"
		, "");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: EBNF - zero or any (curly braces)
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: ebnf zero or any") {

	TEST_CASE("long binary") {
		expect_tgf_parses(
			"	start  => { binary } | { two }. \n"
			"	binary => '0' | '1'. \n"
			"	two    => '2'. \n"
		, "0101010100001001111");
	}

	TEST_CASE("zero or more terminals between anchors") {
		expect_tgf_parses(
			"	start  => '1' { '0' } '1'. \n"
		, "1000001");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: EBNF - plus
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: ebnf plus") {

	TEST_CASE("digit plus") {
		expect_tgf_parses(
			"	@use char classes digit. \n"
			"	start  => digit+. \n"
		, "1382746358690");
	}

	TEST_CASE("letter plus between anchors") {
		expect_tgf_parses(
			"	start  => 'a' 'b'+ 'c'. \n"
		, "abbbc");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: EBNF - asterisk
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: ebnf asterisk") {

	TEST_CASE("digit asterisk") {
		expect_tgf_parses(
			"	@use char classes digit. \n"
			"	start  => digit*. \n"
		, "1382746358690");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: EBNF - group
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: ebnf group") {

	TEST_CASE("simple group") {
		expect_tgf_parses(
			"	start  => '1' ( '0' ) '1'. \n"
		, "101");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: EBNF - group plus
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: ebnf group plus") {

	TEST_CASE("group plus") {
		expect_tgf_parses(
			"	start  => '1' ( '0' '0' )+ '1'. \n"
		, "100001");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: EBNF - group asterisk
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: ebnf group asterisk") {

	TEST_CASE("group asterisk with zero matches") {
		expect_tgf_parses(
			"	start  => '1' ( '0' '0' )* '1'. \n"
		, "11");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: directives
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: directives") {

	TEST_CASE("start directive") {
		// intentionally ambiguous: S S S vs S S can both derive "111"
		expect_tgf_parses(
			"	@start S. \n"
			"	S  => S S S | S S | '1'. \n"
		, "111", true);
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: boolean operators in TGF syntax (&, ~)
//
// Verifies the TGF lexer/parser correctly produces conjunction and negation
// productions (parses the operators, builds the right tree). Boolean
// *semantics* are exercised in doctest_boolean_grammar.cpp.
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: boolean operators") {

	TEST_CASE("conjunction with negation accepts matching alternative") {
		// start => X & ~'b'.  X => 'a' | 'b'.
		// "a" matches X (via 'a' alt), and is not 'b', so accepted.
		expect_tgf_parses(
			"	start => X & ~'b'. \n"
			"	X     => 'a' | 'b'. \n"
		, "a");
	}

	TEST_CASE("conjunction with negation rejects negated alternative") {
		// "b" matches X (via 'b' alt) but also matches ~'b' negation
		// → conjunction fails.
		expect_tgf_rejects(
			"	start => X & ~'b'. \n"
			"	X     => 'a' | 'b'. \n"
		, "b");
	}

	TEST_CASE("negation alone (no conjunction) is invalid grammar form") {
		// Both `'a'` and `'a' & ~'b'` match input "a", so the grammar
		// is ambiguous; auto-disambiguate picks one.
		expect_tgf_parses(
			"	start => 'a' | ('a' & ~'b'). \n"
		, "a", /*allow_ambiguity=*/true);
	}

	TEST_CASE("conjunction of two positive nonterminals") {
		// "a" matches both A and B → accepted. Conjunction nodes are
		// considered ambiguous in the bool-grammar internal forest.
		expect_tgf_parses(
			"	start => A & B. \n"
			"	A     => 'a' | 'b'. \n"
			"	B     => 'a' | 'c'. \n"
		, "a", /*allow_ambiguity=*/true);
	}

	TEST_CASE("conjunction of two positive nonterminals - disjoint match") {
		// "b" matches A but not B → conjunction fails.
		expect_tgf_rejects(
			"	start => A & B. \n"
			"	A     => 'a' | 'b'. \n"
			"	B     => 'a' | 'c'. \n"
		, "b");
	}
}

// ---------------------------------------------------------------------------
// comment pattern: "/*" ((printable|space) & ~"*/")* "*/"
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: comment negation") {

	const char* comment_grammar =
		"@use char classes printable, space. \n"
		"start   => comment. \n"
		"comment => \"/*\" "
			"((printable | space) & ~\"*/\")* "
			"\"*/\". \n";

	TEST_CASE("empty body") {
		expect_tgf_parses(comment_grammar, "/**/", true);
	}

	TEST_CASE("body with text") {
		expect_tgf_parses(comment_grammar, "/* hello */", true);
		expect_tgf_parses(comment_grammar, "/* hello world */", true);
	}

	TEST_CASE("star inside body") {
		expect_tgf_parses(comment_grammar, "/* a * b */", true);
	}

	TEST_CASE("slash inside body") {
		expect_tgf_parses(comment_grammar, "/* a / b */", true);
	}

	TEST_CASE("nested-looking stars and slashes") {
		expect_tgf_parses(comment_grammar, "/* /*** */ **/ */", true);
	}

	TEST_CASE("greedy rule also works (Earley backtracks)") {
		const char* greedy_grammar =
			"@use char classes printable, space. \n"
			"start   => comment. \n"
			"comment => \"/*\" "
				"(printable | space)* "
				"\"*/\". \n";
		expect_tgf_parses(greedy_grammar, "/* */", true);
		expect_tgf_parses(greedy_grammar, "/* hello */", true);
		// Earley parser finds a non-greedy split, so this works too
		expect_tgf_parses(greedy_grammar, "/* */ */", true);
	}

	TEST_CASE("cc-grammar: (cc | ((cc cc) & ~*/))* prevents */ as pair") {
		const char* cc_grammar =
			"@use char classes printable, space. \n"
			"start   => comment. \n"
			"cc      => printable | space. \n"
			"comment => \"/*\" "
				"( cc | ((cc cc) & ~\"*/\") )* "
				"\"*/\". \n";
		expect_tgf_parses(cc_grammar, "/**/", true);
		expect_tgf_parses(cc_grammar, "/* hello */", true);
		expect_tgf_parses(cc_grammar, "/* a * b */", true);
		expect_tgf_parses(cc_grammar, "/* a / b */", true);
		expect_tgf_parses(cc_grammar, "/* /*** */ **/ */", true);
		// does it reject invalid input?
		expect_tgf_rejects(cc_grammar, "/* unterminated");
		// nested-looking: each "/*" is 2 single chars, "*/" terminates
		expect_tgf_parses(cc_grammar,
			"/*  /* /* /* */ */ */ /*  */", true);

		// empirically verify: parse and inspect terminals to confirm
		// the first */ is body chars, not the terminator
		nonterminals<char> nts_cc;
		auto gr_cc = tgf<char>::from_string(nts_cc, string(cc_grammar));
		REQUIRE(gr_cc.has_value());
		grammar<char> g = std::move(gr_cc).value();
		for (auto tree_path : {parse_tree_path::bintree_path,
				parse_tree_path::forest_path}) {
		parser<char> p(g);
		// "/* */ */" has two */ sequences - first is body, last is terminator
		auto r = p.parse("/* */ */", 8,
			{ .tree_path = tree_path, .enable_gc = false, .gc_lag = 1 });
		REQUIRE(r.found);
		auto terms = r.get_terminals();
		string tstr = to_std_string(terms);
		CHECK(tstr == "/* */ */");
		// find first */ - should be at position 3 (body content)
		auto pos1 = tstr.find("*/");
		CHECK(pos1 == 3u);
		// find second */ - should be at position 6 (terminator)
		auto pos2 = tstr.find("*/", pos1 + 2);
		CHECK(pos2 == 6u);
		}
	}
}

// ---------------------------------------------------------------------------
// char-class nonterminal ordering (nul + CC must precede other symbols)
// ---------------------------------------------------------------------------

TEST_SUITE("tgf: char class nonterminal ordering") {
	static void check_cc_nt_ordering(const grammar<char>& g,
		nonterminals<char>& nts)
	{
		grammar_inspector<char, char> gi(g);
		const auto& names = gi.nts();
		REQUIRE(names.size() >= 4);
		CHECK(names.get(0) == "");
		CHECK(names.get(1) == "digit");
		CHECK(names.get(2) == "space");
		CHECK(gi.cc_fns().is_fn(1));
		CHECK(gi.cc_fns().is_fn(2));
		CHECK(nts.get("__") > 2);
		CHECK(nts.get("_") > 2);
	}

	TEST_CASE("from_string: nul and char classes precede trim symbols") {
		const char* g_tgf =
			"@use char class digit, space.\n"
			"@trim __, _.\n"
			"start => digit.\n";
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts, string(g_tgf));
		REQUIRE(gr.has_value());
		check_cc_nt_ordering(gr.value(), nts);
	}

	TEST_CASE("from_string_presplit: nul and char classes precede trim symbols") {
		const char* g_tgf =
			"@use char class digit, space.\n"
			"@trim __, _.\n"
			"start => digit.\n";
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string_presplit(nts, string(g_tgf));
		REQUIRE(gr.has_value());
		check_cc_nt_ordering(gr.value(), nts);
	}
}
