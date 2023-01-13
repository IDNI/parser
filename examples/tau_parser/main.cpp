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

typedef char char_t;

struct tau_parser {
	const char* tau_tgf =
	"	@use_char_class eof, alpha, alnum, space, printable. "
	// whitespace and comments
	"\n	eol            => '\n' | '\r' | eof. "
	"\n	ws             => ws_required | null. "
	"\n	ws_required    => space ws | ws_comment ws. "
	"\n	ws_comment     => '#' eol | '#' printable+ eol. "

	"\n	tau            => tau ws \"&&\" ws tau "
	"\n			| tau ws \"||\" ws tau "
	"\n			| '!' ws tau "
	"\n			| '(' ws tau ws ')' "
	"\n			| \"ex\" ws_required var ws_required tau "
	"\n			| \"all\" ws_required var ws_required tau "
	"\n			| bf ws '=' ws bf "
	"\n			| bf ws \"!=\" ws bf. "
	"\n	bf             => bf ws '&' ws bf "
	"\n			| bf ws '|' ws bf "
	"\n			| '~' ws bf "
	"\n			| bf ws '+' ws bf "
	"\n			| '(' ws bf ws ')'  "
	"\n			| var "
	"\n			| const "
	"\n			| macro. "
	"\n	const          => {elem} | {tau} | '0' | '1'. "
	"\n	var            => sym. "
	"\n	sym            => chars. "
	"\n	chars          => alpha | chars alnum. "
	"\n	prefix         => \"@prefix\" { ws_required "
	"\n				[ (\"all\" | \"ex\") ws_required ] "
	"\n					var }. "
	"\n	arg            => var | const | macro. "
	"\n	macro_sgn      => sym ws '(' ws [ arg ws {',' ws arg ws} ] ')'."
	"\n	macro_def      => macro_sgn ws \":=\" ws bf."
	"\n	macro          => macro_sgn. "

	"\n	statement      => prefix | macro_def | tau. "
	"\n	start          => (ws statement ws '.')* ws. "
	;
	tau_parser() :
		g(tgf<char_t>::from_string(nts, tau_tgf)), p(g) {
			DBG(g.print_internal_grammar(
				cout << "parsed grammar rules: \n") << "\n";)
		}
	bool eval(const string& s) {
		auto f = p.parse(s.c_str(), s.size());
		if (!p.found()) { 
			auto error = p.get_error();
			cerr << error.to_str();
			return true; 
		}
		auto next_g = [](parser<char_t>::pforest::graph &fg) {
			auto tree = fg.extract_trees();
			tree->to_print(cout << "\n\n------\n"), cout << endl;
			return true;
		};
		f->extract_graphs(f->root(), next_g);
		return true;
	}
private:
	nonterminals<char_t> nts{};
	grammar<char_t> g;
	parser<char_t> p;
	size_t id(const string& s) { return nts.get(s); }
};

int main() {
	tau_parser taup;
	string line;
	while (getline(cin, line)) {
		if (line.size() == 0) continue;
		cout << "> " << line << "";
		if (!taup.eval(line)) return 1;
	}
}
