[back to index](../README.md#classes-and-structs)

# parser::result

`result` is a structure returned by a `parse` call. It contains a parsed forest, input and provides means to extract parsed information, trees and input characters.


## attributes

### const bool found;

`found` contains `true` if the starting symbol was successfuly parsed over the whole input.
Otherwise it contains `false`.


### const parser::error parse_error;

If `found` is `false`, `parse_error` contains information why parsing failed.


### const shaping_options shaping;

Contains information how to shape a parsed tree when using `get_shaped_forest_tree()`
or `get_shaped_bintree()` methods.


## methods

### pforest* get_forest() const;

Returns pointer to the parsed forest. In bintree parse mode the forest is built lazily on first call (recorded as `reconstruct_forest` in diagnostics).


### tref get_bintree();
### tref get_bintree(const pnode& n);

Returns the parse tree as a bintree (`tref`). In forest parse mode the tree is extracted lazily on first call (`reconstruct_bintree` with `extract graph`, `inline grammar`, and `extract tree2` substeps in diagnostics).

`get_tree2()` / `get_tree2(const pnode& n)` are aliases.


## Tree representations

`result` provides two families of tree-access and shaping APIs, distinguished
by the tree representation they operate on:

| Family      | Return type | Preferred naming   | Legacy names (aliases)                                                 |
|-------------|-------------|--------------------|------------------------------------------------------------------------|
| Forest-tree | `psptree`   | `*_forest_tree*`   | `get_trimmed_tree`, `get_shaped_tree`, `get_tree`, `inline_tree`, etc. |
| Bintree     | `tref`      | `*_bintree*`       | `get_tree2`, `get_shaped_tree2`, etc.                                  |

Forest-tree methods work with the parse forest and return `psptree` (shared
pointer to `ptree`). Bintree methods work with a binary-tree representation
(`tref`) and are native when `parse_options::tree_path == bintree_path` (the
default).

All legacy names remain available as inline wrappers that delegate to the
preferred implementations.


## Forest-tree API (psptree)


### psptree get_trimmed_forest_tree(const pnode& n) const;
### psptree get_trimmed_forest_tree(const pnode& n, const shaping_options opts) const;

*Legacy name: `get_trimmed_tree` (still available as an alias).*

Transforms forest into a tree and applies trimming according to grammar options
or shaping options provided as an argument.

It also transforms ambiguous nodes as children of `__AMB__` nodes.

This is called by `get_shaped_forest_tree()`.

### psptree inline_forest_tree_nodes(const psptree& t, psptree& parent) const;
### psptree inline_forest_tree_nodes(const psptree& t, psptree& parent, const shaping_options opts) const;

*Legacy name: `inline_tree_nodes` (still available as an alias).*

Applies inlining to a tree (w/o tree paths) according to grammar options or
shaping options provided as an argument.

This is called by `inline_forest_tree()`.


### psptree inline_forest_tree_paths(const psptree& t) const;
### psptree inline_forest_tree_paths(const psptree& t, const shaping_options opts) const;

*Legacy name: `inline_tree_paths` (still available as an alias).*

Applies tree paths inlining to a tree according to grammar options or shaping
options provided as an argument.

This is called by `inline_forest_tree()`.


### psptree inline_forest_tree(psptree& t) const;
### psptree inline_forest_tree(psptree& t, const shaping_options opts) const;

*Legacy name: `inline_tree` (still available as an alias).*

Applies inlining to a tree according to grammar options or shaping options
provided as an argument. Calls `inline_forest_tree_nodes()` and
`inline_forest_tree_paths()`.

This is called by `get_shaped_forest_tree()`.


### psptree trim_forest_tree_child_terminals(const psptree& t) const;
### psptree trim_forest_tree_child_terminals(const psptree& t, const shaping_options opts) const;

*Legacy name: `trim_children_terminals` (still available as an alias).*

Trims children terminals from a tree according to grammar options or shaping
options provided as an argument.

This is called by `get_shaped_forest_tree()`.

### psptree get_shaped_forest_tree() const;
### psptree get_shaped_forest_tree(const shaping_options opts) const;
### psptree get_shaped_forest_tree(const pnode& n) const;
### psptree get_shaped_forest_tree(const pnode& n, const shaping_options opts) const;

*Legacy name: `get_shaped_tree` (still available as an alias).*

Transforms forest into a tree and applies shaping according to grammar options
or shaping options provided as an argument.


### psptree get_forest_tree();
### psptree get_forest_tree(const pnode& n);

*Legacy name: `get_tree` (still available as an alias).*

Extracts the first parse tree from the parsed forest.


## Bintree API (tref)


### tref get_trimmed_bintree(tref ref) const;
### tref get_trimmed_bintree(tref ref, const shaping_options opts) const;

*Legacy name: `get_trimmed_tree2` (still available as an alias).*

Trims a bintree according to grammar options or shaping options provided as an
argument.

This is called by `get_shaped_bintree()`.


### tref inline_bintree_nodes(tref t) const;
### tref inline_bintree_nodes(tref t, const shaping_options opts) const;

*Legacy name: `inline_tree_nodes2` (still available as an alias).*

Applies inlining to a bintree (w/o tree paths) according to grammar options or
shaping options provided as an argument.

This is called by `inline_bintree()`.


### tref inline_bintree_paths(tref t) const;
### tref inline_bintree_paths(tref t, const shaping_options opts) const;

*Legacy name: `inline_tree_paths2` (still available as an alias).*

Applies tree paths inlining to a bintree according to grammar options or shaping
options provided as an argument.

This is called by `inline_bintree()`.


### tref inline_bintree(tref t) const;
### tref inline_bintree(tref t, const shaping_options opts) const;

*Legacy name: `inline_tree2` (still available as an alias).*

Applies inlining to a bintree according to grammar options or shaping options
provided as an argument. Calls `inline_bintree_nodes()` and
`inline_bintree_paths()`.

This is called by `get_shaped_bintree()`.


### tref trim_bintree_child_terminals(tref ref) const;
### tref trim_bintree_child_terminals(tref ref, const shaping_options opts) const;

*Legacy name: `trim_children_terminals2` (still available as an alias).*

Trims children terminals from a bintree according to grammar options or shaping
options provided as an argument.

This is called by `get_shaped_bintree()`.


### tref get_shaped_bintree();
### tref get_shaped_bintree(const shaping_options opts);
### tref get_shaped_bintree(tref t);
### tref get_shaped_bintree(tref t, const shaping_options opts);

*Legacy name: `get_shaped_tree2` (still available as an alias).*

Transforms forest into a bintree and applies shaping according to grammar
options or shaping options provided as an argument. If no `tref` argument is
given, uses `get_bintree()` to obtain the root.


### bool good() const;

`true` if input is good = stream is good or mmap is opened.


### std::basic_string<C> get_input();

Returns the input as a string (input's char type, ie. C).


### std::basic_string<T> get_terminals() const;

Read terminals from input (input's terminal type, ie. T).


### std::basic_string<T> get_terminals(const pnode& n) const;
### std::basic_ostream<T>& get_terminals_to_stream(std::basic_ostream<T>& os, const pnode& n) const;

Read terminals from input at position span of a provided node.


### std::optional<int_t> get_terminals_to_int(const pnode& n) const;

Reads terminals of a node and converts them to int. If the conversion fails or the int is out of range returns no value.


### bool is_ambiguous() const;

Returns true if the parse forest is ambiguous (contains more than one tree).


### bool has_single_parse_tree() const;

Returns `true` if the parse forest is not ambiguous (contains only a single tree).


### std::set<std::pair<pnode, pnodes_set>> ambiguous_nodes() const;

Returns ambiguous nodes.


### std::ostream& print_ambiguous_nodes(std::ostream& os) const;

Prints ambiguous nodes.


### nodes_and_edges get_nodes_and_edges() const;

Returns all nodes and edges of the forest. It is a pair of vectors of nodes and edges. edge is defined as a pair of node ids.

```
using node_edge       = std::pair<pnode, pnode>;
using edges           = std::vector<typename pforest::edge>;
using nodes_and_edges = std::pair<pnodes, edges>;
```


### bool inline_prefixed_nodes(pgraph& g, const std::string& prefix);

Removes all prefixed symbols from the whole graph by replacing them with their immediate children nodes.


### bool inline_grammar_transformations(pgraph& g);

Inlines nodes created by EBNF and binarize transformation prefixes.
