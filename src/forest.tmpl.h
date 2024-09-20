// LICENSE
// This soNodeTware is free for use and redistribution while including this
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
#ifndef __IDNI__PARSER__FOREST_TMPL_H__
#define __IDNI__PARSER__FOREST_TMPL_H__
#include <optional>
#include "forest.h"
namespace idni {

template <typename NodeT>
std::ostream& forest<NodeT>::tree::to_print(std::ostream& os, size_t l,
	std::set<size_t> skip, bool nulls) const
{
	const NodeT &value = this->value;
	if (skip.size() && value.first.nt() &&
		skip.find(value.first.n()) != skip.end())
			return os;
	if (!nulls && value.first.is_null()) return os;
	os << "\n";
	for (size_t t = 0; t < l; t++) os << "\t";
	if (value.first.nt()) os << value.first
		<< "(" << value.first.n() << ")";
	else if (value.first.is_null()) os << "null";
	else os << value.first;
	os << "[" << value.second[0] << ", "
		<< value.second[1] << "]";
	for (auto& d : child)
		d->to_print(os, l + 1, skip, nulls);
	return os;
}

template<typename NodeT>
typename forest<NodeT>::sptree forest<NodeT>::graph::extract_trees() {
	return _extract_trees(root);
}

template <typename NodeT>
typename forest<NodeT>::sptree forest<NodeT>::graph::_extract_trees(
	node& r, int_t)
{
	std::deque<sptree> stk;
	sptree troot = NULL;
	troot = std::make_shared<tree>(r);
	stk.push_back(troot);
	std::map<NodeT, size_t> edgcount;
	while (!stk.empty()) {
		sptree cur = stk.back();
		stk.pop_back();
		// skip for terminal
		if (!cur->value->first.nt()) continue;
		auto fit = this->find(cur->value);
		if (fit == this->end()) continue;
		auto& ns = fit->second;
		if (!ns.size()) continue;
		auto it = ns.begin();
		if (ns.size() >= 1) {
			// select which descendants to traverse not already done;
			if (edgcount[cur->value] == ns.size()) continue;
			for (size_t i = 0; i < edgcount[cur->value]; i++, ++it);

			edgcount[cur->value]++;
		}
		for (auto& cnode : *it)
			cur->child.push_back(std::make_shared<tree>(cnode));
		// pushing in stack in reverse order to keep left to right child dfs
		stk.insert(stk.end(), cur->child.rbegin(), cur->child.rend());
	}
	return troot;
}

template <typename NodeT>
forest<NodeT>::node forest<NodeT>::root() const { return rt; }

template <typename NodeT>
void forest<NodeT>::root(const node& n) { rt = n; }

template <typename NodeT>
void forest<NodeT>::clear() { g.clear(); }

template <typename NodeT>
bool forest<NodeT>::contains(const node& n) const {
	return g.find(n) != g.end(); }

template <typename NodeT>
forest<NodeT>::nodes_set& forest<NodeT>::operator[](const node& p) {
	return g[p];
}

template <typename NodeT>
const forest<NodeT>::nodes_set& forest<NodeT>::operator[](const node& p) const {
	return g[p];
}

template <typename NodeT>
size_t forest<NodeT>::count_trees() const { return count_trees(root()); }

template <typename NodeT>
size_t forest<NodeT>::count_trees(const node& root) const {
	std::map<node, size_t> ndc;
	auto cb_exit = [&ndc](const node& croot, auto& ambset) {
		for (auto& pack : ambset) {
			size_t pkc = 1; // count of the pack
			for (auto& sym : pack)
				if (sym->first.nt() && ndc[sym] != 0) {
					size_t x = pkc * ndc[sym];
					if (pkc != 0 && x / pkc != ndc[sym]) {
						MS(std::cout<<"Overflow\n");
						ndc[croot]=SIZE_MAX;
						return;
					}
					pkc = x;
				}
			ndc[croot] += pkc; // adding to curroot count
		}
	};
	traverse(root, NO_ENTER, cb_exit);
	return ndc[root];
}

// a dfs based approach to detect cycles for
// any traversable type
template<typename NodeT>
template<typename TraversableT>
bool forest<NodeT>::detect_cycle(TraversableT& gr) const {
	std::map<NodeT, bool> inprog;
	auto cb_enter = [&inprog](const NodeT& n) {
		if (n.first.nt()) inprog[n] = true;
	};
	auto cb_revisit = [&inprog, &gr](const NodeT& n) {
		if (inprog[n] == true) gr.cycles.insert(n);
		return false;
	};
	auto cb_exit = [&inprog](const NodeT& n, auto&) {
		if (n.first.nt()) inprog[n] = false;
	};
	gr.cycles.clear();
	return traverse(gr, gr.root, cb_enter, cb_exit, cb_revisit);
}

template <typename NodeT>
bool forest<NodeT>::is_binarized() const {
	for (auto& kv : this->g) for (auto& rhs : kv.second)
		if (rhs.size() > 2) return false;
	return true;
}

template <typename NodeT>
bool forest<NodeT>::_extract_graph_uniq_node(std::set<node>& done,
	std::vector<node>& todo, graphv& graphs, size_t trid,
	cb_next_graph_t cb_next_graph, bool& no_stop) const
{
	bool ret = true;
	while (todo.size() && no_stop) {
		const node root = todo.back();
		todo.pop_back();
		if (!root->first.nt()) continue;
		const auto& it = g.find(root);
		if (it == g.end() || !done.insert(root).second) continue;
		const nodes_set& packs = it->second;
		auto curgraph = graphs[trid];
		auto curdone = done;
		auto curtodo = todo;
		size_t ambpid = 0;
		for (auto& pack : packs) {
			if (ambpid == 0) {
				graphs[trid].insert({ root, { pack } });
				for (auto& node : pack) todo.push_back(node);
			} else {
				graphs.emplace_back(curgraph);
				graphs.back().insert({ root, { pack } });
				size_t ntrid = graphs.size() - 1;
				auto ntodo(curtodo);
				auto ndone(curdone);
				for (auto& node : pack) ntodo.push_back(node);
				ret &= _extract_graph_uniq_node(ndone, ntodo,
					graphs, ntrid, cb_next_graph, no_stop);
			}
			ambpid++;
		}
	}
	if (no_stop) no_stop = cb_next_graph(graphs[trid]);
	return ret;
}

template <typename NodeT>
bool forest<NodeT>::_extract_graph_uniq_edge(std::map<node, size_t>& ndmap,
	std::set<edge>& done, nodes& todo, graphv& graphs,
	size_t gid, cb_next_graph_t cb_next_graph, bool& no_stop) const
{
	bool ret = true;
	while (todo.size() && no_stop) {
		const NodeT root = todo.back();
		todo.pop_back();
		if (!root.first.nt()) continue;
		const auto& it = g.find(root);
		if (it == g.end()) continue;
		const nodes_set& packs = it->second;
		graph curgraph = graphs[gid];
		std::set<edge> curdone(done);
		nodes curtodo(todo);
		size_t rootid = ndmap[root], ambpid = -1;
		bool derived = false;
		for (auto& pack : packs) {
			++ambpid;
			if (curdone.find({ rootid, rootid + ambpid + 1 })
				!= curdone.end()) continue;
			if (!derived) {
				done.insert({ rootid, rootid + ambpid + 1 }),
				derived = true;
				if (graphs[gid].find(root) == graphs[gid].end())
					graphs[gid].insert( { root, { pack } });
				else graphs[gid][root].insert(pack);
				for (auto& node : pack)
					if (node->first.nt() && done.insert({
						rootid+ambpid+1, ndmap[node] })
						.second)
							todo.push_back(node);
			} else {
				auto ntodo(curtodo);
				auto ndone(curdone);
				ndone.insert({ rootid, rootid + ambpid + 1 });
				graphs.emplace_back(curgraph);
				if (graphs.back().find(root) ==
					graphs.back().end()) graphs.back()
						.insert({ root, { pack } });
				else graphs.back()[root].insert(pack);
				size_t ngid = graphs.size() - 1;
				for (auto& node : pack)
					if (node->first.nt() && ndone.insert({
						rootid+ambpid+1, ndmap[node]})
						.second)
							ntodo.push_back(node);
				ret &= _extract_graph_uniq_edge(ndmap, ndone,
					ntodo, graphs, ngid, cb_next_graph,
					no_stop);
			}
		}
	}
	if (no_stop) no_stop = cb_next_graph(graphs[gid]);
	return ret;

}

template <typename NodeT>
typename forest<NodeT>::graphv forest<NodeT>::extract_graphs(
	const node& root, cb_next_graph_t cb_next_graph, bool unique_edge) const
{
	graphv graphs;
	std::set<node> dn;
	std::set<edge> de;
	nodes todo;
	todo.push_back(root);
	graphs.emplace_back();
	graphs.back().root = root;
	std::map<node, size_t> ndmap;
	int_t id = 0;
	for (auto& it : g) {
		ndmap[it.first] = id++;
		id += it.second.size(); // ambig node ids;
	}
	bool no_stop = true;
	if (unique_edge) _extract_graph_uniq_edge(ndmap, de, todo, graphs, 0,
						cb_next_graph, no_stop);
	else _extract_graph_uniq_node(dn, todo, graphs,0,cb_next_graph,no_stop);
	return graphs;
}

template <typename NodeT>
template <typename cb_enter_t, typename cb_exit_t,
	typename cb_revisit_t, typename cb_ambig_t>
bool forest<NodeT>::traverse(cb_enter_t cb_enter, cb_exit_t cb_exit,
	cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const
{
	return traverse(root(), cb_enter, cb_exit, cb_revisit,cb_ambig);
}

template <typename NodeT>
template <typename cb_enter_t, typename cb_exit_t,
	typename cb_revisit_t, typename cb_ambig_t>
bool forest<NodeT>::traverse(const node& root, cb_enter_t cb_enter,
	cb_exit_t cb_exit, cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const
{
	std::set<node> done;
	return _traverse(this->g, root, done, cb_enter, cb_exit, cb_revisit,
		cb_ambig);
}

template <typename NodeT>
template <typename cb_enter_t, typename cb_exit_t,
	typename cb_revisit_t, typename cb_ambig_t>
bool forest<NodeT>::traverse(const node_graph& gr, const node& root,
	cb_enter_t cb_enter, cb_exit_t cb_exit,
	cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const
{
	std::set<node> done;
	return _traverse(gr, root, done, cb_enter, cb_exit, cb_revisit,
		cb_ambig);
}

template <typename NodeT>
template <typename cb_enter_t, typename cb_exit_t,
	typename cb_revisit_t, typename cb_ambig_t>
bool forest<NodeT>::_traverse(const node_graph& g, const node& root,
	std::set<node>& done, cb_enter_t cb_enter, cb_exit_t cb_exit,
	cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const
{
//#define DEBUG_TRAVERSE
#ifdef DEBUG_TRAVERSE
	std::cout << "enter: \t\t" << root.first.to_std_string() << "\n";
	std::cout << "\t" << (root.first.nt() ? "NT" : " T") << " ";
	if (root.first.nt()) std::cout << "n: " << root.first.n()
		<< " `" << root.first.to_std_string();
	else std::cout << "t: `" << to_std_string(root.first.t()) << "`";
	std::cout << "\n";
#endif
	bool ret = true;
	nodes_set pack;
	if (root->first.nt()) {
		auto it = g.find(root);
		if (it == g.end()) return false;
		pack = it->second;
	}
	cb_enter(root);
	done.insert(root);
	nodes_set choosen_pack = pack.size() > 1
		? cb_ambig(root, pack) : pack;
	for (auto& nodes : choosen_pack)
		for (auto& chd : nodes)
			if (done.find(chd) == done.end() || cb_revisit(chd))
				ret &= _traverse(g, chd, done, cb_enter,
					cb_exit, cb_revisit, cb_ambig);
#ifdef DEBUG_TRAVERSE
	std::cout << "exit: \t" << root.first.to_std_string() << "\n";
#endif
	cb_exit(root, choosen_pack);
	return ret;
}

template <typename NodeT>
bool forest<NodeT>::replace_nodes(graph& g, nodes& s) {
	bool changed = false;
	for (auto& n : s) {
		//DBG(assert(g[n].size() == 1);)
		if (g.find(n) != g.end() &&
				replace_node(g, n, *(g[n].begin())))
			changed = true, g.erase(n);
	}
	return changed;
}

template <typename NodeT>
bool forest<NodeT>::replace_node(graph& g, const node& torepl,
	const nodes& repl)
{
	bool gchange = false;
	//return false if torepl is in repl to avoid infinite loop
	for (auto& n : repl) if (n == torepl) return false;
	for (auto& kv : g)
		for (auto rhs_it = kv.second.begin();
				rhs_it != kv.second.end(); rhs_it++)
		{
			//std::cout << "\n comparing" <<torepl.first << "with ";
			auto newrhs = *rhs_it;
			bool lchange = false;
			//keep replacing torepl's any occurence in the newrhs
			for (bool change = true; change; ) {
				size_t rpos = 0; change = false;
				for ( ; rpos < newrhs.size(); rpos++) {
					//std::cout<< newrhs.at(rpos).first <<std::endl;
					if (newrhs.at(rpos) == torepl) {
						// std::cout<<"making change" << std::endl;
						//erase the current torepl from rpos position
						auto inspos = newrhs.erase(
							newrhs.begin() + rpos);
						//do replacement at its new position
						newrhs.insert(inspos,
							repl.begin(),
							repl.end());
						/*
						for (auto& v : newrhs)
							std::cout << v.first ;
						std::cout << std::endl;
						std::cout<<"done making change\n";
						*/
						lchange = change = true; break;
					}
				}
			}
			if (lchange) {
				//std::cout<<"making change2" << std::endl;
				rhs_it = kv.second.erase(rhs_it);
				rhs_it = kv.second.insert(rhs_it, newrhs);
				gchange = true;
				/*std::cout<<"done making change2" << std::endl;
				for (auto& v : *rhs_it)
					std::cout << v.first ;
				std::cout << std::endl;
				*/
			}
		}
	return gchange;
}

#ifdef DEBUG
template <typename NodeT>
std::ostream& forest<NodeT>::print_data(std::ostream& os) const {
	os << "number of nodes: " << g.size() << std::endl;
	for (const auto& n : g) {
		os << n.first->first.to_std_string() << "\n";
		for (const auto& p : n.second) {
			os << "\t";
			for (const auto& c : p)
				os << " `" << c->first.to_std_string() << "`";
			os << "\n";
		}
	}
	return os;
}
#endif

} // idni namespace
#endif // __IDNI__PARSER__FOREST_TMPL_H__
