# IDNI parser library

This C++ parsing library was created because other available parsers were missing features required for development of our non-turing but still very expressive languages:
- **TML** (Tau Meta Language), a logical programming language
- **Tau**, a software specification language...

This library additionally provides a parser for TGF (Tau Grammar form), an EBNF-like language for describing boolean grammars.

It also provides a `tgf` tool which takes a grammar described in a TGF file and it can help debug the grammar or generate C++ code of the parser.


## Features

- Boolean (including conjunctive) grammars
	- C++ structures with operators for [creating grammars programatically](docs/programatic_grammar.md)
	- [TGF](docs/tgf.md) - Tau Grammar Form language for describing grammars in an EBNF-like format
- Earley based parser with garbage collection producing parse forest containing all the parsed trees
- scannerless (lexerless) parsing
- forest traversal with callbacks and extraction of trees and graphs to deal with transformations and with ambiguity
- `tgf` tool to test grammars in TGF file or to generate a C++ code representing a parser for a given TGF file.
- predefined or custom classes of characters defined by a function (`<cctype>`-like functions)
- optional [recoding](docs/recoders.md) of an input into another type which can can be used for lexing, reencoding or accessing binary data
- [own UTF-8 support](docs/utf8.md)
- no dependencies on other libraries


## Basic terminology

**Literal** refers to a specific value or sequence of characters that appears exactly as it is in the input or in the grammar rules. It represents a fixed, concrete element that must be matched in order for a particular rule or pattern to be satisfied.
Literal can be terminal or non-terminal.

A **terminal** literal represents a specific token or value in the input language. It corresponds to the actual elements that appear in the input stream during parsing. Terminal literals are also referred to as terminals or terminal symbols.

A **non-terminal** literal represents a syntactic category or a placeholder that can be replaced by a sequence of terminals and/or non-terminals. Non-terminal literals are used to define the structure of the language and serve as intermediate building blocks for generating valid sentences or expressions.

A **production rule** consists of two parts: a left-hand side (LHS) and a right-hand side (RHS). The left-hand side contains a single non-terminal symbol, and the right-hand side contains a sequence of terminals, non-terminals, or both. The production rule indicates that the non-terminal on the left-hand side can be rewritten or expanded into the sequence of symbols on the right-hand side.

**Grammar** is a formal system that defines the syntax and structure of a language. It consists of terminals, non-terminals, production rules and a starting symbol (which is usually a non-terminal).

**Conjunctive Grammar** is a grammar which allows usage of conjunctions of literal sequences in production rules. The conjunction acts as a logical "AND" operation, indicating that all the conjuncted literal sequences must be matched for the whole rule to be satisfied.

**Boolean Grammar** is a conjunctive grammar which allows also usage of negation of literal sequences. Negation acts as a logical "NOT" operation, indicating that if the negated literal sequence is matched the whole rule fails to be satisfied.

**DNF** or **Disjunctive Normal form** is a specific form of representing logical formulas, Boolean expressions or production rules in our case. In DNF, a logical formula is represented as a disjunction (OR) of one or more conjunctions (AND) of literals.

**EBNF** or **Extended Backus–Naur form** is a family of metasyntax notations, any of which can be used to express a context-free grammar.

**TGF** or **Tau Grammar Form** is an EBNF based form to describe grammars which are usable by this library. TGF understands EBNF syntax (+, *, [] and {}) so it is easier to define repetition or optionality.


## Namespace

Library uses an `idni` namespace for its declarations.

For simplicity, all examples in library documentation expect declaration `using namespace idni;`.


## Templating

Most of the structures provided by this library are templates to enable generic parsing of any input data while parsing any type of terminals. Usually they are declared like this:

`template <typename C = char, typename T = C> ...` where
- `C` is an input type of input data (usually characters).
- `T` is a type of a terminal.

Fully supported and tested types are `char` and `char32_t` (Unicode support). Though it is possible to supply encoder and decoder functions to your parser and these can convert input elements of any type into terminals of any type.

