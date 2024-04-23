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

struct basic_arithmetic {
	basic_arithmetic() :
		cc(predefined_char_classes<char>({ "space", "digit" }, nts)),
		add(nt("add")), digit(nt("digit")), div(nt("div")),
		expr(nt("expr")), integer(nt("integer")),
		integer1(nt("integer1")), mul(nt("mul")), neg(nt("neg")),
		pos(nt("pos")), space(nt("space")), start(nt("start")),
		sub(nt("sub")), ws(nt("ws")),
		g(nts, ba_prods(), start, cc), p(g) { }
	bool eval(const string& s) {
		auto res = p.parse(s.c_str(), s.size());
		if (!res.found) {
			return cerr << "\ninput text does not seem to be an "
				"expression statement\n" << res.parse_error
				<< endl, false;
		}
		cout << evaluate(res) << "\n";
		return true;
	}
private:
	nonterminals<char> nts{};
	char_class_fns<char> cc;
	prods<char> nul{lit<char>{}},
		add, digit, div, expr, integer, integer1, mul, neg, pos, space,
		start, sub, ws;
	grammar<char> g;
	parser<char> p;
	lit<char> nt(const string& s){ return lit<>{nts.get(s),&nts}; };
	prods<char> ba_prods() {
		prods<char> q, plus{'+'}, minus{'-'}, lparen{'('};
		q(start,    (ws + expr + ws) | ws);
		q(ws,       (space + ws) | nul);
		q(expr,     pos | neg);
		q(expr,     add | sub);
		q(expr,     mul | div);
		q(expr,     integer);
		q(expr,     (lparen + ws + expr + ws + ')'));
		q(mul,      expr + ws + '*' + ws + expr);
		q(div,      expr + ws + '/' + ws + expr);
		q(add,      expr + ws + '+' + ws + expr);
		q(sub,      expr + ws + '-' + ws + expr);
		q(pos,      plus + ws + expr);
		q(neg,      minus + ws + expr);
		q(integer,  digit + integer1);
		q(integer1, (digit + integer1) | nul);
		return q;
	}
	int_t evaluate(parser<char>::result& res) {
		vector<int_t> x;  // intermediate evaluations (nested)
		auto cb_enter = [&x, &res, this](const auto& n) {
			//DBG(cout << "entering: `" << n.first.to_std_string() << "` ["<<n.second[0]<<","<<n.second[1]<<"]\n";)
			if (n.first == integer) {
				auto i = res.get_terminals_to_int(n);
				if (!i) cerr << "integer out of range\n";
				else x.push_back(i.value());
			}
		};
		auto cb_exit = [&x, this](const auto& n, const auto&) {
			//DBG(cout << "exiting: `" << n.first.to_std_string() << "`\n";)
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
		res.get_forest()->traverse(cb_enter, cb_exit);
		return x.size() ? x.back() : 0;
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
	return 0;
}
