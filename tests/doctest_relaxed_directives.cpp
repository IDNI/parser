#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"
#include "format/tgf/tgf.h"

using namespace idni;

// Helper: build a grammar from a TGF string. The caller must keep
// the nonterminals alive as long as the grammar is in use (grammar
// stores a reference to nonterminals).
static grammar<char> build_grammar(
	nonterminals<char>& nts, const char* tgf_src)
{
	auto gr = tgf<char>::from_string(nts, std::string(tgf_src));
	REQUIRE(gr.has_value());
	return std::move(gr).value();
}

// ============================================================
// @trim (simple)
// ============================================================

TEST_CASE("directive: @trim simple") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit, space.\n"
		"@trim _, __.\n"
		"start => digit.\n"
		"_ => __ | null.\n"
		"__ => space | __ space.\n");
	REQUIRE(g.opt.shaping.to_trim.count(g.nt("_").n()) == 1);
	REQUIRE(g.opt.shaping.to_trim.count(g.nt("__").n()) == 1);
}

TEST_CASE("directive: @trim single nonterminal") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@trim ws.\n"
		"start => digit.\n"
		"ws => ' ' ws | null.\n");
	REQUIRE(g.opt.shaping.to_trim.count(g.nt("ws").n()) == 1);
}

// ============================================================
// @trim all terminals
// ============================================================

TEST_CASE("directive: @trim all terminals") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@trim all terminals.\n"
		"start => digit.\n");
	REQUIRE(g.opt.shaping.trim_terminals == true);
}

TEST_CASE("directive: @trim all terminals except children of") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit, alpha.\n"
		"@trim all terminals except children of X, Y.\n"
		"start => digit.\n"
		"X => alpha.\n"
		"Y => alpha.\n");
	REQUIRE(g.opt.shaping.trim_terminals == true);
	REQUIRE(g.opt.shaping.dont_trim_terminals_of.count(
		g.nt("X").n()) == 1);
	REQUIRE(g.opt.shaping.dont_trim_terminals_of.count(
		g.nt("Y").n()) == 1);
}

// ============================================================
// @trim children
// ============================================================

TEST_CASE("directive: @trim children") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit, space.\n"
		"@trim children X, Y.\n"
		"start => digit.\n"
		"X => 'a'.\n"
		"Y => 'b'.\n");
	REQUIRE(g.opt.shaping.to_trim_children.count(
		g.nt("X").n()) == 1);
	REQUIRE(g.opt.shaping.to_trim_children.count(
		g.nt("Y").n()) == 1);
}

TEST_CASE("directive: @trim children terminals") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit, space.\n"
		"@trim children terminals X, Y.\n"
		"start => digit.\n"
		"X => 'a'.\n"
		"Y => 'b'.\n");
	REQUIRE(g.opt.shaping.to_trim_children_terminals.count(
		g.nt("X").n()) == 1);
	REQUIRE(g.opt.shaping.to_trim_children_terminals.count(
		g.nt("Y").n()) == 1);
}

// ============================================================
// @inline
// ============================================================

TEST_CASE("directive: @inline char classes") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@inline char classes.\n"
		"start => digit.\n");
	REQUIRE(g.opt.shaping.inline_char_classes == true);
}

TEST_CASE("directive: @inline char class (singular)") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@inline char class.\n"
		"start => digit.\n");
	REQUIRE(g.opt.shaping.inline_char_classes == true);
}

TEST_CASE("directive: @inline nonterminals") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@inline A, B.\n"
		"start => digit.\n"
		"A => 'a'.\n"
		"B => 'b'.\n");
	// Each dir_arg produces one entry: {A} and {B} -> 2 entries
	REQUIRE(g.opt.shaping.to_inline.size() >= 2);
}

// ============================================================
// @disable
// ============================================================

TEST_CASE("directive: @disable auto disambiguation") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@disable auto disambiguation.\n"
		"start => digit.\n");
	REQUIRE(g.opt.auto_disambiguate == false);
}

// ============================================================
// @ambiguous
// ============================================================

TEST_CASE("directive: @ambiguous") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@ambiguous expr, term.\n"
		"start => digit.\n"
		"expr => term '+' term.\n"
		"term => digit.\n");
	REQUIRE(g.opt.nodisambig_list.count(g.nt("expr").n()) == 1);
	REQUIRE(g.opt.nodisambig_list.count(g.nt("term").n()) == 1);
}

// ============================================================
// @enable
// ============================================================

TEST_CASE("directive: @enable") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@enable guard1, guard2.\n"
		"start => digit.\n");
	REQUIRE(g.opt.enabled_guards.count("guard1") == 1);
	REQUIRE(g.opt.enabled_guards.count("guard2") == 1);
}

