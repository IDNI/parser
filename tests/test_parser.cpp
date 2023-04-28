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

template <typename T>
int test_out(int c, const typename grammar<T>::grammar &g,
	const basic_string<T> &inputstr,
	typename parser<T>::pforest &f)
{
	stringstream ptd;
	stringstream ssf;

	g.print_internal_grammar(ssf, "\\l");
	string s = ssf.str();
	ssf.str({});

	ssf << "forest" << c << ".dot";
	ofstream file(ssf.str());
	to_dot<T>(ptd, std::as_const(f), to_std_string(inputstr), s);
	file << ptd.str();
	file.close();
	ssf.str({});
	ptd.str({});

	ssf << "forest_facts" << c << ".tml";
	ofstream file1(ssf.str());
	to_tml_facts<T>(ptd, std::as_const(f));
	file1 << ptd.str();
	file1.close();
	ssf.str({});
	ptd.str({});

	ssf << "forest_grammar_rules" << c << ".tml";
	ofstream file2(ssf.str());
	to_tml_rules<T>(ptd, std::as_const(f));
	file2 << ptd.str();
	file2.close();

	size_t i = 0;
	auto dump_files = [&] (typename parser<T>::pgraph &g,
							std::string suffix = "") {
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
	auto cb_next_graph = [&](typename parser<T>::pgraph &g){
		f.detect_cycle(g);
		dump_files(g);
		if (options<T>.binarize) 
			f.remove_binarization(g),
			dump_files(g, "rembin");
		i++;

		return i < stop_after ? true : false;
	};

	f.extract_graphs(f.root(), cb_next_graph, opt_edge);
	return 1;
}
template <typename T>
bool run_test(const prods<T> &ps, nonterminals<T> &nts,
	const prods<T> &start, const basic_string<T> &input,
	char_class_fns<T> cc = {}, bool dump = false, string contains = "",
	string error_expected = "")
{
	grammar<T> g(nts, ps, start, cc);
	parser<T> e(g, options<T>);

	//cout << ".";
	//cerr << "parsing: `" << to_std_string(input) << "` [" << input.size() << "]" << endl;

	auto f = e.parse(input.c_str(), input.size());
	bool found = e.found();
	//f->print_data(cout << "dumping forest:\n") << "\n\n";
	bool contained = false;
	if (found && (dump || contains.size()))
	{
		f->traverse([&dump, &contains, &contained](const auto &n)
					{
			if (contains == n.first.to_std_string()) contained=true;
			if (dump) cout << "entering: `" <<
				n.first.to_std_string() << "`\n"; },
					[&dump](const auto &n, const auto &)
					{
						if (dump)
							cout << "exiting: `" << n.first.to_std_string() << "`\n";
					});
	}
	cout << "#found " << found << "\n";
	cout << "#count " << f->count_trees() << std::endl;
	// if (contains.size())
	//	cout << "contains: `" << contains << "`: " << contained << endl;

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

	return test_out<T>(c++, g, input, *f), true;
}
void help(const string& opt) {
	cout << "Invalid option: " << opt << "\n" <<
		"Valid options: \n \
			-[enable|disable]_incrgen 		enables incremental generation of forest \n \
			-[enable|disable]_binarization 		enables binarization and leftright optimization of forest \n \
			-unique_node		retrieves graphs from forest based on nodes, not edges \n \
			-stop_after_[1|5]	stop retrieving further graphs after 1 or 5 count \n"
		<< endl;
}
int main(int argc, char **argv)
{
	bool binarize = false;
	bool incr_gen = false;

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
		else if (opt == "-unique_node")
			opt_edge = false;
		else if (opt == "-stop_after_1")
			stop_after = 1;
		else if (opt == "-stop_after_5")
			stop_after = 5;
		else help(opt), exit(1);
	}
	std::cout<< "Running with options: ";
	for_each(args.begin(), args.end(),[](string &a){std::cout<< a << " ";});
	std::cout<<std::endl;

	options<char>.binarize =        options<char32_t>.binarize = binarize;
	options<char>.incr_gen_forest =	options<char32_t>.incr_gen_forest =
								incr_gen;
	nonterminals<> nt;
	char_class_fns cc = predefined_char_classes<>(
		{ "eof", "alpha", "alnum", "digit" }, nt);

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
		cr('\r'), lf('\n'), eof(nt("eof")), eol(nt("eol"));

	bool failed = false;
	auto fail = [&failed]() { cerr << "\nFAIL\n"; failed = true; };

	// Using Elizbeth Scott paper example 2, pg 64
	ps(start, b | start + start);
	if (!run_test<char>(ps, nt, start, "bbb")) fail();
	//negative tests
	if (!run_test<char>(ps, nt, start, "bbba",{},false,"","Unexpected"))
									 fail();
	if (!run_test<char>(ps, nt, start, "a", {}, false, "", "\"a\"")) fail();
	if (!run_test<char>(ps, nt, start, "a", {}, false, "", "\"b\"")) fail();
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

	// test eof / eol
	ps(start, (a + eol));
	ps(eol,   cr | lf | eof);
	if (!run_test<char>(ps, nt, start, "a\n", cc)) fail();
	if (!run_test<char>(ps, nt, start, "a\r", cc)) fail();
	if (!run_test<char>(ps, nt, start, "a",   cc)) fail();
	ps.clear();

	return failed ? 1 : 0;
}
