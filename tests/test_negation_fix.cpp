// Targeted test for the negation fix: ~nonterminal with JSON-like grammar.
//
// Grammar:
//   unescaped => anychar & ~('"' | '\\' | cntrl & ascii & ~'\x09' & ~'\x7F').
//
// The inner conjunction cntrl & ascii & ~'\x09' & ~'\x7F' creates a
// synthetic nonterminal.  The outer ~(...) negates the disjunction,
// exercising the exact code path the fix addresses.
//
// Build:
//   g++ -std=c++23 -O1 -DTAU_PARSER_BUILD_HEADER_ONLY \
//       -I../src -I../build/release/_deps/unordered_dense-src/include \
//       -o test_negation_fix test_negation_fix.cpp && ./test_negation_fix

#include "parser.h"
#include <iostream>
#include <cstdlib>

using namespace idni;

static grammar<char>::options make_grammar_options() {
	grammar<char>::options go;
	go.transform_negation = false; // match TGF behaviour
	return go;
}

static void expect_parses(grammar<char>& g, const char* input,
	const char* label)
{
	parser<char> p(g);
	auto r = p.parse(input, strlen(input));
	if (!r.found) {
		std::cerr << "FAIL [" << label << "]: '" << input
			<< "' should PARSE but was REJECTED\n";
		std::exit(1);
	}
	std::cout << "  \033[32m✓\033[0m " << label << " ACCEPTED\n";
}

static void expect_rejects(grammar<char>& g, const char* input,
	const char* label)
{
	parser<char> p(g);
	auto r = p.parse(input, strlen(input));
	if (r.found) {
		std::cerr << "FAIL [" << label << "]: '" << input
			<< "' should be REJECTED but was ACCEPTED\n";
		std::exit(1);
	}
	std::cout << "  \033[32m✓\033[0m " << label << " REJECTED\n";
}

int main() {
	nonterminals<char> nts;
	auto start   = nts("start");
	auto unesc   = nts("unescaped");
	auto anychar = nts("anychar");

	// Predefined char classes
	char_class_fns<char> ccfns = predefined_char_classes<char>({
		"any", "cntrl", "ascii"
	}, nts);

	auto any_nt   = nts("any");
	auto cntrl_nt = nts("cntrl");
	auto ascii_nt = nts("ascii");

	prods<char> ps, start_p(start), unesc_p(unesc), anychar_p(anychar);
	prods<char> any_p(any_nt);
	prods<char> quote('"');
	prods<char> backslash('\\');
	prods<char> tab('\x09');
	prods<char> del('\x7F');

	// unescaped => anychar & ~('"' | '\\' | cntrl & ascii & ~'\x09' & ~'\x7F')
	ps(start_p, unesc_p);
	ps(anychar_p, any_p);
	ps(unesc_p, anychar_p & ~(quote | backslash
		| (prods<char>(cntrl_nt) & prods<char>(ascii_nt) & ~tab & ~del)));

	grammar<char> g(nts, ps, start_p, ccfns, make_grammar_options());

	std::cout << "=== JSON-like negation test ===\n";
	std::cout << "Grammar: unescaped => anychar & ~('\"' | '\\\\'"
		" | cntrl & ascii & ~'\\x09' & ~'\\x7F')\n\n";

	// Characters that SHOULD be accepted (not excluded):
	expect_parses(g, "a",   "'a' (0x61) - printable, not excluded");
	expect_parses(g, "0",   "'0' (0x30) - digit, not excluded");
	expect_parses(g, " ",   "SPACE (0x20) - printable, not excluded");
	expect_parses(g, "\x09","TAB (0x09) - control, but excluded from"
		" the inner conjunction via ~'\\x09'");

	// DEL (0x7F) - control & ascii, but excluded from inner via ~'\x7F'
	// So the inner conjunction cntrl & ascii & ~'\x09' & ~'\x7F'
	// should NOT match DEL - it fails the ~'\x7F' conjunct.
	// Therefore the outer negation ~(...) should SUCCEED.
	expect_parses(g, "\x7F","DEL (0x7F) - control & ascii, but excluded"
		" from inner via ~'\\x7F'");

	// Characters that SHOULD be rejected:
	expect_rejects(g, "\"", "'\"' (0x22) - double-quote, excluded");
	expect_rejects(g, "\\", "'\\\\' (0x5C) - backslash, excluded");

	// C0 controls (other than TAB and DEL) should be rejected
	// because they match cntrl & ascii & ~'\x09' & ~'\x7F'
	expect_rejects(g, "\x00","NUL (0x00) - C0 control, should be rejected");
	expect_rejects(g, "\x01","SOH (0x01) - C0 control, should be rejected");
	expect_rejects(g, "\x0A","LF  (0x0A) - C0 control, should be rejected");
	expect_rejects(g, "\x0D","CR  (0x0D) - C0 control, should be rejected");
	expect_rejects(g, "\x1B","ESC (0x1B) - C0 control, should be rejected");

	// C1 controls (0x80-0x9F) are cntrl but NOT ascii (in 7-bit),
	// so they should be ACCEPTED
	expect_parses(g, "\x80","C1 control (0x80) - cntrl but not 7-bit ascii,"
		" accepted");

	std::cout << "\n\033[32m=== ALL TESTS PASSED ===\033[0m\n";
	return 0;
}
