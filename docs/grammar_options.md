[back to index](../README.md#classes-and-structs)

# grammar::options

grammar's `options` is a plain struct which can be passed to a grammar when it is instantiated.

## attributes


### bool transform_negation;

If this is `true`, and it is `true` by default, any negation in a grammar is transformed into a negation tracking rule when the grammar is instantiated.
`false` is used for example to instantiate grammar from a generated parser because its rules are already product of this negation transormation.


### bool auto_disambiguate;

This is `true` by default. Parser disambiguates parse trees according to an order of production rule appearance in the grammar. Setting this to `false` would make parser to provide all possible parse trees.

This can be also disabled by TGF directive `@disable disambiguation.`


### std::set<size_t> nodisambig_list;

If `auto_disambiguate` is set to `true` this list contains nonterminal ids of symbols we don't want to disambiguate and we want to keep ambiguity in the resulting parse forest.

This list can be set by TGF directive: `@ambiguous symbol1, symbol2.`


### shaping_options shaping;

parsed tree [`shaping_options`](shaping_options.md)
