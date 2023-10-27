#include <sstream>
#include <fstream>
#include <utility>
#include "../src/parser.h"

#include "testing.h"

using namespace std;
using namespace idni;

template <typename T = char>
typename parser<T>::options options;

static bool opt_edge = true;
static size_t c = 0;
static size_t stop_after = SIZE_MAX;
// if true enables stress tests
bool stress = false;

// saves resulting forest and graphs into files with various formats
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
		f.detect_cycle(g);
		dump_files(g);
		if (options<T>.binarize && f.remove_binarization(g))
			dump_files(g, "rembin");
		i++;

		return i < stop_after ? true : false;
	};

	f.extract_graphs(f.root(), cb_next_graph, opt_edge);
	return 1;
}

template <typename T>
void print_ambig_nodes(typename parser<T>::pforest& f) {
	if (!f.is_ambiguous()) return;
	std::cout << "ambiguous nodes:\n";
	for (auto& n : f.ambiguous_nodes()) {
		std::cout << "\t `" << n.first.first << "` [" <<
			n.first.second[0] << "," << n.first.second[1] << "]\n";
		size_t d = 0;
		for (auto ns : n.second) {
			std::cout << "\t\t " << d++ << "\t";
			for (auto nt : ns) std::cout <<	" `" << nt.first << "`["
				<< nt.second[0] << "," << nt.second[1] << "] ";
			std::cout << "\n";
		}
	}
}

// run test from tgf
bool run_test_tgf(const char* g_tgf, const string input) {
	if (!testing::info()) return true;
	bool expect_fail = false;

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


	if (testing::verbosity > 1) test_out(c++, g, input.size() > 100 ?
				input.substr(0,100):input, *f);

	cout<< "Stress test end ..." <<endl;

	if (!p.found()) {
		DBG(g.print_internal_grammar(cout << "grammar productions:\n") << endl;)
		if (testing::verbosity > 0) cerr << p.get_error().to_str();
		if (!expect_fail)
			return cout<<"FAILED\n", testing::failed = true, false;
	}
	cout << "OK\n";
	//f->print_data(cout << "FOREST:\n") << endl;
	return /*test_out<char>(c++, g, input, *f),*/ true;
}

// runs a test
template <typename T>
bool run_test(const prods<T>& ps, nonterminals<T>& nts,
	const prods<T>& start, const basic_string<T>& input,
	char_class_fns<T> cc = {}, string contains = "",
	string error_expected = "", bool dump = false)
{
	if (!testing::info()) return true;
	bool expect_fail = error_expected.size() > 0;

	grammar<T> g(nts, ps, start, cc);
	parser<T> e(g, options<T>);

	if (testing::verbosity == 0) dump = false;
	else g.print_internal_grammar(cout << "\ngrammar productions:\n") <<
		"\n\nparsing: `" << to_std_string(input) <<
		"` [" << input.size() << "]" << endl;
	auto f = e.parse(input.c_str(), input.size());
	bool found = e.found();
	string msg{};
	if (!found) msg = e.get_error()
			.to_str(parser<T>::error::info_lvl::INFO_DETAILED);
	bool contained = false;
	if (found && (dump || contains.size())) {
		auto cb_enter = [&dump, &contains, &contained](const auto& n) {
			if (contains == n.first.to_std_string()) contained=true;
			if (dump) cout << "entering: `" <<
				n.first.to_std_string() << "`\n";
		};
		auto cb_exit = [&dump](const auto& n, const auto&) {
			if (dump) cout << "exiting: `" <<
				n.first.to_std_string() << "`\n";
		};
		f->traverse(cb_enter, cb_exit);
	}
	if (testing::verbosity > 0) {
		cout << "\n";
		cout << "#found " << found << "\n";
		if (found)
			cout << "#trees " << f->count_trees() << "\n",
			cout << "#ambiguous " << f->is_ambiguous() << "\n";
		cout << "#test number " << c << "\n";
		cout << "#expect fail " << expect_fail << "\n";
		cout << "#error expected '" << error_expected << "'\n" << endl;
	} else if (found && testing::verbosity > 1)
		test_out<T>(c++, g, input, *f), print_ambig_nodes<T>(*f);

	if (expect_fail) found =
		msg.find(error_expected) == decltype(error_expected)::npos;
	if (msg.size() && (!expect_fail || testing::verbosity > 0))
		cout << "\nError message: '" << msg << "'\n";
	if ((found == expect_fail) || (found && contains.size() && !contained))
		return cout << "\tFAILED\n", testing::failed = true, false;
	return cout << "\tOK", true;
}

