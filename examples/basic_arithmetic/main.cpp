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
typedef prods<char_t> prods_t;
typedef lit<char_t> lit_t;
typedef nonterminals<char_t> nonterminals_t;
typedef grammar<char_t> grammar_t;
typedef parser<char_t> parser_t;
typedef char_class_fns<char_t> char_class_fns_t;
typedef parser<char_t>::pforest forest_t;

struct basic_arithmetic {
	basic_arithmetic() :
		cc(predefined_char_classes<char_t>({ "eof",
				"digit", "space", "printable" }, nts)),
		addsub(nt("addsub")), digit(nt("digit")), eof(nt("eof")),
		eol(nt("eol")), expr_op(nt("expr_op")), expr(nt("expr")),
		factor(nt("factor")), integer(nt("integer")),
		integer_rest(nt("integer_rest")), muldiv(nt("muldiv")),
		printable(nt("printable")),
		printable_chars(nt("printable_chars")),
		printable_chars1(nt("printable_chars1")),
		sign(nt("sign")), space(nt("space")), start(nt("start")),
		statement(nt("statement")), statements(nt("statements")),
		statements1(nt("statements1")), term(nt("term")),
		term_op(nt("term_op")), ws_comment(nt("ws_comment")),
		ws_required(nt("ws_required")), ws(nt("ws")),
		g(nts, ba_prods(), start, cc), p(g) { }
	bool eval(const string& s) {
		auto f = p.parse(s);
		if (!f) { cerr << "input text does not seem to be an "
				"expression statement\n"; return false; }
		for (auto r : evaluate_forest(*f)) cout << r << "\n";
		return true;
	}
private:
	nonterminals_t nts{};
	char_class_fns_t cc;
	prods_t null_{'\0'},
		addsub, digit, eof, eol, expr_op, expr, factor, integer,
		integer_rest, muldiv, printable, printable_chars,
		printable_chars1, sign, space, start, statement, statements,
		statements1, term, term_op, ws_comment, ws_required, ws;
	grammar_t g;
	parser_t p;
	lit_t nt(const string& s) { return lit_t{ nts.get(s), &nts }; };
	prods_t ba_prods() {
		prods_t q, hash{'#'}, cr{'\r'}, nl{'\n'}, plus{'+'},
			star{'*'}, lparen{'('};
		q(ws_comment,       (hash + eol) |
					(hash + printable_chars + eol));
		q(ws_required,      (space + ws) | (ws_comment + ws));
		q(ws,               ws_required | null_);
		q(eol,              cr | nl | eof);
		q(printable_chars,  printable + printable_chars1);
		q(printable_chars1, (printable + printable_chars1) | null_);
		q(sign,             plus | '-');
		q(integer,          digit + integer_rest);
		q(integer_rest,     (digit + integer_rest) | null_);
		q(expr,             expr_op | term);
		q(term,             term_op | factor);
		q(expr_op,          expr + ws + addsub + ws + term);
		q(term_op,          term + ws + muldiv + ws + factor);
		q(addsub,           plus | '-');
		q(muldiv,           star | '/');
		q(factor,           (lparen + ws + expr + ws + ')') |
					integer | (sign + ws + factor));
		q(statement,        ws + expr);
		q(statements,       statement + statements1);
		q(statements1,      (eol + statement + statements1) | null_);
		q(start,            (statements + ws) | null_);
		return q;
	}
	vector<int_t> evaluate_forest(forest_t& f) {
		enum op { NONE, ADD, SUB, MUL, DIV };
		struct {
			bool is_digit = false, is_sign = false, is_op = false;
			vector<int_t> r;  // result for each statement
			vector<int_t> e;  // intermediate evaluations (nested)
			vector<op> et;    // operation types (nested)
			vector<bool> neg; // negated factors (nested)
			vector<int8_t> d; // integer digits
		} x;
		auto cb_enter = [&x, this](const auto& n) {
			//DBG(cout << "entering: `" << n.first.to_std_string() << "`\n";)
			if (!n.first.nt()) { // terminals
				char_t c = n.first.c();
				if      (x.is_digit) x.d.push_back(c - 48);
				else if (x.is_sign && c == '-')
						x.neg.back() = !x.neg.back();
				else if (x.is_op) x.et.emplace_back(
					  c == '+' ? ADD
					: c == '-' ? SUB
					: c == '*' ? MUL
					: c == '/' ? DIV
					: NONE);
				return;
			} // nonterminals
			const auto& l = n.first;
			if      (l == factor)  x.neg.push_back(false);
			else if (l == integer) x.e.emplace_back(0);
			else if (l == addsub ||
				 l == muldiv)  x.is_op    = true;
			else if (l == digit)   x.is_digit = true;
			else if (l == sign)    x.is_sign  = true;
		};
		auto cb_exit = [&x, this](const auto& n, const auto&) {
			//DBG(cout << "exiting: `" << n.first.to_std_string() << "`\n";)
			if (!n.first.nt()) return;
			const auto& l = n.first;
			if      (l == addsub ||
				 l == muldiv)    x.is_op    = false;
			else if (l == digit)     x.is_digit = false;
			else if (l == sign)      x.is_sign  = false;
			else if (l == statement) x.r.push_back(x.e[0]), x.e ={};
			else if (l == integer) {
				size_t p = 1;
				while (x.d.size()) x.e.back() += x.d.back() * p,
							p *= 10, x.d.pop_back();
			}
			else if (l == factor) {
				if (x.neg.back()) x.e.back() = -x.e.back();
				x.neg.pop_back();
			}
			else if (l == expr_op || l == term_op) {
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
		f.traverse(cb_enter, cb_exit);
		return x.r;
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
