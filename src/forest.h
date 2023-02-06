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
#ifndef __IDNI__PARSER__FOREST_H__
#define __IDNI__PARSER__FOREST_H__
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <deque>
#include "defs.h"
namespace idni {

template <typename NodeT>
struct forest {
	typedef NodeT node;
	typedef std::vector<node> nodes;
	typedef std::set<nodes> nodes_set;
	typedef std::map<node, nodes_set> node_graph;
	typedef std::pair<node, node> node_edge;
	typedef std::pair<size_t, size_t> edge;
	typedef std::vector<edge> edges;
	typedef std::pair<nodes, edges> nodes_and_edges;
	
	// shared packed parse forest (SPPF) that captures ambiguities while
	// reusing shared sub structure within the forest
	node_graph g;
	// parse tree extracted from graphs of the forest
	struct tree {
		node value;
		std::vector<std::shared_ptr<struct tree>> child;
		std::ostream& to_print(std::ostream &os, size_t l = 0) {
			os << "\n";
			for (size_t t = 0; t < l; t++) os << " ";
			os << value.first;
			for (auto& d : child) d->to_print(os, l + 1);
			return os;
		}
	};
	typedef std::shared_ptr<tree> sptree;

	// a least maximal core graph without ambiguity/repeating nodes/edges
	// possibly with cycles or shared nodes. 
	// no cycles and no sharing implies its a tree
	struct graph: public node_graph{
		node root;
		// nodes that lead to cycle
		std::set<node> cycles;
		sptree extract_trees();
		private:
		sptree _extract_trees(node& r, int_t choice = 0);	
	};

	bool detect_cycle(graph& g) const;

	//vector of graph with callback
	typedef std::vector<graph> graphv;
	typedef std::function<bool(graph&)> cb_next_graph_t;

