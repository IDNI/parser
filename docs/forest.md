[back to index](../README.md#classes-and-structs)

# forest

```
template <typename NodeT> struct forest;
```

`forest` is a structure which can contain multiple trees. Nodes can have multiple sets of children where more than one set represents splitting of trees meaning the part from root to the node is shared among all trees splitted.

It offers methods counting trees, traversal or tree or graph extractions.

`forest` is a generic structure since the type of nodes `NodeT` is templated.

### parse forest

This library uses `forest` to represent all parse trees parsed from a given input. Each `parser<C, T>::parse(...)` call produces a `parser<C, T>::result` which contins a parsed forest (accessible with `get_forest()`). Parse nodes uses type (`NodeT`) `std::pair<lit<C, T>, std::array<size_t, 2>>` where `lit<C, T>` is a parsed literal and the array contains position span of a literal in an input string.

`parser` provides type [`pnode`](parser_pnode.md) for used as `NodeT` templated type and type `pforest` for a parse forest:

```
class parser {
	// ...
public:
	using pforest forest<pnode>;
	// ...
}
```

## constructor

```
forest();
```
There are no arguments. Forest is always instantiated as an empty one.

```
forest<int> forest_of_ints;
forest<std::pair<std::string, size_t>> forest_of_pairs_of_string_and_size;
forest<token> forest_of_tokens;
```


## methods

### NodeT root() const;

Returns the root node of a forest.

### void root(const NodeT& n);

Sets the root node of a forest.

### void clear();

Clears the forest by removing all its nodes.

### bool contains(const NodeT& n);

Returns true if the forest contains a node `n`.

### std::set<std::vector<NodeT>>& operator[](const NodeT& p);

For a given node `p` sets the new set of subforests with children nodes and return it back.


### const std::set<std::vector<NodeT>>& operator[](const NodeT& p) const;

For a given node `p` returns set of subforests with children nodes.


### size_t count_trees(const NodeT& p) const;

Counts and returns a number of trees under a `p` node.


### size_t count_trees() const;

Counts and returns a number of trees in a whole forest.


### bool is_binarized() const;

Returns true if the forest is binarized, ie. each node has up to 2 children.


### template<typename TraversableT> bool detect_cycle(TraversableT& g) const;

Returns true if there exist a cycle in a `g`.

```
// having a forest f and a graph fg
if (f->detect_cycle(fg)) cout << "cycle detected\n";
else cout << "no cycles\n";
```


### std::vector<graph> extract_graphs(const NodeT& root, cb_next_graph_t cb_next_graph, bool unique_edge = true) const;

Extracts graphs from the forest starting at a `root` node. For every extracted graph `cb_next_graph` callback is called.

If `unique_edge` is set to `true` it ensures that edges in resulting graphs are unique.

```
auto next_g = [](parser<C, T>::pforest::graph& fg) {
	// ... do anything with the graph fg ...
	return true; // return true to continue in extracting next graph
};
f->extract_graphs(f->root(), next_g);
```


### bool traverse(...)

Traverse method utilizes a visitor pattern. It takes callbacks for events happening during a traversal. These callbacks are templatable.

```
template <typename cb_enter_t, typename cb_exit_t, typename cb_revisit_t, typename cb_ambig_t>
bool traverse(const NodeT& root,
	cb_enter_t cb_enter,
	cb_exit_t cb_exit,
	cb_revisit_t cb_revisit,
	cb_ambig_t cb_ambig) const;
```
Traverses from a `root` node.

```
template <typename cb_enter_t, typename cb_exit_t, typename cb_revisit_t, typename cb_ambig_t>
bool traverse(const node_graph& g, const NodeT& root,
	cb_enter_t cb_enter,
	cb_exit_t cb_exit,
	cb_revisit_t cb_revisit,
	cb_ambig_t cb_ambig) const;
```
Traverses a graph `gr` from a `root` node.

```
template <typename cb_enter_t, typename cb_exit_t, typename cb_revisit_t, typename cb_ambig_t>
bool traverse(cb_enter_t cb_enter,
	cb_exit_t cb_exit,
	cb_revisit_t cb_revisit,
	cb_ambig_t cb_ambig) const;
```
Traverses the whole forest.

`cb_exit`, `cb_revisit` and `cb_ambig` callbacks are optional arguments.

`cb_enter` is called when the `traverse()` enters a node. Callback receives the entered node as an argument.

`cb_exit` is called when the `traverse()` exits a node. Callback receives the node it is exiting as an argument.

`cb_revisit` is called before entering a child of a node if the child node was already visited before. The child node is passed as an argument. This callback has to return `true` to continue traversing the child node or `false` to skip it.

`cb_ambig` is called after entering a node which splits into more than one tree. The node is passed as a first argument, a set of vectors of nodes is passed as a second argument representing sets of children nodes where each set member represents a different tree.

Example of a traversal printing nodes and ambiguities of a parsed forest
```
function print_node(ostream& os, const parser<>::pnode& n) {
	return os << n.first << " [" << n.second[0] << ", " << n.second[1] << "]";
};

void main() {
	grammar g(tgf<>::from_file("arithmetic.tgf"));
	parser p(g);
	auto r = p.parse("1+2*3");
	if (r.found) {
		cerr << r.parse_error.to_str() << endl;
		return;
	}
	r.get_forest()->traverse(
		[](const parser<>::pnode& n) {
			print_node(cout << "entering node: ", n) << "\n";
		},
		[](const parser<>::pnode& n, forest<parser<>::pnode>::nodes_set& ns) {
			print_node(cout << "exiting node: ", n) << "\n";
		},
		[](const parser<>::pnode& n) {
			print_node(cout << "node revisited: ", n) << "\n";
			return false;
		},
		[](const parser<>::pnode& n, forest<parser<>::pnode>::nodes_set& ambset) {
			print_node(cout << "ambiguous node: ", n) << "\n";
			size_t t = 1;
			for (auto& a : ambset) {
				cout << "tree " << t++ << ":\n";
				for (auto& x : a) print_node(cout << "\t", x) << "\n";
			}
			return ambset;
		}
	);
}
```


### bool replace_nodes(graph& g, nodes& s);

Replace each node with its immediate children, assuming its only one pack (unambigous) the caller to ensure the right order to avoid cyclic dependency if any. deletes from graph g as well. return true if any one of the nodes' replacement succeeds


### bool replace_node(graph& g, const node& torep,const nodes& replacement);

Replaces node 'torep' in one pass with the given nodes 'replacement' everywhere in the forest and returns true if changed. Does not care if its recursive or cyclic, its caller's responsibility to ensure
