# IDNI parser library

This C++ parser generator was created in an effort to facilitate the development of the Tau language. The predecessor to the Tau language was TML and its parser was hand-coded in C++.
As the TML language evolved in complexity, it became increasingly difficult to evolve the C++ parser to include all the language features we wanted to add to TML.
That is why at the beginning of the Tau project, we started by first developing this parser generator based on the Earley parsing algorithm. It afforded us the ability to
effortlessly evolve the syntax of Tau language as we wished without the need to maintain very complex C++ code. We chose the Earley parsing algorithm because it is capable
of parsing any context free language while remaining efficient even in the presence of ambiguities in the grammar.

To make it even easier to define grammars, we created TGF (or the Tau Grammar format), an EBNF-like language for describing boolean grammars and included a `tgf` tool
which takes a grammar described in a TGF file and generates C++ code of the corresponding parser. It can also help debug the grammar.


## Features

- Boolean (including conjunctive) grammars
	- C++ structures with operators for [creating grammars programmatically](docs/programatic_grammar.md)
	- [TGF](docs/tgf.md) - Tau Grammar Form language for describing grammars in an EBNF-like format
- Earley based parser with garbage collection producing parse forest containing all the parsed trees
- scannerless (lexerless) parsing
- forest traversal with callbacks and extraction of trees and graphs to deal with transformations and with ambiguity
- `tgf` tool to test grammars in TGF file or to generate a C++ code representing a parser for a given TGF file.
- predefined or custom classes of characters defined by a function (`<cctype>`-like functions)
- optional [recoding](docs/recoders.md) of an input into another type which can can be used for lexing, reencoding or accessing binary data
- [own UTF-8 support](docs/utf8.md)
- no dependencies on other libraries


## How to use

To use the library or `tgf` tool you first have to compile this project. This requires CMake and a compiler supporting C++23 with GNU extensions (`-std=gnu++23`).

On Windows with Mingw-w64 you can use `w64-debug.sh` or `w64-release.sh` script, and for Linux there are `debug.sh` and `release.sh` scripts for compiling all source files. After successful compilation you can find the artifactis in `build-Debug` and `build-Release` folders.


### TGF tool

This library comes with a CLI executable `tgf` which features viewing, testing and debugging grammars written in TGF and it can also generate a C++ code from TGF grammar which is then usable in a C++ project.

More detailed information about this CLI can be found on page [`TGF tool`](docs/tgf_tool.md)

TGF is an EBNF-based form to describe grammars. Specification of the form can be found on page [`Tau Grammar Form`](docs/tau_grammar_form.md).

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

### JSON parser

[An example](examples/csv_parser9/main.cpp) of a complete JSON parser that reads a JSON element from a file, traverses the resulting tree and outputs a string representing the original JSON data.

## API

### Basic terminology

**Literal** refers to a specific value or sequence of characters that appears exactly as it is in the input or in the grammar rules. It represents a fixed, concrete element that must be matched in order for a particular rule or pattern to be satisfied.
Literal can be terminal or non-terminal.

A **terminal** literal represents a specific token or value in the input language. It corresponds to the actual elements that appear in the input stream during parsing. Terminal literals are also referred to as terminals or terminal symbols.

A **non-terminal** literal represents a syntactic category or a placeholder that can be replaced by a sequence of terminals and/or non-terminals. Non-terminal literals are used to define the structure of the language and serve as intermediate building blocks for generating valid sentences or expressions.

A **production rule** consists of two parts: a left-hand side (LHS) and a right-hand side (RHS). The left-hand side contains a single non-terminal symbol, and the right-hand side contains a sequence of terminals, non-terminals, or both. The production rule indicates that the non-terminal on the left-hand side can be rewritten or expanded into the sequence of symbols on the right-hand side.

**Grammar** is a formal system that defines the syntax and structure of a language. It consists of terminals, non-terminals, production rules and a starting symbol (which is usually a non-terminal).

**Conjunctive Grammar** is a grammar which allows usage of conjunctions of literal sequences in production rules. The conjunction acts as a logical "AND" operation, indicating that all the conjuncted literal sequences must be matched for the whole rule to be satisfied.

**Boolean Grammar** is a conjunctive grammar which also allows usage of negation of literal sequences. Negation acts as a logical "NOT" operation, indicating that if the negated literal sequence is matched the whole rule fails to be satisfied.

