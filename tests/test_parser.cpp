#include <sstream>
#include <fstream>
#include <utility>
#include "../src/parser.h"

using namespace std;
using namespace idni;

template <typename T = char>
typename parser<T>::options options;

static bool opt_edge = true;
static size_t c = 0;
static size_t stop_after = SIZE_MAX;
static bool binarize = false;
static bool incr_gen = false;
static bool stress = false;
static bool enable_gc = false;

template <typename T>
int test_out(int c, const typename grammar<T>::grammar& g,
	const basic_string<T>& inputstr,
	typename parser<T>::pforest& f)
{
	stringstream ptd;
	stringstream ssf;

	g.print_internal_grammar(ssf, "\\l");
	string s = ssf.str();
	ssf.str({});

	ssf << "forest" << c << ".dot";
	ofstream file(ssf.str());
	to_dot<T>(ptd, as_const(f), to_std_string(inputstr), s);
	file << ptd.str();
	file.close();
	ssf.str({});
	ptd.str({});

	ssf << "forest_facts" << c << ".tml";
	ofstream file1(ssf.str());
	to_tml_facts<T>(ptd, as_const(f));
	file1 << ptd.str();
	file1.close();
	ssf.str({});
	ptd.str({});

	ssf << "forest_grammar_rules" << c << ".tml";
	ofstream file2(ssf.str());
	to_tml_rules<T>(ptd, as_const(f));
	file2 << ptd.str();
	file2.close();

	size_t i = 0;
	auto dump_files = [&](typename parser<T>::pgraph& g, string suffix="") {
		ssf.str({});
		ptd.str({});
		ssf << "graph" << c << "_" << i << suffix << ".dot";
		ofstream filet(ssf.str());
		to_dot<T, T, typename parser<T>::pgraph>(ptd, g, to_std_string(inputstr), s);
		filet << ptd.str();
		filet.close();
		ssf.str({});
		ptd.str({});
		ssf << "graph_grammar_rules" << c << "_" << i << suffix << ".tml";
		filet.open(ssf.str());
		to_tml_rules<T>(ptd, g);
		filet << ptd.str();
		filet.close();

		typename parser<T>::psptree tr= g.extract_trees();
		ssf.str({});
		ptd.str({});
		ssf << "tree" << c << "_" << i << suffix << ".dot";
		filet.open(ssf.str());
		to_dot<T>(ptd, tr, to_std_string(inputstr), s);
		filet << ptd.str();
		filet.close();
	};
	auto cb_next_graph = [&](typename parser<T>::pgraph& g){
		//std::cout<<"Has cycle :"<< f.detect_cycle(g)<<std::endl;
		f.detect_cycle(g);
		dump_files(g);
		if (binarize && f.remove_binarization(g))
			dump_files(g, "rembin");
		i++;

		return i < stop_after ? true : false;
	};
	//if(!stress)
	f.extract_graphs(f.root(), cb_next_graph, opt_edge);
	return 1;
}
bool run_tests(const char* g_tgf, const string input) {
	nonterminals<char> nts;
	grammar<char> g = tgf<char>::from_string(nts, g_tgf);
	//g.print_data(cout << "grammar data:\n") << endl;
	//g.print_internal_grammar(cout << "grammar rules:\n") << endl;
	if (g.size() == 0) return false;
	parser<char> p(g, options<char>);
	cout<< "\n Stress test Started ..."<<c <<endl;
	auto f = p.parse(input.c_str(), input.size());
	cout << "#found " << p.found() << "\n";
	cout << "#count " << f->count_trees() << std::endl;
	cout << "#ambiguous " << f->is_ambiguous() << endl;
	

	test_out(c++, g, input.size() > 100 ? 
				input.substr(0,100):input, *f);
	
	cout<< "Stress test end ..." <<endl;

	if (!p.found()) {
		DBG(g.print_internal_grammar(cout << "grammar productions:\n") << endl;)
		auto error = p.get_error();
		cerr << error.to_str();
		return false;
	}

	//f->print_data(cout << "FOREST:\n") << endl;
    return /*test_out<char>(c++, g, input, *f),*/ true;
}

