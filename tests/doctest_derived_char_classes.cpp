// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Locks for the derived character class analysis.
//
// The analysis only reads a loaded grammar. It returns one entry per
// nonterminal it visited. An entry is derived (an accepted rule used by a
// rejected rule), inner (an accepted rule used only by accepted rules),
// unused (an accepted rule used by no source production) or rejected with
// a reason.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"
#include "grammar_inspector.h"
#include "format/json/json.h"
#include "tgf/parser_gen.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace idni;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// A loaded grammar owns its nonterminals on the heap, so the grammar's
// reference to them survives moving the container.
struct loaded {
	unique_ptr<nonterminals<char>> nts = make_unique<nonterminals<char>>();
	optional<grammar<char>> g;
	explicit operator bool() const { return g.has_value(); }
	grammar<char>& operator*() { return *g; }
	const grammar<char>& operator*() const { return *g; }
};

// Each case loads its own grammar, so the file text is read once and kept.
static const string& read_source(const string& rel_path) {
	static map<string, string> cache;
	if (auto it = cache.find(rel_path); it != cache.end()) return it->second;
	ifstream ifs(string(PROJECT_SOURCE_DIR) + "/" + rel_path);
	REQUIRE(ifs.is_open());
	ostringstream ss;
	ss << ifs.rdbuf();
	return cache[rel_path] = ss.str();
}

static loaded load_tgf(const string& src) {
	loaded L;
	auto gr = tgf<char>::from_string(*L.nts, src);
	if (gr.has_value()) L.g.emplace(std::move(gr).value());
	return L;
}

// The terminal type is char32_t, as the tgf tool uses for its grammars.
struct loaded32 {
	unique_ptr<nonterminals<char, char32_t>> nts =
		make_unique<nonterminals<char, char32_t>>();
	optional<grammar<char, char32_t>> g;
};

static loaded32 load_tgf32(const string& src) {
	loaded32 L;
	auto gr = tgf<char, char32_t>::from_string(*L.nts, src);
	if (gr.has_value()) L.g.emplace(std::move(gr).value());
	return L;
}

template <typename T>
static set<string> names_of(const grammar<char, T>& g,
	typename cc_rule_info<T>::state want)
{
	set<string> r;
	for (const auto& e : g.derive_char_classes_report())
		if (e.st == want) r.insert(g.get_nts().get(e.nt));
	return r;
}

static string reason_of(const grammar<char>& g, const string& name) {
	for (const auto& e : g.derive_char_classes_report())
		if (g.get_nts().get(e.nt) == name) return e.reason;
	return "";
}

static bool has_reason(const grammar<char>& g, const string& reason) {
	for (const auto& e : g.derive_char_classes_report())
		if (e.st == cc_rule_info<char>::state::rejected
			&& e.reason == reason) return true;
	return false;
}

template <typename T>
static optional<cc_rule_info<T>> rule_of(const grammar<char, T>& g,
	const string& name)
{
	for (const auto& e : g.derive_char_classes_report())
		if (g.get_nts().get(e.nt) == name) return e;
	return nullopt;
}

// ---------------------------------------------------------------------------
// the JSON grammar
// ---------------------------------------------------------------------------

// An ambiguous expression grammar used by the TGF tests.
static const char* amb_tgf =
	"start => expr.\n"
	"expr => expr '+' expr | expr '*' expr | 'x'.\n";

static const set<string> json_derived = {
	"unescaped", "nonzerodigit", "ascii_digit", "ascii_xdigit",
	"esc", "__E____0", "__E_exponent_7"
};

static const set<string> json_inner = {
	"__N_2", "__E_unescaped_12", "__N_1", "__N_0", "zero"
};

// The JSON grammar from the TGF text and the generated grammar share the
// same rules, so the analysis must give the same report for both.
TEST_CASE("JSON TGF text: derived set is the JSON grammar table") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	CHECK(names_of<char>(*L, cc_rule_info<char>::state::derived)
		== json_derived);
}

TEST_CASE("JSON TGF text: inner set is the JSON grammar table") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	CHECK(names_of<char>(*L, cc_rule_info<char>::state::inner)
		== json_inner);
}

