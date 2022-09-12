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
typename parser<CharT>::parse_forest parser<CharT>::resolve_priorities(
	parse_forest r) const
{
	auto cb_enter   =   [](const pnode&) {};
	auto cb_revisit =   [](const auto&)  { return false; };
	auto cb_exit    = [&r](const pnode& root, const pnodes_set& choice_set){
		DBG(assert(choice_set.size() <= 1));
		if (root.first.nt()) r[root] = choice_set;
	};
	auto cb_disambig_by_priority = [this](const pnode& root,
		const pnodes_set& ambset)
	{
		DBG(assert(root.first.nt()));
		int_t cur = -1, pchoice = INT32_MIN,
			maxp = INT32_MIN, pref_choice = INT32_MIN;
		for (auto& choice : ambset) {
			std::vector<lit> gprod;
			gprod.push_back(root.first);
			//get the production
			for (auto& nd : choice) gprod.emplace_back(nd.first);
			++cur;
			// find the top priority if any
			auto it = priority.find(gprod);
			if (it != priority.end() && it->second >= maxp)
				maxp = it->second, pchoice = cur;
			// find the preference if any
			if (prefer.count(gprod)) pref_choice = cur;
		}
		if (pchoice == INT32_MIN && pref_choice == INT32_MIN)
			pchoice = 0, std::cerr << "\n Could not resolve "
				"ambiguity, defaulting to first choice!\n";
		pnodes_set choosen;
		auto it = next(ambset.begin(),
			int_t(pchoice >= 0 ? pchoice : pref_choice));
		choosen.insert(*it);
		return choosen;
	};
	r.traverse(root(),
		cb_enter, cb_exit, cb_revisit, cb_disambig_by_priority);
	return r;
}
template <typename CharT>
typename parser<CharT>::ostream& parser<CharT>::flatten(ostream& os,
	const pnode& root)
{
	auto cb_enter = [&os](const auto& n) {
		if (!n.first.nt() && (n.first.c()!=(CharT)0)) os << n.first.c();
	};
	f.traverse(root, cb_enter);
	return os;
}
template <typename CharT>
typename parser<CharT>::string parser<CharT>::flatten(const pnode& root) {
	stringstream ss;
	flatten(ss, root);
	return ss.str();
}

#ifdef DEBUG
template <typename CharT>
idni::ostream_t& operator<<(idni::ostream_t& os,
	const typename idni::parser<CharT>::lit& l)
{
	if (l.nt()) return os << l.n();
	else return os << (l.c() == '\0' ? "\0" : l.c());
}
template <typename CharT>
idni::ostream_t& operator<<(idni::ostream_t& os,
	const std::vector<typename idni::parser<CharT>::lit>& v)
{
	int i = 0;
	for (auto &l : v) os << l, i++ == 0 ? os << "->": os <<' ';
	return os;
}
template <typename CharT>
ostream_t& parser<CharT>::print(
	ostream_t& os, const item& i) const
{
	os << i.from << " " << i.set << " ";
	//put(put(os, i.set) << " ", i.from) << " ";
	for (size_t n = 0; n != G[i.prod].size(); ++n) {
		if (n == i.dot) os << "• ";
		if (G[i.prod][n].nt())
			os << to_std_string(d.get(G[i.prod][n].n())) << " ";
		else if (G[i.prod][n].c() == (CharT) '\0') os << "ε " << " ";
		else os << to_std_string(G[i.prod][n].c()) << " ";
		if (n == 0) os << "\t => ";
	}
	if (G[i.prod].size() == i.dot) os << "•";
	return os;
}
template<typename CharT>
ostream_t& parser<CharT>::print_data(ostream_t& os) const {
	os << "G:\n";
	for (size_t x = 0; x != G.size(); ++x) {
		os << x << ": ";
		for (size_t l = 0; l != G[x].size(); ++l)
			os << to_std_string(to_string(G[x][l])),
			l == 0 ? os << " => " : os << " ";
		os << "\n";
	}
	os << "nts:\n";
	for (auto& x : nts) {
		os << to_std_string(to_string(x.first)) << " -";
		for (auto& y : x.second) os << " " << y;
		os << "\n";
	}
	os << "S:\n";
	for (auto& x : S) {
		os << "---:\n";
		for (auto& y : x) {
			if (y.from != y.set)
				print(os << "\t " << y.prod << " \t", y) <<"\n";
		}
	}
	os << "completed S:\n";
	for (auto& x : S) {
		os << "---:\n";
		for (auto& y : x) {
			if (y.from != y.set && y.dot == G[y.prod].size())
				print(os << "\t " << y.prod << " \t", y) <<"\n";
		}
	}
	return os;
}
template<typename CharT>
ostream_t& parser<CharT>::print_grammar(ostream_t& os,
	const typename parser<CharT>::grammar& g) const
{
	for (const auto& x : g)
		for (const auto& y : x.second) {
			os << '\t' << to_std_string(x.first) << " =";
			for (const auto& z : y) {
				os << " \"";
				if (z.size() == 0) os << "";
				else os << to_std_string(z);
				os << "\"";
			}
			os << "\n";
		}
	return os;
}
template<typename CharT>
ostream_t& parser<CharT>::print_dictmap(ostream_t& os,
	const std::map<parser<CharT>::string, size_t>& dm) const
{
	os << "d.m:\n";
	for (const auto& x : dm) os <<
		'\t' << to_std_string(x.first) << ' ' << x.second << "\n";
	return os;
}
#endif // DEBUG

