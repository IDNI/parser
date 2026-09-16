// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for Boolean grammar extensions: conjunction (&), negation (~),
// cascading uncomplete, and fixpoint resolution.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"
#include "recoders.h"

using namespace std;
using namespace idni;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// Parser/grammar option matrix is selected at compile time via -D defines so
// the same source file can be compiled into multiple test binaries.
static grammar<char>::options make_grammar_options() {
	grammar<char>::options go;
#ifdef TAU_TEST_NO_AUTODISAMBG
	go.auto_disambiguate = false;
#endif
	return go;
}

static parser<char>::options make_parser_options() {
	parser<char>::options o;
#ifdef TAU_TEST_BINARIZE
	o.binarize = true;
#endif
#ifdef TAU_TEST_GC
	// enable_gc is in parse_options, not constructor options
#endif
#ifdef TAU_TEST_INCRGEN
	o.incr_gen_forest = true;
#endif
	return o;
}

// Asserts: input parses successfully. On rejection, FAIL_CHECK reports both
// the input and the actual error message.
//
// To surface ambiguity that auto_disambiguate=true would silently resolve, a
// shadow parse with auto_disambiguate=false runs and asserts non-ambiguous
// unless allow_ambiguity is set. The shadow check is skipped under
// TAU_TEST_NO_AUTODISAMBG.
static void expect_parses(grammar<char>& g, const string& input,
	bool allow_ambiguity = false)
{
	for (auto tree_path : {parse_tree_path::bintree_path,
			parse_tree_path::forest_path}) {
	parser<char> p(g, make_parser_options());
	auto r = p.parse(input.c_str(), input.size(),
		{ .tree_path = tree_path, .enable_gc = false, .gc_lag = 1 });
	if (!r.found) {
		string msg = r.parse_error.to_str(
			parser<char>::error::info_lvl::INFO_BASIC);
		FAIL_CHECK("expected '" << input << "' to parse, got error: '"
			<< msg << "'");
		continue;
	}
#if !defined(TAU_TEST_NO_AUTODISAMBG)
	if (allow_ambiguity) continue;
	// Shadow check: temporarily disable auto_disambiguate, verify no ambiguity.
	bool saved_ad = g.opt.auto_disambiguate;
	g.opt.auto_disambiguate = false;
	parser<char>::options po_shadow{};
	parser<char> p_shadow(g, po_shadow);
	g.opt.auto_disambiguate = saved_ad;
	auto r_shadow = p_shadow.parse(input.c_str(), input.size(),
		{ .tree_path = tree_path, .enable_gc = false, .gc_lag = 1 });
	if (r_shadow.found && r_shadow.is_ambiguous())
		FAIL_CHECK("expected unambiguous parse for '" << input
			<< "', shadow parse with auto_disambig=false is ambiguous");
#endif
	}
}

// Asserts: parse fails AND the error message contains expected_error.
// On failure reports both expected and actual messages.
static void expect_parse_error(grammar<char>& g, const string& input,
	const string& expected_error)
{
	for (auto tree_path : {parse_tree_path::bintree_path,
			parse_tree_path::forest_path}) {
	parser<char> p(g, make_parser_options());
	auto r = p.parse(input.c_str(), input.size(),
		{ .tree_path = tree_path, .enable_gc = false, .gc_lag = 1 });
	if (r.found) {
		FAIL_CHECK("expected '" << input << "' to fail with '"
			<< expected_error << "', but parse succeeded");
		continue;
	}
	string msg = r.parse_error.to_str(
		parser<char>::error::info_lvl::INFO_BASIC);
	CHECK_MESSAGE(msg.find(expected_error) != string::npos,
		"input '" << input << "': expected error to contain '"
		<< expected_error << "', got: '" << msg << "'");
	}
}

// Asserts: parse fails (no error message check).
static void expect_rejects(grammar<char>& g, const string& input)
{
	for (auto tree_path : {parse_tree_path::bintree_path,
			parse_tree_path::forest_path}) {
	parser<char> p(g, make_parser_options());
	auto r = p.parse(input.c_str(), input.size(),
		{ .tree_path = tree_path, .enable_gc = false, .gc_lag = 1 });
	if (r.found)
		FAIL_CHECK("expected '" << input
			<< "' to be rejected, but parse succeeded");
	}
}

// Asserts: parse succeeds AND a specific nonterminal name appears in the
// forest traversal.
static void expect_parse_contains(grammar<char>& g, const string& input,
	const string& nt_name)
{
	parser<char> p(g, make_parser_options());
	auto r = p.parse(input.c_str(), input.size());
	if (!r.found) {
		FAIL_CHECK("expected '" << input << "' to parse, got error: '"
			<< r.parse_error.to_str(
				parser<char>::error::info_lvl::INFO_BASIC)
			<< "'");
		return;
	}
	auto* f = r.get_forest();
	bool contained = false;
	auto cb_enter = [&](const auto& n) {
		if (nt_name == n->first.to_std_string())
			contained = true;
	};
	auto cb_exit = [](const auto&, const auto&) {};
	f->traverse(cb_enter, cb_exit);
	CHECK_MESSAGE(contained,
		"input '" << input << "': expected nonterminal '" << nt_name
		<< "' in forest, but it was not present");
}

