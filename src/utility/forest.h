// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#ifndef __IDNI__PARSER__FOREST_H__
#define __IDNI__PARSER__FOREST_H__
#include <array>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <ranges>
#include <deque>
#include <sstream>
#include "../defs.h"
#include "tree.h"

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

/**
 * @brief A structure which can contain multiple trees.
 * Nodes can have multiple sets of children where more than one set represents
 * splitting of trees meaning the part from root to the node is shared among
 * all trees splitted.
 */
template <typename NodeT>
struct forest {

private:
	/// Node pointer in forest
	struct nptr_t {
		friend NodeT;
	private:
		/// Points to pnode	
		const NodeT *id;
		size_t hash; 
		/**
		 * Maintains a refcount for # of id pointers to NodeT obj in NodeT::nid
		 * map both externally and from inside nid
		 */
		static size_t nc;
		
	public:
		nptr_t(const NodeT *_id = nullptr) : id(_id) { //if(id)
			hash = 0;
			if(id) hash = id->hashit();
			nc++; }
		nptr_t(const nptr_t& rhs) {  id = rhs.id; //if(id)
			hash = rhs.hash;
			nc++; }
		nptr_t(const nptr_t&& rhs) { id = rhs.id; //if(id)
			hash = rhs.hash;
			nc++; }
		// did not define conversion constructor.
		// as we use NodeT::nptr_t aka NodeT::node() operator
		inline operator NodeT() const {
			DBG(assert(id != nullptr);)
			return *id;
		}
		inline const NodeT* operator->() const { return id; }
		inline nptr_t& operator=(const nptr_t& rhs) {
			//if(  !id && rhs.id)
			//if(&rhs != this) nc++;
			//else if( id && !rhs.id) nc--;
			if (&rhs != this) id = rhs.id, hash = rhs.hash;
			return *this;
		}
		inline nptr_t& operator=(const nptr_t&& rhs) {
			//if( //!id && 	rhs.id)
			//if(&rhs != this) nc++;
			//else if( id && !rhs.id) nc--;
			if (&rhs != this) id = rhs.id, hash = rhs.hash;
			return *this;
		}
		inline bool operator<(const nptr_t& rhs) const {
			return hash < rhs.hash;
			//return id < rhs.id;
		}
		inline bool operator==(const nptr_t& rhs) const {
			if (hash == rhs.hash) {
				DBG(assert(id == rhs.id));
				return true;
			}
			return false;
		}
		~nptr_t() {
			//DBG(std::cout <<"-"<< NodeT::nid().size() <<" "<<nc );
			//if(id){
				if (nc == (NodeT::nid().size() + 1)) {
					//DBG(std::cout<<"GCing nodes:  "<< nc-1 <<std::endl);
					nc--;
					NodeT::nid().clear();
					//DBG(std::cout <<"-D"<< NodeT::nid().size() <<" "<<nc) ;
				}
				else if (nc > 0) nc--;
				id = 0; //dont delete as nid.clear takes responsibility
			//}
		}
		//inline lit<C,T> &first() const { DBG(assert(id!=0)); return id->first; }
		//inline std::array<size_t, 2>& second() const { DBG(assert(id!=0)); return id->second; }
	};
public:
	using node       = nptr_t;
	using nodes      = std::vector<node>;
	using nodes_set  = std::set<nodes>;
	using node_graph = std::map<node, nodes_set>;
	using edge       = std::pair<size_t, size_t>;
	
	/// A tree extracted from forst<NodeT>::graph
	struct tree {
		/// The original node extracted from a graph (forest).
		node value;
		/// A vector of tree's children - shared pointers to subtrees.
		std::vector<std::shared_ptr<struct tree>> child;
		/// Prints the tree recursively to a stream os. l is used for indentation level.
		std::ostream& to_print(std::ostream& os, size_t l = 0,
			std::set<size_t> skip = {}, bool nulls = false) const;
	};
	using sptree = std::shared_ptr<tree>;
	
