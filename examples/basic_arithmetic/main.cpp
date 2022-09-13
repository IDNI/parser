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
#include "glanguage.h"
using namespace std;
using namespace idni;
typedef parser<char32_t> parser_t;
vector<int_t> evaluate(parser_t& p) {
	enum op { NONE, ADD, SUB, MUL, DIV };
	struct {
		bool is_digit = false, is_sign = false, is_op = false;
		vector<int_t> r;  // results for each statement
		vector<int_t> e;  // intermediate evaluations (nested)
		vector<op> et;    // operation types (nested)
		vector<bool> neg; // negated factors (nested)
		vector<int8_t> d; // integer digits
	} x;
	auto cb_enter = [&p, &x](const auto& n) {
		if (!n.first.nt()) { // terminals
			if      (x.is_digit) x.d.push_back(n.first.c() - 48);
			else if (x.is_sign && n.first.c() == U'-')
					x.neg.back() = !x.neg.back();
			else if (x.is_op) x.et.emplace_back(
				  n.first.c() == U'+' ? ADD
				: n.first.c() == U'-' ? SUB
				: n.first.c() == U'*' ? MUL
				: n.first.c() == U'/' ? DIV
				: NONE);
			return;
		} // nonterminals
		const auto& s = p.dict(n.first.n());
		if      (s == U"factor") x.neg.push_back(false);
		else if (s == U"integer") x.e.emplace_back(0);
		else if (s == U"addsub" ||
			 s == U"muldiv")  x.is_op    = true;
		else if (s == U"digit")   x.is_digit = true;
		else if (s == U"sign")    x.is_sign  = true;
	};
	auto cb_exit = [&p, &x](const auto& n, const auto&) {
		if (!n.first.nt()) return;
		const auto& s = p.dict(n.first.n());		
		if      (s == U"addsub" ||
			 s == U"muldiv")  x.is_op    = false;
		else if (s == U"digit")   x.is_digit = false;
		else if (s == U"sign")    x.is_sign  = false;
		else if (s == U"statement") x.r.push_back(x.e[0]), x.e = {};
		else if (s == U"integer") {
			size_t p = 1;
			while (x.d.size()) x.e.back() += x.d.back() * p,
						p *= 10, x.d.pop_back();
		}
		else if (s == U"factor") {
			if (x.neg.back()) x.e.back() = -x.e.back();
			x.neg.pop_back();
		}
		else if (s == U"expr_op" || s == U"term_op") {
			auto v = x.e.back(); x.e.pop_back();
			switch (x.et.back()) {
				case ADD: x.e.back() += v; break;
				case SUB: x.e.back() -= v; break;
				case MUL: x.e.back() *= v; break;
				case DIV: x.e.back() /= v; break;
				case NONE: ;
			}
			x.et.pop_back();
		}
	};
	p.parsed_forest().traverse(p.root(), cb_enter, cb_exit);
	return x.r;
}
int main() {
	parser_t::parser_options o;
	o.cc_fns = { "eof", "digit", "space", "printable" };
	parser_t::grammar g;
#define DYNAMIC_GRAMMAR
#ifdef DYNAMIC_GRAMMAR
	auto g_file = "./examples/basic_arithmetic/basic_arithmetic.g";
	if (!glanguage::load_g<char32_t>(g_file, g)) {
#endif
		g = parser_t::grammar // include g grammar as a c++ structure
		#include "basic_arithmetic.grammar.inc.h"
#ifdef DYNAMIC_GRAMMAR
	}
#endif
	parser_t p(g, o);
	string line;
	while (getline(cin, line)) {
		if (line.size() == 0) continue;
		DBG(cout << "input: '" << line << "'\n";)
		if (p.recognize(to_u32string(line))) {
			for (auto x : evaluate(p)) cout << x << "\n";
		} else {
			cerr << "input text does not seem to be an "
					"expression statement\n";
			return 1;
		}
	}
}
