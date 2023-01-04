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

struct tau_parser {
	tau_parser() :
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
		bool match = false; 
		auto f = p.parse(s, match);
		if (!match) { 
			cerr << "input text does not match syntax.. try again\n";
			return true; 
		}
		auto next_g = [](forest_t::graph &g){
			auto tree = g.extract_trees();
			tree->to_print(std::cout);
			return false;
		};
		f->extract_graphs( f->root(), next_g);
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
	
};

int main() {
	tau_parser taup;
	string line;
	while (getline(cin, line)) {
		if (line.size() == 0) continue;
		cout << "> " << line << " = ";
		if (!taup.eval(line)) return 1;
	}
}