**DNF** or **Disjunctive Normal form** is a specific form of representing logical formulas, Boolean expressions or production rules in our case. In DNF, a logical formula is represented as a disjunction (OR) of one or more conjunctions (AND) of literals.

**EBNF** or **Extended Backus–Naur form** is a family of metasyntax notations, any of which can be used to express a context-free grammar.

**TGF** or **Tau Grammar Form** is an EBNF based form to describe grammars which are usable by this library. TGF understands EBNF syntax (+, *, [] and {}) so it is easier to define repetition or optionality.

### Namespace

Library uses an `idni` namespace for its declarations.

For simplicity, all examples in library documentation expect declaration `using namespace idni;`.

### Templating

Most of the structures provided by this library are templates to enable generic parsing of any input data while parsing any type of terminals. Usually they are declared like this:

`template <typename C = char, typename T = C> ...` where
- `C` is an input type of input data (usually characters).
- `T` is a type of a terminal.

Fully supported and tested types are `char` and `char32_t` (Unicode support). Though it is possible to supply encoder and decoder functions to your parser and these can convert input elements of any type into terminals of any type.

If `T` differs from `C` it is required to provide decoder and encoder functions in `parser::options`. `parser` uses these recoders to convert input data into terminals and back. See [recoders](docs/recoders.md) for more information.

<a name="overview-of-types"></a>

### Classes and structs

Click on the type name in the following list to see more detailed documentation of its API:
- [`nonterminals`](docs/nonterminals.md) - id map of non-terminals to their names
- [`lit`](docs/lit.md), [`lits`](docs/lits.md), [`conjs`](docs/conjs.md), [`disjs`](docs/disjs.md), [`prod`](docs/prod.md) and [`prods`](docs/prods.md) - basic building blocks for creating a grammar programmatically. `prods` represents a grammar containing production rules as disjunctions of conjunctions of sequences of literals.
- [`char_class_fn`, `char_class_fns`](docs/char_class_fns.md) - Character class function is an alternative way to create a production rule. It is a function returning true if a terminal character is a member of a character class. It can be custom or there are predefined cctype-like functions (isalnum, isdigit...) provided. `char_class_fns` is a container for such functions.
- [`shaping_options`](docs/shaping_options.md) - options for shaping a parsed tree
- [`grammar::options`](docs/grammar_options.md) - options for grammar
- [`grammar`](docs/grammar.md) - represents a grammar created from production rules `prods` and character class functions `char_class_fns`.
- [`parser::options`](docs/parser_options.md) - plain struct of options changing behavior of the parser
- [`parser`](docs/parser.md) - uses a `grammar` for parsing an input and produces a `result`
- [`parser::result`](docs/parser_result.md) - result of parsing containing the parsed forest
- [`parser::error`](docs/parser_error.md) - detailed information about a parse error
- [`parser::input`](docs/parser_input.md) - wrapper for an input data pointer, an input stream or a file. It provides an API to access it from a parser or a decoder
- [`parser::pnode`](docs/parser_pnode.md) - parser's forest node
- [`forest`](docs/forest.md) - a result of parsing containing all the parsed trees. Provides means of traversal or extraction of trees or graphs.
- [`forest::graph`](docs/forest_graph.md) - a graph extracted from a forest
- [`forest::tree`](docs/forest_tree.md) - a tree extracted from a graph
- [`tgf`](docs/tgf.md) - Tau Grammar Form parser - reads a grammar in TGF format from a string or a file. An alternative way to describe a grammar instead of creating it programatically.
- [`traverser`](docs/traverser.md) - struct for traversing and accessing rewriter trees
- [`rewriting`](docs/rewriting.md) - API for rewriting a resulting parse tree
- [`measure`](docs/measure.md) - simple struct for measuring time

### Classes and structs usable for building command line interfaces

- cli - provides CLI arguments management and processing
- repl - provides terminal (currently only Linux is supported) REPL UI
- term_colors - provides terminal color escape codes

### Functions

There are several areas covered by functions provided by this library

- [predefined character class functions](docs/charclasses.md) - cctype-like predefined character class functions
- [recoders](docs/recoders.md) - decoder and encoder for UTF-8 in `char` <-> Unicode in `char32_t`
- [UTF-8](docs/utf8.md) - UTF-8 and Unicode support
- [devhelpers](docs/devhelpers.md) - helper forest transformations to various formats (TML facts, TML rules, DOT) useful when developing a parser
