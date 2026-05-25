[back to index](../README.md#classes-and-structs)

# tgf

```
template <typename C = char, typename T = C> struct tgf;
```

The simplest way to create a grammar is to use Tau Grammar Format which is an EBNF-like language for describing grammars.

Library implements a TGF parser which parses TGF from a string or from a file and it transforms the parsed forest into a `grammar` which can be used by a `parser`.

## functions

```
static idni::diagnostics::result<grammar<C, T>>
from_string(nonterminals<C, T>& nts_,
	const std::basic_string<C>& s)
```

```
static idni::diagnostics::result<grammar<C, T>>
from_file(nonterminals<C, T>& nts_,
	const std::string& filename)
```

Usage:
```
nonterminals nts;
grammar g = diagnostics::exit_on_fail(tgf<char>::from_string(nts,
		"greeting => \"hi\" | \"hello\"."));
parser p(g);
auto r = p.parse("hi");    // r.found == true
     r = p.parse("hello"); // r.found == true
     r = p.parse("table"); // r.found == false
```

## special keywords

### `null` — epsilon (the empty production)

`null` is a built-in TGF keyword that denotes epsilon. Use it on the
right-hand side of a rule when the production may match the empty input.
It is **not** a nonterminal — do not write a `null => ...` rule of your
own; doing so shadows the keyword.

```
# matches either no value at all (empty cell) or the literal "x"
cell => null | "x".
```

This is how the CSV grammar (`src/format/csv.tgf`) models an empty cell:

```
nullvalue => null.
```

If you need to match the *literal three-character string* `null` (as JSON
does), spell it as a string literal:

```
null_sym => "null".
```
