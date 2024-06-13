[back to index](../README.md#classes-and-structs)

# forest<NodeT>::tree

`forest<NodeT>::tree` is a tree extracted from `forst<NodeT>::graph`.

## attributes

### NodeT value;

`value` is the original node extracted from a graph (forest).

### std::vector<std::shared_ptr\<struct tree>> child;

`child` is a vector of tree's children - shared pointers to subtrees.

## methods

### std::ostream& to_print(std::ostream& os, size_t l = 0);

Prints the tree recursively to a stream `os`. `l` is used for indentation level.

```
auto tree = fg.extract_trees();
tree->to_print(cout << "\n\n------\n"), cout << endl;
```