TEST_CASE("generated JSON grammar: derived set matches the TGF text") {
	const auto& g = json_parser_data::grammar;
	CHECK(names_of<char32_t>(g, cc_rule_info<char32_t>::state::derived)
		== json_derived);
}

TEST_CASE("generated JSON grammar: inner set matches the TGF text") {
	const auto& g = json_parser_data::grammar;
	CHECK(names_of<char32_t>(g, cc_rule_info<char32_t>::state::inner)
		== json_inner);
}

// The unescaped predicate accepts every character that is not a quote, a
// backslash, or an ASCII control character other than DEL.
TEST_CASE("unescaped predicate accepts plain, non-ASCII and DEL") {
	const auto& g = json_parser_data::grammar;
	optional<char_class_fn<char32_t>> fn;
	for (const auto& e : g.derive_char_classes_report())
		if (g.get_nts().get(e.nt) == "unescaped") fn = e.fn;
	REQUIRE(fn.has_value());
	CHECK((*fn)(U'a'));
	CHECK((*fn)(char32_t(0x00E9)));
	CHECK((*fn)(char32_t(0x007F)));
}

TEST_CASE("unescaped predicate rejects quote, backslash and a control char") {
	const auto& g = json_parser_data::grammar;
	optional<char_class_fn<char32_t>> fn;
	for (const auto& e : g.derive_char_classes_report())
		if (g.get_nts().get(e.nt) == "unescaped") fn = e.fn;
	REQUIRE(fn.has_value());
	CHECK_FALSE((*fn)(U'"'));
	CHECK_FALSE((*fn)(U'\\'));
	CHECK_FALSE((*fn)(char32_t(0x0001)));
}

// ---------------------------------------------------------------------------
// accept and reject cases
// ---------------------------------------------------------------------------

TEST_CASE("two rules that both use ascii are both derived") {
	loaded L = load_tgf(
		"@use char class any, ascii.\n"
		"start => x y.\n"
		"x => ascii & ~'a'.\n"
		"y => ascii & ~'b'.\n");
	REQUIRE(L.g.has_value());
	auto d = names_of<char>(*L, cc_rule_info<char>::state::derived);
	CHECK(d.count("x") == 1);
	CHECK(d.count("y") == 1);
}

TEST_CASE("a cycle is rejected with reason cycle") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => any & ~B.\n"
		"B => A.\n");
	REQUIRE(L.g.has_value());
	CHECK(has_reason(*L, "cycle"));
}

TEST_CASE("a rule with an off guard reads only the enabled production") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => 'a'.\n"
		"A[g] => \"bb\".\n");
	REQUIRE(L.g.has_value());
	auto e = rule_of<char>(*L, "A");
	REQUIRE(e.has_value());
	CHECK(e->st != cc_rule_info<char>::state::rejected);
	CHECK(e->fn('a'));
	CHECK_FALSE(e->fn('b'));
}

TEST_CASE("the same rule is rejected with sequence when the guard is on") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => 'a'.\n"
		"A[g] => \"bb\".\n");
	REQUIRE(L.g.has_value());
	L.g->productions_enable("g");
	auto e = rule_of<char>(*L, "A");
	REQUIRE(e.has_value());
	CHECK(e->st == cc_rule_info<char>::state::rejected);
	CHECK(e->reason == "sequence");
}

TEST_CASE("CSV TEXTDATA with the comma guard enabled") {
	loaded32 L = load_tgf32(read_source("src/format/csv/csv.tgf"));
	REQUIRE(L.g.has_value());
	L.g->set_enabled_productions({ "comma", "lf", "no_header" });
	auto e = rule_of<char32_t>(*L.g, "TEXTDATA");
	REQUIRE(e.has_value());
	CHECK(e->st == cc_rule_info<char32_t>::state::derived);
	CHECK_FALSE(e->fn(U','));
	CHECK(e->fn(U' '));
	CHECK(e->fn(U'a'));
	// printable() excludes tab in both guard states.
	CHECK_FALSE(e->fn(U'\t'));
	CHECK(e->guards.count("comma") == 1);
}

