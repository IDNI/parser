[back to index](../README.md#classes-and-structs)

# traverser

traverser object can be created from a rewriter tree node and provides API for traversing its subtree and extract information from it.

It works in a way that it starts with a provided node or list of nodes. It has a value of all existing nodes it points to. By using operator `|` and `||` with a nonterminal it allows to traverse into child nodes. If a traverser reaches a node which does not exist it does not point to any node, ie it loses its value.

With these `|` and `||` operators it is also possible to use custom extractors and filters.

Traverser has two template parameters:
- `node_variant_t` - variant of the rewriter tree node.
- `parser_t` - parser type. It should have the same API as parser struct and additionally it has to contain an enum type called `nonterminal` with list of nonterminals translating each nonterminal to its id equivalent to the `nonterminals` container. C++ parsers generated from TGF by TGF tool offers such API (they define such enum).

## type aliases

struct provides and uses type aliases:
```
using node_t        = idni::rewriter::sp_node<node_variant_t>;
using symbol_t      = typename parser_t::symbol_type;
using nonterminal_t = typename parser_t::nonterminal;
using extractor_t   = extractor<node_variant_t, parser_t>;
```

## constructor

```
traverser();
traverser(const node_t& n);
traverser(const std::vector<node_t>& n);
```

Traverser can be created without a node, with a single node or with a vector of nodes.

Usage:
```
using node_variant_t = std::variant<lit<char>>>;

// take tgf parser instance
auto& p = tgf_parser::instance();

// parse input
char* s = "@start S.";
size_t l = strlen(s);
auto r = p.parse(s, l);
if (!r.found) exit(1); // parse error

// create a rewriter tree
char dummy = '\0'; // dummy transformer (TODO: should be optional)
auto source = idni::rewriter::make_node_from_tree<
	tgf_parser, char, node_variant_t>(dummy, r.get_shaped_tree());

// create a traverser for the source node and its sub tree
traverser<node_variant_t, tgf_parser> t(source);
```

## methods

### bool has_value() const;

Returns true if it contains (points to) at least one node.

Usage:
```
// ... continuing from the constructor usage example
auto t = source | tgf_parser::directive;
// t.has_value() == true
```

### const node_t& value() const;

Returns the first node from all it is pointing to.


### const std::vector<node_t>& values() const;

Returns all nodes traverser is pointing to.


### std::vector<traverser<node_variant_t, parser_t>> traversers() const;
### std::vector<traverser<node_variant_t, parser_t>> operator()() const;

Returns a vector of traversers for each node it points to.


### bool is_non_terminal_node(const node_t& n) const;
### bool is_non_terminal_node() const;
### bool is_non_terminal(const node_t& n, const nonterminal_t& nt) const;
### bool is_non_terminal(const nonterminal_t& nt) const;

Helper methods to check if a node is a nonterminal or if a node is a concrete nonterminal.


### traverser operator|(const nonterminal_t& nt) const;
### traverser operator||(const nonterminal_t& nt) const;

By using one of these operators with a nonterminal enum type value it traversers each node to its child if the child is of `nt` nonterminal type. These children are then a new list of nodes traverser is pointing to. If there is no such child, traverser loses all its nodes and it has no value.

`|` is for a single node. `||` is for multiple nodes.

Usage:
```
// ... continuing from the constructor usage example

// get all statement children nodes of the source root node
auto statements  = t || tgf_parser::statement;

// get all directive children nodes of all statements
auto directives  = statements || tgf_parser::directive;

// for each directive traverser
for (const auto& d : directives()) {
	// traverse from a directive node through directive_body node to a start_dir node
	auto start_dir = d | tgf_parser::directive_body | tgf_parser::start_dir;
	if (start_dir.has_value())
		; //...
}
```


### template \<typename result_t> result_t operator|(const extractor<node_variant_t, parser_t, result_t>& e) const;
### template \<typename result_t> result_t operator||(const extractor<node_variant_t, parser_t, result_t>& e) const;

If a traverser is used with one of these operators followed by an extracgtor it transforms traverser nodes according to the extractor and it returns `result_t` type.

`|` is for a single node. `||` is for multiple nodes.

## prepared extractors

### static constexpr const extractor<node_variant_t, parser_t>& get_only_child_extractor();

Returns an extractor usable to traverse to a single child if a single child exists.

Usage:
```
// ... continuing from the constructor usage example

// get all statements
auto statements = t || tgf_parser::statement;

// get reference to prepared get_only_child_extractor
auto& get_only_child = tgf_parser::get_only_child_extractor();

// for each statement traverser
for (const auto& s : statements()) {
	auto dir_or_prod = s | get_only_child;
	// dir_or_prod is now pointing to a statement child node being it either production or directive
}
```

### static constexpr const extractor<node_variant_t, parser_t, std::string>& get_terminal_extractor();

Returns an extractor usable to extract a string containing terminals collected from subtrees of nodes traverser is pointing to.

Usage:
```
// ... continuing from the constructor usage example

// get all symbols appearing in @trim directive.
auto trim_syms = t || tgf_parser::statement || tgf_parser::directive
	|| tgf_parser::directive_body || tgf_parser::trim_dir || tgf_parser::sym;

// get reference to a terminal extractor
auto& get_terminals = tgf_parser::get_terminal_extractor();

// print all symbols to be trimmed
std::cout << "to trim:";
for (const auto& ts : trim_syms())
	std::cout << ' ' << (ts | get_terminals);
```


### static constexpr const extractor<node_variant_t, parser_t, typename parser_t::nonterminal>& get_nonterminal_extractor();

Returns an extractor usable to extract nonterminal enum type from a node traverser is pointing to.

Usage:
```
// ... continuing from the constructor usage example

// get all directive bodies
auto directives  = t || tgf_parser::statement || tgf_parser::directive
	|| tgf_parser::directive_body;

// get references to prepared extractors
auto& get_only_child = tgf_parser::get_only_child_extractor();
auto& get_nonterminal = tgf_parser::get_nonterminal_extractor();

// for each directive body traverser
for (const auto& x : directives()) {
	auto d = x | get_only_child; // traverse to a child which is a single one
	tgf_parser::nonterminal dir_nt = d | get_nonterminal; // get its nonterminal id
	switch (dir_nt) {
		case tgf_parser::start_dir: //...
		case tgf_parser::trim_dir:  //...
		// ...
	}
}
```
