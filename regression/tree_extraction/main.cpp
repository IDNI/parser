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

struct tree_extraction {
	const char* te_tgf =
	"@use_char_class eof, space, digit, xdigit, alpha, alnum, punct, printable. "
	"chars			=> alpha (alnum)*."
	"cbf_rule 		=> cbf_head | chars '=' cbf dot."
	"cbf_head		=> cbf."
	"cbf 			=> bf | chars."
	"bf				=> '1' | '0'."
	"start			=> cbf_rule."
	;

	const string te_input = "something=0";

	tree_extraction() : g(tgf<char>::from_string(nts, te_tgf)), p(g) { }

	int_t run() {
		auto f = p.parse(te_input.c_str(), te_input.size());
		forest<std::pair<lit<char, char>, std::array<size_t, 2UL>>>::sptree t;
		auto get_tree = [&f, &t] (auto& g ){
				f->remove_recursive_nodes(g);
				t = g.extract_trees();
				return false;
		};
		f->extract_graphs(f->root(), get_tree);
		return 0;
	}

private:

	nonterminals<char> nts{};

	grammar<char> g;

	parser<char> p;

	size_t id(const string& s) { return nts.get(s); }

};

int main() {
	tree_extraction().run();
}
