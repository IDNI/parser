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
#include <cassert>
#include <string.h>
#include "parser.h"
using namespace std;
using namespace idni;

struct csv_parser {
	const char* csv_tgf =
	"	@use_char_class space, digit. "
	" 	el			=> '\n'. "
	"	ws           => space*. "
	"	integer      => digit+. "
	"	line     => ws integer  (ws ',' ws integer ws)*  . "
	"	start        => ws ( line el )*  ws . "
	;
	csv_parser() :
		g(tgf<char>::from_string(nts, csv_tgf)), p(g, { true, false  } ) {}
	bool eval(const string& s) {
		auto f = p.parse(s.c_str(), s.size());
		if (!f || !p.found()) {
			return cerr << p.get_error().to_str(), false; }
		auto cb_next_g = [&f] (decltype(p)::pgraph& g){
			g.extract_trees()->to_print(cout);
			f->remove_binarization(g);
			f->remove_recursive_nodes(g);
			cout<< "\nafter removal of _temp/_R..\n";
			g.extract_trees()->to_print(cout);
			return true;
		};
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
