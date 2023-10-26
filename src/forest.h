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
		std::ostream& to_print(std::ostream& os, size_t l = 0) {
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
	struct graph : public node_graph {
		node root;
		/// nodes that lead to cycle
		std::set<node> cycles;
		/// builds and returns a tree with nodes
		sptree extract_trees();
	private:
		sptree _extract_trees(node& r, int_t choice = 0);
	};
	std::set<node> cycles;
	template<typename TraversableT>
	bool detect_cycle(TraversableT& g) const;

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
	bool is_binarized() const;
	/// removes all __temp symbols from the graph everywhere
	/// by replacing them with their immediate children nodes
	bool remove_binarization(graph&);
	/// removes all __R symbols from the graph everywhere
	/// by replacing them with their immediate children nodes
	bool remove_recursive_nodes(graph&);

	std::set<std::pair<node, nodes_set>> ambiguous_nodes() const;
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
		no_enter   = [](const node&) {};
		no_exit    = [](const node&, const nodes_set&) {};
		no_revisit = [](const node&) { return true; };
		do_revisit = [](const node&) { return false; };
		no_ambig   = [](const node&, const nodes_set& ns) { return ns;};
	}
	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool traverse(const node& root, cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const;

	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool traverse(const node_graph& gr, const node& root,
		cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const;
	// traverse() with default parameters
	template <typename cb_enter_t>
	bool traverse(const node& root, cb_enter_t cb_enter) const {
		return traverse(root, cb_enter, no_exit, no_revisit, no_ambig);
	}
	template <typename cb_enter_t, typename cb_exit_t>
	bool traverse(const node& root, cb_enter_t cb_enter, cb_exit_t cb_exit)
		const
	{
		return traverse(root, cb_enter, cb_exit, no_revisit, no_ambig);
	}
	template <typename cb_enter_t, typename cb_exit_t,typename cb_revisit_t>
	bool traverse(const node& root, cb_enter_t cb_enter, cb_exit_t cb_exit,
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
	/// replace each node with its immediate children,
	/// assuming its only one pack (unambigous)
	/// the caller to ensure the right order to avoid cyclic
	/// dependency if any. deletes from graph g as well.
	/// return true if any one of the nodes' replacement
	/// succeeds
	bool replace_nodes(graph& g, std::vector<NodeT>& s);
	/// replaces node 'torep' in one pass with the given nodes
	/// 'replacement' everywhere in the forest and returns true
	/// if changed. Does not care if its recursive or cyclic, its
	/// caller's responsibility to ensure
	bool replace_node(graph& g, const node& torep,const nodes& replacement);
};

} // idni namespace
#include "forest.tmpl.h"  // template definitions for forest
#endif // __IDNI__PARSER__FOREST_H__