TEST_CASE("CSV TEXTDATA with the tab guard enabled") {
	loaded32 L = load_tgf32(read_source("src/format/csv/csv.tgf"));
	REQUIRE(L.g.has_value());
	L.g->set_enabled_productions({ "tab", "lf", "no_header" });
	auto e = rule_of<char32_t>(*L.g, "TEXTDATA");
	REQUIRE(e.has_value());
	CHECK(e->st == cc_rule_info<char32_t>::state::derived);
	CHECK(e->fn(U','));
	CHECK_FALSE(e->fn(U'\t'));
	CHECK(e->fn(U'a'));
	CHECK(e->guards.count("tab") == 1);
}

TEST_CASE("a @dynamic rule is rejected with reason dynamic") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"@dynamic d.\n"
		"start => d.\n"
		"d => 'a'.\n");
	REQUIRE(L.g.has_value());
	CHECK(reason_of(*L, "d") == "dynamic");
}

TEST_CASE("a zero literal is rejected with reason zero literal") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => 'a'.\n"
		"A => '\\x00'.\n");
	REQUIRE(L.g.has_value());
	CHECK(reason_of(*L, "A") == "zero literal");
}

TEST_CASE("a rule with no source production is rejected with reason no production") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => C.\n");
	REQUIRE(L.g.has_value());
	CHECK(reason_of(*L, "C") == "no production");
}

// A chain of single-character rules grows the copied predicate until it
// passes the construction budget.
static string chain_grammar(size_t n) {
	ostringstream os;
	os << "@use char class ascii.\n";
	os << "start => A" << n << ".\n";
	os << "A0 => ascii.\n";
	for (size_t i = 1; i <= n; ++i)
		os << "A" << i << " => A" << (i - 1) << ".\n";
	return os.str();
}

TEST_CASE("a predicate over the construction budget is rejected with reason too large") {
	loaded L = load_tgf(chain_grammar(200));
	REQUIRE(L.g.has_value());
	CHECK(has_reason(*L, "too large"));
}

// ---------------------------------------------------------------------------
// the TGF grammar
// ---------------------------------------------------------------------------

TEST_CASE("TGF grammar derives unescaped_c, unescaped_s and pat_char") {
	loaded L = load_tgf(read_source("src/format/tgf/tgf.tgf"));
	REQUIRE(L.g.has_value());
	auto d = names_of<char>(*L, cc_rule_info<char>::state::derived);
	CHECK(d.count("unescaped_c") == 1);
	CHECK(d.count("unescaped_s") == 1);
	CHECK(d.count("pat_char") == 1);
}

// ---------------------------------------------------------------------------
// recognition with the scanner off, on and on again
// ---------------------------------------------------------------------------

// Parses one input and returns whether it was accepted. The caller owns the
// parser, so the grammar can be switched between calls.
template <typename T>
static bool parses(parser<char, T>& p, const string& input,
	size_t start = SIZE_MAX)
{
	typename parser<char, T>::parse_options po;
	po.start = start;
	auto r = p.parse(input.data(), input.size(), po);
	return r.found;
}

// The same cases must give the same answer off, on and on again. The second
// on pass must not grow G for characters that were seen before.
template <typename T>
static void same_on_off_on(grammar<char, T>& g,
	const vector<pair<string, bool>>& cases, size_t start = SIZE_MAX)
{
	parser<char, T> p(g, default_parser_options<char, T>());
	g.derive_char_classes(false);
	for (const auto& [in, want] : cases)
		CHECK(parses<T>(p, in, start) == want);
	g.derive_char_classes(true);
	for (const auto& [in, want] : cases)
		CHECK(parses<T>(p, in, start) == want);
	size_t g_on1 = grammar_inspector<char, T>(g).G().size();
	g.derive_char_classes(false);
	for (const auto& [in, want] : cases)
		CHECK(parses<T>(p, in, start) == want);
	g.derive_char_classes(true);
	for (const auto& [in, want] : cases)
		CHECK(parses<T>(p, in, start) == want);
	size_t g_on2 = grammar_inspector<char, T>(g).G().size();
	CHECK(g_on2 == g_on1);
}

TEST_CASE("JSON recognition is the same off, on and on again") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	same_on_off_on<char>(*L, {
		{ "\"abc\"", true },
		{ "{}", true },
		{ "[1,2.5,true,null]", true },
		{ "{\"a\":[1,2],\"b\":\"x\"}", true },
		{ "\"\\n\"", true },
		{ "\"a", false },
		{ "", false },
		{ "[1,]", false },
	});
}

