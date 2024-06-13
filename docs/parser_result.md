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

Contains information how to shape parsed tree when using `get_shaped_tree()` method.


## methods

### pforest* get_forest() const;

Returns pointer to the parsed forest.


### psptree get_trimmed_tree(const pnode& n) const;
### psptree get_trimmed_tree(const pnode& n, const shaping_options opts) const;

Transforms forest into tree and applies trimming according to grammar options or shaping options provided as an argument.

It also transforms ambiguous nodes as children of __AMB__ nodes.

This is called by `get_shaped_tree()`.

### psptree inline_tree_nodes(const psptree& t, psptree& parent) const;
### psptree inline_tree_nodes(const psptree& t, psptree& parent, const shaping_options opts) const;

Applies inlining to a tree (w/o tree paths) according to grammar options or shaping options provided as an argument.

This is called by `inline_tree()`.


### psptree inline_tree_paths(const psptree& t) const;
### psptree inline_tree_paths(const psptree& t, const shaping_options opts) const;

Applies tree paths inlining to a tree according to grammar options or shaping options provided as an argument.

This is called by `inline_tree()`.


### psptree inline_tree(psptree& t) const;
### psptree inline_tree(psptree& t, const shaping_options opts) const;

Applies inlining to a tree according to grammar options or shaping options provided as an argument. Calls `inline_tree_nodes()` and `inline_tree_paths()`.

This is called by `get_shaped_tree()`.


### psptree trim_children_terminals(const psptree& t) const;
### psptree trim_children_terminals(const psptree& t, const shaping_options opts) const;

Trims children terminals from a tree according to grammar options or shaping options provided as an argument.

This is called by `get_shaped_tree()`.

### psptree get_shaped_tree() const;
### psptree get_shaped_tree(const shaping_options opts) const;
### psptree get_shaped_tree(const pnode& n) const;
### psptree get_shaped_tree(const pnode& n, const shaping_options opts) const;

Transforms forest into a tree and applies shaping according to grammar options or shaping options provided as an argument.


### psptree get_tree();
### psptree get_tree(const pnode& n);

Extracts the first parse tree from the parsed forest.


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