// ---------------------------------------------------------------------------
// TEST SUITE: basic negation
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: basic negation") {

	TEST_CASE("start rule: a & ~b accepts 'a'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		prods<char> ps, start(start_lit);
		prods<char> a('a'), b('b');
		ps(start, a & ~b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("start rule: a & ~b rejects 'b'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		prods<char> ps, start(start_lit);
		prods<char> a('a'), b('b');
		ps(start, a & ~b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_rejects(g, "b");
		expect_parse_error(g, "b",
			"Unexpected 'b' at 1:1");
	}

	TEST_CASE("start rule: a & ~b rejects 'c'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		prods<char> ps, start(start_lit);
		prods<char> a('a'), b('b');
		ps(start, a & ~b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_rejects(g, "c");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: negation with nonterminals
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: negation with nonterminals") {

	TEST_CASE("X & ~b where X => a|b: accepts 'a'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X");
		prods<char> ps, start(start_lit), X(X_lit);
		prods<char> a('a'), b('b');
		ps(start, X & ~b);
		ps(X, a | b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("X & ~b where X => a|b: rejects 'b'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X");
		prods<char> ps, start(start_lit), X(X_lit);
		prods<char> a('a'), b('b');
		ps(start, X & ~b);
		ps(X, a | b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_rejects(g, "b");
		expect_parse_error(g, "b",
			"Unexpected 'b' at 1:1");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: negated nonterminal reference (docs/fix_negation.md regression)
//
// `~NT`, where NT is a user-defined nonterminal - or a parenthesized group,
// which the builder wraps into a synthetic nonterminal - must reject the
// characters NT matches. The original code searched the parent production's
// items for the conjunct and never saw the referenced NT's completions, so
// `~NT` was a no-op (every character accepted). The fix detects the
// referenced NT's completion at the same span via completion_count. This
// mirrors the doc's `unescaped => anychar & ~('"' | (cntrl & ascii))` with
// explicit characters standing in for the char classes.
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: negated nonterminal (fix_negation.md)") {

	// start => letter & ~vowel ; letter => a|b|c|d|e ; vowel => a|e
	// consonants accepted, vowels rejected.
	TEST_CASE("letter & ~vowel: accepts consonants 'b','d'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto letter_lit = nts("letter");
		auto vowel_lit = nts("vowel");
		prods<char> ps, start(start_lit), letter(letter_lit),
			vowel(vowel_lit);
		prods<char> a('a'), b('b'), c('c'), d('d'), e('e');
		ps(start, letter & ~vowel);
		ps(letter, a | b | c | d | e);
		ps(vowel, a | e);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "b");
		expect_parses(g, "d");
	}

	TEST_CASE("letter & ~vowel: rejects vowels 'a','e'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto letter_lit = nts("letter");
		auto vowel_lit = nts("vowel");
		prods<char> ps, start(start_lit), letter(letter_lit),
			vowel(vowel_lit);
		prods<char> a('a'), b('b'), c('c'), d('d'), e('e');
		ps(start, letter & ~vowel);
		ps(letter, a | b | c | d | e);
		ps(vowel, a | e);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_rejects(g, "a");
		expect_rejects(g, "e");
	}

	// Same behavior, but the negated disjunction is an inline group that
	// the builder wraps into a synthetic nonterminal - the shape the doc
	// reports as broken (`~( ... )`).
	TEST_CASE("letter & ~(a|e) inline group: accepts 'c', rejects 'a','e'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto letter_lit = nts("letter");
		prods<char> ps, start(start_lit), letter(letter_lit);
		prods<char> a('a'), b('b'), c('c'), d('d'), e('e');
		ps(start, letter & ~(a | e));
		ps(letter, a | b | c | d | e);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "c");
		expect_rejects(g, "a");
		expect_rejects(g, "e");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: deep nonterminal negation
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: deep nonterminal negation") {

	TEST_CASE("enabled & ~disabled pattern") {
		// char_ => enabled & ~disabled
		// enabled => A | B, disabled => A
		// A => 'a', B => 'b'
		// Only 'b' should pass (it's enabled but not disabled).
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto expression_lit = nts("expression");
		auto char__lit = nts("char_");
		auto enabled_lit = nts("enabled");
		auto disabled_lit = nts("disabled");
		auto A_lit = nts("A");
		auto B_lit = nts("B");
		prods<char> ps, start(start_lit),
			expression(expression_lit), char_(char__lit),
			enabled(enabled_lit), disabled(disabled_lit),
			A(A_lit), B(B_lit);
		prods<char> a('a'), b('b');
		ps(start, expression);
		ps(expression, char_);
		ps(char_, enabled & ~disabled);
		ps(enabled, A | B);
		ps(disabled, A);
		ps(A, a);
		ps(B, b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "b");
		expect_rejects(g, "a");
		expect_parse_error(g, "a",
			"Unexpected 'a' at 1:1");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: conjunction without negation
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: conjunction without negation") {

	TEST_CASE("(A+c) & ~(B+c) accepts 'ac'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A");
		auto B_lit = nts("B");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit);
		prods<char> a('a'), b('b'), c('c');
		ps(start, (A + c) & ~(B + c));
		ps(A, a | c);
		ps(B, b | c);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "ac");
	}

	TEST_CASE("(A+c) & ~(B+c) rejects 'bc'") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A");
		auto B_lit = nts("B");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit);
		prods<char> a('a'), b('b'), c('c');
		ps(start, (A + c) & ~(B + c));
		ps(A, a | c);
		ps(B, b | c);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_rejects(g, "bc");
		expect_parse_error(g, "bc",
			"Unexpected \"bc\" at 1:1 (1)");
	}

	TEST_CASE("(A+c) & ~(B+c) rejects 'cc' (both A and B match c)") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A");
		auto B_lit = nts("B");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit);
		prods<char> a('a'), b('b'), c('c');
		ps(start, (A + c) & ~(B + c));
		ps(A, a | c);
		ps(B, b | c);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_rejects(g, "cc");
		expect_parse_error(g, "cc",
			"Unexpected \"cc\" at 1:1 (1)");
	}

	TEST_CASE("pure conjunction A & B") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A");
		auto B_lit = nts("B");
		auto X_lit = nts("X");
		auto T_lit = nts("T");
		prods<char> ps, start(start_lit),
			A(A_lit), B(B_lit), X(X_lit), T(T_lit);
		prods<char> b('b');
		ps(start, A & B);
		ps(A, X);
		ps(X, start | b);
		ps(B, T | b);
		ps(T, start);
		// intentionally ambiguous (legacy "all_cycles" case)
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "b", true);
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: keyword exclusion pattern
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: keyword exclusion") {

	TEST_CASE("identifier => chars & ~keyword: 'var123' is identifier") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto identifier_lit = nts("identifier");
		auto keyword_lit    = nts("keyword");
		auto chars_lit      = nts("chars");
		auto alpha_lit      = nts("alpha");
		auto alnum_lit      = nts("alnum");
		prods<char> ps, start(start_lit),
			identifier(identifier_lit), keyword(keyword_lit),
			chars(chars_lit), alpha(alpha_lit), alnum(alnum_lit);
		ps(start, identifier | keyword);
		ps(identifier, chars & ~keyword);
		ps(chars, alpha | (chars + alnum));
		ps(keyword, {"print"});
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "var123");
	}

	TEST_CASE("identifier => chars & ~keyword: 'print' is keyword") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto identifier_lit = nts("identifier");
		auto keyword_lit    = nts("keyword");
		auto chars_lit      = nts("chars");
		auto alpha_lit      = nts("alpha");
		auto alnum_lit      = nts("alnum");
		prods<char> ps, start(start_lit),
			identifier(identifier_lit), keyword(keyword_lit),
			chars(chars_lit), alpha(alpha_lit), alnum(alnum_lit);
		ps(start, identifier | keyword);
		ps(identifier, chars & ~keyword);
		ps(chars, alpha | (chars + alnum));
		ps(keyword, {"print"});
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "print");
	}

	TEST_CASE("'var123' derivation contains 'identifier' node") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto identifier_lit = nts("identifier");
		auto keyword_lit    = nts("keyword");
		auto chars_lit      = nts("chars");
		auto alpha_lit      = nts("alpha");
		auto alnum_lit      = nts("alnum");
		prods<char> ps, start(start_lit),
			identifier(identifier_lit), keyword(keyword_lit),
			chars(chars_lit), alpha(alpha_lit), alnum(alnum_lit);
		ps(start, identifier | keyword);
		ps(identifier, chars & ~keyword);
		ps(chars, alpha | (chars + alnum));
		ps(keyword, {"print"});
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parse_contains(g, "var123", "identifier");
	}

	TEST_CASE("'print' derivation contains 'keyword' node") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto identifier_lit = nts("identifier");
		auto keyword_lit    = nts("keyword");
		auto chars_lit      = nts("chars");
		auto alpha_lit      = nts("alpha");
		auto alnum_lit      = nts("alnum");
		prods<char> ps, start(start_lit),
			identifier(identifier_lit), keyword(keyword_lit),
			chars(chars_lit), alpha(alpha_lit), alnum(alnum_lit);
		ps(start, identifier | keyword);
		ps(identifier, chars & ~keyword);
		ps(chars, alpha | (chars + alnum));
		ps(keyword, {"print"});
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parse_contains(g, "print", "keyword");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: nonzero digit pattern
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: nonzero digit") {

	TEST_CASE("start accepts standalone '0' via zero alternative") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit   = nts("start");
		auto nzdigit_lit = nts("nzdigit");
		auto digits_lit  = nts("digits");
		auto digit_lit   = nts("digit");
		prods<char> ps, start(start_lit),
			nzdigit(nzdigit_lit), digits(digits_lit), digit(digit_lit),
			nll(lit<char>{});
		prods<char> zero('0');
		ps(start, zero | (nzdigit + digits));
		ps(digits, (digit + digits) | nll);
		ps(nzdigit, digit & ~zero);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "0");
	}

	TEST_CASE("start accepts '12' via nzdigit + digits") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit   = nts("start");
		auto nzdigit_lit = nts("nzdigit");
		auto digits_lit  = nts("digits");
		auto digit_lit   = nts("digit");
		prods<char> ps, start(start_lit),
			nzdigit(nzdigit_lit), digits(digits_lit), digit(digit_lit),
			nll(lit<char>{});
		prods<char> zero('0');
		ps(start, zero | (nzdigit + digits));
		ps(digits, (digit + digits) | nll);
		ps(nzdigit, digit & ~zero);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "12");
	}

	TEST_CASE("nzdigit (digit & ~zero) rejects leading zero in '01'") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit   = nts("start");
		auto nzdigit_lit = nts("nzdigit");
		auto digits_lit  = nts("digits");
		auto digit_lit   = nts("digit");
		prods<char> ps, start(start_lit),
			nzdigit(nzdigit_lit), digits(digits_lit), digit(digit_lit),
			nll(lit<char>{});
		prods<char> zero('0');
		ps(start, zero | (nzdigit + digits));
		ps(digits, (digit + digits) | nll);
		ps(nzdigit, digit & ~zero);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_rejects(g, "01");
		expect_parse_error(g, "01",
			"Unexpected '0' at 1:1 (1)");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: string escaping (complex Boolean pattern)
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: string escaping") {

	// char_ => (a|b|c) & ~(quote|escape)
	// Only non-quote, non-escape chars are string characters.

	TEST_CASE("empty string") {
		nonterminals<char> nts;
		auto start_lit       = nts("start");
		auto char__lit       = nts("char_");
		auto escape_lit      = nts("escape");
		auto string_char_lit = nts("string_char");
		auto string_chars_lit= nts("string_chars");
		auto string__lit     = nts("string");
		prods<char> ps, start(start_lit),
			char_(char__lit), escape(escape_lit),
			string_char(string_char_lit),
			string_chars(string_chars_lit),
			string_(string__lit),
			nll(lit<char>{});
		prods<char> a('a'), b('b'), c('c');
		lit<char> q_str{'"'}, esc{'\\'};
		ps(char_, (a | b | c) & ~(prods<char>(q_str) | prods<char>(esc)));
		ps(escape, prods<char>(esc) + (prods<char>(q_str) | prods<char>(esc)));
		ps(string_char, char_ | escape);
		ps(string_chars, (string_char + string_chars) | nll);
		ps(string_, prods<char>(q_str) + string_chars + prods<char>(q_str));
		ps(start, string_);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "\"\"");
	}

	TEST_CASE("simple string") {
		nonterminals<char> nts;
		auto start_lit       = nts("start");
		auto char__lit       = nts("char_");
		auto escape_lit      = nts("escape");
		auto string_char_lit = nts("string_char");
		auto string_chars_lit= nts("string_chars");
		auto string__lit     = nts("string");
		prods<char> ps, start(start_lit),
			char_(char__lit), escape(escape_lit),
			string_char(string_char_lit),
			string_chars(string_chars_lit),
			string_(string__lit),
			nll(lit<char>{});
		prods<char> a('a'), b('b'), c('c');
		lit<char> q_str{'"'}, esc{'\\'};
		ps(char_, (a | b | c) & ~(prods<char>(q_str) | prods<char>(esc)));
		ps(escape, prods<char>(esc) + (prods<char>(q_str) | prods<char>(esc)));
		ps(string_char, char_ | escape);
		ps(string_chars, (string_char + string_chars) | nll);
		ps(string_, prods<char>(q_str) + string_chars + prods<char>(q_str));
		ps(start, string_);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "\"abc\"");
	}

	TEST_CASE("string with escaped quotes") {
		nonterminals<char> nts;
		auto start_lit       = nts("start");
		auto char__lit       = nts("char_");
		auto escape_lit      = nts("escape");
		auto string_char_lit = nts("string_char");
		auto string_chars_lit= nts("string_chars");
		auto string__lit     = nts("string");
		prods<char> ps, start(start_lit),
			char_(char__lit), escape(escape_lit),
			string_char(string_char_lit),
			string_chars(string_chars_lit),
			string_(string__lit),
			nll(lit<char>{});
		prods<char> a('a'), b('b'), c('c');
		lit<char> q_str{'"'}, esc{'\\'};
		ps(char_, (a | b | c) & ~(prods<char>(q_str) | prods<char>(esc)));
		ps(escape, prods<char>(esc) + (prods<char>(q_str) | prods<char>(esc)));
		ps(string_char, char_ | escape);
		ps(string_chars, (string_char + string_chars) | nll);
		ps(string_, prods<char>(q_str) + string_chars + prods<char>(q_str));
		ps(start, string_);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "\"\\\"a\\\"c\\\"\"");
	}

	TEST_CASE("unescaped quote mid-string fails") {
		nonterminals<char> nts;
		auto start_lit       = nts("start");
		auto char__lit       = nts("char_");
		auto escape_lit      = nts("escape");
		auto string_char_lit = nts("string_char");
		auto string_chars_lit= nts("string_chars");
		auto string__lit     = nts("string");
		prods<char> ps, start(start_lit),
			char_(char__lit), escape(escape_lit),
			string_char(string_char_lit),
			string_chars(string_chars_lit),
			string_(string__lit),
			nll(lit<char>{});
		prods<char> a('a'), b('b'), c('c');
		lit<char> q_str{'"'}, esc{'\\'};
		ps(char_, (a | b | c) & ~(prods<char>(q_str) | prods<char>(esc)));
		ps(escape, prods<char>(esc) + (prods<char>(q_str) | prods<char>(esc)));
		ps(string_char, char_ | escape);
		ps(string_chars, (string_char + string_chars) | nll);
		ps(string_, prods<char>(q_str) + string_chars + prods<char>(q_str));
		ps(start, string_);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_rejects(g, "\"\"c\"");
		expect_parse_error(g, "\"\"c\"",
			"Unexpected '\"' at 1:2 (2)");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: overlapping groups with negation
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: overlapping groups") {

	// T | X & ~Y where X => a|1, T => Y, Y => 1
	// 'a' should parse via X & ~Y (Y doesn't match 'a')
	// '1' should parse via T (since X & ~Y fails because Y matches '1')

	TEST_CASE("'a' parses via X & ~Y") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X"), Y_lit = nts("Y"), T_lit = nts("T");
		prods<char> ps, start(start_lit), X(X_lit), Y(Y_lit), T(T_lit);
		prods<char> a('a'), one('1');
		ps(start, T | X & ~Y);
		ps(X, a | one);
		ps(T, Y);
		ps(Y, one);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("'1' parses via T") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X"), Y_lit = nts("Y"), T_lit = nts("T");
		prods<char> ps, start(start_lit), X(X_lit), Y(Y_lit), T(T_lit);
		prods<char> a('a'), one('1');
		ps(start, T | X & ~Y);
		ps(X, a | one);
		ps(T, Y);
		ps(Y, one);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "1");
	}

	TEST_CASE("'a' derivation contains X node") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X"), Y_lit = nts("Y"), T_lit = nts("T");
		prods<char> ps, start(start_lit), X(X_lit), Y(Y_lit), T(T_lit);
		prods<char> a('a'), one('1');
		ps(start, T | X & ~Y);
		ps(X, a | one);
		ps(T, Y);
		ps(Y, one);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parse_contains(g, "a", "X");
	}

	TEST_CASE("'1' derivation contains T node") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X"), Y_lit = nts("Y"), T_lit = nts("T");
		prods<char> ps, start(start_lit), X(X_lit), Y(Y_lit), T(T_lit);
		prods<char> a('a'), one('1');
		ps(start, T | X & ~Y);
		ps(X, a | one);
		ps(T, Y);
		ps(Y, one);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parse_contains(g, "1", "T");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: operator priority via Boolean negation
