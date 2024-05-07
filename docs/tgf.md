[back to index](../README.md#overview-of-types)

# tgf

```
template <typename C = char, typename T = C> struct tgf;
```

The simplest way to create a grammar is to use Tau Grammar form which is an EBNF-like language for describing grammars.

Library implements a TGF parser which can parse TGF from a string or from a file and it transforms the parsed forest into a `grammar` which can be used by a `parser`.

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
auto f = p.parse("hi");    // p.found() == true
     f = p.parse("hello"); // p.found() == true
     f = p.parse("table"); // p.found() == false
```

## tgf tool

This library provides a command line tool `tgf` which takes a TGF file and allows to do several things:
- generates a parser in C++.
- runs a parser for a given input string
- displays information about a grammar

```
tgf <tgf file> [ <command> [ <command options> ] ]
```

where commands are:
```
	gen		generate a parser code
	parse		parse an input string, file or stdin
	show		show information about grammar / parser
```

options for `gen` command:
```
tgf <tgf file> gen [ <options> ]
	--char-type        -C      type of input character
	--decoder          -d      decoder function
	--encoder          -e      encoder function
	--help             -h      detailed information about options
	--name             -n      name of the generated parser struct
	--output           -o      output file
	--start            -s      starting literal
	--terminal-type    -T      type of terminal character
```

options for `parse` command:
```
tgf <tgf file> parse [ <options> ]
        --char-type        -C      type of input character
        --detailed-error   -d      parse error is more verbose
        --grammar          -g      prints grammar
        --help             -h      detailed information about options
        --input            -i      parse input from file or STDIN if -
        --input-expression -e      parse input from provided string
        --print-ambiguity  -a      prints ambiguity info, incl. ambig. nodes
        --print-graphs     -p      prints parsed graph
        --print-input      -I      prints input
        --root-cause-error -D      parse error is more verbose
        --start            -s      starting literal
        --terminal-type    -T      type of terminal character
        --terminals        -t      prints all parsed terminals serialized
        --tml-facts        -f      prints parsed graph in tml facts
        --tml-rules        -r      prints parsed graph in tml rules
```

Options for `show` command:
```
tgf <tgf file> show [ <options> ]
        --char-type        -C      type of input character
        --grammar          -g      prints grammar
        --help             -h      detailed information about options
        --start            -s      starting literal
        --terminal-type    -T      type of terminal character
```

For more information about decoder and encoder functions see [recoders](recoders.md)

## TGF syntax

TGF uses statements ending with a dot `.`. Statement of TGF can be either a production rule or it can be a directive `@use char classes`.

### production rules

Left side is a nonterminal name. Right side are literals to match with an input to satisfy the rule.

Examples:
```
binary_digit => '0'.
binary_digit => '1'.
number       => binary_digit+.
expression   => number op number.
expression   => number.
op           => '&'.
op           => '|'.
```

### terminals

Terminal character is quoted in apostrophes: `'t'`, `'0'`, `'$'`.
```
new_line => '\n'.
```

Sequence of terminal characters can be provided as a quoted string: `"Hi"`, `"function"`, `"DEFINE"`.

```
rw_function => "function".
```
is equivalent to:
```
rw_function => 'f' 'u' 'n' 'c' 't' 'i' 'o' 'n'.
```

### null

Null terminal has a special keyword `null`. It means that a production rule can be satisfied if the match is empty, ie. the parsed item can have 0 length.

With null and recursion we can say that a `program` consists of `statements` and `statements` can be empty (`null`) or any number of statements (`statements` are repeated after a `statement` but again they can be `null`).
```
program    => statements.
statements => statement statements.
statements => null.
```

`null` cannot be part of a literal sequence. It represents the whole body (right) part of the rule. Following rules make no sense:
```
wrong_usage0 => expression null.
wrong_usage1 => null keyword.
wrong_usage2 => identifier null variable.
```


### disjunction

If a non-terminal has multiple production rules it can be written as a single rule with disjunction. Character `|` is used:

```
statement  => rule.
statement  => directive.
statements => statement statements.
statements => null.
```
is equivalent to:
```
statement  => rule | directive.
statements => statement statements | null.
```

### conjunction

When using conjunction `&` between literal sequences it is required that both literal sequences match the input to satisfy the rule.

In this example an input must match both `alnum` and `alpha` to satisfy `letter`:
```
letter => alnum & alpha.
```

### negation

Negation of a literal sequence `~` makes the literal sequence to satisfy its rule only if it does not match the input. If it matches it won't satisfy it.

Following example shows that `identifier` is satisfied if an input matches `chars` but not `keyword`. In other words if an input matches a `keyword` it isn't an `identifier`.
```
identifier => chars & ~keyword.
```

### EBNF syntax

TGF supports `{ }` for zero or more occurrences, `[ ]` for optional occurance, and `( )` for grouping terms. Further there are postfix operators `*` (alternative to `{ }`) and `+` for at least one occurance.

#### { } and *

Literals surrounded with `{ }` or a literal (or group `( )` of literals) followed by `*` can occur any times or does not have to occur.

For example a `white_space` can be any number of `space` or nothing
```
white_space => { space }.
# or alternatively
white_space => space*.
```

Non-EBNF equivalent:
```
white_space => spaces.
spaces      => space spaces | null.
```


#### [ ]

Literals surrounded with `[ ]` are considered optional. They can occur but they do not have to.

End of line `eol` can be just `\n` or it can be `\r\n`
```
eol => [ '\r' ] '\n'.
```
Non-EBNF equivalent:
```
eol => '\r' '\n' | '\n'.
```

#### +

Literal or a group followed by `+` means it can be repeated.

```
digits     => digit+.
bin_digits => ( '0' | '1' )+.
```
Non-EBNF equivalent:
```
digits          => digit digits_rest.
digits_rest     => digit digits_rest | null.
bin_digit       => '0' | '1'.
bin_digits      => bin_digit bin_digits_rest.
bin_digits_rest => bin_digit bin_digits_rest | null.
```

#### ( )

Parenthesis `( )` are used to group literals into an element where we can apply `*` or `+` postfix operators. Since this grouping creates a temporary production rule for souch a group, it is possible to use disjunction/conjunction and negation.

```
program    => (directive | declaration | definition | call)*.
bin_number => ('0' | '1')+.
```
Non-EBNF equivalent:
```
program    => statements.
statements => statement statements | null.
statement  => directive | declaration | definition | call.
bin_number => bin_digit bin_digits.
bin_digits => bin_digit bin_digits | null.
bin_digit  => '0' | '1'.
```

### @use char classes \<list of names>.

This directive takes comma separated list of predefined character class function names.

Usable names are the same names usable in `predefined_character_classes()` (see [character class functions](char_class_fns.md) for more information).

It can be one of: `alnum`, `alpha`, `blank`, `cntrl`, `digit`, `graph`, `lower`, `printable`, `punct`, `space`, `upper`, `xdigit` or `eof`.

Usage:
```
@use char classes alpha, digit, space.
```