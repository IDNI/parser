[back to index](../README.md#overview-of-types)

# tgf

```
template <typename C = char, typename T = C> struct tgf;
```

The simplest way to create a grammar is to use Tau Grammar form which is an EBNF-like language for describing grammars.

Library implements a TGF parser which parses TGF from a string or from a file and it transforms the parsed forest into a `grammar` which can be used by a `parser`.

## functions

```
static grammar<C, T> from_string(nonterminals<C, T>& nts_,
	const std::basic_string<C>& s,
	const std::basic_string<C>& start_nt = from_cstr<C>("start"))
```

```
static grammar<C, T> from_file(nonterminals<C, T>& nts_,
	const std::string& filename,
	const std::basic_string<C>& start_nt = from_cstr<C>("start"))
```

Usage:
```
nonterminals nts;
parser p(tgf::from_string(nts,
	"greeting => \"hi\" | \"hello\".",
	"greeting"));
auto r = p.parse("hi");    // r.found == true
     r = p.parse("hello"); // r.found == true
     r = p.parse("table"); // r.found == false
```