TEST_CASE("CSV recognition with the comma guard is the same off, on and on again") {
	loaded32 L = load_tgf32(read_source("src/format/csv/csv.tgf"));
	REQUIRE(L.g.has_value());
	L.g->set_enabled_productions({ "comma", "lf", "no_header" });
	same_on_off_on<char32_t>(*L.g, {
		{ "a,b", true },
		{ "a,b,c\n1,2,3", true },
		{ "\"a,b\",c", true },
		{ "a\tb", false },
	});
}

TEST_CASE("CSV recognition with the tab guard is the same off, on and on again") {
	loaded32 L = load_tgf32(read_source("src/format/csv/csv.tgf"));
	REQUIRE(L.g.has_value());
	L.g->set_enabled_productions({ "tab", "lf", "no_header" });
	same_on_off_on<char32_t>(*L.g, {
		{ "a\tb", true },
		{ "a\tb\tc\n1\t2\t3", true },
		{ "a,b", true },
	});
}

TEST_CASE("TGF recognition is the same off, on and on again") {
	loaded L = load_tgf(read_source("src/format/tgf/tgf.tgf"));
	REQUIRE(L.g.has_value());
	same_on_off_on<char>(*L, {
		{ amb_tgf, true },
		{ "start => 'a'.", true },
		{ "=> 'a'.", false },
		{ "not a grammar", false },
	});
}

TEST_CASE("a derived class as the per-call start is the same off, on and on again") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	size_t u = L.g->nt("unescaped").n();
	same_on_off_on<char>(*L, {
		{ "a", true },
		{ "\"", false },
		{ "\\", false },
	}, u);
}

TEST_CASE("CSV guard change follows the guards with the scanner on") {
	loaded32 L = load_tgf32(read_source("src/format/csv/csv.tgf"));
	REQUIRE(L.g.has_value());
	parser<char, char32_t> p(*L.g, default_parser_options<char, char32_t>());
	p.get_grammar().set_enabled_productions({ "comma", "lf", "no_header" });
	p.get_grammar().derive_char_classes(true);
	CHECK(parses<char32_t>(p, "a,b"));
	CHECK_FALSE(parses<char32_t>(p, "a\tb"));
	p.get_grammar().set_enabled_productions({ "tab", "lf", "no_header" });
	CHECK(parses<char32_t>(p, "a\tb"));
	CHECK(parses<char32_t>(p, "a,b"));
	p.get_grammar().set_enabled_productions({ "comma", "lf", "no_header" });
	CHECK(parses<char32_t>(p, "a,b"));
	CHECK_FALSE(parses<char32_t>(p, "a\tb"));
}

TEST_CASE("a zero inside and at the end is the same off and on") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => x.\n"
		"x => any.\n");
	REQUIRE(L.g.has_value());
	parser<char> p(*L.g);
	string zero_only = "\0";
	string zero_at_end = "a";
	zero_at_end.push_back('\0');
	p.get_grammar().derive_char_classes(false);
	bool off_zero = parses<char>(p, zero_only);
	bool off_end = parses<char>(p, zero_at_end);
	p.get_grammar().derive_char_classes(true);
	CHECK(parses<char>(p, zero_only) == off_zero);
	CHECK(parses<char>(p, zero_at_end) == off_end);
}

TEST_CASE("a malformed JSON string is rejected in both modes") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	parser<char> p(*L.g);
	p.get_grammar().derive_char_classes(false);
	CHECK_FALSE(parses<char>(p, "\"abc"));
	p.get_grammar().derive_char_classes(true);
	CHECK_FALSE(parses<char>(p, "\"abc"));
}

TEST_CASE("with the scanner on unescaped is a derived character class") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	L.g->derive_char_classes(true);
	size_t u = L.g->nt("unescaped").n();
	grammar_inspector<char, char> gi(*L.g);
	CHECK(gi.cc_fns().is_derived(u));
	CHECK(gi.cc_fns().is_fn(u));
	CHECK(L.g->is_cc_fn(u));
}

TEST_CASE("a rule grown by add_dynamic_production_from is rejected as dynamic") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => 'a'.\n");
	REQUIRE(L.g.has_value());
	auto l = L.g->nt("A");
	L.g->add_dynamic_production_from(l, "b");
	auto e = rule_of<char>(*L.g, "A");
	REQUIRE(e.has_value());
	CHECK(e->st == cc_rule_info<char>::state::rejected);
	CHECK(e->reason == "dynamic");
}

