[back to index](../README.md#tgf---tau-grammar-form)

# Tau Grammar Format

TGF is a format for describing grammars. This library also provides a [`tgf`](tgf.md) parser, [`tgf tool`](tgf-tool.md) CLI for viewing, testing and debugging TGF grammars and generating C++ parsers.

## TGF syntax

TGF uses statements ending with a dot `.`

Statement can be either a production rule or it can be a directive.


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

Put a terminal character in apostrophes: `'t'`, `'0'`, `'$'`.
```
dot      => '.'.
new_line => '\n'.
```

Put a sequence of terminal characters in double quotes: `"Hi"`, `"function"`, `"DEFINE"`.

```
kw_function => "function".
```
is equivalent to:
```
kw_function => 'f' 'u' 'n' 'c' 't' 'i' 'o' 'n'.
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

Negation of a literal sequence `~` makes the literal sequence to satisfy its rule only if it does not match the input. If it matches, it does not satisfy the rule.

Following example shows that `identifier` is satisfied if an input matches `chars` but not `keyword`. If an input matches a `keyword`, it is not an `identifier`.
```
identifier => chars & ~keyword.
```

### EBNF syntax

TGF supports `{ }` for repeated occurrences, `[ ]` for optional occurance, and `( )` for grouping alternations. Further there are postfix operators `*` (alternative to `[{ }]`), `+` (alternative to `{ }`) and `?` (alternative to `[ ]`).

#### ( )

Parenthesis `( )` are used to group literals into an element where we can apply `*` or `+` postfix operators. Since this grouping creates a temporary production rule for souch a group, it is also possible to use disjunction/conjunction and negation insied the group.

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


#### [ ] or ?

Literals surrounded with `[ ]` or a literal or group `(...)` followed by `?` are considered optional. They can occur but they do not have to.

End of line `eol` can be just `\n` or it can be `\r\n`:
```
eol              => ['\r'] '\n'.
eol              => '\r'? '\n'.
```
Symbol for disambiguation can be "disambig", "auto-disambig", "disambiguation", "autodisambiguation":
```
sep              => '-' | '_' | space | null.
disambig_symbol  => ["auto" '-'?] "disambig" "uation"?.
```
Non-EBNF equivalent:
```
eol              => '\r' '\n' | '\n'.
disambig_symbol  => disambig_prefix "disambig" disambig_postfix.
disambig_prefix  => "auto" sep | null.
sep              => '-' | null.
disambig_postfix => "uation" | null.
```

#### { } or +

Literals surrounded with `{ }` or a literal or group `(...)` followed by `+` means it can be repeated.

```
white_space => { space }.
digits      => digit+.
bin_digits  => ('0' | '1')+.
```
Non-EBNF equivalent:
```
white_space     => space | white_space space.
digits          => digit digits_rest.
digits_rest     => digit digits_rest | null.
bin_digit       => '0' | '1'.
bin_digits      => bin_digit bin_digits_rest.
bin_digits_rest => bin_digit bin_digits_rest | null.
```

#### [{ }] or *

Literals surrounded with `[{ }]` or a literal or group `(...)`  followed by `*` can occur any times or does not have to occur.

For example a `white_space` can be any number of `space` or nothing
```
optional_white_space => [{ space }].
# or alternatively
optional_white_space => space*.
```

Non-EBNF equivalent:
```
@use char class space.
white_space => space | white_space space | null.
```

### shorthand rules

Shorthand rules are a way to write rules in a more compact way.

Using syntax `:symbol_name` after a factor of a rule creates a new rule `symbol_name => factor.` and uses the symbol_name in the rule instead of the factor.

Usage:
```
bf_cb_args1    => __ bf:bf_cb_arg.

term           => ('(' _ alternation _ ')'):group
                | ('[' _ alternation _ ']'):optional_group
                | ('{' _ alternation _ '}'):repeat_group
                |                           terminal
                |                           sym.
```
is equivalent to:
```
bf_cb_args1    => __ bf_cb_arg.
bf_cb_arg      => bf.

term           => group | optional_group | repeat_group | terminal | sym.
group          => '(' _ alternation _ ')'.
optional_group => '[' _ alternation _ ']'.
repeat_group   => '{' _ alternation _ '}'.

```

### directives

#### `@use`

This directive loads rules from other grammars or predefined rule lists.
Currently it takes comma separated list of predefined character class function names used after a keyword `char class` or `char classes`.
It writes the grammar option `char-classes`, with one entry `class` or `class as name` for each class.

Usable names are the same names usable in `predefined_character_classes()` (see [character class functions](char_class_fns.md) for more information).

It can be one of: `alnum`, `alpha`, `blank`, `cntrl`, `digit`, `graph`, `lower`, `printable`, `punct`, `space`, `upper`, `xdigit`, `eof`, `any` or `ascii`.
These plain classes answer as `<cctype>` in the C locale, also for `char32_t` terminals.

For Unicode classes, use the same names with the prefix `unicode_`: `unicode_alnum`, `unicode_alpha`, `unicode_blank`, `unicode_cntrl`, `unicode_digit`, `unicode_graph`, `unicode_lower`, `unicode_printable`, `unicode_punct`, `unicode_space`, `unicode_upper` or `unicode_xdigit`.
For `char32_t` terminals they use the Unicode tables (see [charclasses](charclasses.md)).

Usage:
```
@use char class space.
@use char classes alpha, digit, space.
```

`<class> as <name>` renames a class in the grammar.
The new name is the only name of the class in that grammar.
```
@use char classes alpha as letter, digit.
word => letter+.
```

The new name can be the name of a predefined class.
All renames of a grammar apply at the same time, so two classes can swap their names.
The left side of `as` is always a predefined class.
```
@use char classes digit as d, alpha as digit.
```
Here `digit` is the class `alpha`, and `d` is the class `digit`.

These are errors at grammar load:
- A rule uses the old name of a renamed class, for example `alpha` after `alpha as letter`.
- The old name occurs again in the list, for example `alpha as letter, alpha` or `alpha as a, alpha as b`.
- Two classes get the same name, for example `digit, alpha as digit`.
- One class occurs two times in the list, for example `digit, digit`.

Each error names the class name.
A rule that uses this name gets no other warning.
The generated parser, `@inline char classes`, the syntax highlighting and the `tgf` tool use only the new name.
The generated parser gives the renames to `predefined_char_classes` as a map, see [character class functions](char_class_fns.md).
The syntax highlighting reads the name in the grammar.
So `digit as num` does not show as a number.

A rename gives a Unicode class a short name:
```
@use char classes unicode_printable as printable, digit as ascii_digit, unicode_digit as digit.
comment => '#' printable* '\n'.
```
Here `digit` is the Unicode class and `ascii_digit` is the `<cctype>` class.

#### `@start`

Start directives tells parser what is the starting symbol. Allows to pick another symbol as a starting one instead of a default `start`.
It writes the grammar option `start`. The default of the option is `start`.

Usage:
```
@start S.
S => S S | S | '1'.
```

#### `@disable`

Disable directive can disable a grammar option.

Currently, only `auto_disambiguate` is available for disabling.
`@disable disambiguation` writes the grammar option `auto-disambiguate`. `@disable x` removes the guard `x` from the grammar option `enabled-productions`, and `@enable x` adds it.

Usage:
```
@disable disambiguation.
```


#### `@ambiguous`

Accepts a comma separated list of symbols which are not disambiguated according to production rule order of grammar appearance.
It writes the grammar option `nodisambig-list`.

`@ambiguous symbol1, symbol2.`


#### `@dynamic`

A `@dynamic` nonterminal takes its alternatives from the host program at run time, not from the grammar file. Each alternative is a plain terminal string.
The directive writes the grammar option `dynamic`, with one entry `name=value` for each value, or `name=` for a name with no value.

A `defaults` clause after the nonterminal name sets initial values. `,` separates the values inside one `defaults` clause. `;` separates several `@dynamic` declarations on one line. A value is a symbol or a quoted string, and a quoted string may hold an escape.
A default value has one character or more. An empty value, for example `""`, is a load error.

A `@dynamic` nonterminal with no values matches nothing. This is not an error and produces no warning.

Add values at run time with `grammar::add_dynamic` (see [grammar methods](grammar.md#methods)).

A cast like `(u8[4])x` is ambiguous with an indexed variable unless the grammar lists every type name in advance. `@dynamic type_name.` removes that requirement. The parser accepts the cast only for a type name the host adds later.

Usage:
```
@dynamic type_name.
@dynamic ba_type defaults sbf, tau.
@dynamic ba_type defaults tau, sbf; a.
@dynamic ba_type defaults "a b", sbf.
```


#### `@variants` and `@variant`

A variant is a named profile of production guards.
`@variants <name>: <guard>, <guard>, ...; <name>: <guard>, ... .` defines one profile per pair. It writes the grammar option `variants`, with one entry `name=guard` for each guard.
`@variant <name>.` selects a profile. It writes the grammar option `variant`, and the guards of the profile become the grammar option `enabled-productions`.

A name is one symbol, and it may hold `_`. The first profile of the file is the default, unless `@variant` selects another one. A second `@variants` adds profiles. A second definition of a name is a load error, and so is a pair without a colon or without a guard. Two profiles may have the same guards. A selection of a name that no profile has is an error, and the guards do not change.

A later write of `enabled-productions` keeps `variant` when the selected profile has the same guards. Else it sets `variant` to the first profile in name order with the same guards, or to empty when no profile has them.
The selected or default profile sets the guards after the `@enable` and `@disable` lines of the file, whatever their order.
A generated parser keeps the profiles and the selected name in its grammar options, so a host can select a profile by name.

Usage:
```
@variants rfc4180: comma, crlf, no_header;
	tab_lf: tab, lf, no_header.
@variant tab_lf.
```

The `tgf` tool selects a variant with `--variant <name>` and with the REPL command `set variant <name>`.

#### `@inline`

Inlining is a parsed tree shaping action which takes a node and replaces it with children or with a child from its subtree.
It writes the grammar options `inline` and `inline-char-classes`.

It accepts comma separated arguments which can be one of:
- single symbol name which is to be replaced with all of its children
- keyword `char classes` to inline all char class functions
- treepath: in a format `symbol > child_symbol > grand_child_symbol` denoting a path in a tree which has to match to replace `symbol` with a `grand_child_symbol`

Usage:
```
@inline char classes, chars, src, expr > block > expr.
```


#### `@trim`

Trim is a parsed tree shaping action which removes a node and its subtree from the parsed tree.
The root of the parsed tree is never removed, also when `@trim` names it.
It writes the grammar option `trim`.

Usage:
```
@trim white_space, comment.
```


#### `@trim children`

Trim children removes children and its subtrees from a node.
It writes the grammar option `trim-children`.

Usage:
```
@trim children if_kw, then_kw, else_kw.
```


#### `@trim all terminals`

This directive removes all terminals from the parsed tree.
It writes the grammar option `trim-terminals`. The names after `except children of` go to the grammar option `dont-trim-terminals-of`.

Usage:
```
@trim all terminals.
```


#### `@trim children terminals`

This directive removes terminal children from the nonterminals in a comma separated list.
It writes the grammar option `trim-children-terminals`.

Usage:
```
@trim children terminals statement, directive.
```
