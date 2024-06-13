[back to index](../README.md#classes-and-structs)

# rewriting

namespace `idni::rewriter` provides functions and types useful for rewriting parsed trees.


## struct node

```
template <typename symbol_t>
struct node;
```

Node of a rewriter tree. `symbol_t` is a type of node's value.


### attributes


#### symbol_t value;

Value stored in the node. `symbol_t` is a template type of the `node`.


#### std::vector<std::shared_ptr<node>> child;

Vector of shared pointers to node's children.


## sp_node

```
template <typename symbol_t>
using sp_node = std::shared_ptr<node<symbol_t>>;
```

`sp_node<symbol_t>` is a shared pointer node templated with `symbol_t` type.


## environment

```
template<typename node_t>
using environment = std::map<node_t, node_t>;
```

An environment is a map from captures to tree nodes, it is used to keep track of the captures that have been unified and their corresponding tree nodes.


## rule

```
template<typename node_t>
using rule = std::pair<node_t, node_t>;
```

A rule is a pair of a pattern and a substitution. It is used to rewrite a tree.


## make_node

```
template <typename symbol_t>
sp_node<symbol_t> make_node(const symbol_t& s,
                            const std::vector<sp_node<symbol_t>>& ns);
```

Node factory function. Creates a node with `s` as a value and `ns` as children.


## creating rewriter tree from parser


### drop_location_t drop_location

Parser produced nodes contain not only a a literal value but they contain also an node's input position span. Rewriter uses `drop_location` transformation to tranform parser produced node into `symbol_t` by removing input position span information and if `symbol_t` is `std::variant` it transforms literal into literal variant.

```
template <typename parse_symbol_t, typename symbol_t>
auto drop_location = [](const parse_symbol_t& n)-> symbol_t { return n.first; };

template <typename parse_symbol_t, typename symbol_t>
using drop_location_t = decltype(drop_location<parse_symbol_t, symbol_t>);
```


### make_node_from_string / make_node_from_stream / make_node_from_file

Make a tree from the given input string:
```
template<typename parser_t, typename transformer_t, typename symbol_t>
sp_node<symbol_t> make_node_from_string(
	const transformer_t& transformer,
	const std::string source, idni::parser<>::parse_options options = {})
```

Make a tree from the given input stream:
```
template<typename parser_t, typename transformer_t, typename symbol_t>
sp_node<symbol_t> make_node_from_stream(const transformer_t& transformer,
	std::istream& is, idni::parser<>::parse_options options = {})
```

Make a tree from the given input file:
```
template<typename parser_t, typename transformer_t, typename symbol_t>
sp_node<symbol_t> make_node_from_file(const transformer_t& transformer,
	const std::string& filename, idni::parser<>::parse_options options = {})
```

Make a tree from the given parse result and log parse error if parse fails:
```
template<typename parser_t, typename transformer_t, typename symbol_t>
sp_node<symbol_t> make_node_from_parse_result(
	const transformer_t& transformer, typename parser_t::result& r)
```
This function is used after parse is called on a given input in previous make_node_from_* functions.

Make a tree from the given tree extracted from a forest struct:
```
template<typename parser_t, typename transformer_t, typename symbol_t>
sp_node<symbol_t> make_node_from_tree(
	const transformer_t& /*transformer*/, typename parser_t::sptree_type t)
```


## traverser filter functions

Simple function objects with types to be used as default values for the traversers.


### all_t all

Takes all nodes. It is always true.

```
template <typename node_t>
static const auto all = [](const node_t&) { return true; };

template <typename node_t>
using all_t = decltype(all<node_t>);
```


### none_t none

Takes none because it's always false.

```
template <typename node_t>
static const auto none = [](const node_t&) { return false; };

template <typename node_t>
using none_t = decltype(none<node_t>);
```


### identity_t identity

Returns itself.

```
template <typename node_t>
static const auto identity = [](const node_t& n) { return n; };

template <typename node_t>
using identity_t = decltype(identity<node_t>);
```


## visitor traversers


### post_order_traverser

```
template <typename wrapped_t, typename predicate_t, typename input_node_t,
	typename output_node_t = input_node_t>
struct post_order_traverser {
	post_order_traverser(wrapped_t& wrapped, predicate_t& query);
	output_node_t operator()(const input_node_t& n);
};
```

Visitor that traverses the tree in post-order (avoiding visited nodes).


### post_order_query_traverser

```
template <typename wrapped_t, typename predicate_t, typename input_node_t,
	typename output_node_t = input_node_t>
struct post_order_query_traverser {
	post_order_query_traverser(wrapped_t& wrapped, predicate_t& query);
	output_node_t operator()(const input_node_t& n);
};
```

Visitor that traverses the tree in post-order (avoiding visited nodes) and uses a query to filter nodes.


### post_order_tree_traverser

```
template <typename wrapped_t, typename predicate_t, typename input_node_t,
	typename output_node_t = input_node_t>
struct post_order_tree_traverser {
	post_order_tree_traverser(wrapped_t& wrapped, predicate_t& query);
	output_node_t operator()(const input_node_t& n);
};
```

Visitor that traverses the tree in post-order (repeating visited nodes if necessary).


### post_order_recursive_traverser

```
template<typename node_t>
struct post_order_recursive_traverser {
	auto operator() (const node_t n, auto& query, auto& wrapped);
};
```


### map_transformer

```
template <typename wrapped_t, typename input_node_t,
	typename output_node_t = input_node_t>
struct map_transformer {
	map_transformer(wrapped_t& wrapped) : wrapped(wrapped) {}
	output_node_t operator()(const input_node_t& n);
};
```

Visitor that produces nodes transformed accordingly to the given transformer. It only works with post order traversals.


### map_node_transformer

```
template <typename wrapped_t, typename input_node_t,
	typename output_node_t = input_node_t>
struct map_node_transformer {
	map_node_transformer(wrapped_t& wrapped) : wrapped(wrapped) {}
	output_node_t operator()(const input_node_t& n);
};
```

Visitor that produces nodes transformed accordingly to the given transformer. It only works with post order traversals.


### replace_node_transformer

```
template <typename wrapped_t, typename input_node_t,
	typename output_node_t = input_node_t>
struct replace_node_transformer {
	replace_node_transformer(wrapped_t& wrapped);
	output_node_t operator()(const input_node_t& n);
};
```

Visitor that produces nodes transformed accordingly to the given transformer. It only works with post order traversals.


### replace_transformer

```
template <typename node_t>
struct replace_transformer {
	replace_transformer(std::map<node_t, node_t>& changes);
	node_t operator()(const node_t& n);
};
```
Visitor that produces nodes transformed accordingly to the given transformer. It only works with post order traversals.


### select_top_predicate

```
template <typename predicate_t, typename node_t>
struct select_top_predicate {
	select_top_predicate(predicate_t& query, std::vector<node_t>& selected);
	bool operator()(const node_t& n);
};
```

Visitor that selects top nodes that satisfy a predicate and stores them in the supplied vector. It only works with post order traversals and never produces duplicates.


### select_subnodes_predicate

```
template <typename predicate_t, typename extractor_t, typename node_t>
struct select_subnodes_predicate {
	select_subnodes_predicate(predicate_t& query, extractor_t extractor,
		std::vector<node_t>& selected);
	bool operator()(const node_t& n);
};
```

Visitor that selects nodes that satisfy a predicate and stores the subnodes extracted from them in the supplied vector. It only works with post order traversals and never produces duplicates.


### select_all_predicate

```
template <typename predicate_t, typename node_t>
struct select_all_predicate {
	select_all_predicate(predicate_t& query, std::vector<node_t>& selected);
	bool operator()(const node_t& n);
};
```

Visitor that selects nodes that satisfy a predicate and stores them in the supplied vector.


### find_top_predicate

```
template <typename predicate_t, typename node_t>
struct find_top_predicate {
	find_top_predicate(predicate_t& query, std::optional<node_t>& found);
	bool operator()(const node_t& n);
};
```

Visitor that selects nodes that satisfy a predicate and stores them in the supplied vector.


### true_predicate

```
template<typename node_t>
struct true_predicate {
	bool operator()(const node_t&) const { return true; }
};

template <typename node_t>
using true_predicate_t = true_predicate<node_t>;
```

Always true predicate


### false_predicate

```
template<typename node_t>
struct false_predicate {
	bool operator()(const node_t&) const { return false; }
};

template <typename node_t>
using false_predicate_t = false_predicate<node_t>;
```

Always false predicate


### and_predicate

```
template <typename l_predicate_t, typename r_predicate_t>
struct and_predicate {
	and_predicate(l_predicate_t& p1, r_predicate_t& p2);
	template<typename node_t>
	bool operator()(const node_t& n) const;
};

template <typename l_predicate_t, typename r_predicate_t>
using and_predicate_t = and_predicate<l_predicate_t, r_predicate_t>;
```

Conjunction of the wrapped predicates.


### or_predicate

```
template <typename l_predicate_t, typename r_predicate_t>
struct or_predicate {
	or_predicate(l_predicate_t& p1, r_predicate_t& p2);
	template<typename node_t>
	bool operator()(const node_t& n) const;
};

template <typename l_predicate_t, typename r_predicate_t>
using or_predicate_t = or_predicate<l_predicate_t, r_predicate_t>;
```

Disjuction of the wrapped predicates.


### neg_predicate

```
template <typename predicate_t>
struct neg_predicate {
	neg_predicate(predicate_t& p);
	template<typename node_t>
	bool operator()(const node_t& n) const;
};

template <typename predicate_t>
using neg_predicate_t = neg_predicate<predicate_t>;
```

Negation of the wrapped predicate.


### while_not_found_predicate

```
template <typename node_t>
struct while_not_found_predicate {
	while_not_found_predicate(std::optional<node_t>& found);
	bool operator()(const node_t& /*n*/) const;
};
```

True while found is not set (found), it aborts the traversal once found has been set.

### find_visitor

```
template <typename predicate_t, typename node_t>
struct find_visitor {
	find_visitor(predicate_t& query, std::optional<node_t>& found);
	node_t operator()(const node_t& n);
};
```

To be used in conjunction with while_not_found_predicate. It sets found when the predicate is satisfied by a node (set in found).


## select and find

find is used to find a first node that satisfy a predicate.

select is used to find all


### select_top

```
template <typename predicate_t, typename node_t>
std::vector<node_t> select_top(const node_t& input, predicate_t& query);
```

Select all top nodes that satisfy a predicate and return them.


### select_subnodes

```
template <typename predicate_t, typename extractor_t, typename node_t>
std::vector<node_t> select_subnodes(const node_t& input, predicate_t& query,
	extractor_t extractor);
```

Select all subnodes that satisfy a predicate according to the extractor and return them.


### select_all

```
template <typename predicate_t, typename node_t>
std::vector<node_t> select_all(const node_t& input, predicate_t& query);
```

Select all top nodes that satisfy a predicate and return them.


### find_top

```
template <typename predicate_t, typename node_t>
std::optional<node_t> find_top(const node_t& input, predicate_t& query);
```

Find the first node that satisfy a predicate and return it.


### find_bottom

```
template <typename predicate_t, typename node_t>
std::optional<node_t> find_bottom(const node_t& input, predicate_t& query)
```

Find the first node that satisfy a predicate and return it.


## trimming and replacing

### trim_top

```
template <typename predicate_t, typename symbol_t,
	typename node_t = sp_node<symbol_t>>
node_t trim_top(const node_t& input, predicate_t& query);
```

Delete all top nodes that satisfy a predicate.


### replace

```
template <typename node_t>
node_t replace(const node_t& n, std::map<node_t, node_t>& changes);
```

Find the first node that satisfy a predicate and return it.


### replace_if

```
template <typename node_t, typename predicate_t>
node_t replace_if(const node_t& n, std::map<node_t, node_t>& changes, predicate_t& query);
```

Replace nodes in n according to changes while skipping subtrees specified by query


## matching and apply

### pattern_matcher

```
template <typename node_t, typename is_capture_t>
struct pattern_matcher {
	using pattern_t = node_t;
	pattern_matcher(const pattern_t& pattern, environment<node_t>& env,
		const is_capture_t& is_capture);
	bool operator()(const node_t& n);

};
```

This predicate matches when there exists a environment that makes the pattern match the node.


### pattern_matcher_if

```
template <typename node_t, typename is_capture_t, typename predicate_t>
struct pattern_matcher_if {
	using pattern_t = node_t;
	pattern_matcher_if(const pattern_t& pattern,
		environment<node_t>& env,
		is_capture_t& is_capture, predicate_t& predicate);
	bool operator()(const node_t& n);
};
```

This predicate matches when there exists a environment that makes the pattern match the node ignoring the nodes detected as skippable.


### apply

```
template <typename node_t, typename matcher_t>
node_t apply(const node_t& s, const node_t& n, matcher_t& matcher)
```

Apply a substitution to a rule according to a given matcher, this method is use internaly by apply and apply with skip.


### apply_if

```
template <typename node_t, typename is_capture_t, typename predicate_t>
node_t apply_if(const rule<node_t>& r, const node_t& n,
	is_capture_t& c, predicate_t& predicate)
```

Apply a rule to a tree using the predicate to pattern_matcher and skipping unnecessary subtrees


### apply_rule

```
template <typename node_t, typename is_capture_t>
node_t apply_rule(const rule<node_t>& r, const node_t& n, const is_capture_t& c)
```

Apply a rule to a tree using the predicate to pattern_matcher.