// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for TGF grammar parsing - char terminals, literals, comments,
// EBNF (optional, zero-or-any, plus, asterisk, group), directives,
// TGF-syntax boolean operators (&, ~) and the bound grammar options.
//
// Stress tests live in doctest_stress.cpp.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"
#include "testing.h"
#include "grammar_inspector.h"
#include "tgf/parser_gen.h"

#include <filesystem>
#include <sstream>

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
	set_grammar_option(g_shadow, "auto-disambiguate", false);
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
			"start => digit _ __.\n"
			"_ => space.\n"
			"__ => space space.\n";
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts, string(g_tgf));
		REQUIRE(gr.has_value());
		check_cc_nt_ordering(gr.value(), nts);
	}

	TEST_CASE("from_string_presplit: nul and char classes precede trim symbols") {
		const char* g_tgf =
			"@use char class digit, space.\n"
			"@trim __, _.\n"
			"start => digit _ __.\n"
			"_ => space.\n"
			"__ => space space.\n";
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string_presplit(nts, string(g_tgf));
		REQUIRE(gr.has_value());
		check_cc_nt_ordering(gr.value(), nts);
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: grammar options
// ---------------------------------------------------------------------------

static const char* options_tgf =
	"start => x ws x.\n"
	"x => 'x'.\n"
	"ws => ' '.\n";

static grammar<char> options_grammar(nonterminals<char>& nts) {
	auto gr = tgf<char>::from_string(nts, string(options_tgf));
	REQUIRE(gr.has_value());
	return std::move(gr).value();
}

static string text_of(options_repository& repo, string_view name) {
	auto t = repo.get_text(name);
	REQUIRE(t.has_value());
	return t.value();
}

static size_t id_of(const nonterminals<char>& nts, const string& name) {
	auto id = nts.find(name);
	REQUIRE(id.has_value());
	return *id;
}

TEST_SUITE("grammar options") {

	TEST_CASE("bind_options binds the grammar fields") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		REQUIRE(g.bind_options().has_value());
		CHECK(text_of(repo, "trim") == "");
		REQUIRE(repo.set_text("trim", "ws").has_value());
		CHECK(g.opt().shaping.to_trim == set<size_t>{ id_of(nts, "ws") });
		CHECK(text_of(repo, "trim") == "ws");
		REQUIRE(repo.set_text("inline", "start > x").has_value());
		CHECK(g.opt().shaping.to_inline == set<vector<size_t>>{
			{ id_of(nts, "start"), id_of(nts, "x") } });
		CHECK(text_of(repo, "inline") == "start > x");
		repo.set("auto-disambiguate", false);
		CHECK(!g.opt().auto_disambiguate);
		REQUIRE(repo.set_text("enabled-productions", "a, b").has_value());
		CHECK(g.opt().enabled_guards == set<string>{ "a", "b" });
		REQUIRE(repo.set_text("derive-char-classes", "false").has_value());
		CHECK(!g.opt().derive_char_classes);
	}

	TEST_CASE("a name the grammar does not have is an error") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		REQUIRE(g.bind_options().has_value());
		const size_t size = nts.size();
		auto r = repo.set_text("trim", "nope");
		CHECK(r.has_error());
		ostringstream printed;
		r.report().print(printed);
		CHECK(printed.str().find("nope") != string::npos);
		CHECK(g.opt().shaping.to_trim.empty());
		CHECK(nts.size() == size);
	}

	TEST_CASE("a write before the bind goes into the field") {
		options_repository repo;
		options_scope scope(repo);
		REQUIRE(repo.declare(grammar_option_set).has_value());
		REQUIRE(repo.set_text("trim", "ws").has_value());
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		REQUIRE(g.bind_options().has_value());
		CHECK(g.opt().shaping.to_trim == set<size_t>{ id_of(nts, "ws") });
	}

	TEST_CASE("a moved grammar keeps its options bound") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		REQUIRE(g.bind_options().has_value());
		grammar<char> g2 = std::move(g);
		REQUIRE(repo.set_text("trim", "x").has_value());
		CHECK(g2.opt().shaping.to_trim == set<size_t>{ id_of(nts, "x") });
	}

	TEST_CASE("a surface of the tool stays for the next grammar") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		{
			grammar<char> g = options_grammar(nts);
			REQUIRE(g.bind_options().has_value());
			REQUIRE(repo.add_surface("trim", { "parse", 't' })
				.has_value());
		}
		grammar<char> g2 = options_grammar(nts);
		CHECK(g2.bind_options().has_value());
	}

	TEST_CASE("the last value stays after the grammar ends") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		{
			grammar<char> g = options_grammar(nts);
			REQUIRE(g.bind_options().has_value());
			REQUIRE(repo.set_text("trim", "ws").has_value());
		}
		CHECK(text_of(repo, "trim") == "ws");
	}

	static diagnostics::result<grammar<char>> load(nonterminals<char>& nts,
		const string& directives)
	{
		return tgf<char>::from_string(nts, directives + options_tgf);
	}

	TEST_CASE("the reader writes a directive into the grammar") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		auto gr = load(nts, "@trim ws.\n");
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		CHECK(g.opt().shaping.to_trim == set<size_t>{ id_of(nts, "ws") });
		REQUIRE(g.bind_options().has_value());
		CHECK(text_of(repo, "trim") == "ws");
	}

	TEST_CASE("a directive name the grammar does not have fails the load") {
		nonterminals<char> nts;
		auto gr = load(nts, "@trim nope.\n");
		CHECK(!gr.has_value());
		ostringstream printed;
		gr.report().print(printed);
		CHECK(printed.str().find("nope") != string::npos);
	}

	TEST_CASE("@enable and @disable edit the enabled productions in order") {
		nonterminals<char> nts;
		auto gr = load(nts, "@enable a.\n@disable a.\n");
		REQUIRE(gr.has_value());
		CHECK(gr.value().opt().enabled_guards.empty());
	}

	TEST_CASE("the bind pushes a value stored before it") {
		options_repository repo;
		options_scope scope(repo);
		REQUIRE(repo.declare(grammar_option_set).has_value());
		REQUIRE(repo.set_text("trim", "x").has_value());
		nonterminals<char> nts;
		auto gr = load(nts, "@trim ws.\n");
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		CHECK(g.opt().shaping.to_trim == set<size_t>{ id_of(nts, "ws") });
		REQUIRE(g.bind_options().has_value());
		CHECK(g.opt().shaping.to_trim == set<size_t>{ id_of(nts, "x") });
	}

	static bool parses(parser<char>& p, const string& in) {
		return p.parse(in.c_str(), in.size()).found;
	}

	TEST_CASE("start is a grammar option") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		auto gr = load(nts, "@start x.\n");
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		REQUIRE(g.bind_options().has_value());
		parser<char> p(g);
		CHECK(text_of(repo, "start") == "x");
		CHECK(parses(p, "x"));
		REQUIRE(repo.set_text("start", "start").has_value());
		CHECK(text_of(repo, "start") == "start");
		CHECK(parses(p, "x x"));
		auto r = repo.set_text("start", "nope");
		CHECK(r.has_error());
		ostringstream printed;
		r.report().print(printed);
		CHECK(printed.str().find("nope") != string::npos);
		CHECK(repo.set_text("start", "").has_error());
		CHECK(text_of(repo, "start") == "start");
	}

	TEST_CASE("char-classes loads the classes and a rename") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			string("@use char class alpha.\nstart => alpha | d.\n"));
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		REQUIRE(g.bind_options().has_value());
		parser<char> p(g);
		CHECK(text_of(repo, "char-classes") == "alpha");
		CHECK(!parses(p, "5"));
		REQUIRE(repo.set_text("char-classes", "alpha, digit as d")
			.has_value());
		CHECK(text_of(repo, "char-classes") == "alpha, digit as d");
		CHECK(parses(p, "5"));
		CHECK(parses(p, "a"));
		CHECK(repo.set_text("char-classes", "nosuch").has_error());
		CHECK(text_of(repo, "char-classes") == "alpha, digit as d");
	}

	TEST_CASE("highlight holds one entry per name") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		auto gr = load(nts, "@highlight keyword : x, ws.\n");
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		REQUIRE(g.bind_options().has_value());
		CHECK(text_of(repo, "highlight") == "keyword: x, keyword: ws");
		REQUIRE(repo.set_text("highlight", "keyword: x, operator: ws")
			.has_value());
		using pairs = vector<pair<string, vector<string>>>;
		CHECK(g.opt().highlights == pairs{ { "keyword", { "x" } },
			{ "operator", { "ws" } } });
		CHECK(text_of(repo, "highlight") == "keyword: x, operator: ws");
		CHECK(repo.set_text("highlight", "nosuch: x").has_error());
	}

	TEST_CASE("highlight-auto is the flag of @highlight auto") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		auto gr = load(nts, "@highlight auto.\n");
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		CHECK(g.opt().highlight_heuristics);
		REQUIRE(g.bind_options().has_value());
		repo.set("highlight-auto", false);
		CHECK(!g.opt().highlight_heuristics);
		CHECK(text_of(repo, "highlight-auto") == "false");
	}

	TEST_CASE("dynamic holds one entry per value") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts, string(
			"@dynamic tn defaults a.\n@dynamic other.\n"
			"start => tn | other.\n"));
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		REQUIRE(g.bind_options().has_value());
		parser<char> p(g);
		CHECK(text_of(repo, "dynamic") == "other=, tn=a");
		CHECK(parses(p, "a"));
		CHECK(!parses(p, "b"));
		REQUIRE(repo.set_text("dynamic", "other=, tn=a, tn=b")
			.has_value());
		CHECK(text_of(repo, "dynamic") == "other=, tn=a, tn=b");
		CHECK(parses(p, "b"));
		REQUIRE(repo.set_text("dynamic", "tn=b").has_value());
		CHECK(text_of(repo, "dynamic") == "tn=b");
		CHECK(!parses(p, "a"));
		CHECK(parses(p, "b"));
		CHECK(repo.set_text("dynamic", "noeq").has_error());
	}

	TEST_CASE("an empty @dynamic value is a load error that names it") {
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts, string(
			"@dynamic empty_tn defaults \"\".\nstart => empty_tn.\n"));
		CHECK(!gr.has_value());
		ostringstream printed;
		gr.report().print(printed);
		CHECK(printed.str().find(
			parser_strings::messages::dynamic_empty_value)
				!= string::npos);
		CHECK(printed.str().find("empty_tn") != string::npos);
	}

	TEST_CASE("the default of start is start") {
		options_repository repo;
		options_scope scope(repo);
		REQUIRE(repo.declare(grammar_option_set).has_value());
		CHECK(text_of(repo, "start") == "start");
	}

	static const string variant_productions =
		"start[a] => 'a'.\nstart[b] => 'b'.\n";
	static const string variants_tgf =
		"@variants va: a; vb: b.\n" + variant_productions;

	// A grammar of variants_tgf with @p directives before it, bound to
	// the current repository.
	static grammar<char> variant_grammar(nonterminals<char>& nts,
		const string& directives = {})
	{
		auto gr = tgf<char>::from_string(nts, directives + variants_tgf);
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		REQUIRE(g.bind_options().has_value());
		return g;
	}

	TEST_CASE("the first profile is the default") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = variant_grammar(nts);
		parser<char> p(g);
		CHECK(text_of(repo, "variants") == "va=a, vb=b");
		CHECK(text_of(repo, "variant") == "va");
		CHECK(g.opt().enabled_guards == set<string>{ "a" });
		CHECK(parses(p, "a"));
		CHECK(!parses(p, "b"));
	}

	TEST_CASE("@variant selects a profile of guards") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = variant_grammar(nts, "@variant vb.\n");
		parser<char> p(g);
		CHECK(text_of(repo, "variant") == "vb");
		CHECK(g.opt().enabled_guards == set<string>{ "b" });
		CHECK(parses(p, "b"));
		CHECK(!parses(p, "a"));
		REQUIRE(repo.set_text("variant", "va").has_value());
		CHECK(g.opt().enabled_guards == set<string>{ "a" });
		CHECK(parses(p, "a"));
		CHECK(!parses(p, "b"));
	}

	TEST_CASE("@variant takes a name with an underscore") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts, string(
			"@variants a: x; b_c: y.\n@variant b_c.\n"
			"start[x] => 'x'.\nstart[y] => 'y'.\n"));
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		REQUIRE(g.bind_options().has_value());
		CHECK(text_of(repo, "variant") == "b_c");
		CHECK(g.opt().enabled_guards == set<string>{ "y" });
	}

	TEST_CASE("a second @variants adds a profile") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts, "@variants va: a; vb: b.\n"
			"@variants vc: a, b.\n" + variant_productions);
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		REQUIRE(g.bind_options().has_value());
		CHECK(text_of(repo, "variants") == "va=a, vb=b, vc=a, vc=b");
		CHECK(text_of(repo, "variant") == "va");
		REQUIRE(repo.set_text("variant", "vc").has_value());
		CHECK(g.opt().enabled_guards == set<string>{ "a", "b" });
	}

	TEST_CASE("a write of enabled-productions names its profile or none") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = variant_grammar(nts);
		REQUIRE(repo.set_text("enabled-productions", "b").has_value());
		CHECK(text_of(repo, "variant") == "vb");
		REQUIRE(repo.set_text("enabled-productions", "a, b")
			.has_value());
		CHECK(text_of(repo, "variant") == "");
	}

	TEST_CASE("an unknown variant is an error that changes nothing") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = variant_grammar(nts);
		auto r = repo.set_text("variant", "nope");
		CHECK(r.has_error());
		ostringstream printed;
		r.report().print(printed);
		CHECK(printed.str().find("nope") != string::npos);
		CHECK(text_of(repo, "variant") == "va");
		CHECK(g.opt().enabled_guards == set<string>{ "a" });
	}

	TEST_CASE("a profile named twice is a load error") {
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@variants va: b.\n" + variants_tgf);
		CHECK(!gr.has_value());
		ostringstream printed;
		gr.report().print(printed);
		CHECK(printed.str().find(
			parser_strings::messages::variant_defined_twice)
				!= string::npos);
		CHECK(printed.str().find("va") != string::npos);
	}

	TEST_CASE("@variants without a colon is a load error") {
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@variants a b.\n" + variant_productions);
		CHECK(!gr.has_value());
		ostringstream printed;
		gr.report().print(printed);
		CHECK(printed.str().find(
			parser_strings::messages::invalid_variant_profile)
				!= string::npos);
	}

	TEST_CASE("two profiles with one set of guards are an error") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = variant_grammar(nts);
		auto r = repo.set_text("variants", "va=a, vc=a");
		CHECK(r.has_error());
		ostringstream printed;
		r.report().print(printed);
		CHECK(printed.str().find(
			parser_strings::messages::variant_same_guards)
				!= string::npos);
		CHECK(text_of(repo, "variants") == "va=a, vb=b");
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: parser options
// ---------------------------------------------------------------------------