// Exercises the grammar:
//   expression => sum | mul & ~sum | digit
//   sum        => expression + '+' + expression
//   mul        => expression + '*' + expression
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: operator priority (cascading uncomplete)") {

	TEST_CASE("1+2*3 parses (sum at top, mul*inner)") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		prods<char> ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + '+' + expression);
		ps(mul, expression + '*' + expression);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "1+2*3");
	}

	TEST_CASE("3*2+1 parses (sum at top, mul inner)") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		prods<char> ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + '+' + expression);
		ps(mul, expression + '*' + expression);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "3*2+1");
	}

	TEST_CASE("2*3 parses as pure mul (no sum)") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		prods<char> ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + '+' + expression);
		ps(mul, expression + '*' + expression);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "2*3");
	}

	TEST_CASE("single digit parses") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		prods<char> ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + '+' + expression);
		ps(mul, expression + '*' + expression);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "5");
	}

	TEST_CASE("1+2 parses as sum") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		prods<char> ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + '+' + expression);
		ps(mul, expression + '*' + expression);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "1+2");
	}

	TEST_CASE("1+2+3 parses (nested sums)") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		prods<char> ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + '+' + expression);
		ps(mul, expression + '*' + expression);
		// inherently ambiguous: (1+2)+3 vs 1+(2+3)
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "1+2+3", true);
	}

	TEST_CASE("1*2*3 parses (nested muls, no sum)") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		prods<char> ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + '+' + expression);
		ps(mul, expression + '*' + expression);
		// inherently ambiguous: (1*2)*3 vs 1*(2*3)
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "1*2*3", true);
	}

	TEST_CASE("1+2*3+4 parses (mixed ops)") {
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		prods<char> ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + '+' + expression);
		ps(mul, expression + '*' + expression);
		// inherently ambiguous: top-level sum can split at either '+'
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "1+2*3+4", true);
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: cascading uncomplete does NOT over-cascade
// These tests specifically verify that when a conjunctive production
// fails (e.g., mul & ~sum), the cascade only removes derived items if
// the head nonterminal has NO other valid completion at that span.
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: cascading uncomplete correctness") {

	TEST_CASE("disjunction survives when one alternative's conjunction fails") {
		// Grammar: S => A | B & ~A
		// A => 'x', B => 'x'
		// 'x' matches A, so B & ~A fails. But S => A still holds.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), B_lit = nts("B");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit);
		prods<char> x('x');
		ps(start, A | B & ~A);
		ps(A, x);
		ps(B, x);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "x");
	}

	TEST_CASE("parent nonterminal survives failed sibling conjunction") {
		// Grammar:
		//   S => E
		//   E => P | Q & ~P | 'z'
		//   P => E '+' E
		//   Q => E '*' E
		//
		// For "1+2": E completes via P. Q doesn't complete (no '*').
		// Q & ~P is irrelevant (Q didn't complete). P completes, so
		// E should complete via P. Cascading must not break this.
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit = nts("start");
		auto E_lit = nts("E"), P_lit = nts("P"), Q_lit = nts("Q");
		auto digit_lit = nts("digit");
		prods<char> ps, start(start_lit),
			E(E_lit), P(P_lit), Q(Q_lit), digit(digit_lit);
		ps(start, E);
		ps(E, P | Q & ~P | digit);
		ps(P, E + '+' + E);
		ps(Q, E + '*' + E);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "1+2");
	}

	TEST_CASE("multiple disjunctions: failed conjunction doesn't kill siblings") {
		// Grammar:
		//   S => X | Y & ~X | Z
		//   X => 'a' 'b'
		//   Y => 'a' 'b'   (same as X)
		//   Z => 'c' 'd'
		// For "ab": Y & ~X fails (X also matches). But S => X still works.
		// For "cd": S => Z works directly.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X"), Y_lit = nts("Y"), Z_lit = nts("Z");
		prods<char> ps, start(start_lit), X(X_lit), Y(Y_lit), Z(Z_lit);
		prods<char> a('a'), b('b'), c('c'), d('d');
		ps(start, X | Y & ~X | Z);
		ps(X, a + b);
		ps(Y, a + b);
		ps(Z, c + d);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "ab");
		expect_parses(g, "cd");
		expect_rejects(g, "ac");
	}

	TEST_CASE("chain: E => F, F => G | H & ~G - cascade scoped correctly") {
		// If H & ~G fails at F level, E should still work if G
		// succeeded and F completes via G.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto E_lit = nts("E"), F_lit = nts("F");
		auto G_lit = nts("G"), H_lit = nts("H");
		prods<char> ps, start(start_lit), E(E_lit), F(F_lit), G(G_lit), H(H_lit);
		prods<char> a('a');
		ps(start, E);
		ps(E, F);
		ps(F, G | H & ~G);
		ps(G, a);
		ps(H, a); // same as G, so H & ~G will fail
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("three-level cascade: only propagates when truly needed") {
		// Grammar:
		//   S => Top
		//   Top => E '+' E
		//   E => M | D & ~M | digit
		//   M => E '*' E
		//   D => E '/' E
		//
		// "1+2*3": Top works. E[0..5] completes via Top? No - Top is
		// not E. E completes via digit, M, D & ~M.
		// "1+2": Top works, E via digit on each side.
		nonterminals<char> nts;
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		auto start_lit = nts("start");
		auto Top_lit   = nts("Top");
		auto E_lit     = nts("E");
		auto M_lit     = nts("M"), D_lit = nts("D");
		auto digit_lit = nts("digit");
		prods<char> ps, start(start_lit),
			Top(Top_lit), E(E_lit), M(M_lit), D(D_lit), digit(digit_lit);
		ps(start, Top);
		ps(Top, E + '+' + E);
		ps(E, M | D & ~M | digit);
		ps(M, E + '*' + E);
		ps(D, E + '/' + E);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		expect_parses(g, "1+2");
		expect_parses(g, "1+2*3");
		expect_parses(g, "3*2+1");
		expect_parses(g, "4/2+1");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: edge cases
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: edge cases") {

	TEST_CASE("negation of never-matching rule always succeeds") {
		// S => a & ~b  where b never matches 'a'
		nonterminals<char> nts;
		auto start_lit = nts("start");
		prods<char> ps, start(start_lit);
		prods<char> a('a'), b('b');
		ps(start, a & ~b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("negation of always-matching rule always fails") {
		// S => X & ~X  where X => a
		// 'a' matches X, so ~X fails, so the whole thing fails.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X");
		prods<char> ps, start(start_lit), X(X_lit);
		prods<char> a('a');
		ps(start, X & ~X);
		ps(X, a);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_rejects(g, "a");
	}

	TEST_CASE("empty input with nullable conjunction") {
		// S => null (empty production)
		nonterminals<char> nts;
		auto start_lit = nts("start");
		prods<char> ps, start(start_lit), nll(lit<char>{});
		ps(start, nll);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "");
	}

	TEST_CASE("multiple negations in sequence") {
		// S => (a|b|c) & ~a & ~b  =>  only c should match
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X");
		prods<char> ps, start(start_lit), X(X_lit);
		prods<char> a('a'), b('b'), c('c');
		ps(start, X & ~a & ~b);
		ps(X, a | b | c);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "c");
		expect_rejects(g, "a");
		expect_rejects(g, "b");
	}
}

// ---------------------------------------------------------------------------
// char32_t terminal variants - same grammars, parser<char, char32_t>
// Input is char* (UTF-8), terminals are char32_t via utf8_to_u32_conv.
// ---------------------------------------------------------------------------

using c32_grammar = grammar<char, char32_t>;
using c32_prods   = prods<char, char32_t>;
using c32_cc      = char_class_fns<char32_t>;

static bool c32_parses(c32_grammar& g, const string& input,
	bool allow_ambiguity = false)
{
	parser<char, char32_t>::options o;
	o.codec.decode = idni::utf8_to_u32_conv;
	o.codec.encode = idni::u32_to_utf8_conv;
	parser<char, char32_t> p(g, o);
	auto r = p.parse(input.c_str(), input.size());
	if (!r.found || allow_ambiguity) return r.found;
#if !defined(TAU_TEST_NO_AUTODISAMBG)
	// Shadow check: temporarily disable auto_disambiguate.
	bool saved_ad = g.opt.auto_disambiguate;
	g.opt.auto_disambiguate = false;
	parser<char, char32_t>::options po_shadow{};
	po_shadow.codec.decode = idni::utf8_to_u32_conv;
	po_shadow.codec.encode = idni::u32_to_utf8_conv;
	parser<char, char32_t> p_shadow(g, po_shadow);
	g.opt.auto_disambiguate = saved_ad;
	auto r_shadow = p_shadow.parse(input.c_str(), input.size());
	if (r_shadow.found && r_shadow.is_ambiguous()) return false;
#endif
	return true;
}

TEST_SUITE("boolean grammar: char32_t terminals") {

	TEST_CASE("basic negation: a & ~b accepts 'a'") {
		nonterminals<char, char32_t> nts;
		auto start_lit = nts("start");
		c32_prods ps, start(start_lit);
		c32_prods a(U'a'), b(U'b');
		ps(start, a & ~b);
		c32_grammar g(nts, ps, start, {}, {});
		CHECK(c32_parses(g, "a"));
	}

	TEST_CASE("basic negation: a & ~b rejects 'b'") {
		nonterminals<char, char32_t> nts;
		auto start_lit = nts("start");
		c32_prods ps, start(start_lit);
		c32_prods a(U'a'), b(U'b');
		ps(start, a & ~b);
		c32_grammar g(nts, ps, start, {}, {});
		CHECK_FALSE(c32_parses(g, "b"));
	}

	TEST_CASE("negation with nonterminals: X & ~b") {
		nonterminals<char, char32_t> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X");
		c32_prods ps, start(start_lit), X(X_lit);
		c32_prods a(U'a'), b(U'b');
		ps(start, X & ~b);
		ps(X, a | b);
		c32_grammar g(nts, ps, start, {}, {});
		CHECK(c32_parses(g, "a"));
		CHECK_FALSE(c32_parses(g, "b"));
	}

	TEST_CASE("operator priority: expression => sum | mul & ~sum | digit") {
		nonterminals<char, char32_t> nts;
		c32_cc cc = predefined_char_classes<char, char32_t>(
			{ "eof", "alpha", "alnum", "digit",
			  "printable" }, nts);
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		c32_prods ps, start(start_lit),
			expression(expression_lit), sum(sum_lit), mul(mul_lit), digit(digit_lit);
		ps(start, expression);
		ps(expression, sum | mul & ~sum | digit);
		ps(sum, expression + U'+' + expression);
		ps(mul, expression + U'*' + expression);
		c32_grammar g(nts, ps, start, cc, {});
		CHECK(c32_parses(g, "1+2*3"));
		CHECK(c32_parses(g, "3*2+1"));
		CHECK(c32_parses(g, "2*3"));
		CHECK(c32_parses(g, "5"));
		CHECK(c32_parses(g, "1+2"));
	}

	TEST_CASE("cascading: disjunction survives failed conjunction") {
		nonterminals<char, char32_t> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), B_lit = nts("B");
		c32_prods ps, start(start_lit), A(A_lit), B(B_lit);
		c32_prods x(U'x');
		ps(start, A | B & ~A);
		ps(A, x);
		ps(B, x);
		c32_grammar g(nts, ps, start, {}, {});
		CHECK(c32_parses(g, "x"));
	}

	TEST_CASE("cascading: chain E=>F, F=>G | H & ~G") {
		nonterminals<char, char32_t> nts;
		auto start_lit = nts("start");
		auto E_lit = nts("E"), F_lit = nts("F");
		auto G_lit = nts("G"), H_lit = nts("H");
		c32_prods ps, start(start_lit), E(E_lit), F(F_lit), G(G_lit), H(H_lit);
		c32_prods a(U'a');
		ps(start, E);
		ps(E, F);
		ps(F, G | H & ~G);
		ps(G, a);
		ps(H, a);
		c32_grammar g(nts, ps, start, {}, {});
		CHECK(c32_parses(g, "a"));
	}

	TEST_CASE("keyword exclusion: identifier => chars & ~keyword") {
		nonterminals<char, char32_t> nts;
		c32_cc cc = predefined_char_classes<char, char32_t>(
			{ "eof", "alpha", "alnum", "digit",
			  "printable" }, nts);
		auto start_lit      = nts("start");
		auto identifier_lit = nts("identifier");
		auto keyword_lit    = nts("keyword");
		auto chars_lit      = nts("chars");
		auto alpha_lit      = nts("alpha");
		auto alnum_lit      = nts("alnum");
		c32_prods ps, start(start_lit),
			identifier(identifier_lit), keyword(keyword_lit),
			chars(chars_lit), alpha(alpha_lit), alnum(alnum_lit);
		ps(start, identifier | keyword);
		ps(identifier, chars & ~keyword);
		ps(chars, alpha | (chars + alnum));
		ps(keyword, {"print"});
		c32_grammar g(nts, ps, start, cc, {});
		CHECK(c32_parses(g, "var123"));
		CHECK(c32_parses(g, "print"));
	}

	TEST_CASE("multiple negations: (a|b|c) & ~a & ~b => only c") {
		nonterminals<char, char32_t> nts;
		auto start_lit = nts("start");
		auto X_lit = nts("X");
		c32_prods ps, start(start_lit), X(X_lit);
		c32_prods a(U'a'), b(U'b'), c(U'c');
		ps(start, X & ~a & ~b);
		ps(X, a | b | c);
		c32_grammar g(nts, ps, start, {}, {});
		CHECK(c32_parses(g, "c"));
		CHECK_FALSE(c32_parses(g, "a"));
		CHECK_FALSE(c32_parses(g, "b"));
	}
}

