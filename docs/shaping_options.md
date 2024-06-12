[back to index](../README.md#overview-of-types)

# shaping_options

`shaping_options` contains several settings which are used to shape parsed trees when shaping is used (`parser::result::get_shaped_tree()`).

It can be provided to a grammar in `grammar::options` as attribute `shaping`.

## attributes


### std::set<size_t> to_trim;

nonterminal ids of symbols to trim (including their children) away from the resulting tree.

List can be provided in TGF with `@trim whitespace, comment.`


### std::set<size_t> to_trim_children;

nonterminal ids of symbols which children will be trimmed away from the resulting tree.

List can be provided in TGF with `@trim children sym1, sym2.`


### std::set<size_t> to_trim_children_terminals;

nonterminal ids of symbols which children terminals will be trimmed away from the resulting tree leaving nonterminals.

List of nonterminals can be provided in TGF with `@trim children terminals sym1, sym2.`


### bool trim_terminals;

If `trim_terminals` is `true` it trims all terminals from the resulting parse tree.
It is `false` by default.

This can be set to `true` in TGF by `@trim all terminals.`


### std::set<std::vector<size_t>> to_inline;

Inline replaces a node with its children. `to_inline` contains vectors of nonterminal ids.

If a vector contains only a single nonterminal, it means that the nonterminal is replaced by its children.

If a vector contains more than one nonterminal, it searches the parsed tree for occurance of a tree path of nonterminals denoted by nonterminal ids in the vector. First nonterminal is replaced by the last nonterminal node in the vector.

This can be populated by `@inline` directive.

`@inline chars.` Node `chars` is replaced by its children.

`@inline expr > block > expr.` Node `expr` containing a child node `block` which contains a child node `expr` is replaced by the `expr` child


### bool inline_char_classes;

If `inline_char_classes` is `true`. Shaping will also inline all character class functions. Is a short alternative to adding them into `to_inline` or `@inline` one by one.

Default value is `false`.

In TGF this can be set to `true` by adding `char classes` to `@inline` directive.

`@inline char classes.`
