// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <cassert>
#include <string.h>
#include "parser.h"
using namespace std;
using namespace idni;

struct csv_parser {
	const char* csv_tgf =
	"	@use char classes space, digit.\n"
	" 	el      => '\n'.\n"
	"	_       => space*.\n"
	"	integer => digit+.\n"
	"	line    => _ integer ( _ ',' _ integer _ )*."
	"	start   => _ ( line el )* _.\n"
	;
	csv_parser() :
		g(tgf<char>::from_string(nts, csv_tgf)), p(g, { true, true }) {}
	bool eval(const string& s) {
		auto res = p.parse(s.c_str(), s.size());
		if (!res.found) return cerr << res.parse_error, false;
		auto cb_next_g = [&res] (decltype(p)::pgraph& g){
			g.extract_trees()->to_print(cout);
			res.inline_grammar_transformations(g);
			cout<< "\nafter removal of _temp/_R..\n";
			g.extract_trees()->to_print(cout);
			return true;
		};
		auto f = res.get_forest();
		f->extract_graphs(f->root(), cb_next_g);
		return true;
	}
private:
	nonterminals<char> nts{};
	grammar<char> g;
	parser<char> p;
};

int main() {
	csv_parser csv;
	string line, lines;
	cin.clear();
	while (true) {
		cin >> line;
		if (line == "q") break;
		lines.append(line);
		lines.append("\n");
	}
	cout << lines;
	csv.eval(lines);
}
