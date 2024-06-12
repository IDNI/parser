[back to index](../README.md#overview-of-types)

# forest

```
template <typename NodeT> struct forest;
```

`forest` is a structure which can contain multiple trees. Nodes can have multiple sets of children where more than one set represents splitting of trees meaning the part from root to the node is shared among all trees splitted.

It offers methods for traversal or tree or graph extractions.

`forest` is a generic structure since the type of nodes is templated.

### parse forest

This library uses `forest` to represent all parse trees parsed from a given input. Each `parser<C, T>::parse(...)` call produces a `forest` with a type `std::pair<lit<C, T>, std::array<size_t, 2>>` where `lit<C, T>` is a parsed literal and the array contains starting and ending position of a literal in an input string.

`parser` declares types `pnode` for a parse node and `pforest` for a parse forest:
```
typedef typename std::pair<lit<C, T>, std::array<size_t, 2>> pnode;
typedef forest<pnode> pforest;
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


### bool is_binarized() const;

Returns true if the forest is binarized, ie. each node has up to 2 children.


### size_t count_trees(const NodeT& p) const;

Counts and returns a number of trees under a `p` node.


### size_t count_trees() const;

Counts and returns a number of trees in a whole forest.


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

### template<typename TraversableT> bool detect_cycle(TraversableT& g) const;

Returns true if there exist a cycle in a `g`.

```
// having a forest f and a graph fg
if (f->detect_cycle(fg)) cout << "cycle detected\n";
else cout << "no cycles\n";
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

```

function print_node(ostream& os, parser<>::pnode& n) {
	return os << n.first << " [" << n.second[0] << ", " << n.second[1] << "]";
};

void main() {
	grammar g(tgf<>::from_file("arithmetic.tgf"));
	parser p(g);
	auto f = p.parse("1+2*3");
	if (!p.found()) {
		cerr << p.get_error().to_str() << endl;
		return;
	}
	f->traverse(
		[](const parser<>::pnode& n) {
			print_node(cout << "entering node: ", n) << "\n";
		},
		[](const parser<>::pnode& n) {
			print_node(cout << "exiting node: ", n) << "\n";
		},
		[](const parser<>::pnode& n) {
			print_node(cout << "node revisited: ", n) << "\n";
			return false;
		},
		[](const parser<>::pnode& n, std::set<std::vector<parser<>::pnode>>& ambset) {
			print_node(cout << "ambiguous node: ", n) << "\n";
			size_t t = 1;
			for (auto& a : ambset) {
				cout << "tree " << t++ << ":\n";
				for (auto& x : a) print_node(cout << "\t", x) << "\n";
			}
			return false;
		}
	);
}
```
