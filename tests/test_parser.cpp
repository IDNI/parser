#include <sstream>
#include <fstream>
#include "../src/parser.h"
using namespace std;
using namespace idni;
template <typename CharT>
typename parser<CharT>::options options;
static bool opt_edge = true;
static size_t c = 0;
static size_t stop_after = SIZE_MAX;

template <typename CharT>
int test_out(int c, const typename grammar<CharT>::grammar &g,
	const basic_string<CharT> &inputstr,
	const typename parser<CharT>::pforest &f)
{
	stringstream ptd;
	stringstream ssf;

	g.print_internal_grammar(ssf, "\\l");
	string s = ssf.str();
	ssf.str({});

	ssf << "graph" << c << ".dot";
	ofstream file(ssf.str());
	to_dot<CharT>(ptd, f, to_std_string(inputstr), s);
	file << ptd.str();
	file.close();
	ssf.str({});
	ptd.str({});

	ssf << "parse_graph" << c << ".tml";
	ofstream file1(ssf.str());
	to_tml_facts<CharT>(ptd, f);
	file1 << ptd.str();
	file1.close();
	ssf.str({});
	ptd.str({});

	ssf << "parse_rules" << c << ".tml";
	ofstream file2(ssf.str());
	to_tml_rules<CharT>(ptd, f);
	file2 << ptd.str();
	file2.close();

	size_t i = 0;
	auto cb_next_graph = [&](typename parser<CharT>::pgraph &g)
	{
		f.detect_cycle(g);
		ssf.str({});
		ptd.str({});
		ssf << "graph" << c << "_" << i << ".dot";
		ofstream filet(ssf.str());
		to_dot<CharT, typename parser<CharT>::pgraph>(ptd, g, to_std_string(inputstr), s);
		filet << ptd.str();
		filet.close();
		ssf.str({});
		ptd.str({});
		ssf << "parse_rules" << c << "_" << i << ".tml";
		filet.open(ssf.str());
		to_tml_rules<CharT>(ptd, g);
		filet << ptd.str();
		filet.close();

		typename parser<CharT>::psptree tr= g.extract_trees();
		ssf.str({});
		ptd.str({});
		ssf << "tree" << c << "_" << i << ".dot";
		filet.open(ssf.str());
		to_dot<CharT>(ptd, tr, to_std_string(inputstr), s);
		filet << ptd.str();
		filet.close();
		i++;

		return i < stop_after ? true : false;
	};

	f.extract_graphs(f.root(), cb_next_graph, opt_edge);
	return 1;
}
template <typename CharT>
bool run_test(const prods<CharT> &ps, nonterminals<CharT> &nts,
	const prods<CharT> &start, const basic_string<CharT> &input,
	char_class_fns<CharT> cc = {}, bool dump = false, string contains = "")
{
	grammar<CharT> g(nts, ps, start, cc);
	parser<CharT> e(g, options<CharT>);

	//cout << "parsing: `" << to_std_string(input) << "` [" << input.size() << "]" << endl;

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
	cout << found << "\n\n";
	// if (contains.size())
	//	cout << "contains: `" << contains << "`: " << contained << endl;
	if (!found || (contains.size() && !contained))
		return false;

	return test_out<CharT>(c++, g, input, *f), true;
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
		else
		{
			cout << "Invalid option: " << opt << endl
				 << "Valid options: \n \
			-[enable|disable]_incrgen 		enables incremental generation of forest \n \
			-[enable|disable]_binarization 		enables binarization and leftright optimization of forest \n \
			-unique_node		retrieves graphs from forest based on nodes, not edges \n \
			-stop_after_[1|5]	stop retrieving further graphs after 1 or 5 count \n"
				 << endl,
				exit(1);
		}
	}
	options<char>.binarize = options<char32_t>.binarize = binarize;
	options<char>.incr_gen_forest =
		options<char32_t>.incr_gen_forest = incr_gen;

	nonterminals<char> nt;
	char_class_fns cc = predefined_char_classes<char>(
		{ "alpha", "alnum", "digit" }, nt);

	prods<char> ps, start(nt("start")), nll('\0'),
		alnum(nt("alnum")), alpha(nt("alpha")), digit(nt("digit")),
		chars(nt("chars")), expression(nt("expression")),
		sum(nt("sum")),	mul(nt("mul")),
		m1(nt("m1")), s1(nt("s1")), m2(nt("m2")), s2(nt("s2")),
		identifier(nt("identifier")), keyword(nt("keyword")),
		a('a'), b('b'), c('c'), n('n'), p('p'), m('m'),
		A(nt("A")), B(nt("B")), T(nt("T")), X(nt("X")),
		one('1'), PO(nt("PO")), IO(nt("IO")),
		plus('+'), minus('-'), mult('*');

	bool failed = false;
	auto fail = [&failed]() { cerr << "\nFAIL\n"; failed = true; };

	// Using Elizbeth Scott paper example 2, pg 64
	ps(start, b | start + start);
	if (!run_test<char>(ps, nt, start, "bbb"))
		fail();
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
	if (!run_test<char>(ps, nt, start, "a") ||
		run_test<char>(ps, nt, start, "b")) fail();
	ps.clear();

	// keyword vs identifier
	ps(start,      identifier | keyword);
	ps(identifier, chars & ~ keyword);
	ps(chars,      alpha | (chars + alnum));
	ps(keyword,    {"print"});
	if (!run_test<char>(ps, nt, start, "var123", cc, false, "identifier") ||
		!run_test<char>(ps, nt, start, "print", cc, false, "keyword"))
			fail();
	ps.clear();

	// operator priority
	ps(start,      expression);
	ps(expression, sum | mul & ~sum | digit);
	ps(sum,        expression + '+' + expression);
	ps(mul,        expression + '*' + expression);
	if (!run_test<char>(ps, nt, start, "1+2*3", cc)
	 || !run_test<char>(ps, nt, start, "3*2+1", cc)
	) fail();
	ps.clear();

	// all cycles 
	ps(start, A & B);
	ps(A, X);
	ps(X, start | b);
	ps(B, T | b);
	ps(T, start);
	if (!run_test<char>(ps, nt, start, "b"))
		fail();
	ps.clear();

	return failed ? 1 : 0;
}