void help(const string& opt) {
	if (opt.size())	cout << "Invalid option: " << opt << "\n";
	cout <<
"Valid options:\n" <<
"	-help        -h         help (this text)\n" <<
"	-print_all   -p         prints names of all tests (includes skipped)\n" <<
"	-verbose     -v         increase verbosity (up to 2)\n" <<
"	-vv                     max verbosity (2)\n" <<
"	-[enable|disable]_incrgen\n" <<
"	              enable/disable incremental generation of forest\n" <<
"	-[enable|disable]_binarization\n" <<
"\t              enables binarization and leftright optimization of forest\n" <<
"	-unique_node\n" <<
"	              retrieves graphs from forest based on nodes, not edges\n"
"	-stop_after_[1|5]\n" <<
"		      stop retrieving further graphs after 1 or 5 count\n" <<
"Enabling/disabling tests (can be combined and used more than once):\n" <<
"	-group <group>          run <group> of tests\n" <<
"	-disable_group <group>  disable <group> of tests\n" <<
"	-test <test>            run <test>\n" <<
"	-disable_test <test>    disable running of <test>\n" <<
"	-id <id>                run N-th subtest where <id> is N\n" <<
	endl;
}

void process_args(int argc, char **argv) {
	bool binarize = false;
	bool incr_gen = false;

	vector<string> args(argv + 1, argv + argc);

	auto missing = [](const string& opt) {
		cerr << "Missing argument for options: " << opt << "\n";
		exit(1);
	};

	for (auto it = args.begin(); it != args.end(); ++it) {
		auto opt = *it;
		if (opt == "-enable_binarization")
			binarize = true;
		else if (opt == "-enable_incrgen")
			incr_gen = true;
		else if (opt == "-disable_binarization")
			binarize = false;
		else if (opt == "-disable_incrgen")
			incr_gen = false;
		else if (opt == "-unique_node")
			opt_edge = false;
		else if (opt == "-stop_after_1")
			stop_after = 1;
		else if (opt == "-stop_after_5")
			stop_after = 5;
		else if (opt == "-stress")
			stress = true;
		else if (opt == "-print_all" || opt == "-p")
			testing::print_all = true;
		else if (opt == "-verbose" || opt == "-v")
			testing::verbosity++;
		else if (opt == "-vv") testing::verbosity += 2;
		else if (opt == "-group") {
			it++;
			if (it == args.end()) missing(opt);
			testing::group(*it);
		}
		else if (opt == "-test") {
			it++;
			if (it == args.end()) missing(opt);
			testing::test(*it);
		}
		else if (opt == "-disable_group") {
			it++;
			if (it == args.end()) missing(opt);
			testing::disable_group(*it);
		}
		else if (opt == "-disable_test") {
			it++;
			if (it == args.end()) missing(opt);
			testing::disable_test(*it);
		}
		else if (opt == "-id") {
			it++;
			if (it == args.end()) missing(opt);
			std::stringstream is(*it);
			int_t result = 0;
			if (!(is >> result) || result < 0) {
				cerr<<"'"<<*it<<"' is not a positive number.\n";
				exit(1);
			}
			testing::id(result);
		}
		else if (opt == "-help" || opt == "-h") help(""), exit(0);
		else help(opt), exit(1);
	}
	if (testing::verbosity > 0) {
		cout << "Running with options: ";
		for_each(args.begin(), args.end(),
			[](string& a) { cout << a << " "; });
		cout << endl;
	}

	options<char>.binarize =        options<char32_t>.binarize = binarize;
	options<char>.incr_gen_forest =	options<char32_t>.incr_gen_forest =
								incr_gen;
}

