// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include "parser.h"

#include "testing.h"

using namespace std;
using namespace idni;
using testing::run_test, testing::run_test_tgf;

int main(int argc, char **argv)
{
	testing::process_args(argc, argv);

	nonterminals<> nt;
	char_class_fns cc = predefined_char_classes<>(
		{ "eof", "alpha", "alnum", "digit", "printable" }, nt);

	prods<> ps, start(nt("start")), nll(lit<>{}),
		alnum(nt("alnum")), alpha(nt("alpha")), digit(nt("digit")),
		chars(nt("chars")), expression(nt("expression")),
		sum(nt("sum")),	mul(nt("mul")), nzdigit(nt("nzdigit")),
		m1(nt("m1")), s1(nt("s1")), m2(nt("m2")), s2(nt("s2")),
		identifier(nt("identifier")), keyword(nt("keyword")),
		a('a'), b('b'), c('c'), n('n'), p('p'), m('m'), e('e'),
		A(nt("A")), B(nt("B")), T(nt("T")), X(nt("X")), Y(nt("Y")),
		zero('0'), one('1'), PO(nt("PO")), IO(nt("IO")),
		plus('+'), minus('-'), mult('*'),
		cr('\r'), lf('\n'), eof(nt("eof")), eol(nt("eol")),
		string_(nt("string")), string_char(nt("string_char")),
		string_chars(nt("string_chars")),
		string_chars1(nt("string_chars1")),
		char_(nt("char_")), char0(nt("char0")),
		printable(nt("printable")),
		enabled(nt("enabled")), disabled(nt("disabled")),
		number(nt("number")),  digits(nt("digits")),
		q_str(lit<>{'"'}), esc(lit<>{'\\'}), escape(nt("escape"));


/*******************************************************************************
*       BASIC
*******************************************************************************/

	// null
	TEST("basic", "null")
	ps(start, nll);
	run_test<char>(ps, nt, start, "");
	ps.clear();

	// unexpected
	TEST("basic", "unexpected")
	ps(start, a + b + c);
	run_test<char>(ps, nt, start, "abc");
	testing::test_options o;
	o.error_expected = "Unexpected end of file at 1:2 (2)";
	run_test<char>(ps, nt, start, "a", {}, o);
	o.error_expected = "Unexpected 'd' at 1:1 (1)";
	run_test<char>(ps, nt, start, "d", {}, o);
	o.error_expected = "Unexpected end of file at 1:3 (3)";
	run_test<char>(ps, nt, start, "ab", {}, o);
	o.error_expected = "Unexpected 'd' at 1:4 (4)";
	run_test<char>(ps, nt, start, "abcd", {}, o);
	o.error_expected = "Unexpected 'd' at 1:4 (4)";
	run_test<char>(ps, nt, start, "abcde", {}, o);
	o.error_expected = "Unexpected 'a' at 1:4 (4)";
	run_test<char>(ps, nt, start, "abcabc", {}, o);
	ps.clear();

	TEST("basic", "terminal")
	ps(start, a | one | plus | lf);
	run_test<char>(ps, nt, start, "a");
	run_test<char>(ps, nt, start, "1");
	run_test<char>(ps, nt, start, "+");
	run_test<char>(ps, nt, start, "\n");
	o.error_expected = "Unexpected 'b' at 1:1 (1)";
	run_test<char>(ps, nt, start, "b", {}, o);
	o.error_expected = "Unexpected '0' at 1:1 (1)";
	run_test<char>(ps, nt, start, "0", {}, o);
	o.error_expected = "Unexpected '-' at 1:1 (1)";
	run_test<char>(ps, nt, start, "-", {}, o);
	o.error_expected = "Unexpected '\\r' at 1:1 (1)";
	run_test<char>(ps, nt, start, "\r", {}, o);
	o.error_expected = "";
	ps.clear();

	// char classes
	TEST("basic", "char_classes")
	ps(start, digit | alpha);
	run_test<char>(ps, nt, start, "0", cc);
	run_test<char>(ps, nt, start, "a", cc);
	ps.clear();

	// char classes
	TEST("basic", "char_classes_recursive")
	ps(start, number | identifier);
	ps(identifier, alpha + chars);
	ps(chars, (alnum + chars) | nll);
	ps(number, digits);
	ps(digits, digit | (digit + digits));
	run_test<char>(ps, nt, start, "12345", cc);
	run_test<char>(ps, nt, start, "ident", cc);
	ps.clear();

	// test eof
	TEST("basic", "eof")
	ps(start, eof);
	run_test<char>(ps, nt, start, "", cc);     // 0
	ps.clear();
	ps(start, a + eof);
	run_test<char>(ps, nt, start, "a", cc);    // 1
	ps.clear();
	ps(start, eol);
	ps(eol,   cr | lf | eof);
	run_test<char>(ps, nt, start, "\n", cc);   // 2
	run_test<char>(ps, nt, start, "\r", cc);   // 3
	run_test<char>(ps, nt, start, "",   cc);   // 4
	ps.clear();
	ps(start, a + eol);
	ps(eol,   cr | lf | eof);
	run_test<char>(ps, nt, start, "a\n", cc);  // 5
	run_test<char>(ps, nt, start, "a\r", cc);  // 6
	run_test<char>(ps, nt, start, "a",   cc);  // 7
	ps.clear();

	TEST("basic", "custom start")
	ps(start, number | identifier);
	ps(identifier, alpha + chars);
	ps(chars, (alnum + chars) | nll);
	ps(number, digits);
	ps(digits, digit | (digit + digits));
	run_test<char>(ps, nt, start, "12", cc, o); //  0
	run_test<char>(ps, nt, start, "x1", cc, o); //  1
	run_test<char>(ps, nt, start, "pi", cc, o); //  2
	o.start = nt.get("number");
	o.dump = true;
	run_test<char>(ps, nt, start, "12", cc, o); //  3
	o.start = nt.get("digits");
	run_test<char>(ps, nt, start, "12", cc, o); //  4
	o.start = nt.get("identifier");
	run_test<char>(ps, nt, start, "x1", cc, o); //  5
	run_test<char>(ps, nt, start, "pi", cc, o); //  6
	o.start = nt.get("chars");
	run_test<char>(ps, nt, start, "x1", cc, o); //  7
	run_test<char>(ps, nt, start, "pi", cc, o); //  8
	// following does not work because char classes are not ordinary prods
	//o.start = nt.get("digit");
	//run_test<char>(ps, nt, start, "1", cc, o);  //  9
	//run_test<char>(ps, nt, start, "2", cc, o);  // 10
	//o.start = nt.get("alnum");
	//run_test<char>(ps, nt, start, "x", cc, o);  // 11
	//run_test<char>(ps, nt, start, "1", cc, o);  // 12
	o.start = SIZE_MAX;
	ps.clear();


/*******************************************************************************
*       PAPERS
*******************************************************************************/

	// Using Elizbeth Scott paper example 2, pg 64
	TEST("papers", "escott_ex2p64")
	ps(start, b | (start + start));
	o = {}, o.ambiguity_fails = false;
	run_test<char>(ps, nt, start, "bbb", {}, o);
	o.error_expected = "Unexpected 'a' at 1:4 (4)";
	run_test<char>(ps, nt, start, "bbba", {}, o);
	o.error_expected = "Unexpected 'a' at 1:1 (1)";
	run_test<char>(ps, nt, start, "a",    {}, o);
	ps.clear();

	// infinite ambiguous grammar, advanced parsing pdf, pg 86
	// will capture cycles
	TEST("papers", "advparsing_p86")
	ps(start, b | start);
	o.error_expected = "";
	run_test<char>(ps, nt, start, "b", {}, o);
	ps.clear();

	// another ambigous grammar
	TEST("papers", "advparsing")
	ps(start, (a + X + X + c) | start);
	ps(X,     (X + b) | nll);
	run_test<char>(ps, nt, start, "abbc", {}, o);
	ps.clear();

	// highly ambigous grammar, advanced parsing pdf, pg 89
	TEST("papers", "advparsing_p89")
	ps(start, (start + start) | a);
	run_test<char>(ps, nt, start, "aaaaa", {}, o);
	ps.clear();

	// using Elizabeth sott paper, example 3, pg 64.
	TEST("papers", "escott_ex3p64")
	ps(start, (A + T) | (a + T));
	ps(A,     a | (B + A));
	ps(B,     nll);
	ps(T,     b + b + b);
	run_test<char>(ps, nt, start, "abbb", {}, o);
	ps.clear();

	TEST("papers", "recursion_b")
	ps(start, b | (start + start + start) | nll);
	run_test<char>(ps, nt, start, "b", {}, o);
	ps.clear();

	TEST("papers", "recursion_npnmn")
	ps(start, n);
	ps(start,  start + p + start);
	ps(start, start + m + start );
	ps(start, start + e + start);
	run_test<char>(ps, nt, start, "npnmnen", {}, o);
	ps.clear();

	// thesis van, figure 4.11
	TEST("papers", "thesis_van_fig4.11")
	ps(start, one | (start + IO + start) | PO + start);
	ps(IO, plus | minus | mult);
	ps(PO, minus);
	run_test<char>(ps, nt, start, "1+1+1", {}, o);
	ps.clear();

	// thesis van, 4.1 fig example
	TEST("papers", "thesis_van_fig4.1")
	ps(start,       plus + start | start + plus + start |
			start + plus | one);
	run_test<char>(ps, nt, start, "1+++1", {}, o);
	ps.clear();

/*******************************************************************************
*       DISAMBIGUATION
*******************************************************************************/
	//
	TEST("disambig", "same")
	ps(start, start + plus + start );
	ps(start, one);
	run_test<char>(ps, nt, start, "1+1+1", {}, o);
	ps.clear();

/*******************************************************************************
*       ENCODING
*******************************************************************************/

#ifndef WIN32
	// char32_t parser with Unicode
	TEST("encoding", "u32")
	nonterminals<char32_t> nt32;
	prods<char32_t> ps32,
		start32(lit<char32_t>{ nt32.get(U"start"), &nt32 });
	ps32(start32, prods<char32_t>{U"τ"} | U"ξεσκεπάζω" | U"žluťoučký"
		| U"ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗ" | (start32 + start32));
	run_test<char32_t>(ps32, nt32, start32,
		U"τžluťoučkýτᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗτξεσκεπάζωτ", {}, o);
	ps32.clear();
#endif

/*******************************************************************************
*       BOOLEAN
*******************************************************************************/

	// nonzero digit
	TEST("boolean", "nzdigit")
	ps(start,      zero | (nzdigit + digits));
	ps(digits,     (digit + digits) | nll);
	ps(nzdigit,    digit & ~zero);
	run_test<char>(ps, nt, start, "0", cc);
	run_test<char>(ps, nt, start, "12", cc, o);
	o = {}, o.error_expected = "Unexpected '0' at 1:1 (1)";
	run_test<char>(ps, nt, start, "01", cc, o);
	ps.clear();

	// string escaping
	TEST("boolean", "string")
	ps(char_, (a | b | c) & ~(q_str | esc));
	ps(escape,      esc + (q_str | esc));
	ps(string_char, char_ | escape);
	ps(string_chars,  (string_char + string_chars) | nll);
	ps(string_, q_str + string_chars + q_str);
	ps(start, string_);
	run_test<char>(ps,nt,start, "\"\"");                         // 0
	run_test<char>(ps,nt,start, "\"a\"");                        // 1
	run_test<char>(ps,nt,start, "\"abc\"");                      // 2
	run_test<char>(ps,nt,start, "\"\\\"a\\\"c\\\"\"");           // 3
	run_test<char>(ps,nt,start, "\"\\\"a\\\"a\\\"a\\\"a\\\"\""); // 4
	o.error_expected = "Unexpected '\"' at 1:2 (2)";
	run_test<char>(ps,nt,start, "\"\"c\"", {}, o);               // 5
	ps.clear();

	// conjunction and negation in the starting rule
	TEST("boolean", "ac_not_bc")
	ps(start, (A + c) & ~(B + c));
	ps(A,     a | c);
	ps(B,     b | c);
	run_test<char>(ps, nt, start, "ac");
	o.error_expected = "Unexpected \"bc\" at 1:1 (1)";
	run_test<char>(ps, nt, start, "bc", {}, o);
	o.error_expected = "Unexpected \"cc\" at 1:1 (1)";
	run_test<char>(ps, nt, start, "cc", {}, o);
	ps.clear();

	// conjunction and negation in the starting rule
	TEST("boolean", "start")
	ps(start, a & ~b);
	run_test<char>(ps, nt, start, "a");
	o.error_expected = "Unexpected 'b' at 1:1";
	run_test<char>(ps, nt, start, "b", {}, o);
	ps.clear();

	// conjunction and negation with nonterminal
	TEST("boolean", "nonterminal")
	ps(start, X & ~b);
	ps(X,     a | b);
	run_test<char>(ps, nt, start, "a");
	o.error_expected = "Unexpected 'b' at 1:1";
	run_test<char>(ps, nt, start, "b", {}, o);
	ps.clear();

	// conjunction + negation of nonterminals deeper in grammar
	TEST("boolean", "deep_nonterminal")
	ps(start, expression);
	ps(expression, char_);
	ps(char_, enabled & ~disabled);
	ps(enabled,    A | B);
	ps(disabled,   A);
	ps(A,          a);
	ps(B,          b);
	run_test<char>(ps, nt, start, "b");
	o.error_expected = "Unexpected 'a' at 1:1";
	run_test<char>(ps, nt, start, "a", {}, o);
	ps.clear();

	// conjunction and negation of overlapping groups with parent nodes
	TEST("boolean", "overlap")
	//ps(start,      expression);
	ps(start,      T | X & ~Y);
	ps(X,          a | one);
	ps(T,          Y);
	ps(Y,          one);
	o = {}, o.contains = "X";
	run_test<char>(ps, nt, start, "a", {}, o);
	o.contains = "T";
	run_test<char>(ps, nt, start, "1", {}, o);
	ps.clear();

	// keyword vs identifier
	TEST("boolean", "keyword")
	ps(start,      identifier | keyword);
	ps(identifier, chars & ~ keyword);
	ps(chars,      alpha | (chars + alnum));
	ps(keyword,    {"print"});
	o.contains = "identifier";
	run_test<char>(ps, nt, start, "var123", cc, o);
	o.contains = "keyword";
	run_test<char>(ps, nt, start, "print",  cc, o);
	ps.clear();

	// operator priority
	TEST("boolean", "priority")
	ps(start,      expression);
	ps(expression, sum | mul & ~sum | digit);
	ps(sum,        expression + '+' + expression);
	ps(mul,        expression + '*' + expression);
	run_test<char>(ps, nt, start, "1+2*3", cc);
	run_test<char>(ps, nt, start, "3*2+1", cc);
	ps.clear();

	// all cycles
	// TODO this should be ambiguous but not in start
	// conjunction w/o negation cannot make multiple trees
	TEST("boolean", "all_cycles")
	ps(start, A & B);
	ps(A, X);
	ps(X, start | b);
	ps(B, T | b);
	ps(T, start);
	o = {}, o.ambiguity_fails = false;
	run_test<char>(ps, nt, start, "b", {}, o);
	ps.clear();

	TEST("bug", "dynamic_forest")
	ps(A,  start + A);
	ps(A , a);
	ps(start, A);
	ps(start, start );
	o = {}, o.ambiguity_fails = false;
	run_test<char>(ps, nt, start, "aaa", {}, o);
	ps.clear();
/*******************************************************************************
*       STRESS
*******************************************************************************/

	if (testing::stress) {
		if (testing::verbosity > 0)
			cout << "stress test started..." << c << endl;

		TEST("stress", "cbf_rule")
		const char* g1 =
			"@use char classes alpha, alnum."
			"chars    => alpha (alnum)*."
			"cbf_rule => cbf_head | chars '=' cbf."
			"cbf_head => cbf."
			"cbf      => bf | chars."
			"bf       => '1' | '0'."
			"start    => cbf_rule."
			;
		run_test_tgf(g1, string("something=0"));

		TEST("stress", "grammar")
		run_test_tgf(
		"        start => A B C D E F G H A B C D E F G H start \n "
		"               | null. \n"
		"        A     => 'a' | A | null. \n"
		"        B     => 'b' | B | null. \n"
		"        C     => 'c' | C | null. \n"
		"        D     => 'd' | D | null. \n"
		"        E     => 'e' | E | null. \n"
		"        F     => 'f' | F | null. \n"
		"        G     => 'g' | G | null. \n"
		"        H     => 'h' | H | null. \n"
		, string("a"));

		TEST("stress", "input")
		ifstream t("./tests/stress_test.txt");
		stringstream input;
		input << t.rdbuf();
		run_test_tgf("start => '(' start ')' start | null.",
			input.str());
		t.close();

		TEST("stress", "gram_long")
		ifstream t1("./tests/stress_test1.txt");
		stringstream gram;
		gram << t1.rdbuf();
		run_test_tgf(gram.str().c_str(), "");

		if (testing::verbosity > 0)
			cout << "stress test finished" << endl;
	}

	cout << endl;
	if (testing::failed) cout << "FAILED\n";
	return testing::failed ? 1 : 0;
}
