#include "../src/parser.h"
using namespace std;
using namespace idni;

int main() {
	nonterminals<char> nts;
	
	grammar<char> g = tgf<char>::from_string(nts,
	"	@use_char_class eof, digit, space, printable. \n"

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
	);

//	grammar<char> g = tgf<char>::from_string(nts,
//	"	start => A B & D C. \n"
//	"	A  => 'a' A     | null. \n"
//	"	B  => 'b' B 'c' | null. \n"
//	"	C  => 'c' C     | null. \n"
//	"	D  => 'a' D 'b' | null. \n"
//	);

//	grammar<char> g = tgf<char>::from_string(nts,
//	"	start => X & ~'b'. \n"
//	"	X  => 'a' | 'b'. \n"
//	);

	g.print_data(cout, "\t") << endl;

	//nonterminals<char> nts0;
	//auto nt = [&nts0](const string&s){return lit<char>{nts0.get(s),&nts0};};
	//prods<char> Q, a('a'), b('b'), c('c'), nll('\0'),
	//	A(nt("A")), B(nt("B")), C(nt("C")), D(nt("D")), S(nt("start"));
	//Q(S, (A + B) & (D + C));
	//Q(A, (a + A)     | nll);
	//Q(B, (b + B + c) | nll);
	//Q(C, (c + C)     | nll);
	//Q(D, (a + D + b) | nll);
	//grammar<char> g0(nts0, Q, S, {});
	//g0.print_data(cout, "\t") << endl;

	//return 0;

	cout << "PARSER\n";
	parser<char> p(g);
	cout << "PARSE\n";
	const string s = "1+2*3";
	auto f = p.parse(s.c_str(), s.size());
	bool found = p.found();
	cout << (found ? "FOUND" : "FAILED") << endl;
	f->print_data(cout << "FOREST:\n") << endl;
}
