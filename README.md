# The Tau Parsing Library

The Tau parsing library is a high-performance, industrial-strength, continuously maintained parsing tool. It is a C++ library able to parse, and generate a parser to, any Context Free Grammar (CFG) and even all Boolean grammars. 

Compared to existing parsing solutions, the Tau Parsing Library stands out with it simplicity and robustness: just give it a grammar and an input string, and it will give you a representation of all possible parse trees, without any need for additional information, configuration, or callbacks that other CFG parsers commonly require you to provide.


# Generate a parser in 3 easy steps:

First, define the grammar using the Tau Grammar Format (TGF), which is very similar to EBNF.

	@use char classes space, digit.  # introduce space and digit native productions

	start => _ expr _.

	expr  =>  ( '(' _ expr _ ')'  ):group
		| ( expr _ '-' _ expr ):sub
		| ( expr _ '+' _ expr ):add
		| ( expr _ '/' _ expr ):div
		| ( expr _ '*' _ expr ):mul
		| ( '+' _ expr        ):pos
		| ( '-' _ expr        ):neg
		| ( digit+            ):number.

	_     => space*.                 # optional white space of any length



Second, use the TGF command line utility to translate your TGF file into a parser header file using just one command.

	tgf basic_arithmetic.tgf gen


Finally, include the parser header file in your C++ program and initialize your parser object. The parser object can then parse any valid input string in your grammar and return a parse forest or a syntax error.

	#include "basic_arithmetic_parser.generated.h"

	int main() {

		std::string input = "-12*5"; // input expression

		// run parser and get a parse result
		basic_arithmetic_parser::result r = basic_arithmetic_parser::instance()
						.parse(input.c_str(), input.size());

		// check if any successfully parsed tree was found
		if (!r.found) {
			std::cerr << r.parse_error << std::endl;
			return 1;
		}

		// get the parse tree from the result and print it to stdout
		r.get_tree()->to_print(std::cout);

		return 0;
	}


Above program prints:

	start(3)[0, 5]
		_(4)[0, 0]
		expr(5)[0, 5]
			mul(12)[0, 5]
				expr(5)[0, 3]
					neg(18)[0, 3]
						'-'[0, 1]
						_(4)[1, 1]
						expr(5)[1, 3]
							number(20)[1, 3]
								digit(2)[1, 2]
									'1'[1, 2]
								digit(2)[2, 3]
									'2'[2, 3]
				_(4)[3, 3]
				'*'[3, 4]
				_(4)[4, 4]
				expr(5)[4, 5]
					number(20)[4, 5]
						digit(2)[4, 5]
							'5'[4, 5]
		_(4)[5, 5]



## Features

- Boolean grammars
- C++ API (docs/programatic_grammar.md)
- [TGF](docs/tgf.md) - Tau Grammar Format for describing grammars in an EBNF-like format
- Earley based parser with garbage collection producing parse forest containing a compressed representation of all the parsed trees

- Forest traversal API with callbacks and extraction of trees and graphs to deal with transformations and with ambiguity
- `tgf` tool to test grammars in TGF file or to generate a C++ code representing a parser for a given TGF file.
- Predefined or custom classes of characters defined by a function (`<cctype>`-like functions)
- Optional [recoding](docs/recoders.md) of an input string into another type which can can be used for lexing, reencoding or accessing binary data
- [own UTF-8 support](docs/utf8.md)



## How to use

To build the library and the `tgf` tool requires CMake and a compiler supporting C++23 with GNU extensions (`-std=gnu++23`).
Then, for Windows with Mingw-w64 you can use `w64-debug.sh` or `w64-release.sh` script, and for Linux there are `debug.sh` and `release.sh` scripts for compiling all source files. After successful compilation you can find the artifacts in `build-Debug` and `build-Release` folders.


### TGF tool

This library comes with a CLI executable `tgf` which features viewing, testing and debugging grammars written in TGF and it can also generate a C++ code from TGF grammar which is then usable in a C++ project.

More detailed information about this CLI can be found on page [`TGF tool`](docs/tgf_tool.md)

TGF is an EBNF-based format to describe grammars. Description of this format can be found on page [`Tau Grammar Format`](docs/tau_grammar_format.md).

