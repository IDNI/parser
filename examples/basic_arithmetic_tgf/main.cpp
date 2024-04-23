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
	const char* ba_tgf =
	"	@use char classes space, digit.\n"
	"	start        => _ expr _.\n"
	"	_            => space*.\n"
	"	expr	     => pos | neg.\n"
	"       expr         => add | sub.\n"
	"	expr         => mul | div.\n"
	"	expr         => integer.\n"
	"       expr         => '(' _ expr _ ')'.\n"
	"	mul          => expr _ '*' _ expr.\n"
	"	div          => expr _ '/' _ expr.\n"
	"	add          => expr _ '+' _ expr.\n"
	"	sub          => expr _ '-' _ expr.\n"
	"       pos          => '+' _ expr.\n"
	"	neg          => '-' _ expr.\n"
	"	integer      => digit+.\n"
	;
	basic_arithmetic() :
		g(tgf<char>::from_string(nts, ba_tgf)), p(g) { }
	bool eval(const string& s) {
		auto r = p.parse(s.c_str(), s.size());
		if (!r.found) {
			return cerr << "\ninput text does not seem to be an "
				"expression statement", false; }
		cout << evaluate(r) << endl;
		return true;
	}
private:
	nonterminals<char> nts{};
	grammar<char> g;
	parser<char> p;
	size_t id(const string& s) { return nts.get(s); }
	int_t evaluate(parser<char>::result& r) {
		vector<int_t> x;  // intermediate evaluations (nested)
		auto cb_enter = [&x, &r, this](const auto& n) {
			//DBG(cout << "entering: `" << n.first.to_std_string() << "` ["<<n.second[0]<<","<<n.second[1]<<"]\n";)
			if (n.first.nt() && n.first.n() == id("integer")) {
				auto i = r.get_terminals_to_int(n);
				if (!i) cerr << "integer out of range\n";
				else x.push_back(i.value());
			}
		};
		auto cb_exit = [&x, this](const auto& n, const auto&) {
			//DBG(cout << "exiting: `" << n.first.to_std_string() << "`\n";)
			if (!n.first.nt()) return;
			const auto& l = n.first.n();
			if      (l == id("neg")) x.back() = -x.back();
			else if (l == id("add")) (x[x.size()-2] += x.back()),
						x.pop_back();
			else if (l == id("sub")) (x[x.size()-2] -= x.back()),
						x.pop_back();
			else if (l == id("mul")) (x[x.size()-2] *= x.back()),
						x.pop_back();
			else if (l == id("div")) (x[x.size()-2] /= x.back()),
						x.pop_back();
		};
		r.get_forest()->traverse(cb_enter, cb_exit);
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
}
