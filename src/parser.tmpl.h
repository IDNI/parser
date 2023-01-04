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
#ifndef __IDNI__PARSER__PARSER_TMPL_H__
#define __IDNI__PARSER__PARSER_TMPL_H__
#include "parser.h"
namespace idni {

template <typename CharT>
typename parser<CharT>::ostream& flatten(typename parser<CharT>::ostream& os,
	const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& root)
{
	auto cb_enter = [&os](const auto& n) {
		if (!n.first.nt() && (n.first.c()!=(CharT)0)) os << n.first.c();
	};
	f.traverse(root, cb_enter);
	return os;
}
template <typename CharT>
typename parser<CharT>::string flatten(const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& root)
{
	typename parser<CharT>::stringstream ss;
	flatten<CharT>(ss, f, root);
	return ss.str();
}
template <typename CharT>
int_t flatten_to_int(
	const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& root)
{
	return stoi(to_std_string(flatten<CharT>(f, root)));
}
#ifdef DEBUG
template <typename CharT>
idni::ostream_t& operator<<(idni::ostream_t& os,
	const typename idni::parser<CharT>::lit& l)
{
	return os << l.to_string();
}
template <typename CharT>
idni::ostream_t& operator<<(idni::ostream_t& os,
	const std::vector<typename idni::parser<CharT>::lit>& v)
{
	int i = 0;
	for (auto &l : v) os << l, i++ == 0 ? os << "->": os <<' ';
	return os;
}
#endif
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template <typename CharT>
ostream_t& parser<CharT>::print(ostream_t& os, const item& i) const {
	os << "I(" << i.prod << "." << i.con << ", " << i.dot << ", "
		 << i.from << ", " << i.set << "):\t ";
	//os << "g[i.prod].size():" << g[i.prod].size() << " ";
	//size_t j = 0;
	const auto& x = g[i.prod][i.con];
	//os << i.from << " " << i.set << " " << "x.size():" << x.size() << " ";
	os << g(i.prod).to_std_string() << "\t => ";
	if (x.neg) os << "~( ";
	for (size_t n = 0; n != x.size(); ++n) {
		if (n == i.dot) os << "• ";
		os << x[n].to_std_string(from_cstr<CharT>("ε ")) << " ";
	}
	if (x.size() == i.dot) os << "•";
	if (x.neg) os << " )";
	return os << ".";
}
template <typename CharT>
ostream_t& parser<CharT>::print_S(ostream_t& os) const {
	for (size_t n = 0; n != S.size(); ++n) {
		os << "S["<<n<<"]:\n";
		for(const item &x : S[n])
			print(os << "\t", x) << "\n";
	}
	return os;
}
template<typename CharT>
ostream_t& parser<CharT>::print_data(ostream_t& os) const {
	os << "S:\n";
	for (auto& x : S) {
		for (auto& y : x) {
			if (y.from != y.set)
				print(os << "\t " << y.prod << "/" << y.con << " \t", y) <<"\n";
		}
		os << "---:\n";
	}
	os << "completed S:\n";
	for (auto& x : S) {
		for (auto& y : x) {
			if (y.from != y.set && y.dot == g.len(y.prod, y.con))
				print(os << "\t " << y.prod << " \t", y) <<"\n";
		}
		os << "---:\n";
	}
	return os;
}
template<typename CharT>
ostream_t& print_grammar(ostream_t& os,	const grammar<CharT>& g) {
	for (size_t i = 0; i != g.size(); ++i) {
		const auto& x = g[i];
		for (const auto& y : x.second) {
			os << '\t' << to_std_string(x.first) << " =>";
			for (const auto& z : y.first)
				os << " \"" << to_std_string(z) << "\"";
			for (const auto& z : y.second)
				os << ", " << to_std_string(z);
			os << ".\n";
		}
	}
	return os;
}
template<typename CharT>
ostream_t& print_dictmap(ostream_t& os,
	const std::map<typename parser<CharT>::string, size_t>& dm)
{
	for (const auto& x : dm)
		os << '\t' << to_std_string(x.first) << ' ' << x.second << "\n";
	return os;
}
#endif // DEBUG

} // idni namespace
#endif // __IDNI__PARSER__PARSER_TMPL_H__