	node rt;
	node root() const { return rt; }
	void root(const node& n) { rt = n; }
	void clear() { g.clear(); }
	bool contains(const node& n) { return g.find(n) != g.end(); }
	nodes_set& operator[](const node& p)             { return g[p]; }
	const nodes_set& operator[](const node& p) const { return g[p]; }	
	bool is_ambiguous() const;
	bool has_single_parse_tree() const { return !is_ambiguous(); };
	size_t count_trees(const node& root) const;
	size_t count_trees() const { return count_trees(root()); };
	nodes_and_edges get_nodes_and_edges() const;
	graphv extract_graphs(const node& root, cb_next_graph_t cb_next_graph,
		bool unique_edge = true) const;
	std::function<void(const node&)> no_enter;
	std::function<void(const node&, const nodes_set&)> no_exit;
	std::function<bool(const node&)> no_revisit;
	std::function<bool(const node&)> do_revisit;
	std::function<nodes_set(const node&, const nodes_set&)> no_ambig;
	forest() {
		no_enter   = [](const node&) { };
		no_exit    = [](const node&, const nodes_set&) { };
		no_revisit = [](const node&) { return true; };
		do_revisit = [](const node&) { return false; };
		no_ambig   = [](const node&, const nodes_set& ns) { return ns;};
	}
	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool traverse(const node &root, cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const;
	
	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool traverse(const node_graph& gr, const node &root,
		cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const;

	// traverse() with default parameters
	template <typename cb_enter_t>
	bool traverse(const node &root, cb_enter_t cb_enter) const {
		return traverse(root, cb_enter, no_exit, no_revisit, no_ambig);
	}
	template <typename cb_enter_t, typename cb_exit_t>
	bool traverse(const node &root, cb_enter_t cb_enter, cb_exit_t cb_exit)
		const
	{
		return traverse(root, cb_enter, cb_exit, no_revisit, no_ambig);
	}
	template <typename cb_enter_t, typename cb_exit_t,typename cb_revisit_t>
	bool traverse(const node &root, cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit) const
	{
		return traverse(root, cb_enter, cb_exit, cb_revisit, no_ambig);
	}
	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool traverse(cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const
	{
		return traverse(root(), cb_enter, cb_exit, cb_revisit,cb_ambig);
	}
	template <typename cb_enter_t>
	bool traverse(cb_enter_t cb_enter) const {
		return traverse(cb_enter, no_exit, no_revisit, no_ambig);
	}
	template <typename cb_enter_t, typename cb_exit_t>
	bool traverse(cb_enter_t cb_enter, cb_exit_t cb_exit)
		const
	{
		return traverse(cb_enter, cb_exit, no_revisit, no_ambig);
	}
	template <typename cb_enter_t, typename cb_exit_t,typename cb_revisit_t>
	bool traverse(cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit) const
	{
		return traverse(cb_enter, cb_exit, cb_revisit, no_ambig);
	}
#ifdef DEBUG
	std::ostream& print_data(std::ostream& os) const;
#endif
private:
	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool _traverse(const node_graph& g, const node& root,
		std::set<node>& done, cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const;
	bool _extract_graph_uniq_edge(std::map<node,size_t>& ndmap,
		std::set<edge>& done, std::vector<node>& todo, graphv& graphs,
		size_t gid, cb_next_graph_t g, bool& no_stop) const; 
	bool _extract_graph_uniq_node(std::set<node>& done,
		std::vector<node>& todo, graphv& graphs, size_t gid,
		cb_next_graph_t g, bool& no_stop) const;
};

#ifdef DEBUG
template <typename NodeT>
std::ostream& forest<NodeT>::print_data(std::ostream& os) const {
	os << "number of nodes: " << g.size() << std::endl;
	for (const auto& n : g) {
		os << n.first.first.to_std_string() << "\n";
		for (const auto& p : n.second) {
			os << "\t";
			for (const auto &c : p)
				os << " `" << c.first.to_std_string() << "`";
			os << "\n";
		}
	}
	return os;
}
#endif

// a dfs based approach to detect cycles
template<typename NodeT>
bool forest<NodeT>::detect_cycle(graph &gr) const {
	std::map<NodeT, bool> inprog;
	auto cb_enter = [&inprog, &gr](const auto& n) { 
		if (n.first.nt()) 
			inprog[n] = true;
	};
	auto cb_revisit = [&inprog, &gr](const auto& n) { 
		if (inprog[n] == true) 
			gr.cycles.insert(n);
		return false; 
	}; // revisit
	auto cb_exit = [&inprog](const auto& n, auto&) {
		if (n.first.nt()) inprog[n] = false;
	};
	auto cb_child_all = [](const auto&, auto& ambset) {
		return ambset;
	};

	return traverse(gr,gr.root,cb_enter, cb_exit, cb_revisit, cb_child_all);
}


template<typename NodeT>
typename forest<NodeT>::sptree
forest<NodeT>::graph::extract_trees(){

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
	while( !stk.empty() ) {
		sptree cur = stk.back();
		stk.pop_back();
		// skip for terminal
		if (!cur->value.first.nt()) continue;
		auto fit = this->find(cur->value);
		if (fit == this->end()) continue;
		auto &ns = fit->second;
		if (!ns.size()) continue;
		auto it = ns.begin();
		if (ns.size() > 1) {
			// select which descendants to traverse not already done;
			if (edgcount[cur->value] == ns.size()) continue;
			for (size_t i = 0; i < edgcount[cur->value]; i++, ++it);

			edgcount[cur->value]++;
		}
		for (auto& cnode : *(it))
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
		const auto &it = g.find(root);
		if (it == g.end()) continue;
		if (!done.insert(root).second) continue;

		const nodes_set &packs = it->second;
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

		const nodes_set &packs = it->second;
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
				size_t ngid = graphs.size()-1;
				for (auto& node : pack)
					if (node.first.nt() && ndone.insert({
						rootid+ambpid+1, ndmap[node]})
						.second )
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
			if (!done.count(chd) || cb_revisit(chd)) 
				ret &= _traverse(g, chd, done, cb_enter,
					cb_exit, cb_revisit, cb_ambig);
#ifdef DEBUG_TRAVERSE
	std::cout << "exit: \t" << root.first.to_std_string() << "\n";
#endif
	cb_exit(root, choosen_pack);
	return ret;
}

template <typename NodeT>
bool forest<NodeT>::is_ambiguous() const {
	for( auto &kv: this->g )
		if( kv.second.size() > 1 ) return true;
	return false;
}

template <typename NodeT>
size_t forest<NodeT>::count_trees(const node& root) const {
	std::map<node, size_t> ndc;
	auto cb_exit = [&ndc](const node& croot, auto& ambset) {
		for( auto &pack : ambset) {
			size_t pkc = 1; // count of the pack
			for( auto &sym : pack) 
				if(sym.first.nt()) pkc *= ndc[sym];
			ndc[croot] += pkc; // adding to curroot count
		}
	};
	auto cb_no_revisit = [](const node&) { return false; };
	traverse(root, no_enter, cb_exit, cb_no_revisit, no_ambig);
	return ndc[root]; 
}

} // idni namespace
#endif // __IDNI__PARSER__FOREST_H__