#ifdef WITH_TO_METHODS
template <typename CharT>
std::string parser<CharT>::grammar_text() const {
	std::stringstream txt;
	for (const auto &p : G) {
		txt << "\n\\l";
		size_t i = 0;
		for (const auto& l : p){
			if (l.nt()) txt << to_std_string(d.get(l.n()));
			else if (l.c() != (CharT)0) txt << to_std_string(l.c());
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
bool parser<CharT>::to_dot(ostream_t& ss, P&& pt) const {
	auto keyfun = [this](const pnode& k) {
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
	for (auto &it: pt.size() ? pt : f.g) {
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
bool parser<CharT>::to_tml_facts(ostream_t& ss, const parse_forest& frst) const{
	auto to_str = [this](const lit& l) {
		std::stringstream ss;
		ss << std::quoted(to_std_string(to_string(l)));
		return ss.str();
	};
	auto n_e = frst.get_nodes_and_edges();
	auto& n = n_e.first;
	auto& e = n_e.second;
	for (size_t i = 0; i < n.size()-1; ++i)
		ss << "node(" << i << " " << to_str(n[i].first) << " " <<
			n[i].second[0] << " " << n[i].second[1] << ").\n";
	for (auto& x : e) ss << "edge(" << x.first << ", " << x.second <<").\n";
	return true;
}
template <typename CharT>
std::string parser<CharT>::to_tml_rule(const pnode nd) const {
	std::stringstream ss;
	if (nd.first.nt()) ss << to_std_string(d.get(nd.first.n()));
	else if (nd.first.c() == (CharT) 0) ss << "ε";
	else ss << to_std_string(nd.first.c());
	ss << "(" << nd.second[0] << " " << nd.second[1] << ")";
	return ss.str();
}
template <typename CharT>
template <typename P>
bool parser<CharT>::to_tml_rules(ostream_t& ss, P &&pt) const {
	std::set<std::string> terminals;
	for (auto &it: pt.size()? pt : f.g) {
		for (auto &pack : it.second) { 
			ss << to_tml_rule(it.first) << ":-";
			for (size_t i = 0; i < pack.size(); i++) {
				// if terminal
				if (f.g.find(pack[i]) == f.g.end())
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
#endif // WITH_TO_METHODS

} // idni namespace
#endif // __IDNI__PARSER__PARSER_TMPL_H__

