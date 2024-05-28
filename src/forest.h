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
#ifndef __IDNI__PARSER__FOREST_H__
#define __IDNI__PARSER__FOREST_H__
#include <array>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <deque>
#include <sstream>
#include "defs.h"

#ifdef DEBUG
#include <cassert>
#endif

#define NO_ENTER   [](const auto&) {}
#define NO_EXIT    [](const auto&, const auto&) {}
#define NO_REVISIT [](const auto&) { return false; }
#define DO_REVISIT [](const auto&) { return true; }
#define NO_AMBIG   [](const auto&, const auto& ns) { return ns; }

namespace idni {

// shared packed parse forest (SPPF) that captures ambiguities while
// reusing shared sub structure within the forest
// parse tree extracted from graphs of the forest
// a least maximal core graph without ambiguity/repeating nodes/edges
// possibly with cycles or shared nodes.
// no cycles and no sharing implies its a tree

template <typename NodeT>
struct forest { 
	
	// node pointer in forerst
	
	private:
	struct nptr_t {
		friend NodeT;
		private:
		const NodeT *id; // points to pnode
		static size_t nc; // maintains a refcount for # of id pointers
		// to NodeT obj in NodeT::nid map both externally and from inside nid
		public:	
		nptr_t(const NodeT *_id = nullptr) : id(_id) {  //if(id) 	
			nc++;}
		nptr_t(const nptr_t& rhs) {  id = rhs.id; //if(id)
			nc++;}
		nptr_t(const nptr_t&& rhs) {  id = rhs.id; //if(id) 
			nc++;}
		// did not define conversion constructor.
		// as we use NodeT::nptr_t aka NodeT::node() operator 
		inline operator NodeT() const{
			return *id;
		}
		inline const NodeT* operator->() const { return id;}
		inline nptr_t& operator=(const nptr_t& rhs) {
			//if(  !id && rhs.id) 
			//if(&rhs != this) nc++;
			//else if( id && !rhs.id) nc--;
			if(&rhs != this) id = rhs.id;
			return *this;
		}
		inline nptr_t& operator=(const nptr_t&& rhs) {	 
			//if( //!id && 	rhs.id) 
			//if(&rhs != this) nc++;
			//else if( id && !rhs.id) nc--;
			 if(&rhs != this) id = rhs.id;
			 return *this;
		}
		inline bool operator<(const nptr_t& rhs) const {
			return id < rhs.id;
		}
		inline bool operator == (const nptr_t& rhs) const {
			return id == rhs.id;
		}
		inline auto hash() const{
			// go by the pointer value as the nptr_t is already
			// ensuring there is a unique ptr per pnode
			// ...deep hash could be to do based on NodeT::hash
			// ...then different id could be treated same
			return size_t(id);
		}
		~nptr_t() { 
			DBG(assert(nc != 0)); 
			//if(id){	
				if((nc == (NodeT::nid.size() + 1)) ){
					DBG(std::cout<<"GCing nodes:  "<< nc-1 <<std::endl);
					nc--;
					NodeT::nid.clear();
					DBG(std::cout <<"-D"<< NodeT::nid.size() <<" "<<nc) ;
				}
				else if (nc > 0) nc--;
				id = 0; //dont delete as nid.clear takes responsibility
			//}
		}
		//inline lit<C,T> &first() const { DBG(assert(id!=0)); return id->first; }
		//inline std::array<size_t, 2>& second() const { DBG(assert(id!=0)); return id->second; } 	
	};
	struct nhash {
		size_t operator()(const nptr_t &n) const{
			return (size_t)n.hash();
		}
	};
	public:
	using node       = nptr_t;
	using nodes      = std::vector<node>;
	using nodes_set  = std::set<nodes>;
	using node_graph = std::unordered_map<node, nodes_set, nhash>;
	using edge       = std::pair<size_t, size_t>;
	struct tree {
		node value;
		std::vector<std::shared_ptr<struct tree>> child;
		std::ostream& to_print(std::ostream& os, size_t l = 0,
			std::set<size_t> skip = {}, bool nulls = false) const;
	};
	using sptree = std::shared_ptr<tree>;
	struct graph : public node_graph {
		node root;
		/// nodes that lead to cycle
		std::set<node> cycles;
		/// builds and returns a tree with nodes
		sptree extract_trees();
	private:
		sptree _extract_trees(node& r, int_t choice = 0);
	};
	//vector of graph with callback
	using graphv = std::vector<graph>;
	using cb_next_graph_t = std::function<bool(graph&)>;

	node_graph g;
	node rt;
	std::set<node> cycles;

	node root() const;
	void root(const node& n);
	void clear();
	bool contains(const node& n) const;
	nodes_set& operator[](const node& p);
	const nodes_set& operator[](const node& p) const;
	size_t count_trees() const;
	size_t count_trees(const node& root) const;

	bool is_binarized() const;
	template<typename TraversableT>
	bool detect_cycle(TraversableT& g) const;
	graphv extract_graphs(const node& root, cb_next_graph_t cb_next_graph,
		bool unique_edge = true) const;

	using enter_t  =std::function<void(const node&)>;
	using exit_t   =std::function<void(const node&, const nodes_set&)>;
	using revisit_t=std::function<bool(const node&)>;
	using ambig_t  =std::function<nodes_set(const node&, const nodes_set&)>;

	template <typename cb_enter_t, typename cb_exit_t = exit_t,
		typename cb_revisit_t = revisit_t, typename cb_ambig_t =ambig_t>
	bool traverse(const node& root, cb_enter_t cb_enter,
		cb_exit_t cb_exit = NO_EXIT,
		cb_revisit_t cb_revisit = NO_REVISIT,
		cb_ambig_t cb_ambig = NO_AMBIG) const;
	template <typename cb_enter_t, typename cb_exit_t = exit_t,
		typename cb_revisit_t = revisit_t, typename cb_ambig_t =ambig_t>
	bool traverse(const node_graph& gr, const node& root,
		cb_enter_t cb_enter, cb_exit_t cb_exit = NO_EXIT,
		cb_revisit_t cb_revisit = NO_REVISIT,
		cb_ambig_t cb_ambig = NO_AMBIG) const;
	template <typename cb_enter_t, typename cb_exit_t = exit_t,
		typename cb_revisit_t = revisit_t, typename cb_ambig_t =ambig_t>
	bool traverse(cb_enter_t cb_enter,
		cb_exit_t cb_exit = NO_EXIT,
		cb_revisit_t cb_revisit = NO_REVISIT,
		cb_ambig_t cb_ambig = NO_AMBIG) const;
	/// replace each node with its immediate children,
	/// assuming its only one pack (unambigous)
	/// the caller to ensure the right order to avoid cyclic
	/// dependency if any. deletes from graph g as well.
	/// return true if any one of the nodes' replacement
	/// succeeds
	bool replace_nodes(graph& g, nodes& s);
	/// replaces node 'torep' in one pass with the given nodes
	/// 'replacement' everywhere in the forest and returns true
	/// if changed. Does not care if its recursive or cyclic, its
	/// caller's responsibility to ensure
	bool replace_node(graph& g, const node& torep,const nodes& replacement);

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

template<typename NodeT>
size_t forest<NodeT>::nptr_t::nc = 0;

} // idni namespace
#include "forest.tmpl.h"  // template definitions for forest
#endif // __IDNI__PARSER__FOREST_H__