// ---------------------------------------------------------------------------

TEST_SUITE("regression: cascade-uncomplete count guard") {

	TEST_CASE("simple disjunct survives sibling conjunctive failure") {
		// A => 'a' (simple, succeeds for "a") | A & ~B (conjunctive,
		// fails because B also matches 'a'). The simple disjunct's
		// completion of A at [0,1] advanced [start => A .]; cascade
		// triggered by the conjunctive disjunct's failure must not
		// retract it. With the guard removed, [start => A .] is
		// retracted and `r.found` is false.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), B_lit = nts("B");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit);
		prods<char> a('a');
		ps(start, A);
		ps(A, a | (A & ~B));
		ps(B, a);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("operator priority - sum-derived expression survives mul&~sum failure") {
		nonterminals<char> nts;
		auto start_lit      = nts("start");
		auto expression_lit = nts("expression");
		auto sum_lit        = nts("sum");
		auto mul_lit        = nts("mul");
		auto digit_lit      = nts("digit");
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "eof", "alpha", "alnum", "digit", "printable" }, nts);
		prods<char> ps, start(start_lit),
			expression(expression_lit),
			sum(sum_lit), mul(mul_lit),
			digit(digit_lit);
		ps(start_lit, expression);
		ps(expression_lit, sum | mul & ~sum | digit);
		ps(sum_lit, expression + '+' + expression);
		ps(mul_lit, expression + '*' + expression);
		grammar<char> g(nts, ps, start, cc, make_grammar_options());
		g.opt.auto_disambiguate = false;
		parser<char> p(g);
		auto r = p.parse("1+2*3", 5);
		REQUIRE(r.found);
		CHECK_FALSE(r.is_ambiguous());
	}

	TEST_CASE("incr_gen_forest: simple disjunct survives sibling failure") {
		// Same simple-disjunct pattern as above, but with the
		// incremental forest generator enabled. The forest is built
		// per-character; cascade must keep the start advancement alive
		// long enough for the per-character build to pick it up.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), B_lit = nts("B");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit);
		prods<char> a('a');
		ps(start_lit, A);
		ps(A_lit, a | (A & ~B));
		ps(B_lit, a);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		parser<char>::options o;
		o.incr_gen_forest = true;
		parser<char> p(g, o);
		auto r = p.parse("a", 1);
		CHECK(r.found);
	}

	TEST_CASE("binarize: simple disjunct survives sibling failure") {
		// Same pattern with the binarized parser. binarize is a
		// different forest-building path (binarize_comb) and consults
		// rsorted_citem; verifies cascade interacts correctly with it.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), B_lit = nts("B");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit);
		prods<char> a('a');
		ps(start_lit, A);
		ps(A_lit, a | (A & ~B));
		ps(B_lit, a);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		parser<char>::options o;
		o.binarize = true;
		parser<char> p(g, o);
		auto r = p.parse("a", 1);
		CHECK(r.found);
	}

	TEST_CASE("gc: simple disjunct survives sibling failure") {
		// Same pattern with garbage collection enabled. GC shares the
		// refi/gcready bookkeeping that cascade also touches.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), B_lit = nts("B");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit);
		prods<char> a('a');
		ps(start_lit, A);
		ps(A_lit, a | (A & ~B));
		ps(B_lit, a);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		parser<char>::options o;
		parser<char>::parse_options paropts;
		paropts.enable_gc = true;
		parser<char> p(g, o);
		auto r = p.parse("a", 1, paropts);
		CHECK(r.found);
	}

	TEST_CASE("two-level head: cascade through chained non-conj parents") {
		// Top => Mid; Mid => A. When A's conj disjunct fails, the
		// cascade walks Mid's deps then start's deps. The count-guard
		// must short-circuit AT EACH LEVEL because each NT was also
		// completed via the simple disjunct's success path. If the
		// guard is missing or per-level deps are corrupted, this fails.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto Mid_lit = nts("Mid"), A_lit = nts("A"), B_lit = nts("B");
		prods<char> ps, start(start_lit), Mid(Mid_lit), A(A_lit), B(B_lit);
		prods<char> a('a');
		ps(start, Mid);
		ps(Mid, A);
		ps(A, a | (A & ~B));
		ps(B, a);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("three-level head: deeper chain still survives") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto L1_lit = nts("L1"), L2_lit = nts("L2"), L3_lit = nts("L3");
		auto A_lit = nts("A"), B_lit = nts("B");
		prods<char> ps, start(start_lit),
			L1(L1_lit), L2(L2_lit), L3(L3_lit), A(A_lit), B(B_lit);
		prods<char> a('a');
		ps(start, L1);
		ps(L1, L2);
		ps(L2, L3);
		ps(L3, A);
		ps(A, a | (A & ~B));
		ps(B, a);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("two failing conjunctive disjuncts on same head") {
		// A has THREE prods: simple ('a'), conj1 (A & ~B), conj2
		// (A & ~C). For "a", both conj prods fail (B and C both match
		// 'a'). Each failure decrements count[A,0,1] separately.
		// The guard must keep count > 0 (the simple disjunct keeps
		// count incremented per inner-loop re-process).
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), B_lit = nts("B"), C_lit = nts("C");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit), C(C_lit);
		prods<char> a('a');
		ps(start, A);
		ps(A, a | (A & ~B) | (A & ~C));
		ps(B, a);
		ps(C, a);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}

	TEST_CASE("conjunction failing at one span doesn't damage another span") {
		// A => Block & ~Stop where Block matches one or more 'x's and
		// Stop matches exactly two 'x's. So:
		//   "x"   - Block ok, Stop no - conj succeeds, A[0,1] ok
		//   "xx"  - Block ok, Stop ok - conj FAILS at [0,2]
		//   "xxx" - Block ok, Stop no at [0,3] - conj succeeds at [0,3]
		// For "xxx" the parser also encounters A's conj failing at
		// many sub-spans (e.g. trying to start A at position 1 with
		// Block[1,3] and Stop[1,3]) but the parse at [0,3] should
		// still find A. Cascade per-span bookkeeping verified.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), Block_lit = nts("Block"), Stop_lit = nts("Stop");
		prods<char> ps, start(start_lit), A(A_lit), Block(Block_lit), Stop(Stop_lit);
		prods<char> x('x');
		ps(start, A);
		ps(A, Block & ~Stop);
		ps(Block, x | (x + Block));
		ps(Stop, x + x);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "x");
		expect_rejects(g, "xx");
		expect_parses(g, "xxx");
	}

	TEST_CASE("nullable conjunct combined with negation") {
		// A nullable B inside a conjunction. Verifies cascade
		// machinery doesn't choke on zero-width spans.
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto A_lit = nts("A"), B_lit = nts("B"), C_lit = nts("C");
		prods<char> ps, start(start_lit), A(A_lit), B(B_lit), C(C_lit);
		prods<char> a('a'), eps;
		ps(start, A);
		ps(A, a | (B & ~C));
		ps(B, eps | a);     // nullable
		ps(C, a);
		// "a" parses via simple disjunct A => 'a'. The B&~C path
		// fails: B matches 'a', but C also matches 'a' so ~C fails.
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		expect_parses(g, "a");
	}
}

