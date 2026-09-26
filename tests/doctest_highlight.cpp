// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Native verification of syntax_highlighter token extraction.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"
#include "syntax_highlighter.h"

#include <fstream>
#include <sstream>
#include <string>

using namespace std;
using namespace idni;

#ifdef PROJECT_SOURCE_DIR

// Read a file from the source tree relative to the project root.
string read_grammar(const string& rel_path) {
	string full = string(PROJECT_SOURCE_DIR) + "/" + rel_path;
	ifstream ifs(full);
	REQUIRE(ifs.is_open());
	ostringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

// Decode delta-encoded tokens into a list of (type_name, text) pairs.
struct decoded_token {
	string type;
	string text;
	uint32_t line;
	uint32_t col;
};
vector<decoded_token> decode(const vector<uint32_t>& data,
	const string& src)
{
	vector<decoded_token> out;
	const auto& names = token_classifier::token_type_names();
	uint32_t line = 0, col = 0;
	for (size_t i = 0; i + 4 < data.size(); i += 5) {
		uint32_t dl = data[i];
		uint32_t dc = data[i + 1];
		uint32_t len = data[i + 2];
		uint32_t ty = data[i + 3];
		if (dl == 0) col += dc; else { line += dl; col = dc; }
		size_t pos = 0;
		for (size_t l = 0; l < line; ++l)
			pos = src.find('\n', pos) + 1;
		pos += col;
		string text = src.substr(pos, len);
		string tname = ty < names.size() ? names[ty] : "?";
		out.push_back({tname, text, line, col});
	}
	return out;
}

// --- CSV grammar ---

TEST_CASE("csv.tgf compiles and highlights a simple CSV line") {
	string g_src = read_grammar("src/format/csv/csv.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// Inline-only shaping keeps SEP (commas) visible.
	auto data = hl.get_tokens("a,b,c");
	auto tokens = decode(data, "a,b,c");
	// Expect: a, comma, b, comma, c (at least 5 tokens).
	REQUIRE(tokens.size() >= 5);
	CHECK(tokens[0].text == "a");
	CHECK(tokens[1].text == ",");
	CHECK(tokens[2].text == "b");
	CHECK(tokens[3].text == ",");
	CHECK(tokens[4].text == "c");
	// csv.tgf's @highlight lists SEP directly as operator.
	CHECK(tokens[1].type == "operator");
	CHECK(tokens[3].type == "operator");
	// csv.tgf's @highlight lists non_escaped directly as string.
	CHECK(tokens[0].type == "string");
	CHECK(tokens[2].type == "string");
	CHECK(tokens[4].type == "string");
}

TEST_CASE("csv.tgf highlights quoted fields") {
	string g_src = read_grammar("src/format/csv/csv.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// A quoted field, quotes and inner comma alike, merges into one string token.
	auto data = hl.get_tokens("\"hello, world\",b");
	auto tokens = decode(data, "\"hello, world\",b");
	REQUIRE(tokens.size() == 3);
	CHECK(tokens[0].text == "\"hello, world\"");
	CHECK(tokens[0].type == "string");
	CHECK(tokens[1].text == ",");
	CHECK(tokens[1].type == "operator");
	CHECK(tokens[2].text == "b");
	CHECK(tokens[2].type == "string");
}

TEST_CASE("csv.tgf highlights CRLF-terminated rows") {
	// CRLF ends the first record, so the second row starts on the next line.
	string g_src = read_grammar("src/format/csv/csv.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());
	string input = "a,b\r\nc,d";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	REQUIRE(tokens.size() == 6);
	CHECK(tokens[0].text == "a");
	CHECK(tokens[0].type == "string");
	CHECK(tokens[1].text == ",");
	CHECK(tokens[1].type == "operator");
	CHECK(tokens[2].text == "b");
	CHECK(tokens[2].type == "string");
	CHECK(tokens[3].text == "c");
	CHECK(tokens[3].type == "string");
	CHECK(tokens[3].line == 1);
	CHECK(tokens[4].text == ",");
	CHECK(tokens[4].type == "operator");
	CHECK(tokens[5].text == "d");
	CHECK(tokens[5].type == "string");
}

// --- JSON grammar ---

TEST_CASE("json.tgf highlights an object's braces, key, colon and number") {
	string g_src = read_grammar("src/format/json/json.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	string input = R"({"a":1})";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	REQUIRE(tokens.size() == 5);
	CHECK(tokens[0].text == "{");
	CHECK(tokens[0].type == "type");
	CHECK(tokens[1].text == "\"a\"");
	CHECK(tokens[1].type == "string");
	CHECK(tokens[2].text == ":");
	CHECK(tokens[2].type == "type");
	CHECK(tokens[3].text == "1");
	CHECK(tokens[3].type == "number");
	CHECK(tokens[4].text == "}");
	CHECK(tokens[4].type == "type");
}

TEST_CASE("json.tgf highlights true/false/null as keywords") {
	string g_src = read_grammar("src/format/json/json.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// json.tgf's @highlight lists these directly as keyword.
	for (const char* lit : {"true", "false", "null"}) {
		auto data = hl.get_tokens(lit);
		auto tokens = decode(data, lit);
		REQUIRE(tokens.size() == 1);
		CAPTURE(lit);
		CAPTURE(tokens[0].type);
		CHECK(tokens[0].text == lit);
		CHECK(tokens[0].type == "keyword");
	}
}

TEST_CASE("json.tgf highlights numbers") {
	string g_src = read_grammar("src/format/json/json.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	auto data = hl.get_tokens("42");
	auto tokens = decode(data, "42");
	// Digits merge into a single "number" token.
	REQUIRE(tokens.size() == 1);
	CAPTURE(tokens[0].type);
	CHECK(tokens[0].text == "42");
	CHECK(tokens[0].type == "number");
}

TEST_CASE("json.tgf: an object key's quotes merge with its text into one string token") {
	string g_src = read_grammar("src/format/json/json.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	string input = R"({"key": 1})";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	// Both bounding quotes and the key text arrive as a single string token.
	bool found_key = false;
	for (auto& t : tokens) {
		if (t.text == "\"key\"") { found_key = true; CHECK(t.type == "string"); }
		CHECK(t.text != "\"");
	}
	CHECK(found_key);
}

// --- TGF grammar itself (self-hosting) ---

TEST_CASE("tgf.tgf highlights a rule's symbols and its production delimiters") {
	string g_src = read_grammar("src/format/tgf/tgf.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	string input = "start => expr.";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	REQUIRE(tokens.size() == 4);
	CHECK(tokens[0].text == "start");
	CHECK(tokens[0].type == "variable");
	CHECK(tokens[1].text == "=>");
	CHECK(tokens[1].type == "operator");
	CHECK(tokens[2].text == "expr");
	CHECK(tokens[2].type == "variable");
	CHECK(tokens[3].text == ".");
	CHECK(tokens[3].type == "delimiter");
}

// --- TGF quote-character classification ---

TEST_CASE("tgf.tgf: quote chars in char and string literals are string") {
	string g_src = read_grammar("src/format/tgf/tgf.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// terminal_char and terminal_string are @highlight'ed as string.
	bool found_tc = false, found_ts = false;
	for (size_t i = 0; i < hl.nt_count(); ++i) {
		string nm = hl.get_nt_name(i);
		string ty = hl.get_nt_type(i);
		if (nm == "terminal_char")   { found_tc = true; CHECK(ty == "string"); }
		if (nm == "terminal_string") { found_ts = true; CHECK(ty == "string"); }
	}
	CHECK(found_tc);
	CHECK(found_ts);

	// Each quoted literal merges into one string token; x is a plain sym, colored variable.
	// production > ("=>") types the arrow operator; production > ('.') types the terminator.
	string input = "x => 'a' | \"bc\".";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	REQUIRE(tokens.size() == 6);
	CHECK(tokens[0].text == "x");
	CHECK(tokens[0].type == "variable");
	CHECK(tokens[1].text == "=>");
	CHECK(tokens[1].type == "operator");
	CHECK(tokens[2].text == "'a'");
	CHECK(tokens[2].type == "string");
	CHECK(tokens[3].text == "|");
	CHECK(tokens[3].type == "operator");
	CHECK(tokens[4].text == "\"bc\"");
	CHECK(tokens[4].type == "string");
	CHECK(tokens[5].text == ".");
	CHECK(tokens[5].type == "delimiter");
}

TEST_CASE("tgf.tgf self-highlighting: quotes in tgf.tgf source are string") {
	// terminal_string and terminal_char regions must be fully opaque string.
	string g_src = read_grammar("src/format/tgf/tgf.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// Each quoted literal merges into one string token; ch is a plain sym, colored variable.
	// production > ("=>") types the arrow operator; production > ('.') types the terminator.
	string input = "ch => \"'\" | '\\''.";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	REQUIRE(tokens.size() == 6);
	CHECK(tokens[0].text == "ch");
	CHECK(tokens[0].type == "variable");
	CHECK(tokens[1].text == "=>");
	CHECK(tokens[1].type == "operator");
	CHECK(tokens[2].type == "string");
	CHECK(tokens[3].text == "|");
	CHECK(tokens[3].type == "operator");
	CHECK(tokens[4].type == "string");
	CHECK(tokens[5].text == ".");
	CHECK(tokens[5].type == "delimiter");
}

// --- tgf.test.tgf grammar ---

TEST_CASE("tgf.test.tgf: quotes of a char literal and a string literal are string") {
	string g_src = read_grammar("src/format/tgf.test/tgf.test.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// 'a' is a char_lit embedded in the matcher; "b" is a quoted_string item.
	string input = "x > 'a' : \"b\".\n";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	bool found_char = false, found_str = false;
	for (auto& t : tokens) {
		CAPTURE(t.text);
		if (t.text == "'a'") { found_char = true; CHECK(t.type == "string"); }
		if (t.text == "\"b\"") { found_str = true; CHECK(t.type == "string"); }
	}
	CHECK(found_char);
	CHECK(found_str);
}

// --- Error recovery ---

TEST_CASE("incomplete input produces tokens via fallback") {
	string g_src = read_grammar("src/format/json/json.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// Simple incomplete input triggers fallback tokenization.
	auto data = hl.get_tokens("abc 123 +");
	auto tokens = decode(data, "abc 123 +");
	// Fallback classifies abc as variable, 123 as number, + as operator.
	REQUIRE(tokens.size() == 3);
	CHECK(tokens[0].text == "abc");
	CHECK(tokens[0].type == "variable");
	CHECK(tokens[1].text == "123");
	CHECK(tokens[1].type == "number");
	CHECK(tokens[2].text == "+");
	CHECK(tokens[2].type == "operator");
}

TEST_CASE("fallback colors quoted spans as string, not operator") {
	string g_src = read_grammar("src/format/json/json.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// Does not parse as JSON -> whole input goes through the fallback.
	string input = "@@ \"hi there\" 'x'";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);

	// The quoted spans (including their quotes) must be string.
	bool dq = false, sq = false;
	for (auto& t : tokens) {
		CAPTURE(t.text);
		CAPTURE(t.type);
		if (t.text == "\"hi there\"") { dq = true; CHECK(t.type == "string"); }
		if (t.text == "'x'")         { sq = true; CHECK(t.type == "string"); }
		// No bare quote should remain classified operator.
		if (t.text == "\"" || t.text == "'") CHECK(t.type != "operator");
	}
	CHECK(dq);
	CHECK(sq);
}

TEST_CASE("mid-document parse error gives tree tokens then fallback tokens") {
	string g_src =
		"@highlight keyword : a.\n"
		"start => a.\n"
		"a => \"foo\".\n";
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	string input = "foo$$$";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	REQUIRE(tokens.size() == 2);
	CHECK(tokens[0].text == "foo");
	CHECK(tokens[0].type == "keyword");
	CHECK(tokens[1].text == "$$$");
	CHECK(tokens[1].type == "operator");
}

TEST_CASE("empty source produces no tokens with a minimal grammar") {
	syntax_highlighter hl("start => 'a'.");
	REQUIRE(hl.good());
	auto data = hl.get_tokens("");
	CHECK(data.empty());
}

TEST_CASE("empty source produces no tokens") {
	string g_src = read_grammar("src/format/json/json.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());
	auto data = hl.get_tokens("");
	CHECK(data.empty());
}

// --- Bad grammar ---

TEST_CASE("bad grammar gives good() == false, diagnostics, no exception") {
	syntax_highlighter hl("start => (.\n");
	CHECK_FALSE(hl.good());
	CHECK_FALSE(hl.diagnostics().empty());
	auto data = hl.get_tokens("anything");
	CHECK(data.empty());
}

// --- Heuristics on/off and the resolved flag ---

TEST_CASE("no @highlight and no auto emits no token") {
	syntax_highlighter hl("start => \"function\".\n");
	REQUIRE(hl.good());
	auto data = hl.get_tokens("function");
	CHECK(data.empty());
}

TEST_CASE("same grammar with @highlight auto emits tokens") {
	syntax_highlighter hl("@highlight auto.\nstart => \"function\".\n");
	REQUIRE(hl.good());
	auto data = hl.get_tokens("function");
	CHECK_FALSE(data.empty());
}

TEST_CASE("constructor false suppresses tokens even when grammar says auto") {
	syntax_highlighter hl(
		"@highlight auto.\nstart => \"function\".\n", false);
	REQUIRE(hl.good());
	auto data = hl.get_tokens("function");
	CHECK(data.empty());
}

TEST_CASE("constructor true emits tokens even when grammar doesn't say auto") {
	syntax_highlighter hl("start => \"function\".\n", true);
	REQUIRE(hl.good());
	auto data = hl.get_tokens("function");
	CHECK_FALSE(data.empty());
}

// --- @highlight directive behavior ---

TEST_CASE("@highlight inside a comment has no effect") {
	syntax_highlighter hl(
		"# @highlight keyword : start.\nstart => \"function\".\n");
	REQUIRE(hl.good());
	auto data = hl.get_tokens("function");
	CHECK(data.empty());
}

TEST_CASE("@highlight multi-pair directive applies both pairs") {
	syntax_highlighter hl(
		"@highlight keyword : a; comment : b.\n"
		"start => a b.\n"
		"a => \"x\".\n"
		"b => \"y\".\n");
	REQUIRE(hl.good());
	auto data = hl.get_tokens("xy");
	auto tokens = decode(data, "xy");
	REQUIRE(tokens.size() == 2);
	CHECK(tokens[0].text == "x");
	CHECK(tokens[0].type == "keyword");
	CHECK(tokens[1].text == "y");
	CHECK(tokens[1].type == "comment");
}

TEST_CASE("@highlight with an unknown type warns") {
	syntax_highlighter hl("@highlight keywrod : start.\nstart => \"x\".\n");
	REQUIRE(hl.good());
	CHECK(hl.diagnostics().find("Unknown highlight type") != string::npos);
}

TEST_CASE("@highlight of an undefined plain name warns unproductive") {
	syntax_highlighter hl(
		"@highlight keyword : nosuch.\nstart => \"x\".\n");
	REQUIRE(hl.good());
	CHECK(hl.diagnostics().find("Unproductive nonterminal") != string::npos);
}

TEST_CASE("clean TGF-like document gives tree tokens: @ is keyword, . is delimiter") {
	// A small representative grammar stands in for the repository's tgf.tgf.
	string g_src =
		"@use char class alpha.\n"
		"@highlight keyword : directive_marker; delimiter : dot.\n"
		"start => directive_marker sym dot.\n"
		"directive_marker => \"@\".\n"
		"sym => alpha+.\n"
		"dot => \".\".\n";
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	string input = "@foo.";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	bool found_at = false, found_dot = false;
	for (auto& t : tokens) {
		if (t.text == "@") { found_at = true; CHECK(t.type == "keyword"); }
		if (t.text == ".") { found_dot = true; CHECK(t.type == "delimiter"); }
	}
	CHECK(found_at);
	CHECK(found_dot);
}

TEST_CASE("a typed ancestor colors a terminal through an untyped intermediate nonterminal") {
	// wrapper is typed, middle is not: 'x' must take wrapper's type, not middle's no_type.
	syntax_highlighter hl(
		"@highlight keyword : wrapper.\n"
		"start => wrapper.\n"
		"wrapper => middle.\n"
		"middle => 'x'.\n");
	REQUIRE(hl.good());

	auto data = hl.get_tokens("x");
	auto tokens = decode(data, "x");
	REQUIRE(tokens.size() == 1);
	CHECK(tokens[0].text == "x");
	CHECK(tokens[0].type == "keyword");
}

// --- treemr patterns in @highlight ---

TEST_CASE("treemr pattern with a capture types only the captured nodes") {
	syntax_highlighter hl(
		"@use char class alpha.\n"
		"@highlight variable : sym.\n"
		"@highlight operator : production > (\"=>\").\n"
		"start => production.\n"
		"production => sym \"=>\" sym.\n"
		"sym => alpha+.\n");
	REQUIRE(hl.good());

	string input = "a=>b";
	auto tokens = decode(hl.get_tokens(input), input);
	REQUIRE(tokens.size() == 3);
	CHECK(tokens[0].text == "a");
	CHECK(tokens[0].type == "variable");
	CHECK(tokens[1].text == "=>");
	CHECK(tokens[1].type == "operator");
	CHECK(tokens[2].text == "b");
	CHECK(tokens[2].type == "variable");
}

TEST_CASE("treemr pattern without captures types the whole match root span") {
	syntax_highlighter hl(
		"@use char class alpha.\n"
		"@highlight operator : production > \"=>\".\n"
		"start => production.\n"
		"production => sym \"=>\" sym.\n"
		"sym => alpha+.\n");
	REQUIRE(hl.good());

	string input = "a=>b";
	auto tokens = decode(hl.get_tokens(input), input);
	REQUIRE(tokens.size() == 1);
	CHECK(tokens[0].text == "a=>b");
	CHECK(tokens[0].type == "operator");
}

TEST_CASE("a bad treemr pattern is diagnosed and other entries still work") {
	syntax_highlighter hl(
		"@use char class alpha.\n"
		"@highlight keyword : sym.\n"
		"@highlight operator : (bad.\n"
		"start => sym.\n"
		"sym => alpha+.\n");
	REQUIRE(hl.good());
	CHECK(hl.diagnostics().find("invalid treemr pattern")
		!= string::npos);

	string input = "abc";
	auto tokens = decode(hl.get_tokens(input), input);
	REQUIRE(tokens.size() == 1);
	CHECK(tokens[0].text == "abc");
	CHECK(tokens[0].type == "keyword");
}

TEST_CASE("a later treemr pattern wins on the same node") {
	syntax_highlighter hl(
		"@use char class alpha.\n"
		"@highlight keyword : production > (\"=>\").\n"
		"@highlight operator : production > (\"=>\").\n"
		"start => production.\n"
		"production => sym \"=>\" sym.\n"
		"sym => alpha+.\n");
	REQUIRE(hl.good());

	string input = "a=>b";
	auto tokens = decode(hl.get_tokens(input), input);
	REQUIRE(tokens.size() == 1);
	CHECK(tokens[0].text == "=>");
	CHECK(tokens[0].type == "operator");
}

// --- glob_match ---

TEST_CASE("glob_match: exact and star patterns") {
	CHECK(token_classifier::glob_match("foo", "foo"));
	CHECK_FALSE(token_classifier::glob_match("foo", "foobar"));
	CHECK(token_classifier::glob_match("*", "anything"));
	CHECK(token_classifier::glob_match("*", ""));
}

TEST_CASE("glob_match: prefix and suffix patterns") {
	CHECK(token_classifier::glob_match("*foo", "xxfoo"));
	CHECK_FALSE(token_classifier::glob_match("*foo", "fooxx"));
	CHECK(token_classifier::glob_match("foo*", "fooxx"));
	CHECK_FALSE(token_classifier::glob_match("foo*", "xxfoo"));
}

TEST_CASE("glob_match: foo*bar table") {
	CHECK(token_classifier::glob_match("foo*bar", "foobar"));
	CHECK(token_classifier::glob_match("foo*bar", "fooXbar"));
	CHECK_FALSE(token_classifier::glob_match("foo*bar", "bar_foo"));
	CHECK_FALSE(token_classifier::glob_match("foo*bar", "fooba"));
}

TEST_CASE("glob_match: a*b rejects a name without a") {
	CHECK_FALSE(token_classifier::glob_match("a*b", "xb"));
	CHECK_FALSE(token_classifier::glob_match("a*b", "xyz"));
	CHECK(token_classifier::glob_match("a*b", "ab"));
}

TEST_CASE("glob_match: multiple middle segments in order") {
	CHECK(token_classifier::glob_match("a*b*c*", "aXbYcZ"));
	CHECK(token_classifier::glob_match("a*b*c*", "abc"));
	CHECK_FALSE(token_classifier::glob_match("a*b*c*", "acb"));
}

// --- Content-driven classification heuristics ---

TEST_CASE("content heuristic classifies without naming conventions") {
	// Rule names a/b/c/d carry no hints; classification comes from production structure.
	const char* g_src =
		"@use char class digit, alpha.\n"
		"@inline char classes.\n"
		"start => a sp b sp c sp d.\n"
		"a  => \"function\".\n"   // fixed alpha literal  -> keyword
		"b  => \"==\".\n"          // fixed punct literal  -> operator
		"c  => '\"' alpha+ '\"'.\n" // quote-delimited body -> string
		"d  => digit+.\n"          // digit class (via +)  -> number
		"sp => ' '.\n";
	syntax_highlighter hl(g_src, true);
	REQUIRE(hl.good());

	string input = "function == \"hi\" 42";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	REQUIRE(tokens.size() == 4);

	CAPTURE(tokens[0].type);
	CAPTURE(tokens[1].type);
	CAPTURE(tokens[2].type);
	CAPTURE(tokens[3].type);

	CHECK(tokens[0].text == "function");
	CHECK(tokens[0].type == "keyword");
	CHECK(tokens[1].text == "==");
	CHECK(tokens[1].type == "operator");
	CHECK(tokens[2].text == "\"hi\"");
	CHECK(tokens[2].type == "string");
	CHECK(tokens[3].text == "42");
	CHECK(tokens[3].type == "number");
}

TEST_CASE("digit/xdigit char classes classify as number") {
	syntax_highlighter hl(
		"@use char class digit, xdigit.\n"
		"start => digit+ | xdigit+.\n", true);
	REQUIRE(hl.good());

	bool saw_digit = false, saw_xdigit = false;
	for (size_t i = 0; i < hl.nt_count(); ++i) {
		string nm = hl.get_nt_name(i);
		if (nm == "digit")  { CHECK(hl.get_nt_type(i) == "number");
			saw_digit = true; }
		if (nm == "xdigit") { CHECK(hl.get_nt_type(i) == "number");
			saw_xdigit = true; }
	}
	CHECK(saw_digit);
	CHECK(saw_xdigit);

	// The terminals matched via the digit class are colored number.
	auto data = hl.get_tokens("42");
	auto tokens = decode(data, "42");
	REQUIRE(tokens.size() == 1);
	CAPTURE(tokens[0].type);
	CHECK(tokens[0].text == "42");
	CHECK(tokens[0].type == "number");
}

// --- merge_pass ---

TEST_CASE("merge_pass joins two adjacent string tokens from different parents") {
	syntax_highlighter hl(
		"@highlight string : a, b.\n"
		"start => a b.\n"
		"a => \"foo\".\n"
		"b => \"bar\".\n");
	REQUIRE(hl.good());

	auto data = hl.get_tokens("foobar");
	auto tokens = decode(data, "foobar");
	REQUIRE(tokens.size() == 1);
	CHECK(tokens[0].text == "foobar");
	CHECK(tokens[0].type == "string");
}

// --- UTF-8 / UTF-16 positions ---

TEST_CASE("UTF-8 astral character gives correct UTF-16 length") {
	syntax_highlighter hl("@use char class any.\nstart => any+.\n", true);
	REQUIRE(hl.good());

	// U+1F600, a 4-byte UTF-8 sequence that counts as 2 UTF-16 units.
	string emoji = "\xF0\x9F\x98\x80";
	string input = "a" + emoji + "b";
	auto data = hl.get_tokens(input);
	REQUIRE(data.size() == 5);
	CHECK(data[0] == 0); // line
	CHECK(data[1] == 0); // col
	CHECK(data[2] == 4); // 'a'(1) + astral(2) + 'b'(1) = 4 UTF-16 units
}

// --- get_token_types / get_nt_name ---

TEST_CASE("token type names match idni::highlight_token_types exactly") {
	auto names = syntax_highlighter::get_token_types();
	REQUIRE(names.size() == highlight_token_types.size());
	for (size_t i = 0; i < highlight_token_types.size(); ++i) {
		CAPTURE(i);
		CHECK(names[i] == highlight_token_types[i]);
	}
}

TEST_CASE("get_nt_name returns names for valid ids") {
	string g_src = read_grammar("src/format/json/json.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	// Iterate until we find "start" or exhaust the container.
	bool found_start = false;
	for (size_t i = 0; i < hl.nt_count(); ++i) {
		string name = hl.get_nt_name(i);
		if (name == "start") { found_start = true; break; }
	}
	CHECK(found_start);
}

// --- flush_run: a token never spans a line ---

TEST_CASE("flush_run drops a comment token's trailing newline") {
	string g_src =
		"@highlight comment : c.\n"
		"start => c x.\n"
		"c => \"# hi\" '\\n'.\n"
		"x => \"ok\".\n";
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	string input = "# hi\nok";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);
	REQUIRE(tokens.size() == 1);
	CHECK(tokens[0].text == "# hi");
	CHECK(tokens[0].type == "comment");
	CHECK(tokens[0].line == 0);
	CHECK(tokens[0].text.size() == 4);
}

TEST_CASE("flush_run splits a CRLF-embedded quoted csv field into one token per line") {
	string g_src = read_grammar("src/format/csv/csv.tgf");
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	string input = "\"ab\r\ncd\",x";
	auto data = hl.get_tokens(input);
	auto tokens = decode(data, input);

	REQUIRE(tokens.size() >= 3);
	CHECK(tokens[0].text == "\"ab");
	CHECK(tokens[0].type == "string");
	CHECK(tokens[0].line == 0);
	CHECK(tokens[1].text == "cd\"");
	CHECK(tokens[1].type == "string");
	CHECK(tokens[1].line == 1);
	CHECK(tokens[2].text == ",");
	CHECK(tokens[2].type == "operator");
	CHECK(tokens[2].line == 1);
	for (auto& t : tokens) {
		CHECK(t.text.find('\r') == string::npos);
		CHECK(t.text.find('\n') == string::npos);
	}
}

// --- get_tokens: an error at end of input skips the redundant prefix parse ---

TEST_CASE("an error at end of input gives fallback tokens for the whole document") {
	string g_src = "start => \"ab\".\n";
	syntax_highlighter hl(g_src);
	REQUIRE(hl.good());

	auto data = hl.get_tokens("a");
	auto tokens = decode(data, "a");

	// No tree exists for a failed parse; the byte reaches the caller as a fallback token only.
	REQUIRE(tokens.size() == 1);
	CHECK(tokens[0].text == "a");
	CHECK(tokens[0].type == "variable");
}

// --- classify_by_content: the first literal alternative decides a mixed rule ---

TEST_CASE("classify_by_content: first literal alternative decides for a mixed word/punct rule") {
	string g_src = "start => x.\nx => \"true\" | \"!\".\n";
	syntax_highlighter hl(g_src, true);
	REQUIRE(hl.good());

	auto data = hl.get_tokens("true");
	auto tokens = decode(data, "true");
	REQUIRE(tokens.size() == 1);
	CHECK(tokens[0].text == "true");
	CHECK(tokens[0].type == "keyword");
}

// --- utf16_units: a span boundary cutting a multi-byte character ---

TEST_CASE("utf16_units counts only the bytes inside a span cut mid-character") {
	string g_src = "@use char class any.\nstart => any any any.\n";
	syntax_highlighter hl(g_src, true);
	REQUIRE(hl.good());

	// The first 3 bytes of the 4-byte U+1F600 sequence; the 4th byte never
	// arrives, so the grammar's own span boundary cuts the character.
	string input = "\xF0\x9F\x98";
	auto data = hl.get_tokens(input);
	REQUIRE(data.size() == 5);
	CHECK(data[2] == 3); // 3 raw bytes, not the 2-unit surrogate pair of a complete sequence
}

// --- Every @highlight pair in the five repository grammars has an effect ---

TEST_CASE("every @highlight pair in the five repository grammars has an effect") {
	struct grammar_case { string path; string sample; };
	vector<grammar_case> cases = {
		{ "src/format/csv/csv.tgf",
			"name,age,city\r\n\"Doe, John\",30,\"New York\"\r\nJane,25,LA\r\n" },
		{ "src/format/json/json.tgf",
			"{\"name\": \"Alice\", \"age\": 30, \"active\": true, "
			"\"tags\": [\"x\", \"y\"], \"note\": \"a\\nb\", \"meta\": null}" },
		{ "src/format/tgf/tgf.tgf",
			"# grammar demo\n"
			"@highlight keyword : kw.\n"
			"start  => kw sym esc num rep alt.\n"
			"kw     => \"let\".\n"
			"esc    => '\\n' | \"a\\tb\".\n"
			"num    => 0x1F.\n"
			"rep    => 'x'*.\n"
			"alt    => 'a' | 'b'.\n"
			"sym    => alpha+.\n" },
		{ "src/tgf/tgf_repl.tgf",
			"parse \"hi\\x41\". set tt on. get s" },
		{ "src/format/tgf.test/tgf.test.tgf",
			"# demo comment\n@raw comment : \"# c\", \"\\x41\".\n" },
	};

	for (auto& gc : cases) {
		CAPTURE(gc.path);
		string g_src = read_grammar(gc.path);

		nonterminals<char, char> nts;
		auto gr = tgf<char, char>::from_string(nts, g_src);
		REQUIRE(gr.has_value());
		auto& highlights = gr.value().opt.highlights;
		REQUIRE_FALSE(highlights.empty());

		syntax_highlighter hl(g_src, false);
		REQUIRE(hl.good());
		auto data = hl.get_tokens(gc.sample);
		auto tokens = decode(data, gc.sample);
		std::set<string> doc_types;
		for (auto& t : tokens) doc_types.insert(t.type);

		for (auto& pr : highlights) {
			CAPTURE(pr.first);
			CHECK(doc_types.count(pr.first) > 0);
		}
	}
}

// --- whitespace-rule naming has no special effect on classification ---

TEST_CASE("a whitespace rule's name does not affect the token list, only its type does") {
	string g1 =
		"@highlight keyword : kw.\n"
		"start => kw _ kw.\n"
		"_      => ' '.\n"
		"kw     => \"let\".\n";
	string g2 =
		"@highlight keyword : kw.\n"
		"start => kw blank kw.\n"
		"blank  => ' '.\n"
		"kw     => \"let\".\n";

	syntax_highlighter hl1(g1, false);
	syntax_highlighter hl2(g2, false);
	REQUIRE(hl1.good());
	REQUIRE(hl2.good());

	string input = "let let";
	auto tokens1 = decode(hl1.get_tokens(input), input);
	auto tokens2 = decode(hl2.get_tokens(input), input);

	REQUIRE(tokens1.size() == tokens2.size());
	for (size_t i = 0; i < tokens1.size(); ++i) {
		CAPTURE(i);
		CHECK(tokens1[i].text == tokens2[i].text);
		CHECK(tokens1[i].type == tokens2[i].type);
		CHECK(tokens1[i].line == tokens2[i].line);
		CHECK(tokens1[i].col == tokens2[i].col);
	}
}

TEST_CASE("with heuristics on, a rule named ws still lets a keyword color and emits no token over the space") {
	string g_src =
		"start => kw ws kw.\n"
		"ws     => ' '.\n"
		"kw     => \"let\".\n";
	syntax_highlighter hl(g_src, true);
	REQUIRE(hl.good());

	string input = "let let";
	auto tokens = decode(hl.get_tokens(input), input);

	REQUIRE(tokens.size() == 2);
	CHECK(tokens[0].text == "let");
	CHECK(tokens[0].type == "keyword");
	CHECK(tokens[1].text == "let");
	CHECK(tokens[1].type == "keyword");
}

#endif // PROJECT_SOURCE_DIR