	/**
	 * A least maximal core graph without ambiguity/repeating nodes/edges
	 * possibly with cycles or shared nodes. No cycles and no sharing implies its a tree.
	 */
	struct graph : public node_graph {
		static const forest forest_inst(){ 
			static forest sf;
			return sf;}
		/// Graph's root node.
		node root;
		/// Nodes that lead to cycle.
		std::set<node> cycles;
		/// Builds and returns a tree with nodes.
		sptree extract_trees();
		htree::sp extract_tree2();
	private:
		sptree _extract_trees(node& r, int_t choice = 0);
		htree::sp _extract_tree2(node& r);
	};
	//vector of graph with callback
	using graphv = std::vector<graph>;
	using cb_next_graph_t = std::function<bool(graph&)>;

	node_graph g;
	node rt;
	std::set<node> cycles;

	/// Returns the root node of a forest.
	node root() const;
	/// Sets the root node of a forest.
	void root(const node& n);
	/// Clears the forest by removing all its nodes.
	void clear();
	/// Returns true if the forest contains a node n.
	bool contains(const node& n) const;
	/// For a given node p sets the new set of subforests with children nodes and return it back.
	nodes_set& operator[](const node& p);
	/// For a given node p returns set of subforests with children nodes.
	const nodes_set& operator[](const node& p) const;
	/// Counts and returns a number of trees under a p node.
	size_t count_trees() const;
	/// Counts and returns a number of trees in a whole forest.
	size_t count_trees(const node& root) const;
	std::pair<size_t, size_t> count_useful_nodes(const node& root) const;

	/// Returns true if the forest is binarized, ie. each node has up to 2 children.
	bool is_binarized() const;
	/// Returns true if there exist a cycle in a g.
	template<typename TraversableT>
	bool detect_cycle(TraversableT& g) const;
	/**
	 * @brief Extracts a graph from the forest.
	 * 
	 * Extracts graphs from the forest starting at a root node. For every
	 * extracted graph cb_next_graph callback is called. 
	 * 
	 * If unique_edge is set to true it ensures that edges in resulting graphs
	 * are unique.
	 */
	graphv extract_graphs(const node& root, cb_next_graph_t cb_next_graph,
		bool unique_edge = true) const;

	using enter_t  =std::function<void(const node&)>;
	using exit_t   =std::function<void(const node&, const nodes_set&)>;
	using revisit_t=std::function<bool(const node&)>;
	using ambig_t  =std::function<nodes_set(const node&, const nodes_set&)>;

	/// Traverse method utilizes a visitor pattern.
	template <typename cb_enter_t, typename cb_exit_t = exit_t,
		typename cb_revisit_t = revisit_t, typename cb_ambig_t =ambig_t>
	bool traverse(const node& root, cb_enter_t cb_enter,
		cb_exit_t cb_exit = NO_EXIT,
		cb_revisit_t cb_revisit = NO_REVISIT,
		cb_ambig_t cb_ambig = NO_AMBIG) const;
	/// Traverse method utilizes a visitor pattern.
	template <typename cb_enter_t, typename cb_exit_t = exit_t,
		typename cb_revisit_t = revisit_t, typename cb_ambig_t =ambig_t>
	bool traverse(const node_graph& gr, const node& root,
		cb_enter_t cb_enter, cb_exit_t cb_exit = NO_EXIT,
		cb_revisit_t cb_revisit = NO_REVISIT,
		cb_ambig_t cb_ambig = NO_AMBIG, bool post_ord = false) const;
	/// Traverse method utilizes a visitor pattern.
	template <typename cb_enter_t, typename cb_exit_t = exit_t,
		typename cb_revisit_t = revisit_t, typename cb_ambig_t =ambig_t>
	bool traverse(cb_enter_t cb_enter,
		cb_exit_t cb_exit = NO_EXIT,
		cb_revisit_t cb_revisit = NO_REVISIT,
		cb_ambig_t cb_ambig = NO_AMBIG) const;
	/// Replace each node with its immediate children,
	/// assuming its only one pack (unambigous)
	/// the caller to ensure the right order to avoid cyclic
	/// dependency if any. deletes from graph g as well.
	/// return true if any one of the nodes' replacement
	/// succeeds
	bool replace_nodes(graph& g, nodes& s);
	/// Replaces node 'torep' in one pass with the given nodes
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
		cb_revisit_t cb_revisit, cb_ambig_t cb_ambig, bool post_ord = false) const;
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
