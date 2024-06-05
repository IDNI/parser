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
using testing::run_test_tgf;

int main(int argc, char **argv) {

	testing::process_args(argc, argv);

	testing::test_options o;
	o.ambiguity_fails = false;

	// test parsing of a TGF having a comment but no statements
	nonterminals<char> nts;
	tgf<char>::from_string(nts, "	# TGF only with ws and ws_comment \n");

	TEST("basic", "char terminals")
	run_test_tgf(" start => 'a'. ", "a");
	run_test_tgf(" start => \"a\". ", "a");
	run_test_tgf(" start => 'a' 'b'. ", "ab");
	run_test_tgf(" start => \"a\" \"b\". ", "ab");
	run_test_tgf(" start => \"ab\". ", "ab");

	TEST("basic", "literals rule")
	run_test_tgf(
	"	start  => \"hi\". \n"
	, "hi");

	TEST("basic", "comment tabs")
	run_test_tgf(
	"	#	comment	with	tabs	\n"
	"	start  => '1'. \n"
	, "1");

	TEST("basic", "ambiguous with nulls")
	run_test_tgf(
	"	start => A B & D C. \n"
	"	A  => 'a' A     | null. \n"
	"	B  => 'b' B 'c' | null. \n"
	"	C  => 'c' C     | null. \n"
	"	D  => 'a' D 'b' | null. \n"
	, "abc", o);

	TEST("boolean", "conjunction and negation")
	run_test_tgf(
	"	start => X & ~'b'. \n"
	"	X  => 'a' | 'b'. \n"
	, "a");

	TEST("ebnf", "optional")
	run_test_tgf(
	"	start  => binary [ two ]. \n"
	"	binary => '0' | '1'. \n"
	"	two    => '2'. \n"
	, "02");
	run_test_tgf(
	"	start  => '1' [ '0' ] '1'. \n"
	, "11");
	run_test_tgf(
	"	start  => [ '0' ]. \n"
	, "");


	TEST("ebnf", "zero or any")
	run_test_tgf(
	"	start  => { binary } | { two }. \n"
	"	binary => '0' | '1'. \n"
	"	two    => '2'. \n"
	, "0101010100001001111");
	run_test_tgf(
	"	start  => '1' { '0' } '1'. \n"
	, "1000001");

	TEST("ebnf", "plus")
	run_test_tgf(
	"	@use char classes digit. \n"
	"	start  => digit+. \n"
	, "1382746358690");
	run_test_tgf(
	"	start  => 'a' 'b'+ 'c'. \n"
	, "abbbc");

	TEST("ebnf", "asterisk")
	run_test_tgf(
	"	@use char classes digit. \n"
	"	start  => digit*. \n"
	, "1382746358690");

	TEST("ebnf", "group")
	run_test_tgf(
	"	start  => '1' ( '0' ) '1'. \n"
	, "101");

	TEST("ebnf", "group plus")
	run_test_tgf(
	"	start  => '1' ( '0' '0' )+ '1'. \n"
	, "100001");

	TEST("ebnf", "group asterisk")
	run_test_tgf(
	"	start  => '1' ( '0' '0' )* '1'. \n"
	, "11");

	TEST("directives", "start directive")
	run_test_tgf(
	"	@start S. \n"
	"	S  => S S S | S S | '1'. \n"
	, "111");

	if (testing::stress) {
		TEST("stress", "basic arithmetic")
		run_test_tgf(
		"	@use char class eof, digit, space, printable. \n"

		"	ws_comment   => '#' eol | '#' printable_chars eol. \n"
		"	ws_required  => space ws | ws_comment ws. \n"
		"	ws           => ws_required | null. \n"
		"	eol          => '\\r' | '\\n' | eof. \n"
		"	printable_chars => printable printable_chars1. \n"
		"	printable_chars1=> printable printable_chars1 | null. \n"

		"	sign         => '+' | '-'. \n"
		"	integer      => digit integer_rest. \n"
		"	integer_rest => digit integer_rest | null. \n"

		"	expr         => expr_op | term. \n"
		"	term         => term_op | factor. \n"
		"	expr_op      => expr ws addsub ws term. \n"
		"	term_op      => term ws muldiv ws factor. \n"
		"	addsub       => '+' | '-'. \n"
		"	muldiv       => '*' | '/'. \n"
		"	factor       => '(' ws expr ws ')' | sign ws factor | integer. \n"

		"	statement    => ws expr. \n"
		"	statements   => statement statements1. \n"
		"	statements1  => eol statement statements1 | null. \n"
		"	start        => statements ws | null. \n"
		, "(1+2)*3/2");

	}

	if (testing::failed) cout << "FAILED\n";
	return testing::failed ? 1 : 0;
}
