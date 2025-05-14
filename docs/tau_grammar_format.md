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

Terminal character is quoted in apostrophes: `'t'`, `'0'`, `'$'`.
```
dot      => '.'.
new_line => '\n'.
```

Sequence of terminal characters can be provided as a quoted string: `"Hi"`, `"function"`, `"DEFINE"`.

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

Negation of a literal sequence `~` makes the literal sequence to satisfy its rule only if it does not match the input. If it matches it won't satisfy it.

Following example shows that `identifier` is satisfied if an input matches `chars` but not `keyword`. In other words if an input matches a `keyword` it isn't an `identifier`.
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

#### @use

This directive is supposed to load rules from other grammars or predefined rule lists.
Currently it takes comma separated list of predefined character class function names used after a keyword `char class` or `char classes`.

Usable names are the same names usable in `predefined_character_classes()` (see [character class functions](char_class_fns.md) for more information).

It can be one of: `alnum`, `alpha`, `blank`, `cntrl`, `digit`, `graph`, `lower`, `printable`, `punct`, `space`, `upper`, `xdigit` or `eof`.

Usage:
```
@use char class space.
@use char classes alpha, digit, space.
```

#### @start

Start directives tells parser what is the starting symbol. Allows to pick another symbol as a starting one instead of a default `start`.

Usage:
```
@start S.
S => S S | S | '1'.
```

#### @disable

Disable directive can disable a grammar option.

Currently, only `auto_disambiguate` is available for disabling.

Usage:
```
@disable disambiguation.
```


#### @ambiguous

Accepts a comma separated list of symbols which are not disambiguated according to production rule order of grammar appearance.

`@ambiguous symbol1, symbol2.`


#### @inline

Inlining is a parsed tree shaping action which takes a node and replaces it with children or with a child from its subtree.

It accepts comma separated arguments which can be one of:
- single symbol name which is to be replaced with all of its children
- keyword `char classes` to inline all char class functions
- treepath: in a format `symbol > child_symbol > grand_child_symbol` denoting a path in a tree which has to match to replace `symbol` with a `grand_child_symbol`

Usage:
```
@inline char classes, chars, src, expr > block > expr.
```


#### @trim

Trim is a parsed tree shaping action which removes a node and its subtree from the parsed tree.

Usage:
```
@trim white_space, comment.
```


#### @trim children

Trim children removes children and its subtrees from a node.

Usage:
```
@trim children if_kw, then_kw, else_kw.
```


#### @trim all terminals

This directive removes all terminals from the parsed tree.

Usage:
```
@trim all terminals.
```


#### @trim children terminals

This directive removes terminal children from nonterminals provided as a comma separated list.

Usage:
```
@trim children terminals statement, directive.
```
