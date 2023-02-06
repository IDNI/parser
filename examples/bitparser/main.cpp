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

auto bconv = [](parser<char, bool>::input& in) {
	vector<bool> r;
	for (int i = 7; i >= 0; --i) r.push_back(in.cur() & (1 << i));
	return in.next(), r;
};
auto b32conv = [](parser<char32_t, bool>::input& in) {
	std::vector<bool> r;
	for (int i = 31; i >= 0; --i) r.push_back(in.cur() & (1 << i));
	return in.next(), r;
};
bool failed = false;
template <typename C, typename T>
int test(const basic_string<C>& inputstr, T tval, T fval,
	typename parser<C, T>::decoder_type conv = 0)
{
	nonterminals<C, T> nt;
	vector<string> pcc_list;
	prods<C, T> nul{lit<C, T>{}}, q, start(nt(from_cstr<C>("start"))),
		rest(nt(from_cstr<C>("rest"))), value(nt(from_cstr<C>("value"))),
		TRUE{lit<C, T>{tval}}, FALSE{lit<C, T>{fval}};
	q(start, value + rest);
	q(rest,  (value + rest) | nul);
	q(value, TRUE | FALSE);
	grammar<C, T>g(nt, q, start, {});
	typename parser<C, T>::options o;
	if (conv) o.char_to_terminals = conv;
	parser<C, T> p(g, o);
	auto f = p.parse(inputstr.c_str(), inputstr.size());
	auto found = p.found();
	if (!found) return failed = true,
		cerr << "ERROR:" << p.get_error().to_str() << endl, 1;
	auto next_g = [](parser<C, T>::pforest::graph &fg) {
		auto tree = fg.extract_trees();
		tree->to_print(cout << "\n\n------\n"), cout << endl;
		return true;
	};
	f->extract_graphs(f->root(), next_g);
	return 0;
}
int main() {
	test<char,     bool>    ( "23", true, false, bconv);
	test<char32_t, bool>    (U"4",  true, false, b32conv);
	return failed ? 1 : 0;
}