// ---------------------------------------------------------------------------
// Boolean grammar examples from the literature.
//
// Sources:
//   - Okhotin, "Conjunctive grammars" (J. Automata, Languages and
//     Combinatorics, 2001) - introduced conjunction (&) for CFGs.
//   - Okhotin, "Boolean grammars" (Information and Computation, 2004) -
//     adds negation (~) on top.
//
// These are the canonical languages cited in the literature as proof that
// Boolean / conjunctive grammars are strictly more expressive than CFGs:
// each contains a non-context-free language captured by intersection of
// CFLs (& between context-free disjuncts).
// ---------------------------------------------------------------------------

TEST_SUITE("boolean grammar: literature") {

	// Okhotin (2001), the canonical proof example: {a^n b^n c^n | n >= 0}.
	//
	// Construction: S = AB & DC where
	//   AB = a* {b^k c^k | k>=0}    (a-block, then matched b/c)
	//   DC = {a^k b^k | k>=0} c*    (matched a/b, then c-block)
	// Their intersection forces #a = #b = #c.
	TEST_CASE("Okhotin 2001: a^n b^n c^n via S = AB & DC") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto AB_lit = nts("AB"), DC_lit = nts("DC");
		auto A_lit = nts("A"), B_lit = nts("B");
		auto C_lit = nts("C"), D_lit = nts("D");
		prods<char> ps, start(start_lit),
			AB(AB_lit), DC(DC_lit),
			A(A_lit), B(B_lit), C(C_lit), D(D_lit), eps(lit<char>{});
		prods<char> a('a'), b('b'), c('c');
		ps(start, AB & DC);
		ps(AB, A + B);
		ps(DC, D + C);
		ps(A, (a + A) | eps);          // A = a*
		ps(B, (b + B + c) | eps);      // B = {b^k c^k}
		ps(C, (c + C) | eps);          // C = c*
		ps(D, (a + D + b) | eps);      // D = {a^k b^k}
		grammar<char> g(nts, ps, start, {}, make_grammar_options());

		// allow_ambiguity=true: the construction generates multiple
		// valid parse trees in the forest (different ways the matched
		// {a^k b^k c^k} structure can be split between AB and DC),
		// which is fine - what matters for membership is r.found.
		expect_parses(g, "", true);
		expect_parses(g, "abc", true);
		expect_parses(g, "aabbcc", true);
		expect_parses(g, "aaabbbccc", true);
		expect_rejects(g, "ab");
		expect_rejects(g, "abcc");
		expect_rejects(g, "aabbc");
		expect_rejects(g, "aabcc");
		expect_rejects(g, "abbc");
	}

	// Cross-serial dependencies (the "Swiss-German" pattern):
	// {a^m b^n c^m d^n | m,n >= 0}.
	//
	// Construction: S = L1 & L2 where
	//   L1 = {a^m b^* c^m d^*} (a/c counts match)
	//   L2 = {a^* b^n c^* d^n} (b/d counts match)
	TEST_CASE("Cross-serial: a^m b^n c^m d^n via L1 & L2") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto P_lit = nts("P"), B1_lit = nts("B1"), R_lit = nts("R");
		auto A2_lit = nts("A2"), Q_lit = nts("Q"), C2_lit = nts("C2");
		prods<char> ps, start(start_lit),
			P(P_lit), B1(B1_lit), R(R_lit),
			A2(A2_lit), Q(Q_lit), C2(C2_lit), eps(lit<char>{});
		prods<char> a('a'), b('b'), c('c'), d('d');
		// L1 inlined: P + R, with P = a^m B1 c^m, B1 = b*, R = d*
		// L2 inlined: A2 + Q, with A2 = a*, Q = b^n C2 d^n, C2 = c*
		ps(start, (P + R) & (A2 + Q));
		ps(P, (a + P + c) | B1);
		ps(B1, (b + B1) | eps);
		ps(R, (d + R) | eps);
		ps(A2, (a + A2) | eps);
		ps(Q, (b + Q + d) | C2);
		ps(C2, (c + C2) | eps);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());

		expect_parses(g, "", true);
		expect_parses(g, "abcd", true);
		expect_parses(g, "aabbccdd", true);
		expect_parses(g, "aabccd", true);   // m=2, n=1
		expect_parses(g, "abbcdd", true);   // m=1, n=2
		expect_rejects(g, "abc");    // missing d
		expect_rejects(g, "abcdd");  // n_b != n_d
		expect_rejects(g, "aabcd");  // n_a != n_c
		expect_rejects(g, "ba");     // wrong order
	}

	// Quadruple agreement: {a^n b^n c^n d^n | n >= 0}.
	// Three-way intersection forces all four counts equal.
	//
	//   L1 = {a^n b^n c^* d^*}  (a/b match)
	//   L2 = {a^* b^* c^n d^n}  (c/d match)
	//   L3 = {a^* b^n c^n d^*}  (b/c match)
	TEST_CASE("a^n b^n c^n d^n via three-way intersection") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto S1_lit = nts("S1"), P1_lit = nts("P1"), R1_lit = nts("R1"), D1_lit = nts("D1");
		auto S2_lit = nts("S2"), A2_lit = nts("A2"), B2_lit = nts("B2"), Q2_lit = nts("Q2");
		auto S3_lit = nts("S3"), A3_lit = nts("A3"), Q3_lit = nts("Q3"), D3_lit = nts("D3");
		prods<char> ps, start(start_lit),
			S1(S1_lit), P1(P1_lit), R1(R1_lit), D1(D1_lit),
			S2(S2_lit), A2(A2_lit), B2(B2_lit), Q2(Q2_lit),
			S3(S3_lit), A3(A3_lit), Q3(Q3_lit), D3(D3_lit),
			eps(lit<char>{});
		prods<char> a('a'), b('b'), c('c'), d('d');
		ps(start, S1 & S2 & S3);
		// S1 = {a^n b^n} c* d*
		ps(S1, P1 + R1);
		ps(P1, (a + P1 + b) | eps);
		ps(R1, (c + R1) | D1);
		ps(D1, (d + D1) | eps);
		// S2 = a* b* {c^n d^n}
		ps(S2, A2 + B2 + Q2);
		ps(A2, (a + A2) | eps);
		ps(B2, (b + B2) | eps);
		ps(Q2, (c + Q2 + d) | eps);
		// S3 = a* {b^n c^n} d*
		ps(S3, A3 + Q3 + D3);
		ps(A3, (a + A3) | eps);
		ps(Q3, (b + Q3 + c) | eps);
		ps(D3, (d + D3) | eps);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());

		expect_parses(g, "", true);
		expect_parses(g, "abcd", true);
		expect_parses(g, "aabbccdd", true);
		expect_parses(g, "aaabbbcccddd", true);
		expect_rejects(g, "abc");
		expect_rejects(g, "abcdd");
		expect_rejects(g, "aabbccd");
		expect_rejects(g, "aabbcd");
		expect_rejects(g, "abbcd");
	}

	// Complement of a finite singleton: Σ* \ {ab} where Σ = {a, b}.
	// Demonstrates negation gives access to the complement.
	TEST_CASE("complement: Σ* \\ {ab} over Σ={a,b}") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto Any_lit = nts("Any"), Eq_lit = nts("Eq");
		prods<char> ps, start(start_lit),
			Any(Any_lit), Eq(Eq_lit), eps(lit<char>{});
		prods<char> a('a'), b('b');
		ps(start, Any & ~Eq);
		ps(Any, (a + Any) | (b + Any) | eps);
		ps(Eq, a + b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());

		expect_parses(g, "");
		expect_parses(g, "a");
		expect_parses(g, "b");
		expect_parses(g, "ba");
		expect_parses(g, "abb");
		expect_parses(g, "aab");
		expect_parses(g, "bab");
		expect_parses(g, "aaaa");
		expect_rejects(g, "ab");
	}

	// Σ* \ Σ* "ab" Σ*: strings over {a, b} not containing "ab" as substring.
	// (Equivalent to the regular language b* a*.)
	TEST_CASE("substring exclusion: no 'ab' substring") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto Any_lit = nts("Any"), Bad_lit = nts("Bad");
		prods<char> ps, start(start_lit),
			Any(Any_lit), Bad(Bad_lit), eps(lit<char>{});
		prods<char> a('a'), b('b');
		ps(start, Any & ~Bad);
		ps(Any, (a + Any) | (b + Any) | eps);
		ps(Bad, Any + a + b + Any);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());

		expect_parses(g, "");
		expect_parses(g, "a");
		expect_parses(g, "b");
		expect_parses(g, "aa");
		expect_parses(g, "bb");
		expect_parses(g, "ba");
		expect_parses(g, "bba");
		expect_parses(g, "bbaa");
		expect_parses(g, "aaaa");
		expect_rejects(g, "ab");
		expect_rejects(g, "abb");
		expect_rejects(g, "abba");
		expect_rejects(g, "bab");
		expect_rejects(g, "bbab");
		expect_rejects(g, "bbabaa");
	}
}

