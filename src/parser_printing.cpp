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
#include <sstream>
#include <iostream>
#include "parser.h"
using namespace std;
namespace idni {

#ifdef DEBUG
template <typename CharT>
typename parser<CharT>::ostream& parser<CharT>::print(
	parser<CharT>::ostream& os, const item& i) const
{
	os << i.set << " " << i.from << " ";
	//put(put(os, i.set) << " ", i.from) << " ";
	for (size_t n = 0; n != G[i.prod].size(); ++n) {
		if (n == i.dot) os << "* ";
		if (G[i.prod][n].nt()) os << d.get(G[i.prod][n].n()) << " ";
		else if (G[i.prod][n].c() == (CharT) '\0') os << "ε " << " ";
		else os << G[i.prod][n].c() << " ";
	}
	if (G[i.prod].size() == i.dot) os << "*";
	return os;
}
#endif

#ifdef WITH_TO_METHODS
template <typename CharT>
std::string parser<CharT>::grammar_text() const {
	std::stringstream txt;
	for (const auto &p : G) {
		txt << "\n\\l";
		size_t i = 0;
		for (const auto& l : p){
			if (l.nt()) txt << to_std_string(d.get(l.n()));
			else if (l.c() != '\0') txt << to_std_string(l.c());
			else txt << "ε";
			txt <<  " ";
			if (i++ == 0) txt << "-> ";
		}
	}
	return txt.str();
}
template <typename CharT>
std::string parser<CharT>::dot_safe(const std::string &s) const {
	std::stringstream ss;
	for (size_t n = 0; n != s.size(); ++n) {
		if (s[n] == '"') ss << '\\';
		ss << s[n];
	}
	return ss.str();
}
template <typename CharT>
template <typename P>
bool parser<CharT>::to_dot(ostream_t& ss, P &&pt) const {
	auto keyfun = [this] (const pnode& k){
		std::stringstream l;
		k.first.nt() ? l << to_std_string(d.get(k.first.n()))
			: k.first.c() == '\0' ? l << "ε"
				: l << to_std_string(k.first.c());
		l << "_" << k.second[0] << "_" << k.second[1] << "_";
		std::string desc = l.str();
		return std::pair<size_t, std::string>(
			std::hash<std::string>()(desc), desc);
	};
	ss << "digraph {\n";
	ss << "_input_" << "[label =\"" << dot_safe(to_std_string(inputstr)) <<
		"\", shape = rectangle]" ;
	ss << "_grammar_" << "[label =\"" << dot_safe(grammar_text()) <<
		"\", shape = rectangle]" ;
	ss << "\nnode" << "[ ordering =\"out\"];";
	ss << "\ngraph" << "[ overlap =false, splines = true];";

	std::unordered_set<std::pair<size_t,size_t>, hasher_t> edgedone;
	edgedone.clear();
	for (auto &it: pt.size() ? pt : pfgraph) {
		auto key = keyfun(it.first);
		ss << "\n" << key.first << "[label=\"" << key.second <<"\"];";
		size_t p = 0;
		std::stringstream pstr;
		for (auto &pack : it.second) {
			pstr<<key.second<<p++;
			auto ambkey = std::hash<std::string>()(pstr.str());
			ss << "\n" << ambkey << "[shape = point,label=\"" <<
				pstr.str() << "\"];";
			if (edgedone.insert({ key.first, ambkey }).second)
				ss << "\n" << key.first << "->" << ambkey <<';';
			for (auto & nn: pack) {
				auto nkey = keyfun(nn);
				ss << "\n" << nkey.first << "[label=\"" <<
					nkey.second << "\"];";
				if (edgedone.insert({ ambkey, nkey.first })
					.second) ss << "\n" << ambkey << "->" <<
						nkey.first<< ';';
			}
			pstr.str({});
		}
	}
	ss << "\n}\n";
	return true;
}
template <typename CharT>
template <typename T>
bool parser<CharT>::to_tml_facts(ostream_t& ss, T &&pt) const {
	auto str_rel_output = [&ss, this] (string rel, size_t id,
		std::vector<std::variant<size_t,typename parser<CharT>::string>>
			args)
	{
		ss << to_std_string(rel) << "(" << id;
		for (auto &a : args) {
			if (std::holds_alternative<size_t>(a))
				ss << " " << std::get<size_t>(a);
			else    ss << " " << to_std_string(std::get<string>(a));
#ifdef DEBUG
			if (std::holds_alternative<size_t>(a))
				std::cout << " " << std::get<size_t>(a);
			else std::cout<<" "<<to_std_string(std::get<string>(a));
#endif
		}
		ss << ").\n";
	};
	iterate_forest(str_rel_output, pt);
	return true;
}
template <typename CharT>
std::string parser<CharT>::to_tml_rule(const pnode nd) const {
	std::stringstream ss;
	if (nd.first.nt()) ss << to_std_string(d.get(nd.first.n()));
	else if (nd.first.c() == (CharT) '\0') ss << "ε";
	else ss << to_std_string(nd.first.c());
	ss << "(" << nd.second[0] << " " << nd.second[1] << ")";
	return ss.str();
}
template <typename CharT>
template <typename P>
bool parser<CharT>::to_tml_rule(ostream_t& ss, P &&pt) const {
	std::set<std::string> terminals;
	for (auto &it: pt.size()? pt : pfgraph ) {
		for (auto &pack : it.second) { 
			ss << to_tml_rule(it.first) << ":-";
			for (size_t i = 0; i < pack.size(); i++) {
				// if terminal
				if (pfgraph.find(pack[i]) == pfgraph.end())
					terminals.insert(to_tml_rule(pack[i]));
				ss << to_tml_rule(pack[i])
					<< (i == pack.size()-1 ? "." : ",");
			};
			ss << "\n";
		}
	}
	for (auto &t: terminals) ss << t << ".\n";
	return true;
}
template bool parser<char>::to_tml_facts(ostream_t& os, ptree_t&& p) const;
template bool parser<char32_t>::to_tml_facts(ostream_t& os, ptree_t&& p) const;
template bool parser<char>::to_tml_rule(ostream_t& os, ptree_t&& p) const;
template bool parser<char32_t>::to_tml_rule(ostream_t& os, ptree_t&& p) const;
template bool parser<char>::to_dot(ostream_t& os, ptree_t&& p) const;
template bool parser<char32_t>::to_dot(ostream_t& os, ptree_t&& p) const;
#endif // WITH_TO_METHODS

} // idni namespace
