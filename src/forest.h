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
#ifndef __IDNI__FOREST_H__
#define __IDNI__FOREST_H__
#include "forest.h"
namespace idni {

template <typename CharT>
template <typename T, typename P>
bool parser<CharT>::iterate_forest(T out_rel, P &&pt) const {
	std::stringstream ss;
	auto get_args = [this](const pnode & k) {
		arg_t args;
		if (k.first.nt()) args.emplace_back(
			string{ '"' } + d.get(k.first.n()) + string{ '"' });
		else if (k.first.c() == (CharT) '\0')
			args.emplace_back(epsilon());
		else args.emplace_back(string({ '"', k.first.c(), '"' }));
		args.emplace_back(k.second[0]);
		args.emplace_back(k.second[1]);
		return args;
	};

	std::map<pnode, size_t> nid;
	size_t id = 0;
	for (auto &it: pt.size() ? pt : pfgraph) {
		nid[it.first] = id;
		// skip ids for one child ambig node
		id += it.second.size() == 1 ? 0 : it.second.size(); // ambig node ids;
		//DBG(assert(it.second.size()!= 0));
		id++;
	}
	ss << std::endl;
	string node_s=from_cstr<CharT>("node"), edge_s=from_cstr<CharT>("edge");
	for (auto &it : pt.size()? pt : pfgraph) {
		arg_t ndesc = get_args(it.first);
		out_rel(node_s, nid[it.first], ndesc);
		size_t p = 0;
		for (auto &pack : it.second) {
			if (it.second.size() > 1) {  //skipping if only one ambigous node, an optimization
				++p;
				out_rel(edge_s, nid[it.first],
						{ nid[it.first] + p } );
				out_rel(node_s, nid[it.first] + p, {} );
			}
			for (auto & nn: pack) {
				if (nid.find(nn) == nid.end()) nid[nn] = id++; // for terminals, not seen before
				arg_t nndesc = get_args(nn);
				out_rel(node_s, nid[nn], nndesc);
				out_rel(edge_s, nid[it.first] + p, { nid[nn] });
			}
		}
	}
	return true;
}

template <typename CharT>
template <typename cb_enter_t, typename cb_exit_t, typename cb_revisit_t,
	typename cb_ambig_t>
bool parser<CharT>::traverse_forest(const pnode &root, cb_enter_t cb_enter,
	cb_exit_t cb_exit, cb_revisit_t cb_revisit, cb_ambig_t cb_ambig)
{
	std::set<pnode> done;
	return _traverse_forest(root, done,
		cb_enter, cb_exit, cb_revisit, cb_ambig);
}

template <typename CharT>
template <typename cb_enter_t, typename cb_exit_t, typename cb_revisit_t,
	typename cb_ambig_t>
bool parser<CharT>::_traverse_forest(const pnode &root, std::set<pnode> &done,
	cb_enter_t cb_enter, cb_exit_t cb_exit, cb_revisit_t cb_revisit,
	cb_ambig_t cb_ambig)
{
	bool ret = true;
	std::set<std::vector<pnode>> pack;
	if (root.first.nt()) {
		auto it = pfgraph.find(root);
		if (it == pfgraph.end()) return false;
		pack = it->second;
	}
	cb_enter(root);
	done.insert(root);

	std::set<std::vector<pnode>> choosen_pack = pack.size() > 1
		? cb_ambig(root, pack) : pack; 
	for (auto &nodes: choosen_pack)
		for (auto &chd: nodes)
			if (!done.count(chd) || cb_revisit(chd)) 
				ret &= _traverse_forest(chd, done, cb_enter,
					cb_exit, cb_revisit, cb_ambig);
	cb_exit(root, choosen_pack);
	return ret;
}
template <typename CharT>
typename parser<CharT>::ptree_t parser<CharT>::get_parsed_tree() {
	pnode root(start, { 0, inputstr.length() });
	ptree_t pt;

	auto cb_enter = [](const pnode&) {};
	auto cb_revisit = [](const auto&) { return false; };
	auto cb_exit = [&pt](const pnode& root,
		const std::set<std::vector<pnode>>& choice_set)
	{
		DBG( assert(choice_set.size() <= 1));
		if (root.first.nt()) pt.insert({ root, choice_set });
	};
	auto cb_disambig_by_priority = [this](const pnode& root,
		const std::set<std::vector<pnode>>& ambset)
	{
		DBG(assert(root.first.nt()));
		int_t cur = -1, pchoice = INT32_MIN, maxp = INT32_MIN,
			pref_choice = INT32_MIN;

		for (auto & choice : ambset) {
			std::vector<lit> gprod;
			gprod.push_back(root.first);
			//get the production
			for( auto & nd : choice)
				gprod.emplace_back(nd.first);
			
			++cur;
			// find the top priority if any
			auto it = priority.find(gprod);
			if (it != priority.end() && it->second >= maxp)
				maxp = it->second, pchoice = cur;
			// find the preference if any
			if (prefer.count(gprod)) pref_choice = cur;
		}
		if (pchoice == INT32_MIN && pref_choice == INT32_MIN) {
			std::cerr << "\n Could not resolve ambiguity,"
				" defaulting to first choice!\n";
			pchoice = 0;
		}
		std::set<std::vector<pnode>> choosen;
		auto it = std::next(ambset.begin(),
			int_t(pchoice >= 0 ? pchoice : pref_choice));
		choosen.insert(*it );
		return choosen;
	};

	traverse_forest(root, cb_enter, cb_exit, cb_revisit,
		cb_disambig_by_priority);
	return pt;
}

template <typename CharT>
size_t parser<CharT>::count_parsed_trees() {
	pnode root(start, { 0, inputstr.length() });
	size_t count = 1;
	auto cb_enter = [](const auto&) {};
	auto cb_exit =  [](const auto&, const auto&) {};
	auto cb_keep_ambig = [&count](const pnode&, auto& ambset) {
		count *= ambset.size();
		return ambset;
	};
	auto cb_revisit =  [](const auto&) { return false; }; // revisit
	traverse_forest(root, cb_enter, cb_exit, cb_revisit, cb_keep_ambig);
	return count; 
}

template <typename CharT>
std::vector<typename parser<CharT>::arg_t>
	parser<CharT>::get_parse_graph_facts()
{
	std::vector<arg_t> rts;
	auto rt_rel_output = [&rts] (string rel, size_t id,
		std::vector<std::variant<size_t, string>> args)
	{
		args.insert(args.begin(), { rel, id });
		rts.emplace_back(args);
	};
	iterate_forest(rt_rel_output);
	return rts;
}

template <typename CharT>
bool parser<CharT>::forest() {
	bool ret = false;
	// clear forest structure if any
	pfgraph.clear();
	bin_tnt.clear();
	tid = 0;
	// set the start root node
	size_t len = inputstr.length();
	pnode root(start, {0,len});
	// preprocess parser items for faster retrieval
	emeasure_time_start(tspfo, tepfo);
	int count = 0;
	for (size_t n = 0; n < len + 1; n++)
		for (const item& i : S[n]) count++, pre_process(i);
	emeasure_time_end(tspfo, tepfo) << " :: preprocess time, " <<
						"size : "<< count << "\n";
	std::cout << "sort sizes : " << sorted_citem.size() << " " <<
						rsorted_citem.size() << " \n";
	// build forest
	emeasure_time_start(tsf, tef);
	ret = bin_lr ? build_forest2(root) : build_forest(root);
	emeasure_time_end(tsf, tef) <<" :: forest time\n";

	std::cout <<"# parse trees " << count_parsed_trees() << "\n";

	return ret; 
}

// collects all possible variations of the given item's rhs while respecting the
// span of the item and stores them in the set ambset. 
template <typename CharT>
void parser<CharT>::sbl_chd_forest(const item &eitem,
	std::vector<pnode> &curchd, size_t xfrom,
	std::set<std::vector<pnode>> &ambset)
{
	//check if we have reached the end of the rhs of prod
	if (G[eitem.prod].size() <= curchd.size() + 1)  {
		// match the end of the span we are searching in.
		if (curchd.back().second[1] == eitem.set)
			ambset.insert(curchd);
		return;
	}
	// curchd.size() refers to index of cur literal to process in the rhs of production
	pnode nxtl = { G[eitem.prod][curchd.size() + 1], {} };
	// set the span start/end of the terminal symbol 
	if (!nxtl.first.nt()) {
		nxtl.second[0] = xfrom;
		// for empty, use same span edge as from
		if (nxtl.first.c() == (CharT) '\0') nxtl.second[1] = xfrom;
		// ensure well-formed combination (matching input) early
		else if (xfrom < inputstr.size() &&
					inputstr.at(xfrom) == nxtl.first.c())
			nxtl.second[1] = ++xfrom ;
		else // if not building the correction variation, prune this path quickly 
			return;
		// build from the next in the line
		size_t lastpos = curchd.size();
		curchd.push_back(nxtl),
		sbl_chd_forest(eitem, curchd, xfrom, ambset);
		curchd.erase(curchd.begin() + lastpos, curchd.end());
	} else {
		// get the from/to span of all non-terminals in the rhs of production.
		nxtl.second[0] = xfrom;
		
		//auto &nxtl_froms = sorted_citem[nxtl.n()][xfrom];
		auto &nxtl_froms = sorted_citem[{ nxtl.first.n(), xfrom }];
		for (auto &v : nxtl_froms) {
			// ignore beyond the span
			if (v.set > eitem.set) continue;
			// store current and recursively build for next nt
			size_t lastpos = curchd.size(); 
			nxtl.second[1] = v.set,
			curchd.push_back(nxtl), xfrom = v.set,
			sbl_chd_forest(eitem, curchd, xfrom, ambset);
			curchd.erase(curchd.begin() + lastpos, curchd.end());
		}
	}
}
// builds the forest starting with root
template <typename CharT>
bool parser<CharT>::build_forest (const pnode& root) {
	if (!root.first.nt()) return false;
	if (pfgraph.find(root) != pfgraph.end()) return false;

	//auto &nxtset = sorted_citem[root.n()][root.second[0]];
	auto &nxtset = sorted_citem[{ root.first.n(), root.second[0] }];
	std::set<std::vector<pnode>> ambset;
	for (const item &cur : nxtset) {
		if (cur.set != root.second[1]) continue;
		DBG(assert(root.first.n() == G[cur.prod][0].n());)
		pnode cnode(G[cur.prod][0], { cur.from, cur.set });
		std::vector<pnode> nxtlits;
		sbl_chd_forest(cur, nxtlits, cur.from, ambset );
		pfgraph[cnode] = ambset;
		for (auto &aset : ambset)
			for (const pnode& nxt : aset)
				build_forest(nxt);
	}	
	return true;
}

template <typename CharT>
bool parser<CharT>::bin_lr_comb(const item& eitem,
	std::set<std::vector<pnode>>& ambset)
{
	std::vector<pnode> rcomb, lcomb;
	if (eitem.dot < 2) return false;

	pnode right = { G[eitem.prod][eitem.dot-1], {} };

	if (!right.first.nt()) {
		right.second[1] = eitem.set;
		if (right.first.c() == (CharT) '\0')
			right.second[0] = right.second[1];
		else if (inputstr.at(eitem.set -1) == right.first.c())
			right.second[0] = eitem.set -1 ;
		else return false;
		rcomb.emplace_back(right);
	} else {
		auto &rightit = rsorted_citem[{ right.first.n(), eitem.set }];
		for (auto &it : rightit)
			if (eitem.from <= it.from) 
				right.second[1] = it.set,
				right.second[0] = it.from,
				rcomb.emplace_back(right);
	}
	// many literals in rhs
	if (eitem.dot > 3) {
		//DBG(print(cout, eitem);)
		std::vector<lit> v(G[eitem.prod].begin() + 1,
					G[eitem.prod].begin() + eitem.dot - 1);
		DBG(assert( bin_tnt.find(v) != bin_tnt.end());)
		pnode left = { bin_tnt[v], {} };
		//DBG(cout << "\n" << d.get(bin_tnt[v].n()) << endl);
		auto &leftit = sorted_citem[{ left.first.n(), eitem.from }];
		// doing left right optimization
		for (auto &it : leftit) 
			for (auto &rit : rcomb)    
				if (it.set == rit.second[0]) {
					left.second[0] = it.from;
					left.second[1] = it.set;
					ambset.insert({ left, rit });
				} 
	}
	// exact two literals in rhs
	else if (eitem.dot == 3) {
		pnode left = { G[eitem.prod][eitem.dot-2], {} };
		lit& l = left.first;
		if (!l.nt()) {
			left.second[0] = eitem.from;
			if (l.c() == (CharT) '\0')
				left.second[1] = left.second[0];
			else if (inputstr.at(eitem.from) == l.c() )
				left.second[1] = eitem.from + 1  ;
			else return false;
			//do Left right optimisation
			for (auto &rit : rcomb)  
				if (left.second[1] == rit.second[0])
					ambset.insert({ left, rit });
		}
		else {
			auto &leftit = sorted_citem[{ l.n(), eitem.from }];
			for (auto &it : leftit) 
				for (auto &rit : rcomb)    
					if (it.set == rit.second[0])
						left.second[0] = it.from,
						left.second[1] = it.set,
						ambset.insert({ left, rit });
		}
	}
	else {
		DBG(assert(eitem.dot == 2));
		for (auto &rit : rcomb)
			if (eitem.from <= rit.second[0])
				ambset.insert({ rit });
	}
	return true;
}

template <typename CharT>
bool parser<CharT>::build_forest2 (const pnode &root) {
	if (!root.first.nt()) return false;
	if (pfgraph.find(root) != pfgraph.end()) return false;

	//auto &nxtset = sorted_citem[root.n()][root.second[0]];
	auto &nxtset = sorted_citem[{ root.first.n(), root.second[0] }];
	std::set<std::vector<pnode>> ambset;
	for (const item &cur: nxtset) {
		if (cur.set != root.second[1]) continue;
		pnode cnode(
			completed(cur) ? G[cur.prod][0] : lit{ root.first.n() },
			{ cur.from, cur.set });
		bin_lr_comb(cur, ambset);
		pfgraph[cnode] = ambset;
		for (auto &aset: ambset)
			for (const pnode& nxt : aset)
				build_forest2(nxt);
	}
	return true;
}

template <typename CharT>
std::vector<typename parser<CharT>::children> parser<CharT>::get_children(
	const pnode nd, bool all) const
{
	std::vector<children> ncs;
	auto label = [this](const pnode p, bool all=1) {
		return p.first.nt()
			? d.get(p.first.n())
			: (all ? string{p.first.c()} : string{});
	};
	auto it = pfgraph.find(nd);
	if (it == pfgraph.end()) return {{}};
	bool amb = false;
#ifdef DEBUG
	if (print_ambiguity && it->second.size() > 1) amb = true, std::cout<<"'"
		<< to_std_string(label(nd, true)) <<"' is ambiguous\n";
#endif
	size_t c = 0;
	for (auto &pack : it->second) {
		if (amb) std::cout << "\tpack " << ++c << ":\t";
		ncs.emplace_back();
		for (auto& p : pack) {
			if (amb) std::cout << "\t'" <<
				to_std_string(label(p, true)) << "' ";
			ncs.back().push_back(std::pair<string, const pnode>(
				label(p, all), p));
		}
		if (amb) std::cout << "\n";
	}
	return ncs;
}

template <typename CharT>
typename parser<CharT>::string parser<CharT>::flatten(const pnode p) const {
	stringstream ss;
	auto it = pfgraph.find(p);
	if (it == pfgraph.end()) return {{}};
	for (auto &pack : it->second) {
		for (auto &p : pack)
			if (p.first.nt()) {
				auto flat = flatten(p);
				if (flat.size() && flat[0] != 0 &&
					flat[0]!=std::char_traits<CharT>::eof())
						ss << flat;
			} else ss << p.first.c();
		break; // traverse only the first pack of each node
	}
	return ss.str();
}

template <typename CharT>
void parser<CharT>::down(const pnode& p, const actions& a, bool ap) const {
	auto label = p.first.is_builtin() ? d.get(p.first.builtin)
		: p.first.nt() ? d.get(p.first.n()) : string{ p.first.c() };
#ifdef DEBUG
	auto print_traversing_info = [&label, &p, this]() {
		std::string wrap = p.first.nt() ? "" : "'";
		std::string l = to_std_string(label);
		bool print_flattened = true;
		if (l.size() == 1 && !::isprint(l[0])) {
			print_flattened = false;
			if (l[0] == 0) l = "ε";
			else {
				std::stringstream ss;
				ss << "\\" << (size_t) l[0];
				l = ss.str();
			}
		}
		std::cout << "down(" << wrap << l << wrap << ")";
		if (print_flattened) {
			auto s = flatten(p);
			l = to_std_string(s.size() > 60 ? s.substr(0, 60) : s);
			if (l.size() && l[0] != 0)
				std::cout << ": \t\"" <<l << "\"";
		}
		std::cout << "\n";		
	};
	if (print_traversing) print_traversing_info();
#endif
	// include also ebnf-transformation generated nodes
	if (label.size() > 1 && label[0]==(CharT)'_' && label[1]==(CharT)'R')
		label = label.substr(2, label.find_last_of((CharT)'_') - 2);
	auto it = a.find(label);
	if (it != a.end()) it->second(p, get_children(p));
	else if (ap) down(get_children(p), a);
}

template <typename CharT>
void parser<CharT>::down(const children& nc, const actions& a) const {
	for (auto&c : nc) down(c.second, a);	
};

template <typename CharT>
void parser<CharT>::down(const variations& ncs, const actions& a) const {
	for (auto&nc : ncs) down(nc, a);
};

template <typename CharT>
void parser<CharT>::topdown(const string& start, const actions& a) const {
	auto it = pfgraph.begin();
	for ( ; it != pfgraph.end() &&
		!(it->first.first.nt() && d.get(it->first.first.n()) == start);
		++it) ;
	if (it != pfgraph.end()) down(it->first, a);
}

} // idni namespace
#endif // __IDNI__FOREST_H__
