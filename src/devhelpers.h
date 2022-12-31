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
#ifndef __IDNI__PARSER__DEVHELPERS_H__
#define __IDNI__PARSER__DEVHELPERS_H__
#include "parser.h"
namespace idni {

template <typename CharT>
bool to_tml_facts(ostream_t& os, const typename parser<CharT>::pforest& f);
template <typename CharT>
bool to_dot(ostream_t& os, const typename parser<CharT>::pforest& f,
	const std::string& inputstr = "",
	const std::string& grammar_text = "");
template <typename CharT, typename P>
bool to_dot(ostream_t& os, P& g,
	const std::string& inputstr = "",
	const std::string& grammar_text = "");
template <typename CharT>
bool to_tml_rules(ostream_t& os, const typename parser<CharT>::pforest& f);
template <typename CharT, typename P = parser<CharT>::pnode_graph>
bool to_tml_rules(ostream_t& os, P& g);
template<typename CharT>
std::string to_tml_rule(const typename parser<CharT>::pnode& nd);
//------------------------------------------------------------------------------
template <typename CharT>
bool to_dot(ostream_t& ss, const typename parser<CharT>::pforest& f,
	const std::string& inputstr,
	const std::string& grammar_text)
{
	return to_dot<CharT>(ss, f.g, inputstr, grammar_text);
}

template <typename CharT, typename P>
bool to_dot(ostream_t& ss, P& g, const std::string& inputstr,
	const std::string& grammar_text)
{
	auto dot_safe = [](const std::string &s) {
		std::stringstream ss;
		for (size_t n = 0; n != s.size(); ++n) {
			if (s[n] == '"') ss << '\\';
			ss << s[n];
		}
		return ss.str();
	};
	auto keyfun = [](const typename parser<CharT>::pnode& k) {
		std::stringstream l;
		l << k.first.to_std_string(from_cstr<CharT>("ε")) <<
			"_" << k.second[0] << "_" << k.second[1] << "_";
		std::string desc = l.str();
		return std::pair<size_t, std::string>(
			std::hash<std::string>()(desc), desc);
	};
	ss << "digraph {\n";
	ss << "_input_" << "[label =\"" << dot_safe(to_std_string(inputstr)) <<
		"\", shape = rectangle]" ;
	ss << "_grammar_" << "[label =\"" << dot_safe(grammar_text) <<
		"\", shape = rectangle]" ;
	ss << "\nnode" << "[ ordering =\"out\"];";
	ss << "\ngraph" << "[ overlap =false, splines = true];";

	if constexpr( std::is_same<P, typename parser<CharT>::psptree>::value ) {
		stringstream pss;
		auto pointerid = [&pss]( auto &p){
			pss << p;
			string s = pss.str();
			pss.str({});
			s[0]='_', s[1]='_';
			return s;
		};
		std::deque<typename parser<CharT>::psptree> stk;
		stk.push_back(g);
		while( !stk.empty() ){
			typename parser<CharT>::psptree cur = stk.back();
			stk.pop_back();
			
			if(!cur->value.first.nt()) continue;
			auto key = keyfun(cur->value);
			ss << "\n" << pointerid(cur) << "["<<"label=\"" << key.second <<"\"];";
			std::stringstream pstr;
			for (auto & nn: cur->child) {
				auto nkey = keyfun(nn->value);
				ss << "\n" << pointerid(nn) << "[label=\"" << nkey.second << "\"];";
				ss << "\n" << pointerid(cur) << "->" << pointerid(nn) << ';';
			}
			stk.insert(stk.end(), cur->child.rbegin(), cur->child.rend());
			pstr.str({});
		}
		ss << "\n}\n";
		return true;
	}
	else {
		std::unordered_set<std::pair<size_t,size_t>,
			typename parser<CharT>::hasher_t> edgedone;
		edgedone.clear();
		for (auto &it : g) {
			auto key = keyfun(it.first);
			std::string ctxt;
			if constexpr ( std::is_same<P, typename parser<CharT>::pgraph>::value ) 
				if(g.cycles.contains(it.first) ) ctxt="shape = doublecircle,";
			
			ss << "\n" << key.first << "["<< ctxt <<"label=\"" << key.second <<"\"];";
			size_t p = 0;
			std::stringstream pstr;
			for (auto &pack : it.second) {
				pstr<<key.second<<p++;
				auto ambkey = std::hash<std::string>()(pstr.str());
				ss << "\n" << ambkey << "[shape = point,label=\"" <<
					pstr.str() << "\"];";
				//if (edgedone.insert({ key.first, ambkey }).second)
					ss << "\n" << key.first << "->" << ambkey <<';';
				for (auto & nn: pack) {
					auto nkey = keyfun(nn);
					ss << "\n" << nkey.first << "[label=\"" <<
						nkey.second << "\"];";
					//if (edgedone.insert({ ambkey, nkey.first }).second)
						ss << "\n" << ambkey << "->" <<
							nkey.first<< ';';
				}
				pstr.str({});
			}
		}
		ss << "\n}\n";
		return true;
	}
}
template <typename CharT>
bool to_tml_facts(ostream_t& ss, const typename parser<CharT>::pforest& f) {
	auto n_e = f.get_nodes_and_edges();
	auto& n = n_e.first;
	auto& e = n_e.second;
	for (size_t i = 0; i < n.size()-1; ++i)
		ss << "node(" << i << " " <<
			std::quoted(n[i].first.to_std_string()) << " " <<
			n[i].second[0] << " " << n[i].second[1] << ").\n";
	for (auto& x : e) ss << "edge(" << x.first << ", " << x.second <<").\n";
	return true;
}
template <typename CharT>
std::string to_tml_rule(const typename parser<CharT>::pnode& nd)
{
	std::stringstream ss;
	ss << nd.first.to_std_string(from_cstr<CharT>("ε")) <<
		"(" << nd.second[0] << " " << nd.second[1] << ")";
	return ss.str();
}
template <typename CharT>
bool to_tml_rules(ostream_t& ss, const typename parser<CharT>::pforest& f) {
	return to_tml_rules<CharT>(ss, f.g);
}
template <typename CharT, typename P>
bool to_tml_rules(ostream_t& ss, P& g) {
	std::set<std::string> terminals;
	for (auto &it : g) {
		for (auto &pack : it.second) { 
			ss << to_tml_rule<CharT>(it.first) << " :- ";
			for (size_t i = 0; i < pack.size(); i++) {
				// if terminal
				if (g.find(pack[i]) == g.end()) terminals
					.insert(to_tml_rule<CharT>(pack[i]));
				ss << to_tml_rule<CharT>(pack[i])
					<< (i == pack.size()-1 ? "." : ", ");
			};
			ss << "\n";
		}
	}
	for (auto &t: terminals) ss << t << ".\n";
	return true;
}

} // idni namespace
#endif // __IDNI__PARSER__DEVHELPERS_H__
