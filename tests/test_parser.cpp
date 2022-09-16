#include <sstream>
#include <fstream>
#include "../src/parser.h"

using namespace std;
using namespace idni;

template <typename CharT>
int test_out(int c, parser<CharT> &e){
	stringstream ptd;
	stringstream ssf;

	ssf<<"graph"<<c<<".dot";
	ofstream file(ssf.str());
	e.to_dot(ptd);
	file << ptd.str();
	file.close();
	ssf.str({});
	ptd.str({});
	
	ssf<<"parse_graph"<<c<<".tml";
	ofstream file1(ssf.str());
	e.to_tml_facts(ptd);
	file1 << ptd.str();
	file1.close();
	ssf.str({});
	ptd.str({});
	
	ssf<<"parse_rules"<<c<<".tml";
	ofstream file2(ssf.str());
	e.to_tml_rules(ptd);
	file2 << ptd.str();
	file2.close();

	return 1;
}

int main(int argc, char**argv ) {
	bool binlr    = false;
	bool incr_gen = true;

	std::vector<std::string> args(argv+1 , argv + argc);	

	for( auto opt : args) {
		if ( opt == "-enable_binlr") binlr = true;
		else if ( opt == "-enable_incrgen") incr_gen = true;
		else if ( opt == "-disable_binlr") binlr = false;
		else if ( opt == "-disable_incrgen") incr_gen = false;
		else {
			cout << "Invalid option: " << opt << endl << "Valid options:" << 
			"[enable|disable]_incrgen , [enable|disable]_binlr" <<endl, exit(1);
		}
	}

	size_t c = 0;

	parser<char>::parser_options o;
	o.bin_lr = binlr;
	o.incr_gen_forest = incr_gen;

	// Using Elizbeth Scott paper example 2, pg 64
	parser<char> e({
			{"start", { { "b" }, { "start", "start" } } }
			//{"start", { { "" }, { "a", "start", "b", "start" } } },
//			{"start", { { "" }, { "A", "start", "B", "start" } } },
//			{"A", { { "" }, { "A", "a" } } },
//			{"B", { { "b" }, { "B", "b" } } }
		}, o);
	cout << e.recognize("bbb") << endl << endl;
	test_out<char>(c++, e);	

	// infinite ambiguous grammar, advanced parsing pdf, pg 86
	// will capture cycles
	parser<char> e1({{"start", { { "b" }, {"start"} }}}, o);
	cout << e1.recognize("b") << endl << endl;
	test_out<char>(c++, e1);	

	// another ambigous grammar
	parser<char> e2({ {"start", { { "a", "X", "X", "c" }, {"start"} }},
				{"X", { {"X", "b"}, { "" } } },
	}, o);
	cout << e2.recognize("abbc") << endl << endl;
	test_out<char>(c++, e2);	

	// highly ambigous grammar, advanced parsing pdf, pg 89
	parser<char> e3({ {"start", { { "start", "start" }, {"a"} }} }, o);
	cout << e3.recognize("aaaaa") << endl << endl;
	test_out<char>(c++, e3);

	//using Elizabeth sott paper, example 3, pg 64.
	parser<char> e4({{"start", { { "A", "T" }, {"a","T"} }},
				{"A", { { "a" }, {"B","A"} }},
				{"B", { { ""} }},
				{"T", { { "b","b","b" } }},
	}, o);
	cout << e4.recognize("abbb") << endl << endl;
	test_out<char>(c++, e4);

	parser<char> e5({{"start", { { "b", }, {"start", "start", "start", "start"}, {""} }}}, 
		o);
	cout << e5.recognize("b") << endl << endl;
	test_out<char>(c++, e5);

	parser<char> e6({{"start", { {"n"}, { "start", "X", "start" }}},
				{"X", { {"p"}, {"m"}}}
	}, o);
	cout << e6.recognize("npnmn") << endl;
	test_out<char>(c++, e6);
/*	cout << e.recognize("aa") << endl << endl;
	cout << e.recognize("aab") << endl << endl;
	cout << e.recognize("abb") << endl << endl;
	cout << e.recognize("aabb") << endl << endl;
	cout << e.recognize("aabbc") << endl << endl;
*/
	parser<char32_t>::parser_options o32;
	o32.bin_lr = binlr;
	o32.incr_gen_forest = incr_gen;
	parser<char32_t> e7({ { U"start", {
		{ U"τ" },
		{ U"ξεσκεπάζω"},
		{ U"žluťoučký" },
		{ U"ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗ" },
		{ U"start", U"start" }
	} } }, o32);
	cout << e7.recognize(U"τžluťoučkýτᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗτξεσκεπάζωτ") << endl << endl;
	test_out<char32_t>(c++, e7);

	return 0;
}