// ---------------------------------------------------------------------------
// tree equivalence with the scanner off and on
// ---------------------------------------------------------------------------

template <typename T>
static string tree_print(tref t) {
	if (!t) return "";
	return parser<char, T>::tree::get(t).print_to_str();
}

// Raw tree, shaped tree and terminals of one parse.
template <typename T>
static string tree_signature(typename parser<char, T>::result& r) {
	if (!r.found) return "reject";
	string s = "accept\nraw=" + tree_print<T>(r.get_tree2())
		+ "\nshaped=" + tree_print<T>(r.get_shaped_tree2())
		+ "\nterms=";
	auto ts = r.get_terminals();
	if constexpr (std::is_same_v<T, char>) s += string(ts.begin(), ts.end());
	else s += to_std_string(ts);
	return s;
}

template <typename T>
static void check_tree_equiv(grammar<char, T>& g,
	const vector<string>& inputs, bool ad, bool bin, parse_tree_path tp)
{
	g.opt.auto_disambiguate = ad;
	typename parser<char, T>::options o =
		default_parser_options<char, T>();
	o.binarize = bin;
	o.parse_opts.tree_path = tp;
	parser<char, T> p(g, o);
	for (const string& in : inputs) {
		g.derive_char_classes(false);
		auto off = p.parse(in.data(), in.size(), o.parse_opts);
		string so = tree_signature<T>(off);
		g.derive_char_classes(true);
		auto on = p.parse(in.data(), in.size(), o.parse_opts);
		string sn = tree_signature<T>(on);
		CHECK_MESSAGE(so == sn, "input: " << in << " ad=" << ad
			<< " bin=" << bin << " tp=" << (int)tp
			<< "\noff: " << so << "\non:  " << sn);
	}
	g.derive_char_classes(false);
}

template <typename T>
static void check_all_modes(grammar<char, T>& g,
	const vector<string>& inputs)
{
	for (bool ad : { true, false })
	for (bool bin : { true, false })
	for (auto tp : { parse_tree_path::bintree_path,
		parse_tree_path::forest_path })
			check_tree_equiv<T>(g, inputs, ad, bin, tp);
}

// Every code point is put in one context the grammar uses. The tree must
// be the same off and on.
template <typename T>
static void check_cp_equiv(grammar<char, T>& g,
	const string& prefix, const string& suffix)
{
	g.opt.auto_disambiguate = true;
	typename parser<char, T>::options o =
		default_parser_options<char, T>();
	parser<char, T> p(g, o);
	vector<char32_t> cps;
	for (char32_t cp = 1; cp <= 0x2FF; ++cp) cps.push_back(cp);
	cps.push_back(0x4E2D);
	cps.push_back(0x1F600);
	g.derive_char_classes(false);
	for (char32_t cp : cps) {
		string in = prefix + to_std_string(cp) + suffix;
		auto off = p.parse(in.data(), in.size(), o.parse_opts);
		string so = tree_signature<T>(off);
		g.derive_char_classes(true);
		auto on = p.parse(in.data(), in.size(), o.parse_opts);
		string sn = tree_signature<T>(on);
		g.derive_char_classes(false);
		CHECK_MESSAGE(so == sn, "cp: " << (int)cp << " input: " << in
			<< "\noff: " << so << "\non:  " << sn);
	}
}

// Error line, column and unexpected text of one parse.
template <typename T>
static string error_pos(grammar<char, T>& g, const string& in, bool on,
	parse_tree_path tp)
{
	g.derive_char_classes(on);
	typename parser<char, T>::options o =
		default_parser_options<char, T>();
	o.parse_opts.tree_path = tp;
	parser<char, T> p(g, o);
	auto r = p.parse(in.data(), in.size(), o.parse_opts);
	if (r.found) return "accept";
	string s = "L" + to_string(r.parse_error.line)
		+ " C" + to_string(r.parse_error.col) + " U[";
	for (const auto& l : r.parse_error.unexp) s += l.to_std_string();
	return s + "]";
}

