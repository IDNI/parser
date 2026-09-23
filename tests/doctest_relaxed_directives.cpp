#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"
#include "format/tgf/tgf.h"

using namespace idni;
using messages = idni::parser_strings::messages;

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

TEST_CASE("directive: @inline tree path a > b stores the full path") {
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@inline a > b.\n"
		"start => digit.\n"
		"a => b.\n"
		"b => 'x'.\n");
	std::vector<size_t> path{ g.nt("a").n(), g.nt("b").n() };
	CHECK(g.opt.shaping.to_inline.count(path) == 1);
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

// ============================================================
// @highlight
// ============================================================

TEST_CASE("directive: @highlight populates highlights option") {
	nonterminals<char> nts;
	auto g = build_grammar(nts, 
		"@use char class digit.\n"
		"@highlight keyword : start;\n"
		"           comment : comment.\n"
		"start => digit.\n"
		"comment => '#' printable | null.\n");
	REQUIRE(g.opt.highlights.size() == 2);
	// first entry: keyword -> [start]
	REQUIRE(g.opt.highlights[0].first == "keyword");
	REQUIRE(g.opt.highlights[0].second.size() == 1);
	REQUIRE(g.opt.highlights[0].second[0] == "start");
	// second entry: comment -> [comment]
	REQUIRE(g.opt.highlights[1].first == "comment");
	REQUIRE(g.opt.highlights[1].second.size() == 1);
	REQUIRE(g.opt.highlights[1].second[0] == "comment");
}

TEST_CASE("directive: @highlight treemr pattern with an edge and a "
	"quoted literal stores the raw text verbatim")
{
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@highlight operator : production > \"=>\".\n"
		"start => digit.\n");
	REQUIRE(g.opt.highlights.size() == 1);
	REQUIRE(g.opt.highlights[0].first == "operator");
	REQUIRE(g.opt.highlights[0].second.size() == 1);
	CHECK(g.opt.highlights[0].second[0] == "production > \"=>\"");
}

TEST_CASE("directive: @highlight pattern with a quoted comma or period "
	"stays in the pattern text")
{
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@highlight delimiter : production > \",\", production > \".\".\n"
		"start => digit.\n");
	REQUIRE(g.opt.highlights.size() == 1);
	REQUIRE(g.opt.highlights[0].first == "delimiter");
	REQUIRE(g.opt.highlights[0].second.size() == 2);
	CHECK(g.opt.highlights[0].second[0] == "production > \",\"");
	CHECK(g.opt.highlights[0].second[1] == "production > \".\"");
}

TEST_CASE("directive: @highlight stores two treemr patterns, a "
	"capture group and an edge into a parenthesized literal")
{
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@highlight delimiter : (sym), directive > ('.').\n"
		"start => digit.\n");
	REQUIRE(g.opt.highlights.size() == 1);
	REQUIRE(g.opt.highlights[0].second.size() == 2);
	CHECK(g.opt.highlights[0].second[0] == "(sym)");
	CHECK(g.opt.highlights[0].second[1] == "directive > ('.')");
}

TEST_CASE("directive: @highlight keeps a plain name and a glob "
	"unchanged alongside a treemr pattern")
{
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@highlight keyword : dir_sym, *_cmd, sym > (%).\n"
		"start => digit.\n");
	REQUIRE(g.opt.highlights.size() == 1);
	REQUIRE(g.opt.highlights[0].second.size() == 3);
	CHECK(g.opt.highlights[0].second[0] == "dir_sym");
	CHECK(g.opt.highlights[0].second[1] == "*_cmd");
	CHECK(g.opt.highlights[0].second[2] == "sym > (%)");
}

TEST_CASE("directive: @highlight string pattern keeps its quotes") {
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@highlight string : \"=>\".\n"
		"start => digit.\n");
	REQUIRE(g.opt.highlights.size() == 1);
	REQUIRE(g.opt.highlights[0].second.size() == 1);
	CHECK(g.opt.highlights[0].second[0] == "\"=>\"");
}

TEST_CASE("directive: @highlight with multiple nonterminals") {
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@highlight number : digit, hex.\n"
		"start => digit.\n"
		"hex => 'a' | 'b'.\n");
	REQUIRE(g.opt.highlights.size() == 1);
	REQUIRE(g.opt.highlights[0].first == "number");
	REQUIRE(g.opt.highlights[0].second.size() == 2);
	REQUIRE(g.opt.highlights[0].second[0] == "digit");
	REQUIRE(g.opt.highlights[0].second[1] == "hex");
}

TEST_CASE("directive: @highlight auto sets the heuristics flag") {
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@highlight auto.\n"
		"start => digit.\n");
	REQUIRE(g.opt.highlight_heuristics == true);
}

TEST_CASE("directive: no @highlight leaves heuristics off") {
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"start => digit.\n");
	REQUIRE(g.opt.highlight_heuristics == false);
}

// True if the report holds a warning whose message text is msg.
static bool has_warning(const idni::diagnostics::report& r,
	std::string_view msg)
{
	for (auto& n : r.nodes())
		if (idni::diagnostics::is_warning(n.tag) && r.str(n.key) == msg)
			return true;
	return false;
}

TEST_CASE("directive: @highlight with an unknown type warns and is skipped") {
	nonterminals<char> nts;
	auto res = tgf<char>::from_string(nts,
		"@use char class digit.\n"
		"@highlight keywrod : start.\n"
		"start => digit.\n");
	REQUIRE(res.has_value());
	auto& rep = res.report();
	CHECK(has_warning(rep, messages::unknown_highlight_type));
	auto g = std::move(res).value();
	REQUIRE(g.opt.highlights.empty());
}

TEST_CASE("directive: @highlight of an undefined name warns unproductive") {
	nonterminals<char> nts;
	auto res = tgf<char>::from_string(nts,
		"@use char class digit.\n"
		"@highlight keyword : nosuch.\n"
		"start => digit.\n");
	REQUIRE(res.has_value());
	auto& rep = res.report();
	CHECK(has_warning(rep, messages::unproductive_nonterminal));
	auto g = std::move(res).value();
	REQUIRE(g.opt.highlights.size() == 1);
}

TEST_CASE("directive: @highlight glob pattern does not warn unproductive") {
	nonterminals<char> nts;
	auto res = tgf<char>::from_string(nts,
		"@use char class digit.\n"
		"@highlight keyword : nosuch*.\n"
		"start => digit.\n");
	REQUIRE(res.has_value());
	auto& rep = res.report();
	CHECK_FALSE(has_warning(rep, messages::unproductive_nonterminal));
	auto g = std::move(res).value();
	REQUIRE(g.opt.highlights.size() == 1);
}

TEST_CASE("directive: @highlight multi-pair directive stores both pairs "
	"in order")
{
	nonterminals<char> nts;
	auto g = build_grammar(nts,
		"@use char class digit.\n"
		"@highlight keyword : a; comment : b.\n"
		"start => digit.\n"
		"a => digit.\n"
		"b => digit.\n");
	REQUIRE(g.opt.highlights.size() == 2);
	REQUIRE(g.opt.highlights[0].first == "keyword");
	REQUIRE(g.opt.highlights[0].second == std::vector<std::string>{"a"});
	REQUIRE(g.opt.highlights[1].first == "comment");
	REQUIRE(g.opt.highlights[1].second == std::vector<std::string>{"b"});
}