// ---------------------------------------------------------------------------
// Exhaustive fuzz tests: enumerate all small inputs over a small alphabet
// and assert the parser's r.found agrees with a hand-coded reference oracle
// for the language. Covers the canonical Boolean-grammar examples.
//
// Length cap is small (8) because Earley on Boolean grammars is O(n^3) in
// the worst case and the exhaustive count is |Σ|^n. With Σ={a,b,c} and
// n<=8: sum_{k=0..8} 3^k = 9841 inputs. With Σ={a,b}: 511 inputs.
// ---------------------------------------------------------------------------

namespace {

// Generate the idx-th string of length len over alphabet (idx in base |alpha|).
static std::string nth_string(size_t idx, size_t len, const char* alpha,
	size_t alpha_n)
{
	std::string s(len, alpha[0]);
	for (size_t i = 0; i < len; i++) {
		s[i] = alpha[idx % alpha_n];
		idx /= alpha_n;
	}
	return s;
}

// Reference oracles.

static bool ref_anbncn(const std::string& s) {
	if (s.size() % 3 != 0) return false;
	size_t k = s.size() / 3;
	for (size_t i = 0; i < k; i++)
		if (s[i] != 'a' || s[k + i] != 'b' || s[2*k + i] != 'c')
			return false;
	return true;
}

static bool ref_ambncmdn(const std::string& s) {
	// a^m b^n c^m d^n
	size_t i = 0, m = 0, n = 0, m2 = 0, n2 = 0;
	while (i < s.size() && s[i] == 'a') { ++m; ++i; }
	while (i < s.size() && s[i] == 'b') { ++n; ++i; }
	while (i < s.size() && s[i] == 'c') { ++m2; ++i; }
	while (i < s.size() && s[i] == 'd') { ++n2; ++i; }
	return i == s.size() && m == m2 && n == n2;
}

static bool ref_anbncndn(const std::string& s) {
	size_t i = 0, n_a = 0, n_b = 0, n_c = 0, n_d = 0;
	while (i < s.size() && s[i] == 'a') { ++n_a; ++i; }
	while (i < s.size() && s[i] == 'b') { ++n_b; ++i; }
	while (i < s.size() && s[i] == 'c') { ++n_c; ++i; }
	while (i < s.size() && s[i] == 'd') { ++n_d; ++i; }
	return i == s.size() && n_a == n_b && n_b == n_c && n_c == n_d;
}

static bool ref_no_ab_substring(const std::string& s) {
	for (size_t i = 0; i + 1 < s.size(); i++)
		if (s[i] == 'a' && s[i + 1] == 'b') return false;
	return true;
}

static bool ref_not_ab(const std::string& s) { return s != "ab"; }

} // anon namespace

