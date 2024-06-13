[back to index](../README.md#classes-and-structs)

# forest<NodeT>::graph

`forest<NodeT>::graph` is a least maximal core graph without ambiguity/repeating nodes/edges possibly with cycles or shared nodes. No cycles and no sharing implies its a tree.

It has the same API as `std::unordered_map<NodeT, std::set<std::vector<NodeT>>>` and adds `root` and `cycles` attributes and `extract_trees()` method.

## attributes

### NodeT root;

`graph`'s root node.

### std::set<NodeT> cycles;

Set of nodes that lead to a cycle.

## methods

### std::shared_ptr<forest<NodeT>::tree> extract_trees();

`extract_trees` extract a tree from a graph.

```
auto next_g = [](parser<C, T>::pforest::graph& fg) {
	auto tree = fg.extract_trees();
	tree->to_print(cout << "\n\n------\n"), cout << endl;
	return true;
};
f->extract_graphs(f->root(), next_g);
```