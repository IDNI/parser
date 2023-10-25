[back to index](../README.md#overview-of-types)

# devhelpers

`devhelper` functions `to_tml_facts`, `to_tml_rules` and `to_dot` can convert a parsed forest into various formats.

These functions shouldn't be necessary for your parser's production build but they can be helpful when debugging your implementation of a language parser, since it provides the parsed forest in a human readable forms or in a form which can be processed.

`devhelper` functions are available only when `WITH_DEVHELPERS` macro is defined when compiling the parser library.

## functions

### to_dot(...);

```
template <typename C, typename T = C>
bool to_dot(std::ostream& os,
	const typename parser<C, T>::pforest& f,
	const std::string& inputstr,
	const std::string& grammar_text);
```
```
template <typename C, typename T = C, typename P = parser<C, T>::pnode_graph>
bool to_dot(std::ostream& os,
	P& g,
	const std::string& inputstr,
	const std::string& grammar_text);
```

Takes a forest `f` (or a graph `g`) and produces a DOT data streamed into `os`.

`inputstr` is meant for an input text and `grammar_text` is meant for a grammar text. Both texts are added into the DOT data but they can be shortened or customized.

DOT files can be used by `dot` utility to generate other formats for example for a graphical visualization (PNG, SVG...).


### to_tml_facts(...);

```
template <typename C, typename T = C>
bool to_tml_facts(std::ostream& os,
	const typename parser<C, T>::pforest& f);
```

Takes a forest `f` and produces all its nodes and edges as facts in TML to ostream `os`.

Nodes and edges facts are provided in relations `node` and `edge` with following attributes:
```
node(node_id literal input_from_position input_to_position).
edge(nodeA_id nodeB_id).
```

Example for a grammar `start => 'H' 'i'.` and an input text `"Hi"`:
```
node(0 start 0 2).
node(1 'H' 0 1).
node(2 'i' 1 2).
edge(0 1).
edge(0 2).
```

### to_tml_rules(...);

```
template <typename C, typename T = C>
bool to_tml_rules(std::ostream& os, const typename parser<C, T>::pforest& f);
```
```
template <typename C, typename T = C, typename P = parser<C, T>::pnode_graph>
bool to_tml_rules(std::ostream& os, P& g);
```

Takes a forest `f` (or a graph `g`) and produces its data as TML. Each node of the forest is either printed as a fact if it is a terminal or a rule if it is a non-terminal. Body of such a rule contains children nodes.
All produced relations have two attributes: starting and ending positions of a literal in the input.

Example of a produced data when using grammar `start => hi | hello. hi => "hi". hello => "hello".` and parsing string `"hi"`:
```
start(0 2) :- hi(0 2).
hi(0 2) :- 'h'(0 1), 'i'(1 2).
'h'(0 1).
'i'(1 2).
```

### to_tml_rule(...);

```
template<typename C, typename T = C>
std::string to_tml_rule(const typename parser<C, T>::pnode& nd);
```

Produces a TML statement for a given forest or graph node `nd` as a TML fact or a TML rule. This method is used by `to_tml_rules(...)`.