## Tutorials

### CSV parser

Beginner tutorial to quickly start using this library.

You can learn how to use this library in a CSV parser tutorial.

Part 1 shows a very simple parser with all that is required.
Each following part expands the previous one.
Read comments since they are always related to a new or a changed code.

- [part 1](examples/csv_parser1/main.cpp) - minimal parser for positive integers
- [part 2](examples/csv_parser2/main.cpp) - using predefined character classes
- [part 3](examples/csv_parser3/main.cpp) - refactoring parser into its components
- [part 4](examples/csv_parser4/main.cpp) - also parse negative integers
- [part 5](examples/csv_parser5/main.cpp) - parse strings and nulls
- [part 6](examples/csv_parser6/main.cpp) - parsing comma separated values
- [part 7](examples/csv_parser7/main.cpp) - parsing new line separated rows
- [part 8](examples/csv_parser8/main.cpp) - replacing the programmatically created grammar with a grammar in TGF
- [part 9](examples/csv_parser9/main.cpp) - using EBNF syntax in TGF
- [part 10](examples/csv_parser10/main.cpp), [csv.tgf](examples/csv_parser10/csv.tgf)
- using tgf tool to generate a parser from a TGF file

### JSON parser

[An example](examples/csv_parser9/main.cpp) of a complete JSON parser that reads a JSON element from a file, traverses the resulting tree and outputs a string representing the original JSON data.

## API

### Basic terminology

The following terminology is standard:

**Literal** is either a terminal or a non-terminal.

A **terminal** literal is a fixed single character (from the chosen alphabet).

A **non-terminal** literal represents a syntactic category or a placeholder that can be replaced by a sequence of terminals and/or non-terminals. Non-terminal literals are used to define the structure of the language.

A **production rule** consists of two parts: a left-hand side (LHS) and a right-hand side (RHS). The left-hand side contains a single non-terminal symbol, and the right-hand side contains a sequence of terminals and non-terminals. The production rule indicates that the non-terminal on the left-hand side can be rewritten or expanded into the sequence of symbols on the right-hand side.

**Grammar** is a formal system that defines a set of strings (a language). It consists of terminals, non-terminals, production rules and a starting symbol (which is a non-terminal).

**Conjunctive Grammar** is a grammar which allows usage of conjunctions of literal sequences in production rules. The conjunction acts as a logical "AND" operation, indicating that all the conjuncted literal sequences must be matched for the whole rule to be satisfied.

**Boolean Grammar** is a conjunctive grammar which also allows usage of negation of literal sequences. Negation acts as a logical "NOT" operation, indicating that if the negated literal sequence is matched the whole rule fails to be satisfied.

**DNF** or **Disjunctive Normal form** is a specific form of representing logical formulas, Boolean expressions or production rules in our case. In DNF, a logical formula is represented as a disjunction (OR) of one or more conjunctions (AND) of literals.

**EBNF** or **Extended Backus–Naur form** is a family of metasyntax notations, any of which can be used to express a context-free grammar.

**TGF** or **Tau Grammar Format** is an EBNF based format to describe grammars which are usable by this library. TGF understands EBNF-like syntax (+, *, [] and {}) so it is easier to define repetition or optionality.

### Namespace

The library uses an `idni` namespace for its declarations.

For simplicity, all examples in library documentation expect declaration `using namespace idni;`.

### Templates

Most of the structures provided by this library are templates in order to support arbitrary alphabets (terminals) as well as to generalize the concept of "string". Usually they are declared like this:

`template <typename C = char, typename T = C> ...` where
- `C` is the input type of the input data (usually characters).
- `T` is the type of terminals.

Fully supported and tested types are `char` and `char32_t` (including Unicode). Though it is possible to supply encoder and decoder functions to the parser to convert input elements of any type into terminals of any other type.

If `T` differs from `C` it is required to provide decoder and encoder functions in `parser::options`. `parser` uses these decoders to convert input data into terminals and back. See [recoders](docs/recoders.md) for more information.

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
- [`tgf`](docs/tgf.md) - Tau Grammar Format parser - reads a grammar in TGF format from a string or a file. An alternative way to describe a grammar instead of creating it programmatically.
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
