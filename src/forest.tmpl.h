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
#include "forest.h"

namespace idni {

#ifdef DEBUG
template <typename NodeT>
std::ostream& forest<NodeT>::print_data(std::ostream& os) const {
	os << "number of nodes: " << g.size() << std::endl;
	for (const auto& n : g) {
		os << n.first.first.to_std_string() << "\n";
		for (const auto& p : n.second) {
			os << "\t";
			for (const auto& c : p)
				os << " `" << c.first.to_std_string() << "`";
			os << "\n";
		}
	}
	return os;
}
#endif
template <typename NodeT>
size_t forest<NodeT>::fhasher_t::hash_size_t(const size_t &val) const{
	return std::hash<size_t>()(val) +
		0x9e3779b9 + (val << 6) + (val >> 2);
}
template <typename NodeT>
size_t forest<NodeT>::fhasher_t::operator()(const NodeT &k) const {
	// lets substitute with better if possible.
	size_t h = 0;
	h ^= hash_size_t(k.second[0]);
	h ^= hash_size_t(k.second[1]);
	h ^= hash_size_t(size_t(
		k.first.nt() ? k.first.n() : k.first.t()));
	return h;
}
// a dfs based approach to detect cycles for
// any traversable type
template<typename NodeT>
template<typename TraversableT>
bool forest<NodeT>::detect_cycle(TraversableT& gr) const {
	std::map<NodeT, bool> inprog;
	auto cb_enter = [&inprog, &gr](const auto& n) {
		if (n.first.nt()) inprog[n] = true;
	};
	auto cb_revisit = [&inprog, &gr](const auto& n) {
		if (inprog[n] == true) gr.cycles.insert(n);
		return false;
	};
	auto cb_exit = [&inprog](const auto& n, auto&) {
		if (n.first.nt()) inprog[n] = false;
	};
	gr.cycles.clear();
	return traverse(gr, gr.root, cb_enter, cb_exit, cb_revisit, no_ambig);
}

template<typename NodeT>
typename forest<NodeT>::sptree forest<NodeT>::graph::extract_trees() {
	return _extract_trees(root);
}

template <typename NodeT>
typename forest<NodeT>::sptree forest<NodeT>::graph::_extract_trees(
	NodeT& r, int_t)
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
		if (!cur->value.first.nt()) continue;
		auto fit = this->find(cur->value);
		if (fit == this->end()) continue;
		auto& ns = fit->second;
		if (!ns.size()) continue;
		auto it = ns.begin();
		if (ns.size() > 1) {
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
typename forest<NodeT>::nodes_and_edges forest<NodeT>::get_nodes_and_edges()
	const
{
	std::map<node, size_t> nid;
	std::map<size_t, node> ns;
	nodes n;
	edges es;
	size_t id = 0;
	for (auto& it : g) {
		nid[it.first] = id;
		// skip ids for one child ambig node
		id += it.second.size() == 1 ? 0 : it.second.size(); // ambig node ids;
		//DBG(assert(it.second.size()!= 0));
		id++;
	}
	for (auto& it : g) {
		ns[nid[it.first]] = it.first;
		size_t p = 0;
		for (auto& pack : it.second) {
			if (it.second.size() > 1) {  //skipping if only one ambigous node, an optimization
				++p;
				ns[nid[it.first] + p] = it.first;
				es.emplace_back(nid[it.first], nid[it.first]+p);
			}
			for (auto& nn : pack) {
				if (nid.find(nn) == nid.end()) nid[nn] = id++; // for terminals, not seen before
				ns[nid[nn]] = nn;
				es.emplace_back(nid[it.first] + p, nid[nn]);
			}
		}
	}
	n.resize(id);
	for (auto& p : ns) n[p.first] = p.second;
	return nodes_and_edges{ n, es };
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
		if (!root.first.nt()) continue;
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
	std::set<edge>& done, std::vector<node>& todo, graphv& graphs,
	size_t gid, cb_next_graph_t cb_next_graph, bool& no_stop) const
{
	bool ret = true;
	while (todo.size() && no_stop) {
		const node root = todo.back();
		todo.pop_back();
		if (!root.first.nt()) continue;
		const auto& it = g.find(root);
		if (it == g.end()) continue;
		const nodes_set& packs = it->second;
		graph curgraph = graphs[gid];
		std::set<edge> curdone(done);
		std::vector<node> curtodo(todo);
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
					if (node.first.nt() && done.insert({
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
					if (node.first.nt() && ndone.insert({
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
	std::vector<node> todo;
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
	if (root.first.nt()) {
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
bool forest<NodeT>::is_binarized() const {
	for (auto& kv: this->g) for (auto& rhs: kv.second)
		if (rhs.size() > 2) return false;
	return true;
}

template <typename NodeT>
bool forest<NodeT>::remove_recursive_nodes(graph& g) {
	//decltype(NodeT().first.t()) prefix []= { '_', 'R' };
	//collect all prefix like nodes for replacement
	std::string prefix = "_R";
	std::vector<NodeT> s;
	for (auto& kv : g) {
		auto name = kv.first.first.to_std_string();
		if (name.find(prefix) != decltype(name)::npos)
			s.insert(s.end(), kv.first);
	}
	return replace_nodes(g, s);
}

template <typename NodeT>
bool forest<NodeT>::remove_binarization(graph& g) {
	//better use parser::tnt_prefix()
	//decltype(NodeT().first.t()) prefix []= { '_','_','t','e','m','p' };
	//collect all prefix like nodes for replacement
	std::string prefix="__temp";
	std::vector<NodeT> s;
	for (auto& kv : g) {
		auto name = kv.first.first.to_std_string();
		if (name.find(prefix) != decltype(name)::npos)
			s.insert(s.end(), kv.first);
	}
	std::cout<<"removing binarization if any " << s.size();
	return replace_nodes(g, s);
}

template <typename NodeT>
bool forest<NodeT>::replace_nodes(graph& g, std::vector<NodeT>& s) {
	bool changed = false;
	for (auto& n : s) {
		DBG(assert(g[n].size() == 1);)
		if (replace_node(g, n, *(g[n].begin())))
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
						//std::cout<<"making change" << std::endl;
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

template <typename NodeT>
bool forest<NodeT>::is_ambiguous() const {
	for (auto& kv : this->g) if (kv.second.size() > 1) return true;
	return false;
}

template <typename NodeT>
std::set<std::pair<NodeT, std::set<std::vector<NodeT>>>>
	forest<NodeT>::ambiguous_nodes() const
{
	std::set<std::pair<node, nodes_set>> r;
	for (auto& kv : this->g) if (kv.second.size() > 1) r.insert(kv);
	return r;
}

template <typename NodeT>
size_t forest<NodeT>::count_trees(const node& root) const {
	std::map<node, size_t> ndc;
	bool isoverflow = false;
	auto cb_exit = [&ndc, &isoverflow](const node& croot, auto& ambset) {
		for (auto& pack : ambset) {
			size_t pkc = 1; // count of the pack
			for (auto& sym : pack)
				if (sym.first.nt() && ndc[sym] != 0) {
					size_t x = pkc * ndc[sym];
					if( pkc != 0 && x / pkc != ndc[sym]  ) {
						MS(std::cout<<"Overflow\n");
						isoverflow = true;
						return; 
					}
					pkc = x;
				}
			ndc[croot] += pkc; // adding to curroot count
		}
	};
	auto cb_no_revisit = [](const node&) { return false; };
	traverse(root, no_enter, cb_exit, cb_no_revisit, no_ambig);
	if(isoverflow) ndc[root] = 0; // mark it to be zero
	return ndc[root];
}

} // idni namespace
#endif // __IDNI__PARSER__FOREST_H__