TEST_SUITE("boolean grammar: fuzz") {

	TEST_CASE("exhaustive a^n b^n c^n over {a,b,c} length 0..7") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto AB_lit = nts("AB"), DC_lit = nts("DC");
		auto A_lit = nts("A"), B_lit = nts("B");
		auto C_lit = nts("C"), D_lit = nts("D");
		prods<char> ps, start(start_lit),
			AB(AB_lit), DC(DC_lit),
			A(A_lit), B(B_lit), C(C_lit), D(D_lit), eps(lit<char>{});
		prods<char> a('a'), b('b'), c('c');
		ps(start, AB & DC);
		ps(AB, A + B);
		ps(DC, D + C);
		ps(A, (a + A) | eps);
		ps(B, (b + B + c) | eps);
		ps(C, (c + C) | eps);
		ps(D, (a + D + b) | eps);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		size_t mismatches = 0;
		for (size_t len = 0; len <= 7; len++) {
			size_t total = 1;
			for (size_t i = 0; i < len; i++) total *= 3;
			for (size_t idx = 0; idx < total; idx++) {
				std::string s = nth_string(idx, len, "abc", 3);
				bool expected = ref_anbncn(s);
				parser<char> p(g, make_parser_options());
				auto r = p.parse(s.c_str(), s.size());
				if (r.found != expected) {
					MESSAGE("input=\"" << s << "\" expected="
						<< expected << " got=" << r.found);
					++mismatches;
				}
			}
		}
		CHECK(mismatches == 0);
	}

	TEST_CASE("exhaustive a^m b^n c^m d^n over {a,b,c,d} length 0..6") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto L1_lit = nts("L1"), L2_lit = nts("L2");
		auto P_lit = nts("P"), B1_lit = nts("B1"), R_lit = nts("R");
		auto A2_lit = nts("A2"), Q_lit = nts("Q"), C2_lit = nts("C2");
		prods<char> ps, start(start_lit),
			L1(L1_lit), L2(L2_lit),
			P(P_lit), B1(B1_lit), R(R_lit),
			A2(A2_lit), Q(Q_lit), C2(C2_lit),
			eps(lit<char>{});
		prods<char> a('a'), b('b'), c('c'), d('d');
		ps(start, L1 & L2);
		ps(L1, P + R);
		ps(P, (a + P + c) | B1);
		ps(B1, (b + B1) | eps);
		ps(R, (d + R) | eps);
		ps(L2, A2 + Q);
		ps(A2, (a + A2) | eps);
		ps(Q, (b + Q + d) | C2);
		ps(C2, (c + C2) | eps);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		size_t mismatches = 0;
		for (size_t len = 0; len <= 6; len++) {
			size_t total = 1;
			for (size_t i = 0; i < len; i++) total *= 4;
			for (size_t idx = 0; idx < total; idx++) {
				std::string s = nth_string(idx, len, "abcd", 4);
				bool expected = ref_ambncmdn(s);
				parser<char> p(g, make_parser_options());
				auto r = p.parse(s.c_str(), s.size());
				if (r.found != expected) {
					MESSAGE("input=\"" << s << "\" expected="
						<< expected << " got=" << r.found);
					++mismatches;
				}
			}
		}
		CHECK(mismatches == 0);
	}

	TEST_CASE("exhaustive a^n b^n c^n d^n over {a,b,c,d} length 0..6") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto S1_lit = nts("S1"), P1_lit = nts("P1"), R1_lit = nts("R1"), D1_lit = nts("D1");
		auto S2_lit = nts("S2"), A2_lit = nts("A2"), B2_lit = nts("B2"), Q2_lit = nts("Q2");
		auto S3_lit = nts("S3"), A3_lit = nts("A3"), Q3_lit = nts("Q3"), D3_lit = nts("D3");
		prods<char> ps, start(start_lit),
			S1(S1_lit), P1(P1_lit), R1(R1_lit), D1(D1_lit),
			S2(S2_lit), A2(A2_lit), B2(B2_lit), Q2(Q2_lit),
			S3(S3_lit), A3(A3_lit), Q3(Q3_lit), D3(D3_lit),
			eps(lit<char>{});
		prods<char> a('a'), b('b'), c('c'), d('d');
		ps(start, S1 & S2 & S3);
		ps(S1, P1 + R1);
		ps(P1, (a + P1 + b) | eps);
		ps(R1, (c + R1) | D1);
		ps(D1, (d + D1) | eps);
		ps(S2, A2 + B2 + Q2);
		ps(A2, (a + A2) | eps);
		ps(B2, (b + B2) | eps);
		ps(Q2, (c + Q2 + d) | eps);
		ps(S3, A3 + Q3 + D3);
		ps(A3, (a + A3) | eps);
		ps(Q3, (b + Q3 + c) | eps);
		ps(D3, (d + D3) | eps);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		size_t mismatches = 0;
		for (size_t len = 0; len <= 6; len++) {
			size_t total = 1;
			for (size_t i = 0; i < len; i++) total *= 4;
			for (size_t idx = 0; idx < total; idx++) {
				std::string s = nth_string(idx, len, "abcd", 4);
				bool expected = ref_anbncndn(s);
				parser<char> p(g, make_parser_options());
				auto r = p.parse(s.c_str(), s.size());
				if (r.found != expected) {
					MESSAGE("input=\"" << s << "\" expected="
						<< expected << " got=" << r.found);
					++mismatches;
				}
			}
		}
		CHECK(mismatches == 0);
	}

	TEST_CASE("exhaustive complement of {ab} over {a,b} length 0..8") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto Any_lit = nts("Any"), Eq_lit = nts("Eq");
		prods<char> ps, start(start_lit),
			Any(Any_lit), Eq(Eq_lit), eps(lit<char>{});
		prods<char> a('a'), b('b');
		ps(start, Any & ~Eq);
		ps(Any, (a + Any) | (b + Any) | eps);
		ps(Eq, a + b);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		size_t mismatches = 0;
		for (size_t len = 0; len <= 8; len++) {
			size_t total = 1u << len;
			for (size_t idx = 0; idx < total; idx++) {
				std::string s = nth_string(idx, len, "ab", 2);
				bool expected = ref_not_ab(s);
				parser<char> p(g, make_parser_options());
				auto r = p.parse(s.c_str(), s.size());
				if (r.found != expected) {
					MESSAGE("input=\"" << s << "\" expected="
						<< expected << " got=" << r.found);
					++mismatches;
				}
			}
		}
		CHECK(mismatches == 0);
	}

	TEST_CASE("exhaustive 'no ab substring' over {a,b} length 0..8") {
		nonterminals<char> nts;
		auto start_lit = nts("start");
		auto Any_lit = nts("Any"), Bad_lit = nts("Bad");
		prods<char> ps, start(start_lit),
			Any(Any_lit), Bad(Bad_lit), eps(lit<char>{});
		prods<char> a('a'), b('b');
		ps(start, Any & ~Bad);
		ps(Any, (a + Any) | (b + Any) | eps);
		ps(Bad, Any + a + b + Any);
		grammar<char> g(nts, ps, start, {}, make_grammar_options());
		size_t mismatches = 0;
		for (size_t len = 0; len <= 8; len++) {
			size_t total = 1u << len;
			for (size_t idx = 0; idx < total; idx++) {
				std::string s = nth_string(idx, len, "ab", 2);
				bool expected = ref_no_ab_substring(s);
				parser<char> p(g, make_parser_options());
				auto r = p.parse(s.c_str(), s.size());
				if (r.found != expected) {
					MESSAGE("input=\"" << s << "\" expected="
						<< expected << " got=" << r.found);
					++mismatches;
				}
			}
		}
		CHECK(mismatches == 0);
	}
}