int main(int argc, char **argv)
{
	process_args(argc, argv);

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
		A(nt("A")), B(nt("B")), T(nt("T")), X(nt("X")), Y(nt("Y")),
		one('1'), PO(nt("PO")), IO(nt("IO")),
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
	run_test<char>(ps, nt, start, "a", {},"","Unexpected end of file at 1:2 (2)");
	run_test<char>(ps, nt, start, "d", {},"","Unexpected 'd' at 1:1 (1)");
	run_test<char>(ps, nt, start, "ab", {},"","Unexpected end of file at 1:3 (3)");
	run_test<char>(ps, nt, start, "abc");
	run_test<char>(ps, nt, start, "abcd", {},"","Unexpected 'd' at 1:4 (4)");
	run_test<char>(ps, nt, start, "abcde", {},"","Unexpected 'd' at 1:4 (4)");
	run_test<char>(ps, nt, start, "abcabc", {},"","Unexpected 'a' at 1:4 (4)");
	ps.clear();


	TEST("basic", "terminal")
	ps(start, a | one | plus | lf);
	run_test<char>(ps, nt, start, "a");
	run_test<char>(ps, nt, start, "1");
	run_test<char>(ps, nt, start, "+");
	run_test<char>(ps, nt, start, "\n");
	run_test<char>(ps, nt, start, "b", {},"","Unexpected 'b' at 1:1 (1)");
	run_test<char>(ps, nt, start, "0", {},"","Unexpected '0' at 1:1 (1)");
	run_test<char>(ps, nt, start, "-", {},"","Unexpected '-' at 1:1 (1)");
	run_test<char>(ps, nt, start, "\r",{},"","Unexpected '\\r' at 1:1 (1)");
	ps.clear();

	// string escaping
	TEST("basic", "string")
	ps(char_, (a | b | c) & ~q_str);
	ps(escape,      esc + q_str);
	ps(string_char, char_ | escape);
	ps(string_chars,  (string_char + string_chars) | nll);
	ps(string_, q_str + string_chars + q_str);
	ps(start, string_);
	run_test<char>(ps,nt,start, "\"\"");                         // 0
	run_test<char>(ps,nt,start, "\"a\"");                        // 1
	run_test<char>(ps,nt,start, "\"abc\"");                      // 2
	run_test<char>(ps,nt,start, "\"a\"c\"", {}, "",
			"Unexpected '\"' at 1:3 (3)");               // 3
	run_test<char>(ps,nt,start, "\"\\\"a\\\"c\\\"\"");           // 4
	run_test<char>(ps,nt,start, "\"\\\"a\\\"a\\\"a\\\"a\\\"\""); // 5
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

/*******************************************************************************
*       RECURSION
*******************************************************************************/

	TEST("recursion", "b")
	ps(start, b | (start + start + start) | nll);
	run_test<char>(ps, nt, start, "b");
	ps.clear();

	TEST("recursion", "npnmn")
	ps(start, n | (start + X + start));
	ps(X,     p | m);
	run_test<char>(ps, nt, start, "npnmn");
	ps.clear();

/*******************************************************************************
*       PAPERS
*******************************************************************************/

	// Using Elizbeth Scott paper example 2, pg 64
	TEST("papers", "escott_ex2p64")
	ps(start, b | (start + start));
	run_test<char>(ps, nt, start, "bbb");
	run_test<char>(ps, nt, start, "bbba",{},"","Unexpected 'a' at 1:4 (4)");
	run_test<char>(ps, nt, start, "a",   {},"","Unexpected 'a' at 1:1 (1)");
	ps.clear();

	// infinite ambiguous grammar, advanced parsing pdf, pg 86
	// will capture cycles
	TEST("papers", "advparsing_p86")
	ps(start, b | start);
	run_test<char>(ps, nt, start, "b");
	ps.clear();

	// another ambigous grammar
	TEST("papers", "advparsing")
	ps(start, (a + X + X + c) | start);
	ps(X,     (X + b) | nll);
	run_test<char>(ps, nt, start, "abbc");
	ps.clear();

	// highly ambigous grammar, advanced parsing pdf, pg 89
	TEST("papers", "advparsing_p89")
	ps(start, (start + start) | a);
	run_test<char>(ps, nt, start, "aaaaa");
	ps.clear();

	// using Elizabeth sott paper, example 3, pg 64.
	TEST("papers", "escott_ex3p64")
	ps(start, (A + T) | (a + T));
	ps(A,     a | (B + A));
	ps(B,     nll);
	ps(T,     b + b + b);
	run_test<char>(ps, nt, start, "abbb");
	ps.clear();

	// thesis van, figure 4.11
	TEST("papers", "thesis_van_fig4.11")
	ps(start, one | (start + IO + start) | PO + start);
	ps(IO, plus | minus | mult);
	ps(PO, minus);
	run_test<char>(ps, nt, start, "1+1+1");
	ps.clear();

	// thesis van, 4.1 fig example
	TEST("papers", "thesis_van_fig4.1")
	ps(start,       plus + start | start + plus + start |
			start + plus | one);
	run_test<char>(ps, nt, start, "1+++1");
	ps.clear();

/*******************************************************************************
*       ENCODING
*******************************************************************************/

	// char32_t parser with Unicode
	TEST("encoding", "u32")
	nonterminals<char32_t> nt32;
	prods<char32_t> ps32,
		start32(lit<char32_t>{ nt32.get(U"start"), &nt32 });
	ps32(start32, prods<char32_t>{U"τ"} | U"ξεσκεπάζω" | U"žluťoučký"
		| U"ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗ" | (start32 + start32));
	run_test<char32_t>(ps32, nt32, start32,
		U"τžluťoučkýτᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗτξεσκεπάζωτ");
	ps32.clear();

/*******************************************************************************
*       BOOLEAN
*******************************************************************************/

	TEST("boolean", "ac_not_bc")
	ps(start, (A + c) & ~(B + c));
	ps(A,     a | c);
	ps(B,     b | c);
	run_test<char>(ps, nt, start, "ac");
	ps.clear();

	// conjunction and negation in the starting rule
	TEST("boolean", "start")
	ps(start, a & ~b);
	run_test<char>(ps, nt, start, "a");
	run_test<char>(ps, nt, start, "b", {}, "", "Unexpected 'b' at 1:1");
	ps.clear();

	// conjunction and negation with nonterminal
	TEST("boolean", "nonterminal")
	ps(start, X & ~b);
	ps(X,     a | b);
	run_test<char>(ps, nt, start, "a");
	run_test<char>(ps, nt, start, "b", {}, "", "Unexpected 'b' at 1:1");
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
	run_test<char>(ps, nt, start, "a", {}, "", "Unexpected 'a' at 1:1");
	ps.clear();

	// conjunction and negation of overlapping groups with parent nodes
	TEST("boolean", "overlap")
	//ps(start,      expression);
	ps(start,      T | X & ~Y);
	ps(X,          a | one);
	ps(T,          Y);
	ps(Y,          one);
	run_test<char>(ps, nt, start, "a", {}, "X");
	run_test<char>(ps, nt, start, "1", {}, "T");
	ps.clear();

	// keyword vs identifier
	TEST("boolean", "keyword")
	ps(start,      identifier | keyword);
	ps(identifier, chars & ~ keyword);
	ps(chars,      alpha | (chars + alnum));
	ps(keyword,    {"print"});
	run_test<char>(ps, nt, start, "var123", cc, "identifier");
	run_test<char>(ps, nt, start, "print",  cc, "keyword");
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
	TEST("boolean", "all_cycles")
	ps(start, A & B);
	ps(A, X);
	ps(X, start | b);
	ps(B, T | b);
	ps(T, start);
	run_test<char>(ps, nt, start, "b");
	ps.clear();

/*******************************************************************************
*       STRESS
*******************************************************************************/

	if (stress) {
		TEST("stress", "cbf_rule")
		const char* g1 =
			"@use_char_class alpha, alnum."
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
		std::ifstream t("./tests/stress_test.txt");
		std::stringstream input;
		input << t.rdbuf();
		run_test_tgf("start =>'(' start ')' start | null.",input.str());
	}

	cout << endl;
	if (testing::failed) cout << "FAILED\n";
	return testing::failed ? 1 : 0;
}
