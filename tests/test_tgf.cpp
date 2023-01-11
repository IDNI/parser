#include "../src/parser.h"
using namespace std;
using namespace idni;

bool run_test(const char* g_tgf, const string& input) {
	nonterminals<char> nts;
	grammar<char> g = tgf<char>::from_string(nts, g_tgf);
	//g.print_data(cout << "grammar data:\n") << endl;
	//g.print_internal_grammar(cout << "grammar rules:\n") << endl;
	if (g.size() == 0) return false;
	parser<char> p(g);
	auto f = p.parse(input.c_str(), input.size());
	if (!p.found()) {
		//DBG(g.print_internal_grammar(cout << "grammar productions:\n") << endl;)
		auto error = p.get_error();
		cerr << error.to_str();
		return false;
	}
	//f->print_data(cout << "FOREST:\n") << endl;
	return true;
}

int main() {

	bool failed = false;
	auto fail = [&failed]() { cerr << "\nFAIL\n"; failed = true; exit(1); };

	if (!run_test(
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
	, "(1+2)*3/2")) fail();

	if (!run_test(
	"	start => A B & D C. \n"
	"	A  => 'a' A     | null. \n"
	"	B  => 'b' B 'c' | null. \n"
	"	C  => 'c' C     | null. \n"
	"	D  => 'a' D 'b' | null. \n"
	, "abc")) fail();

	if (!run_test(
	"	start => X & ~'b'. \n"
	"	X  => 'a' | 'b'. \n"
	, "a")) fail();

	if (!run_test(
	"	start  => binary [ two ]. \n"
	"	binary => '0' | '1'. \n"
	"	two    => '2'. \n"
	, "02")) fail();

	if (!run_test(
	"	start  => { binary } | { two }. \n"
	"	binary => '0' | '1'. \n"
	"	two    => '2'. \n"
	, "0101010100001001111")) fail();

	if (!run_test(
	"	@use_char_class digit. \n"
	"	start  => digit+. \n"
	, "1382746358690")) fail();

	if (!run_test(
	"	@use_char_class digit. \n"
	"	start  => digit*. \n"
	, "1382746358690")) fail();

	if (!run_test(
	"	start  => '1' ( '0' ) '1'. \n"
	, "101")) fail();

	if (!run_test(
	"	start  => '1' ( '0' '0' )+ '1'. \n"
	, "100001")) fail();

	if (!run_test(
	"	start  => '1' ( '0' '0' )* '1'. \n"
	, "11")) fail();

	if (!run_test(
	"	start  => '1' { '0' } '1'. \n"
	, "1000001")) fail();

	if (!run_test(
	"	start  => '1' [ '0' ] '1'. \n"
	, "11")) fail();

	if (!run_test(
	"	start  => [ '0' ]. \n"
	, "")) fail();

	if (!run_test(
	"	start  => \"hi\". \n"
	, "hi")) fail();

	return failed ? 1 : 0;
}