If `T` differs from `C` it is required to provide decoder and encoder functions in `parser::options`. `parser` use these recoders to convert input data into terminals and back. See [recoders](docs/recoders.md) for more information.

<a name="overview-of-types"></a>

## Classes and structs

Click on the type name in the following list to see more detailed documentation of its API:
- [`nonterminals`](docs/nonterminals.md) - id map of non-terminals to their names
- [`lit`](docs/lit.md), [`lits`](docs/lits.md), [`conjs`](docs/conjs.md), [`disjs`](docs/disjs.md), [`prod`](docs/prod.md) and [`prods`](docs/prods.md) - basic building blocks for creating a grammar programatically. `prods` represents a grammar containing production rules as disjunctions of conjunctions of sequences of literals.
- [`char_class_fn`, `char_class_fns`](docs/char_class_fns.md) - Character class function is an alternative way to create a production rule. It is a function returning true if a terminal character is a member of a character class. It can be custom or there are predefined cctype-like functions (isalnum, isdigit...) provided. `char_class_fns` is a container for such functions.
- [`grammar`](docs/grammar.md) - represents a grammar created from production rules `prods` and character class functions `char_class_fns`.
- [`parser`](docs/parser.md) - uses a `grammar` for parsing input which produces a `forest`
- [`parser::options`](docs/parser_options.md) - plain struct of options changing behavior of the parser
- [`parser::input`](docs/parser_input.md) - wraper for an input data pointer, an input stream or a file (by filedescriptor) which provides an API to access it from a parser or a decoder
- [`parser::error`](docs/parser_error.md) - detailed information about a parse error
- [`forest`](docs/forest.md) - a result of parsing containing all the parsed trees. Provides means of traversal or extraction of trees or graphs.
- [`forest::graph`](docs/forest_graph.md) - a graph extracted from a forest
- [`forest::tree`](docs/forest_tree.md) - a tree extracted from a graph
- [`tgf`](docs/tgf.md) - Tau Grammar form parser - reads a grammar in TGF format from a string or a file. An alternative way to describe grammar instead of creating it programatically.


## Functions

There are several areas covered by functions provided by this library

- [predefined character class functions](docs/charclasses.md) - cctype-like predefined character class functions
- [terminals extraction](docs/terminals_extraction.md) - conversion of forest terminals to int or string
- [recoders](docs/recoders.md) - decoder and encoder for UTF-8 in `char` <-> Unicode in `char32_t`
- [UTF-8](docs/utf8.md) - UTF-8 and Unicode support
- [devhelpers](docs/devhelpers.md) - helper forest transformations to various formats (TML facts, TML rules, DOT) useful when developing a parser


## Tutorials

### CSV parser

Beginner tutorial to quick start using this library.

You can learn how to use this library in a CSV parser tutorial.

Part 1 shows a very simple parser with all it's required.
Each following part expands the previous one.
Read comments since they are always related to a new or a changed code.

- [part 1](examples/csv_parser1/main.cpp) - minimal parser of positive integers
- [part 2](examples/csv_parser2/main.cpp) - using predefined character classes
- [part 3](examples/csv_parser3/main.cpp) - refactor parser into its struct
- [part 4](examples/csv_parser4/main.cpp) - parse also negative integers
- [part 5](examples/csv_parser5/main.cpp) - parse strings and nulls, traverse
- [part 6](examples/csv_parser6/main.cpp) - parse comma separated values
- [part 7](examples/csv_parser7/main.cpp) - parse new line separated rows
- [part 8](examples/csv_parser8/main.cpp) - replace programmatically created grammar with a grammar in TGF
- [part 9](examples/csv_parser9/main.cpp) - using EBNF syntax in TGF
- [part 10](examples/csv_parser10/main.cpp), [csv.tgf](examples/csv_parser10/csv.tgf) - using tgf tool to generate a parser from a TGF file
