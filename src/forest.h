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
#include <cassert>
#include <deque>
namespace idni {

template <typename NodeT>
struct forest {
	typedef NodeT node;
	typedef std::vector<node> nodes;
	typedef std::set<nodes> nodes_set;
	typedef std::map<node, nodes_set> node_graph;
	typedef std::pair<size_t, size_t> edge;
	typedef std::vector<edge> edges;
	typedef std::pair<nodes, edges> nodes_and_edges;
	node_graph g;
	typedef std::vector<node_graph> node_graphs;
	
	node rt;
	node root() const { return rt; }
	void root(const node& n) { rt = n; }
	void clear() { g.clear(); }
	bool contains(const node& n) { return g.find(n) != g.end(); }
	nodes_set& operator[](const node& p)             { return g[p]; }
	const nodes_set& operator[](const node& p) const { return g[p]; }	
	size_t count_trees(const node& root) const;
	size_t count_trees() const { return count_trees(root()); };
	nodes_and_edges get_nodes_and_edges() const;
	node_graphs extract_graphs( const node& root, bool unique_edge = false ) const;
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
	ostream_t& print_data(ostream_t& os) const;
#endif
private:
	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool _traverse(const node &root, std::set<node> &done,
		cb_enter_t cb_enter, cb_exit_t cb_exit,
		cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const;
	bool _extract_graph_uniq_edge(std::map<node,size_t> &ndmap, std::set<edge>& done, 
	std::deque<node>& todo, node_graphs &graphs,  size_t tr = 0) const;
	bool _extract_graph_uniq_node(std::set<node>& done, std::deque<node>& todo, 
		node_graphs &graphs, size_t tr = 0) const;
};

#ifdef DEBUG
template <typename NodeT>
ostream_t& forest<NodeT>::print_data(ostream_t& os) const {
	os << "number of nodes: " << g.size() << endl;
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
bool forest<NodeT>::_extract_graph_uniq_node( std::set<node>& done, std::deque<node>& todo,
	node_graphs& graphs, size_t trid ) const{
	
	bool ret = true;
	while( todo.size()) {
		const node root = todo.back();
		todo.pop_back();

		if( !root.first.nt()) continue;
		const auto &it = g.find(root);
		if (it == g.end()) continue;
		if(!done.insert(root).second ) continue;
		
		const nodes_set &packs = it->second;
		auto curtree = graphs[trid];
		auto curdone = done;
		auto curtodo = todo;

		size_t ambpid = 0;
		for(auto& pack : packs) {
			if( ambpid == 0) {
				graphs[trid].insert( {root, {pack} });
				for( auto& node : pack) todo.push_back(node);
			}
			else {
				graphs.emplace_back(curtree);
				graphs.back().insert( {root, {pack} });
				size_t ntrid = graphs.size() - 1;

				auto ntodo(curtodo);
				auto ndone(curdone);
				for( auto& node : pack) ntodo.push_back(node);
			
				ret &= _extract_graph_uniq_node( ndone, ntodo, graphs, ntrid);

			}
			ambpid++;
		}

	}

	return ret;
}

template <typename NodeT>
bool forest<NodeT>::_extract_graph_uniq_edge(std::map<node, size_t> &ndmap, std::set<edge>& done, std::deque<node>& todo,
	node_graphs& graphs, size_t trid ) const {
	
	bool ret = true;
	while( todo.size()) {
		
		const node root = todo.back();
		todo.pop_back();

		if( !root.first.nt()) continue;
		const auto &it = g.find(root);
		if (it == g.end()) continue;
		
		const nodes_set &packs = it->second;
		node_graph curtree = graphs[trid];
		std::set<edge> curdone(done);
		std::deque<node> curtodo(todo);

		size_t ambpid = 0;
		size_t rootid = ndmap[root];
		
		for(auto& pack : packs) {
			if( ambpid == 0) {
				if(!done.insert( {rootid, rootid + ambpid + 1} ).second ) 
					goto _skip;

				if( graphs[trid].find(root) == graphs[trid].end() )
					graphs[trid].insert( {root, {pack} });
				else graphs[trid][root].insert(pack); 
				for( auto& node : pack) 
					if( node.first.nt() &&  
						done.insert({ rootid + ambpid + 1, ndmap[node] }).second )
							todo.push_back(node);
			}
			else {
				auto ntodo(curtodo);
				auto ndone(curdone);
				if(!ndone.insert( {rootid, rootid + ambpid + 1} ).second ) 
					goto _skip;
					
				graphs.emplace_back(curtree);
				if( graphs.back().find(root) == graphs.back().end() )
					graphs.back().insert( {root, {pack} });
				else graphs.back()[root].insert(pack); 

				size_t ntrid = graphs.size() - 1;
				
				for( auto& node : pack) 
					if( node.first.nt() &&  
						ndone.insert({ rootid + ambpid + 1, ndmap[node]}).second )
							ntodo.push_back(node);
				
				ret &= _extract_graph_uniq_edge(ndmap, ndone, ntodo, graphs, ntrid);

			}
			_skip: ambpid++;
		}
	}
	return ret;

}


template <typename NodeT>
typename forest<NodeT>::node_graphs forest<NodeT>::extract_graphs(
	const node& root, bool unique_edge ) const{
	
	node_graphs graphs;
	std::set<node> dn;
	std::set<edge> de;
	std::deque<node> todo;
	todo.push_back(root);
	graphs.emplace_back();
	std::map<node, size_t> ndmap;
	int id = 0;
	for (auto& it : g) {
		ndmap[it.first] = id++;
		id +=  it.second.size(); // ambig node ids;
	}
	if( unique_edge )
		_extract_graph_uniq_edge( ndmap, de, todo, graphs );
	else 
		_extract_graph_uniq_node( dn, todo, graphs );

	return graphs;
/*
	fpath path;
	node_graph tree;
	node_graphs graphs;

	auto cb_enter = [](const auto&) {};
	auto cb_revisit =  [](const auto&) { return false; }; // revisit
		// make tree node while exiting from the forest node
	auto cb_exit = [&tree](const node& root, const nodes_set& ambig_set) {
		DBG( assert(ambig_set.size() <= 1) );
		if (root.first.nt()) tree.insert({root, ambig_set});
	};
	//select one of the ambiguous nodes as per paths[id]
	auto cb_select_one = [&path ](const node&, auto& ambset) {
			std::set<std::vector<node>> selected;
			if(ambset.size() > path.front() ) return selected;
			
			auto it = std::next(ambset.begin(), path.front());
			if( path.size()) path.erase(path.begin());
			selected.insert(*it );
			return selected;
		};
	
	for(size_t i = 0 ; i< paths.size(); i++ ) {
		std::cout << std::endl << "#" << i << std::endl;
		path = paths[i];
		for( size_t tid: path ) std::cout<< tid << " ";
		std::cout << std::endl;
		traverse(root, cb_enter, cb_exit, cb_revisit, cb_select_one);
		graphs.emplace_back(tree);
		tree.clear();
	}
	return graphs;
	*/
}

template <typename NodeT>
template <typename cb_enter_t, typename cb_exit_t,
	typename cb_revisit_t, typename cb_ambig_t>
bool forest<NodeT>::traverse(const node &root, cb_enter_t cb_enter,
	cb_exit_t cb_exit, cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const
{
	std::set<node> done;
	return _traverse(root, done, cb_enter, cb_exit, cb_revisit, cb_ambig);
}

template <typename NodeT>
template <typename cb_enter_t, typename cb_exit_t,
	typename cb_revisit_t, typename cb_ambig_t>
bool forest<NodeT>::_traverse(const node &root, std::set<node> &done,
	cb_enter_t cb_enter, cb_exit_t cb_exit,
	cb_revisit_t cb_revisit, cb_ambig_t cb_ambig) const
{
//#define DEBUG_TRAVERSE
#ifdef DEBUG_TRAVERSE
	std::cout << "enter: \t\t" << root.first.to_std_string() << std::endl;
	std::cout << "\t" << (root.first.nt() ? "NT" : " T") << " ";
	if (root.first.nt()) std::cout << "n: " << root.first.n()
		<< " `" << root.first.to_std_string();
	else std::cout << "c: `" << to_std_string(root.first.c()) << "`";
	std::cout << std::endl;
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
				ret &= _traverse(chd, done, cb_enter, cb_exit,
					cb_revisit, cb_ambig);
#ifdef DEBUG_TRAVERSE
	std::cout << "exit: \t" << root.first.to_std_string() << std::endl;
#endif
	cb_exit(root, choosen_pack);
	return ret;
}

template <typename NodeT>
size_t forest<NodeT>::count_trees(const node& root) const {
	size_t count = 1;
	auto cb_keep_ambig = [&count](const node&, auto& ambset) {
		count *= ambset.size();
		return ambset;
	};
	traverse(root, no_enter, no_exit, do_revisit, cb_keep_ambig);
	return count; 
}

} // idni namespace
#endif // __IDNI__PARSER__FOREST_H__