TEST_SUITE("parser options") {

	static bool parses(parser<char>& p, const string& in) {
		return p.parse(in.c_str(), in.size()).found;
	}

	TEST_CASE("bind_options binds the parse options") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		parser<char> p(g);
		REQUIRE(p.bind_options().has_value());
		CHECK(text_of(repo, "gc") == (parser<char>::parse_options{}
			.enable_gc ? "true" : "false"));
		REQUIRE(repo.set_text("gc", "false").has_value());
		CHECK(text_of(repo, "gc") == "false");
		REQUIRE(repo.set_text("gc-lag", "3").has_value());
		CHECK(text_of(repo, "gc-lag") == "3");
		CHECK(parses(p, "x x"));
	}

	TEST_CASE("error-verbosity and tree-path are texts") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		parser<char> p(g);
		REQUIRE(p.bind_options().has_value());
		CHECK(text_of(repo, "error-verbosity") == "detailed");
		REQUIRE(repo.set_text("error-verbosity", "root-cause")
			.has_value());
		CHECK(text_of(repo, "error-verbosity") == "root-cause");
		CHECK(repo.set_text("error-verbosity", "loud").has_error());
		CHECK(text_of(repo, "error-verbosity") == "root-cause");
		CHECK(text_of(repo, "tree-path") == "bintree");
		REQUIRE(repo.set_text("tree-path", "forest").has_value());
		CHECK(text_of(repo, "tree-path") == "forest");
		CHECK(parses(p, "x x"));
	}

	TEST_CASE("measure turns the other measure flags on") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		parser<char> p(g);
		REQUIRE(p.bind_options().has_value());
		const char* flags[] = { "measure-scopes", "measure-counters",
			"measure-forest", "measure-preprocess" };
		REQUIRE(repo.set_text("measure", "true").has_value());
		for (const char* f : flags) CHECK(text_of(repo, f) == "true");
		REQUIRE(repo.set_text("measure", "false").has_value());
		for (const char* f : flags) CHECK(text_of(repo, f) == "true");
	}

	TEST_CASE("a moved parser keeps its options bound") {
		options_repository repo;
		options_scope scope(repo);
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		parser<char> p(g);
		REQUIRE(p.bind_options().has_value());
		parser<char> p2 = std::move(p);
		REQUIRE(repo.set_text("gc-lag", "5").has_value());
		CHECK(text_of(repo, "gc-lag") == "5");
		// the hook of measure runs only for a bound parser
		REQUIRE(repo.set_text("measure", "true").has_value());
		CHECK(text_of(repo, "measure-scopes") == "true");
		CHECK(parses(p2, "x x"));
	}
}

// ---------------------------------------------------------------------------
// TEST SUITE: parser generator
// ---------------------------------------------------------------------------

TEST_SUITE("parser generator") {

	TEST_CASE("an output directory that does not exist is an error") {
		nonterminals<char> nts;
		grammar<char> g = options_grammar(nts);
		const string dir = (std::filesystem::temp_directory_path()
			/ "tgf-no-such-dir").string() + "/";
		REQUIRE(!std::filesystem::exists(dir));
		parser_gen_options gopt;
		gopt.output_dir = dir;
		gopt.name = "missing_dir_parser";
		auto r = generate_parser_cpp<char>("missing_dir.tgf", g, gopt);
		CHECK(!r.has_value());
		CHECK(r.has_error());
		ostringstream printed;
		r.report().print(printed);
		CHECK(printed.str().find(
			parser_strings::messages::cannot_open_file) != string::npos);
		CHECK(printed.str().find(dir) != string::npos);
	}
}
