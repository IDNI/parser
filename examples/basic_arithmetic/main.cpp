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

struct basic_arithmetic {
	basic_arithmetic() :
		cc(predefined_char_classes<char_t>({ "space", "digit" }, nts)),
		add(nt("add")), addsub(nt("addsub")), digit(nt("digit")),
		div(nt("div")), expr1(nt("expr1")), expr2(nt("expr2")),
		factor(nt("factor")), integer(nt("integer")),
		integer1(nt("integer1")), mul(nt("mul")), muldiv(nt("muldiv")),
		neg(nt("neg")), pos(nt("pos")), space(nt("space")),
		start(nt("start")), sub(nt("sub")), ws(nt("ws")),
		g(nts, ba_prods(), start, cc), p(g) { }
	bool eval(const string& s) {
		auto f = p.parse(s.c_str(), s.size());
		if (!f || !p.found()) {
			return cerr << "\ninput text does not seem to be an "
				"expression statement\n", false; }
		cout << evaluate_forest(*f) << endl;
		return true;
	}
private:
	nonterminals<char_t> nts{};
	char_class_fns<char_t> cc;
	prods<char_t> null_{'\0'},
		add, addsub, digit, div, expr1, expr2, factor, integer,
		integer1, mul, muldiv, neg, pos, space, start, sub, ws;
	grammar<char_t> g;
	parser<char_t> p;
	lit<char_t> nt(const string& s){ return lit<char_t>{nts.get(s),&nts}; };
	prods<char_t> ba_prods() {
		prods<char_t> q, plus{'+'}, minus{'-'}, lparen{'('};
		q(ws,               (space + ws) | null_);
		q(integer,          digit + integer1);
		q(integer1,         (digit + integer1) | null_);
		q(expr1,            addsub | expr2);
		q(expr2,            muldiv | factor);
		q(addsub,           add | (sub & ~add));
		q(muldiv,           mul | (div & ~mul));
		q(add,              expr1 + ws + '+' + ws + expr2);
		q(sub,              expr1 + ws + '-' + ws + expr2);
		q(mul,              expr2 + ws + '*' + ws + factor);
		q(div,              expr2 + ws + '-' + ws + factor);
		q(factor,           pos | neg | integer |
					(lparen + ws + expr1 + ws + ')'));
		q(pos,              plus  + ws + factor);
		q(neg,              minus + ws + factor);
		q(start,            (ws + expr1 + ws) | ws);
		return q;
	}
	int_t evaluate_forest(parser<char_t>::pforest& f) {
		vector<int_t> x;  // intermediate evaluations (nested)
		auto cb_enter = [&x, &f, this](const auto& n) {
			//DBG(cout << "entering: `" << n.first.to_std_string() << "`\n";)
			if (n.first == integer)
				x.push_back(flatten_to_int<char_t>(f, n));
		};
		auto cb_exit = [&x, this](const auto& n, const auto&) {
			//DBG(cout << "exiting: `" << n.first.to_std_string() << "`\n";)
			if (!n.first.nt()) return;
			const auto& l = n.first;
			if      (l == neg) x.back() = -x.back();
			else if (l == add) (x[x.size()-2] += x.back()),
						x.pop_back();
			else if (l == sub) (x[x.size()-2] -= x.back()),
						x.pop_back();
			else if (l == mul) (x[x.size()-2] *= x.back()),
						x.pop_back();
			else if (l == div) (x[x.size()-2] /= x.back()),
						x.pop_back();
		};
		f.traverse(cb_enter, cb_exit);
		return x.back();
	}
};

int main() {
	basic_arithmetic ba;
	string line;
	while (getline(cin, line)) {
		if (line.size() == 0) continue;
		cout << "> " << line << " = ";
		if (!ba.eval(line)) return 1;
	}
}