TEST_CASE("JSON trees match off and on in every mode") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	check_all_modes<char>(*L, {
		"\"a\"", "\"\"", "\"\\n\\t\\\"\\\\/\"",
		"\"\\u0041\"", "\"\\u00e9\"", "\"\xc3\xa9\"",
		"\"\\uD83D\\uDE00\"", "0", "-0", "12", "1234567890",
		"1.5", "1e10", "1E-3", "1e+0", "true", "false",
		"null", "[]", "{}", "{\"a\":[1,2.5,true,null]}",
		"[ ]", "{ }", "\n\t1\r\n",
	});
}

TEST_CASE("CSV trees match off and on in every mode") {
	loaded32 L = load_tgf32(read_source("src/format/csv/csv.tgf"));
	REQUIRE(L.g.has_value());
	L.g->set_enabled_productions({ "comma", "lf", "no_header" });
	check_all_modes<char32_t>(*L.g, {
		"a,b", "a,b,c\n1,2,3", "\"a,b\",c", "\"c\\\"d\"",
		"\"a\nb\"", "\"\",\"\"", "a", ",", "\"a,b\"",
	});
	L.g->set_enabled_productions({ "tab", "lf", "no_header" });
	check_all_modes<char32_t>(*L.g, {
		"a\tb", "a\tb\tc\n1\t2\t3", "\"a\tb\",c", "a,b",
	});
}

TEST_CASE("TGF trees match off and on in every mode") {
	loaded L = load_tgf(read_source("src/format/tgf/tgf.tgf"));
	REQUIRE(L.g.has_value());
	check_all_modes<char>(*L, {
		amb_tgf,
		"start => 'a'.",
		"start => '\\n' | '\\x41' | 'a'.",
		"@use char class alpha.\nstart => alpha+.",
		"a => b. b => 'c'.",
	});
}

TEST_CASE("treemr trees match off and on in every mode") {
	loaded L = load_tgf(read_source("src/format/treemr/treemr.tgf"));
	REQUIRE(L.g.has_value());
	check_all_modes<char>(*L, {
		"rule >> digit > '1'",
		"rule > (^ digit+ term? $)",
		"row > cell ',' cell",
		"x > (^ a | ^ b)",
	});
}

TEST_CASE("JSON code points keep the same tree off and on") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	check_cp_equiv<char>(*L, "\"", "\"");
	check_cp_equiv<char>(*L, "", "");
	check_cp_equiv<char>(*L, "", "1");
	check_cp_equiv<char>(*L, "1e", "");
}

TEST_CASE("CSV code points keep the same tree off and on") {
	loaded32 L = load_tgf32(read_source("src/format/csv/csv.tgf"));
	REQUIRE(L.g.has_value());
	L.g->set_enabled_productions({ "comma", "lf", "no_header" });
	check_cp_equiv<char32_t>(*L.g, "", "");
	check_cp_equiv<char32_t>(*L.g, "a,", "");
	L.g->set_enabled_productions({ "tab", "lf", "no_header" });
	check_cp_equiv<char32_t>(*L.g, "", "");
}

TEST_CASE("TGF code points keep the same tree off and on") {
	loaded L = load_tgf(read_source("src/format/tgf/tgf.tgf"));
	REQUIRE(L.g.has_value());
	check_cp_equiv<char>(*L, "start => '", "'.");
	check_cp_equiv<char>(*L, "start => \"", "\".");
}

// The derived class hint reproduces the negation hint for a genuine
// negation rejection: the positives accept and a negated conjunct fires.
TEST_CASE("the error position and unexpected text are the same off and on") {
	loaded N = load_tgf(
		"@use char class digit.\n"
		"start => '0' | nzdigit digits.\n"
		"nzdigit => digit & ~'0'.\n"
		"digits => digit digits | null.\n");
	REQUIRE(N.g.has_value());
	loaded J = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(J.g.has_value());
	loaded32 C = load_tgf32(read_source("src/format/csv/csv.tgf"));
	REQUIRE(C.g.has_value());
	C.g->set_enabled_productions({ "comma", "lf", "no_header" });
	for (auto tp : { parse_tree_path::bintree_path,
		parse_tree_path::forest_path }) {
		CHECK(error_pos<char>(*N, "01", false, tp)
			== error_pos<char>(*N, "01", true, tp));
		CHECK(error_pos<char>(*J, "\"abc", false, tp)
			== error_pos<char>(*J, "\"abc", true, tp));
		CHECK(error_pos<char32_t>(*C.g, "\"a", false, tp)
			== error_pos<char32_t>(*C.g, "\"a", true, tp));
	}
}