template <typename T>
bool run_test(const prods<T>& ps, nonterminals<T>& nts,
	const prods<T>& start, const basic_string<T>& input,
	char_class_fns<T> cc = {}, bool dump = false, string contains = "",
	string error_expected = "")
{
	grammar<T> g(nts, ps, start, cc);
	parser<T> e(g, options<T>);

	cout << "#test started " << c << endl;
	cerr << "parsing: `" << to_std_string(input) << "` [" << input.size() << "]" << endl;

	auto f = e.parse(input.c_str(), input.size());
	bool found = e.found();
	//f->print_data(cout << "dumping forest:\n") << "\n\n";
	bool contained = false;
	if (found && (dump || contains.size()))
	{
		f->traverse([&dump, &contains, &contained](const auto& n)
					{
			if (contains == n.first.to_std_string()) contained=true;
			if (dump) cout << "entering: `" <<
				n.first.to_std_string() << "`\n"; },
					[&dump](const auto& n, const auto&)
					{
						if (dump) cout << "exiting: `"<<
							n.first.to_std_string()
							<< "`\n";
					});
	}
	cout << "#found " << found << "\n";
	cout << "#count " << f->count_trees() << endl;
	cout << "#ambiguous " << f->is_ambiguous() << endl;
	
	// if (contains.size())
	//	cout << "contains: `" << contains << "`: " << contained << endl;
	test_out<T>(c++, g, input, *f);

	if (!found) cerr << e.get_error().to_str() << endl;

	cout << "#test ended " << c << endl;
	
	if (error_expected.size()) { // negative test case
		auto msg = e.get_error().to_str();
		if (msg.find(error_expected) == decltype(error_expected)::npos)
			return //cerr << "\nFailed to find \"" << error_expected
				//<< "\" in: " << msg << endl,
				false;
		return true;
	}
	if (!found || (contains.size() && !contained))
		return false;


	return true;
}
void help(const string& opt) {
	cout << "Invalid option: " << opt << "\n" <<
		"Valid options: \n \
			-[enable|disable]_incrgen 		enables incremental generation of forest \n \
			-[enable|disable]_binarization 		enables binarization and leftright optimization of forest \n \
			-[enable|disable]_gc	enables garbage collection \n \
			-unique_node		retrieves graphs from forest based on nodes, not edges \n \
			-stop_after_[1|5]	stop retrieving further graphs after 1 or 5 count \n"
		<< endl;
}
int main(int argc, char **argv)
{

	vector<string> args(argv + 1, argv + argc);

	for (auto opt : args)
	{
		if (opt == "-enable_binarization")
			binarize = true;
		else if (opt == "-enable_incrgen")
			incr_gen = true;
		else if (opt == "-disable_binarization")
			binarize = false;
		else if (opt == "-disable_incrgen")
			incr_gen = false;
		else if (opt == "-enable_gc")
			enable_gc = true;
		else if (opt == "-disable_gc")
			enable_gc = false;
		else if (opt == "-unique_node")
			opt_edge = false;
		else if (opt == "-stop_after_1")
			stop_after = 1;
		else if (opt == "-stop_after_5")
			stop_after = 5;
		else if (opt == "-stress")
			stress = true;
		else help(opt), exit(1);
	}
	cout << "Running with options: ";
	for_each(args.begin(), args.end(),[](string& a) { cout << a << " "; });
	cout << endl;

	options<char>.binarize =        options<char32_t>.binarize = binarize;
	options<char>.incr_gen_forest =	options<char32_t>.incr_gen_forest =
								incr_gen;
	options<char>.enable_gc = options<char32_t>.enable_gc = enable_gc;

	nonterminals<> nt;
	char_class_fns cc = predefined_char_classes<>(
		{ "eof", "alpha", "alnum", "digit", "printable" }, nt);

	prods<> ps, start(nt("start")), nll(lit<>{}),
		alnum(nt("alnum")), alpha(nt("alpha")), digit(nt("digit")),
		chars(nt("chars")), expression(nt("expression")),
		sum(nt("sum")),	mul(nt("mul")),
		m1(nt("m1")), s1(nt("s1")), m2(nt("m2")), s2(nt("s2")),
		identifier(nt("identifier")), keyword(nt("keyword")),
		a('a'), b('b'), c('c'), n('n'), p('p'), m('m'),
		A(nt("A")), B(nt("B")), T(nt("T")), X(nt("X")),
		one('1'), PO(nt("PO")), IO(nt("IO")),
		plus('+'), minus('-'), mult('*'),
		cr('\r'), lf('\n'), eof(nt("eof")), eol(nt("eol")),
		string_(nt("string")), string_char(nt("string_char")),
		string_chars(nt("string_chars")),
		string_chars1(nt("string_chars1")),
		char_(nt("char_")), char0(nt("char0")),
		printable(nt("printable")),
		enabled(nt("enabled")), disabled(nt("disabled")),
		q_str(lit<>{'"'}), esc(lit<>{'\\'});

	bool failed = false;
	auto fail = [&failed]() { cerr << "\nFAIL\n"; failed = true; exit(1); };

	// Using Elizbeth Scott paper example 2, pg 64
	ps(start, b | (start + start));
	if (!run_test<char>(ps, nt, start, "bbb")) fail();
	//negative tests
	if (!run_test<char>(ps, nt, start, "bbba",{},false,"","Unexpected")) fail();
	if (!run_test<char>(ps, nt, start, "a", {}, false, "", "'a'")) fail();
	if (!run_test<char>(ps, nt, start, "a", {}, false, "", "'b'")) fail();
	if ( run_test<char>(ps, nt, start, "a", {}, false, "","success"))fail();
	ps.clear();

	// infinite ambiguous grammar, advanced parsing pdf, pg 86
	// will capture cycles
	ps(start, b | start);
	if (!run_test<char>(ps, nt, start, "b")) fail();
	ps.clear();

	// another ambigous grammar
	ps(start, (a + X + X + c) | start);
	ps(X,     (X + b) | nll);
	if (!run_test<char>(ps, nt, start, "abbc")) fail();
	ps.clear();

	// highly ambigous grammar, advanced parsing pdf, pg 89
	ps(start, (start + start) | a);
	if (!run_test<char>(ps, nt, start, "aaaaa")) fail();
	ps.clear();

	// using Elizabeth sott paper, example 3, pg 64.
	ps(start, (A + T) | (a + T));
	ps(A,     a | (B + A));
	ps(B,     nll);
	ps(T,     b + b + b);
	if (!run_test<char>(ps, nt, start, "abbb")) fail();
	ps.clear();

	ps(start, b | (start + start + start) | nll);
	if (!run_test<char>(ps, nt, start, "b")) fail();
	ps.clear();

	ps(start, n | (start + X + start));
	ps(X,     p | m);
	if (!run_test<char>(ps, nt, start, "npnmn")) fail();
	ps.clear();

	// char32_t parser with Unicode
	nonterminals<char32_t> nt32;
	prods<char32_t> ps32,
		start32(lit<char32_t>{ nt32.get(U"start"), &nt32 });
	ps32(start32, prods<char32_t>{U"τ"} | U"ξεσκεπάζω" | U"žluťoučký"
		| U"ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗ" | (start32 + start32));
	if (!run_test<char32_t>(ps32, nt32, start32,
		U"τžluťoučkýτᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗτξεσκεπάζωτ")) fail();
	ps32.clear();

	// thesis van, figure 4.11
	ps(start, one | (start + IO + start) | PO + start);
	ps(IO, plus | minus | mult);
	ps(PO, minus);
	if (!run_test<char>(ps, nt, start, "1+1+1")) fail();
	ps.clear();

	// // thesis van, 4.1 fig example
	ps(start,       plus + start | start + plus + start |
			start + plus | one);
	if (!run_test<char>(ps, nt, start, "1+++1")) fail();
	ps.clear();

	cout << "others" << endl;

	// conjunction + negation
	ps(start, X & ~b);
	ps(X,     a | b);
	if (!run_test<char>(ps, nt, start, "a")) fail();
	if ( run_test<char>(ps, nt, start, "b")) fail();
	ps.clear();

	// conjunction + negation of nonterminals deeper in grammar
	ps(start, expression);
	ps(expression, char_);
	ps(char_, enabled & ~disabled);
	ps(enabled,    A | B);
	ps(disabled,   A);
	ps(A,          a);
	ps(B,          b);
	if (!run_test<char>(ps, nt, start, "b")) fail();
	if ( run_test<char>(ps, nt, start, "a")) fail();
	ps.clear();

	// keyword vs identifier
	ps(start,      identifier | keyword);
	ps(identifier, chars & ~ keyword);
	ps(chars,      alpha | (chars + alnum));
	ps(keyword,    {"print"});
	if (!run_test<char>(ps,nt,start,"var123",cc,false,"identifier")) fail();
	if (!run_test<char>(ps,nt,start,"print", cc,false,"keyword"))    fail();
	ps.clear();

	// operator priority
	ps(start,      expression);
	ps(expression, sum | mul & ~sum | digit);
	ps(sum,        expression + '+' + expression);
	ps(mul,        expression + '*' + expression);
	if (!run_test<char>(ps, nt, start, "1+2*3", cc)) fail();
	if (!run_test<char>(ps, nt, start, "3*2+1", cc)) fail();
	ps.clear();

	// all cycles
	ps(start, A & B);
	ps(A, X);
	ps(X, start | b);
	ps(B, T | b);
	ps(T, start);
	if (!run_test<char>(ps, nt, start, "b")) fail();
	ps.clear();

	ps(char_, a | q_str);
	ps(char0, char_ | (esc + char_));
	ps(string_char, char0 & ~q_str);
	ps(string_chars,  string_char + string_chars1);
	ps(string_chars1, (string_char + string_chars1) | nll);
	ps(string_, (q_str + string_chars + q_str));
	ps(start, string_ | (string_ + start));
	if (!run_test<char>(ps, nt, start, "\"\\\"a\"\"\\\"a\"\"\\\"a\"\"\\\"a\\\"\"")) fail();
	ps.clear();

	// test eof / eol
	ps(start, eof);
	if (!run_test<char>(ps, nt, start, "", cc)) fail();
	ps.clear();
	ps(start, a + eof);
	if (!run_test<char>(ps, nt, start, "a", cc)) fail();
	ps.clear();
	ps(start, eol);
	ps(eol,   cr | lf | eof);
	if (!run_test<char>(ps, nt, start, "\n", cc)) fail();
	if (!run_test<char>(ps, nt, start, "\r", cc)) fail();
	if (!run_test<char>(ps, nt, start, "",   cc)) fail();
	ps.clear();
	ps(start, a + eol);
	ps(eol,   cr | lf | eof);
	if (!run_test<char>(ps, nt, start, "a\n", cc)) fail();
	if (!run_test<char>(ps, nt, start, "a\r", cc)) fail();
	if (!run_test<char>(ps, nt, start, "a",   cc)) fail();
	ps.clear();

	ps(start, a + b + c);
	if (!run_test<char>(ps, nt, start, "a", {},false,"","Unexpected end of file at 1:2 (2)")) fail();
	if (!run_test<char>(ps, nt, start, "d", {},false,"","Unexpected 'd' at 1:1 (1)")) fail();
	if (!run_test<char>(ps, nt, start, "ab", {},false,"","Unexpected end of file at 1:3 (3)")) fail();
	if (!run_test<char>(ps, nt, start, "abcd", {},false,"","Unexpected 'd' at 1:4 (4)")) fail();
	if (!run_test<char>(ps, nt, start, "abcde", {},false,"","Unexpected 'd' at 1:4 (4)")) fail();
	if (!run_test<char>(ps, nt, start, "abcabc", {},false,"","Unexpected 'a' at 1:4 (4)")) fail();
	ps.clear();

	std::cout<<"Completed Basic tests\n ";

	if(stress) {
	
	std::cout<<"Starting Stress tests\n";

	const char* g1 =
	"@use_char_class eof, space, digit, xdigit, alpha, alnum, punct, printable."
	"chars			=> alpha (alnum)*."
	"cbf_rule 		=> cbf_head | chars '=' cbf."
	"cbf_head		=> cbf."
	"cbf 			=> bf | chars."
	"bf				=> '1' | '0'."
	"start			=> cbf_rule."
	;

	if( !run_tests(g1, string("something=0"))) fail();

	const char *g2 = R"(
@use_char_class eof, space, digit, xdigit, alpha, alnum, punct, printable.

#
# as a rule of thumb, to ease navigation of the corresponding tree, the
# non-terminals shouldn't appear in * or + expressions. Otherwise,
# intermediate non terminals would be created and break the expected tree
# structure
#

# whitespace and comments
eol				=> '\n' | '\r' | eof.
ws_comment		=> '#' eol | '#' printable+ eol.
ws_required		=> space ws | ws_comment ws.
ws				=> ws_required | null.

# characters
hex_escape			=> "\\x" xdigit xdigit.
unicode_escape		=> "\\u" xdigit xdigit xdigit xdigit.
char_escape_encode	=> hex_escape | unicode_escape.

# defining char/string/qstring as all chars but its wrapping character
# enables using TAB and new lines in char(')/string(")/bqstring(`)
# sequences

# common stuff copy cut from tml grammar
esc				=> "\\\\".
q_char			=> '\''.
q_str			=> '"'.
q_bqstr			=> '`'.
char_punct		=> punct & ~q_char & ~q_str & ~q_bqstr
					& ~(esc q_char) & ~(esc q_str) & ~(esc q_bqstr).
char0			=> alnum | space | char_escape_encode | char_punct.
char_			=> char0 | esc q_char |     q_str |     q_bqstr.
string_char		=> char0 |     q_char | esc q_str |     q_bqstr.
bqstring_char	=> char0 |     q_char |     q_str | esc q_bqstr.
chars			=> alpha (alnum)*.
char_class    	=> "eof" | "alnum" | "alpha" | "blank" | "cntrl" | "digit"
				| "graph" | "lower" | "printable" | "punct" | "space"
				| "upper" | "xdigit".
digits			=> digit (digit)*.

# common symbols
definition			=> ws ":=" ws.
dot 				=> ws '.' ws.
open_parenthesis	=> ws '(' ws.
close_parenthesis	=> ws ')' ws.
open_bracket		=> ws '[' ws.
close_bracket		=> ws ']' ws.
open_brace			=> ws '{' ws.
close_brace			=> ws '}' ws.
minus				=> ws '-' ws.
colon				=> ws ':' ws.
less				=> ws '<' ws.
comma 				=> ws ',' ws.

# elements
sym				=> chars.

# offsets
offsets			=> open_bracket step (comma offset)* close_bracket.
offset			=> num | capture | shift.
step			=> num | capture | shift.
shift 			=> capture minus num.
num				=> ws digits ws.

variable		=> var | timed.
timed			=> (in | out)  offsets.

# TODO (HIGH) remove ? from vars
capture			=> '$' chars.
var				=> chars.
in				=> "?i_" chars. # instead of '<', easy to remember
out				=> "?o_" chars. # instead of '>', easy to remember

# wff
wff_rule		=> wff_matcher definition wff_body dot.

# needed for rule CBF_DEF_IF
wff_matcher	=> wff | wff_ref.
wff_body	=> wff | bf_eq_cb | bf_neq_cb | wff_has_clashing_subformulas_cb
				| wff_has_subformula_cb | wff_remove_existential.
wff_ref			=> sym offsets wff_ref_args.
wff_ref_args	=> open_parenthesis (variable)* close_parenthesis.

wff 			=> wff_ref | wff_and | wff_neg | wff_xor | wff_conditional
					| wff_or | wff_all | wff_ex | wff_imply | wff_equiv
					| wff_t | wff_f | capture
					| bf_eq | bf_neq | bf_less | bf_less_equal | bf_greater.
wff_and			=> open_parenthesis wff wff_and_sym wff close_parenthesis.
wff_or			=> open_parenthesis wff wff_or_sym wff close_parenthesis.
wff_xor			=> open_parenthesis wff wff_xor_sym wff close_parenthesis.
wff_conditional	=> open_parenthesis wff wff_conditional_sym wff colon wff close_parenthesis.
wff_neg			=> wff_neg_sym wff.
wff_imply		=> open_parenthesis wff wff_imply_sym wff close_parenthesis.
wff_equiv		=> open_parenthesis wff wff_equiv_sym wff close_parenthesis.
wff_all			=> wff_all_sym (variable|capture) ws_required wff.
wff_ex			=> wff_ex_sym (variable|capture) ws_required wff.

# relational operators
#
# they are named bf_* as they involve boolean functions,
# but they are not boolean functions themselves, they return a T/F wff value
# and hence, should be considered as wffs
bf_eq 			=> open_parenthesis bf bf_equality_sym bf close_parenthesis.
bf_neq 			=> open_parenthesis bf bf_nequality_sym bf close_parenthesis.
bf_eq 			=> open_parenthesis bf bf_equality_sym bf close_parenthesis.
bf_neq 			=> open_parenthesis bf bf_nequality_sym bf close_parenthesis.
bf_less			=> open_parenthesis bf bf_less_sym bf close_parenthesis.
bf_less_equal	=> open_parenthesis bf bf_less_equal_sym bf close_parenthesis.
bf_greater		=> open_parenthesis bf bf_greater_sym bf close_parenthesis.

# wff_op_sym
wff_and_sym			=> ws "&&" ws.
wff_or_sym			=> ws "||" ws.
wff_xor_sym			=> ws '^' ws.
wff_conditional_sym	=> ws '?' ws.
wff_neg_sym			=> ws '!' ws.
wff_imply_sym		=> ws "->" ws.
wff_equiv_sym		=> ws "<->" ws.
wff_all_sym			=> ws "all" ws.
wff_ex_sym			=> ws "ex" ws.
wff_t				=> ws 'T' ws.
wff_f				=> ws 'F' ws.

# bf
bf_ref 		=> sym offsets bf_ref_args.
bf_ref_args	=> open_parenthesis (variable)* close_parenthesis.
bf_rule		=> bf_matcher definition bf_body dot.

bf_matcher	=> bf.
bf_body 	=> bf | bf_is_zero_cb | bf_is_one_cb | bf_has_clashing_subformulas_cb
				| bf_has_subformula_cb.
bf			=> bf_constant | bf_and | bf_neg | bf_xor | bf_or
				| bf_all | bf_ex
				# TODO (LOW) check proper use of bf_subs_cb in code
				#
				# we should have a check method that verifies that the user
				# is not uising subs in its code.
				| bf_subs_cb | bf_t | bf_f | variable | capture.
bf_and		=> open_parenthesis bf bf_and_sym bf close_parenthesis.
bf_or		=> open_parenthesis bf bf_or_sym bf close_parenthesis.
bf_xor		=> open_parenthesis bf bf_xor_sym ws bf close_parenthesis.
bf_neg		=> bf_neg_sym bf.
bf_all		=> bf_all_sym ws_required (variable | capture) ws_required bf.
bf_ex		=> bf_ex_sym ws_required (variable | capture) ws_required bf.

# bf_op_sym
bf_and_sym			=> ws '&' ws.
bf_or_sym			=> ws '|' ws.
bf_xor_sym			=> ws '+' ws.
bf_neg_sym			=> ws '~' ws.
bf_equality_sym		=> ws "=" ws.
bf_nequality_sym	=> ws "!=" ws.
bf_less_sym			=> ws '<' ws.
bf_less_equal_sym	=> ws "<=" ws.
bf_greater_sym		=> ws '>' ws.
bf_all_sym			=> ws "fall" ws.
bf_ex_sym			=> ws "fex" ws.
bf_t				=> ws '1' ws.
bf_f				=> ws '0' ws.

# constant
bf_constant		=> open_brace constant close_brace.

#constants
constant		=>  binding | capture
					| bf_and_cb | bf_or_cb | bf_xor_cb | bf_neg_cb.
binding			=> source_binding | named_binding.
named_binding	=> chars.
source_binding	=> type colon source.
type 			=> chars | null.

# source related definition
source0			=> alnum | space | char_escape_encode | char_punct.
source			=> (source0)+.

# callbacks

# callbacks must be used in the following cases:
#
# 1.- underlying boolean algebras operations: and, or, xor, neg, less,
# less_equal, greater, subs, eq, neq, is_zero, is_one,... In this case, no
# other way we have to call the uderlying operations of the boolean algebra
# 2.- speed up computations: has_clashing_subformulas, has_subformula,
# remove_existential... In this case, we could use the the callback to
# avoid the creation of intermediate formulas. For instance, if we want
# to check if a formula has a subformula, we could use the callback to
# avoid the creation of the subformulas to check that point.
# 3.- to create new subformulas in other rules, for instance, to create a
# new formula that is the substitution of a variable by a constant.

# TODO (HIGH) Earley parser doesn't support tabs in comments

bf_and_cb			=> bf_cb_arg bf_and_cb_sym bf_cb_arg.
bf_or_cb			=> bf_cb_arg bf_or_cb_sym bf_cb_arg.
bf_xor_cb			=> bf_cb_arg bf_xor_cb_sym bf_cb_arg.
bf_neg_cb			=> bf_neg_cb_sym bf_cb_arg.
bf_subs_cb			=> bf_subs_cb_sym bf_cb_arg ws_required bf_cb_arg ws_required bf_cb_arg.
bf_eq_cb			=> bf_eq_cb_sym bf_cb_arg ws_required wff_cb_arg ws_required wff_cb_arg.
bf_neq_cb			=> bf_neq_cb_sym bf_cb_arg ws_required wff_cb_arg ws_required wff_cb_arg.
bf_is_zero_cb		=> bf_is_zero_cb_sym bf_cb_arg ws_required bf_cb_arg.
bf_is_one_cb		=> bf_is_one_cb_sym bf_cb_arg ws_required bf_cb_arg.

# extra callbacks to speed up computations
bf_has_clashing_subformulas_cb	=> bf_has_clashing_subformulas_cb_sym bf_cb_arg ws_required bf_cb_arg.
wff_has_clashing_subformulas_cb	=> wff_has_clashing_subformulas_cb_sym wff_cb_arg ws_required wff_cb_arg.
bf_has_subformula_cb 			=> bf_has_subformula_cb_sym bf_cb_arg ws_required bf_cb_arg ws_required bf_cb_arg.
wff_has_subformula_cb 			=> wff_has_subformula_cb_sym wff_cb_arg ws_required wff_cb_arg ws_required wff_cb_arg.
wff_remove_existential			=> wff_remove_existential_cb_sym wff_cb_arg ws_required wff_cb_arg.

# bultin_args
bf_cb_arg	=> capture | bf.
wff_cb_arg	=> capture | wff.

# bf_cb_syms
bf_and_cb_sym			=> ws "bf_and_cb" ws.
bf_or_cb_sym			=> ws "bf_or_cb" ws.
bf_xor_cb_sym			=> ws "bf_xor_cb" ws.
bf_neg_cb_sym			=> ws "bf_neg_cb" ws.
bf_subs_cb_sym			=> ws "bf_subs_cb" ws.
bf_eq_cb_sym			=> ws "bf_eq_cb" ws.
bf_neq_cb_sym			=> ws "bf_neq_cb" ws.
bf_is_zero_cb_sym		=> ws "bf_is_zero_cb" ws.
bf_is_one_cb_sym		=> ws "bf_is_one_cb" ws.

# speed up callbacks syms
# IDEA we could reuse the same symbol
bf_has_clashing_subformulas_cb_sym		=> ws "bf_has_clashing_subformulas_cb" ws.
bf_has_subformula_cb_sym				=> ws "bf_has_subformula_cb" ws.
wff_has_clashing_subformulas_cb_sym		=> ws "wff_has_clashing_subformulas_cb" ws.
wff_has_subformula_cb_sym				=> ws "wff_has_subformula_cb" ws.
wff_remove_existential_cb_sym			=> ws "wff_remove_existential_cb" ws.

# input definition
input			=> in colon open_brace source_binding close_brace.

# main posibilities
inputs			=> less input (input)* dot.
main			=> wff dot.
rule			=> wff_rule | bf_rule .
rules			=> (rule)*.
formula			=> rules main.
library 		=> rules.
# each builder is define on its own string
builder			=> builder_head definition builder_body dot.
builder_head	=> open_parenthesis capture (ws_required capture)* close_parenthesis.
builder_body	=> wff | bf.
start			=> formula | library | builder | inputs.
)";

	if(!run_tests(g2, string("") )) fail();

    if (!run_tests(" start => A B C D E F G H A B C D E F G H start | null. \n \
                A => 'a'| A | null.\n\
                B => 'b'| B | null.\n\
                C => 'c'| C | null.\n\
                D => 'd'| D | null.\n\
                E => 'e'| E | null.\n\
                F => 'f'| F | null.\n\
                G => 'g'| G | null.\n\
                H => 'h'| H | null.\n", 
                string("a"))) fail();
	
	string input = "((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()(((()()()((()()())))))))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()))))((()()()(((()()()((()()()))))((()(((()()()((()()(((()()()((()(((()()()(((((()()()((()()())))))()())))))()))))))))))()((()()()))))((()()()((()(((()()()(((((()()()(((((()()()((()(((()()()((()()(((()()()((()()()))))))))))())))))()())))))()())))))()))))(()()()))))((()()()((()()()))))((()()()((()()()))))((()()()((()()()((()()()))))((()()()))))((()()()((()()()))))((()()()((()((()()()((()()()))))()()))))";
	
	if (!run_tests("start => '(' start ')' start | null. \n", 
                input)) fail();
	}



	return failed ? 1 : 0;
}