// With the switch on the error names the character the derived class
// rejected. The switch is set on explicitly in every call.
TEST_CASE("the switch-on error text names the rejected character") {
	loaded S = load_tgf(
		"start => string.\n"
		"string => '\"' string_chars '\"'.\n"
		"string_chars => string_char string_chars | null.\n"
		"string_char => char_ | escape.\n"
		"char_ => ('a' | 'b' | 'c') & ~('\"' | '\\\\').\n"
		"escape => '\\\\' ('\"' | '\\\\').\n");
	REQUIRE(S.g.has_value());
	loaded J = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(J.g.has_value());
	string ctl1 = "\""; ctl1.push_back('\x01'); ctl1 += "\"";
	string ctl2 = "\""; ctl2.push_back('\x1F'); ctl2 += "\"";
	for (auto tp : { parse_tree_path::bintree_path,
		parse_tree_path::forest_path }) {
		CHECK(error_pos<char>(*S, "\"\"c\"", true, tp)
			== "L1 C3 U['c']");
		CHECK(error_pos<char>(*J, ctl1, true, tp)
			== "L1 C2 U['\\x01']");
		CHECK(error_pos<char>(*J, ctl2, true, tp)
			== "L1 C2 U['\\x1f']");
		CHECK(error_pos<char>(*J, "\"\\x\"", true, tp)
			== "L1 C3 U['x']");
	}
}

// ---------------------------------------------------------------------------
// dynamic growth, ambiguity, generation and copied classes
// ---------------------------------------------------------------------------

TEST_CASE("a dynamic_grow child is not a derived class") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => kw.\n"
		"kw => 'a' | 'b'.\n");
	REQUIRE(L.g.has_value());
	auto kw = L.g->nt("kw");
	parser<char>::options o = default_parser_options<char, char>();
	o.dynamic_grow_nts = { { L.g->nt("start").n(), kw.n() } };
	parser<char> p(*L.g, o);
	// the parser excludes a dynamic_grow child from the analysis
	CHECK_FALSE(grammar_inspector<char, char>(*L.g).cc_fns().is_derived(kw.n()));
	// a committed context value reaches the child as an ordinary rule
	dynamic_context<char> ctx;
	ctx.values[kw.n()].insert("while");
	parser<char>::parse_options po;
	po.dynamic_ctx = &ctx;
	auto parses_while = [&]() { return p.parse("while", 5, po).found; };
	L.g->derive_char_classes(false);
	bool off = parses_while();
	L.g->derive_char_classes(true);
	bool on = parses_while();
	CHECK(off);
	CHECK(on);
	CHECK(off == on);
}

TEST_CASE("add_dynamic on a derived rule is rejected as dynamic") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => 'a'.\n");
	REQUIRE(L.g.has_value());
	size_t a = L.g->nt("A").n();
	CHECK(grammar_inspector<char, char>(*L.g).cc_fns().is_derived(a));
	L.g->add_dynamic("A", { "bb" });
	auto e = rule_of<char>(*L.g, "A");
	REQUIRE(e.has_value());
	CHECK(e->st == cc_rule_info<char>::state::rejected);
	CHECK(e->reason == "dynamic");
	parser<char> p(*L.g);
	CHECK(parses<char>(p, "bb"));
}

TEST_CASE("an ambiguous derived rule keeps the same trees off and on") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => left | right.\n"
		"left => 'a'.\n"
		"right => 'a'.\n");
	REQUIRE(L.g.has_value());
	L.g->opt.nodisambig_list.clear();
	check_all_modes<char>(*L.g, { "a" });
	L.g->opt.nodisambig_list = { L.g->nt("A").n() };
	check_all_modes<char>(*L.g, { "a" });
}

// The generated header lands in a temp file, so the test reads it back.
static string generate_parser_text(const grammar<char>& g) {
	namespace fs = std::filesystem;
	parser_gen_options gopt;
	gopt.output_dir = fs::temp_directory_path().string() + "/";
	gopt.output = "derived_class_gen_test.generated.h";
	gopt.name = "derived_class_gen_test_parser";
	generate_parser_cpp<char>("derived_class_gen_test.tgf", g, gopt);
	ifstream in(gopt.output_dir + gopt.output);
	REQUIRE(in.is_open());
	ostringstream ss;
	ss << in.rdbuf();
	in.close();
	fs::remove(gopt.output_dir + gopt.output);
	return ss.str();
}

// The predefined class list of a generated header.
static string char_classes_text(const string& text) {
	auto b = text.find("predefined_char_classes");
	if (b == string::npos) return "";
	b = text.find("({", b);
	if (b == string::npos) return "";
	auto e = text.find("}", b);
	if (e == string::npos) return "";
	return text.substr(b, e - b + 1);
}

TEST_CASE("generated parser code is the same before and after a parse") {
	loaded L = load_tgf(read_source("src/format/json/json.tgf"));
	REQUIRE(L.g.has_value());
	L.g->derive_char_classes(true);
	string before = generate_parser_text(*L.g);
	CHECK(char_classes_text(before).find("unescaped") == string::npos);
	parser<char> p(*L.g);
	CHECK(parses<char>(p, "\"a\""));
	string after = generate_parser_text(*L.g);
	CHECK(before == after);
	CHECK(char_classes_text(after).find("unescaped") == string::npos);
}

TEST_CASE("the generated JSON grammar keeps the same trees off and on") {
	auto& g = json_parser_data::grammar;
	bool ad = g.opt.auto_disambiguate;
	auto nd = g.opt.nodisambig_list;
	g.opt.nodisambig_list.clear();
	check_all_modes<char32_t>(g, {
		"\"a\"", "\"\\n\\t\\\"\\\\/\"", "0", "-12.5e3",
		"true", "null", "[]", "{}", "{\"a\":[1,2.5,true,null]}",
		"\"a", "[1,]", "",
	});
	g.opt.auto_disambiguate = ad;
	g.opt.nodisambig_list = nd;
	g.derive_char_classes(true);
}

TEST_CASE("an eof literal is rejected with reason eof literal") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => A.\n"
		"A => 'a'.\n"
		"A => '\\xFF'.\n");
	REQUIRE(L.g.has_value());
	CHECK(reason_of(*L, "A") == "eof literal");
}

TEST_CASE("an unused rule is not a character class") {
	loaded L = load_tgf(
		"@use char class any.\n"
		"start => 'a'.\n"
		"u => 'b'.\n");
	REQUIRE(L.g.has_value());
	auto e = rule_of<char>(*L, "u");
	REQUIRE(e.has_value());
	CHECK(e->st == cc_rule_info<char>::state::unused);
	size_t u = L.g->nt("u").n();
	CHECK_FALSE(grammar_inspector<char, char>(*L.g).cc_fns().is_derived(u));
	CHECK_FALSE(L.g->is_cc_fn(u));
}

TEST_CASE("a grammar built from copied cc_fns has no derived classes") {
	nonterminals<char> nts;
	prods<char> ps, start(nts("start")), A(nts("A"));
	char_class_fns<char> cc = predefined_char_classes<char>(
		{ "any" }, nts);
	ps(start, A);
	ps(A, prods<char>('a'));
	grammar<char> g1(nts, ps, start, cc);
	g1.derive_char_classes(true);
	size_t a = g1.nt("A").n();
	CHECK(grammar_inspector<char, char>(g1).cc_fns().is_derived(a));
	parser<char> p1(g1);
	REQUIRE(parses<char>(p1, "a"));

	grammar<char>::options opt;
	opt.derive_char_classes = false;
	grammar<char> g2(nts, ps, start,
		grammar_inspector<char, char>(g1).cc_fns(), opt);
	grammar_inspector<char, char> gi2(g2);
	CHECK_FALSE(gi2.cc_fns().is_derived(a));
	auto pit = gi2.cc_fns().ps.find(a);
	CHECK((pit == gi2.cc_fns().ps.end() || pit->second.empty()));
	g2.derive_char_classes(true);
	parser<char> p2(g2);
	CHECK(parses<char>(p2, "a"));
	CHECK_FALSE(parses<char>(p2, "b"));
}
