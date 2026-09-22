[back to index](../README.md#tgf-tool)

# TGF test files

A `.tgf.test` file lists inputs for a grammar and asserts the shape of each parse tree. The [`tgf`](tgf_tool.md) tool runs a `.tgf.test` file against a grammar with its `test` command.

## Running

```
tgf <grammar.tgf> test [--productions a,b] <file.tgf.test>
```

`--productions` sets the enabled guard set for the grammar, the same option the `parse`, `repl`, and `gen` commands accept.

The tool prints one line per input, then a summary line for the file. Real output from `tests/ambig_bc.tgf.test` against `tests/fixtures/ambig_bc.tgf`:

```
running test: ambig_bc.tgf.test
opening file: ambig_bc.tgf.test
	"x"		OK
	"x"		OK
	"x"		OK (negated)
	"x"		OK (negated)
	"x"		OK (negated)
	"x"		OK (negated)
	"x"		OK
	"x"		OK
	"x"		OK
	"x"		OK (negated)
10 passed, 0 failed
```

`OK` marks an input that parses and matches its matcher. `OK (negated)` marks a `~` row that correctly fails to parse or fails to match. A row that neither parses nor matches prints one of a few `FAIL` forms:

```
	"y"		FAIL (parse error: Syntax Error: Unexpected 'y' at 1:1 (1): y)
	"a,b"		FAIL (tree shape mismatch)
	"x"		FAIL (negated, but parsed and matched)
```

Every `FAIL` line increases the file's failed count and sets the process exit code to `1`. The command takes several files. After the last file, it prints a combined line:

```
total: 20 passed, 0 failed
```

The exit code is `0` only when every file reports zero failed rows.

## File syntax

An entry pairs a matcher with a list of inputs:

```
matcher : input, input, input.
```

A `#` starts a comment that runs to the end of the line. An entry needs no inputs at all, a bare `sym .` still resolves `sym` against the grammar but asserts nothing:

```
A.
```

An input is a bare name or a quoted string. A bare name holds only letters, digits, and underscores, and the tool feeds that text to the parser verbatim:

```
start : true, false, null.
```

A quoted string allows any character between `"` and `"`, plus these escapes: `\"` `\\` `\/` `\a` `\b` `\f` `\n` `\r` `\t` `\v` `\xHH` `\uHHHH` `\UHHHHHHHH`.

```
start > file > record > escaped > ^ 'b' escaped_quote 'c' $ : "\"b\"\"c\"".
```

The start symbol of a top-level entry comes from its matcher. The tool reads the matcher's leading identifier, skipping an optional root anchor `/`, and looks it up in the grammar. In `start > file > record > "abc" : "abc".` the start symbol is `start`. An unknown leading name fails that entry.

## Matchers

A matcher is [treemr](../src/format/treemr/README.md) pattern text, read verbatim and compiled once per distinct matcher string. See the treemr README for the full pattern language. These are the forms used most in the suites, one real example each.

| form | meaning | example |
|------|---------|---------|
| `a > b` | `b` is a direct child of `a` | `A > B : "x" @any.` |
| `a >> b` | `b` sits anywhere below `a` | `start >> number : "0".` |
| `^ ... $` | pin the sequence to the first and last child | `start >> number > ^ '0' $ : "0".` |
| `%` | any single node | `A > (%) : "x" @all.` |
| `'c'` | a node whose collected text is `c` | `start > file > record > escaped > ^ 'a' ',' 'b' $ : "\"a,b\"".` |
| `"text"` | a node whose collected text is `text` | `start > file > record > "abc" : "abc".` |
| `x!` | `x` has no children | `start > file > record > escaped! : "\"\"".` |
| `(a \| b)` | either `a` or `b` matches | `A > (B \| C) : "x" @any.` |
| `x*` `x+` `x?` | zero or more, one or more, zero or one | `start > file > record > non_escaped* : "a,b,c".` |

## Negation

A `~` before an entry or before one input inverts that row's verdict. `~` passes when the input fails to parse, or when it parses but its matcher does not match:

```
~start > file > header : "a,b\r\n1,2".
A > B : ~"x".
```

A `~` before an entry reaches every item and nested entry under it. A `~` before one input reaches only that input.

`~` never turns a setup failure into a pass. These still fail, negated or not:
- a matcher that fails to compile as treemr DSL text
- a tree root missing after shaping (the input needs `@raw`)
- a bad escape sequence inside a quoted input
- conflicting ambiguity mode words on one entry or item

## Words

A word picks how the matcher treats an `__AMB__` ambiguity wrapper, or asks for the tree before shaping:

- `@forbid` (the default): the match root, and every node below it, must hold no ambiguity wrapper.
- `@any`: the matcher passes if at least one alternative under a wrapper satisfies it.
- `@unique`: the matcher passes if exactly one alternative under a wrapper satisfies it.
- `@all`: the matcher passes only if every alternative under a wrapper satisfies it.
- `@raw`: match the tree before `@trim` and `@inline` run, so a node those directives would remove still shows up.

A word after an input applies to that input. A word before an entry's matcher applies to the whole entry, and every item and nested entry under it. An item's own word adds to its entry's, and a mode word overrides the entry's mode. Two conflicting mode words on the same entry or item fail that row.

`tests/ambig_bc.tgf.test` exercises every mode:

```
A > B : "x" @any.
A > C : "x" @any.
A > B : ~"x" @all.
A > (%) : "x" @all.
A > B : "x" @unique.
```

`@raw` shows a node that shaping removes. `csv.tgf` trims `SEP`, so the shaped tree never has one, but the raw tree still does:

```
@raw start >> SEP : "a,b".
```

## Nesting

A `{ }` block after a matcher opens nested entries. Each nested entry extends the enclosing matcher with ` > ` and its own path. From `tests/format/json/json.tgf.test`:

```
start > value {
	true_sym!  : true.
	false_sym! : false.
	null_sym!  : null.
}
```

An item can also carry its own matcher after `input : matcher`. That matcher extends the entry's matcher with ` > `, one pairing per item:

```
start > value : true : true_sym, false : false_sym, null : null_sym.
```

Both forms reach the same result here: `start > value > true_sym` holds for input `true`, and so on for `false` and `null`.

## Running through ctest

`TAU_RUN_GRAMMAR_TESTS` is a CMake option, off by default, that registers the `.tgf.test` suites as ctest tests. Turning it on also forces `TAU_PARSER_BUILD_TGF` on, so it works without `TAU_PARSER_BUILD_TESTS`.

The `grammar-tests` preset turns it on by itself. Every `*-tests` preset (`release-tests`, `debug-tests`, and so on) turns it on too, alongside the C++ test suite. Every `*-all` preset turns it on as part of `TAU_PARSER_BUILD_ALL`.

```
cmake --preset grammar-tests
cmake --build --preset grammar-tests
ctest --preset grammar-tests --timeout 120
```

The preset registers nine suites:

```
tgf.test.tgf.test
treemr.tgf.test
ambig_bc.tgf.test
csv_rfc4180.tgf.test
csv_tab.tgf.test
csv_lf.tgf.test
csv_tab_lf.tgf.test
csv_header.tgf.test
json.tgf.test
100% tests passed out of 9
```

`tests/grammar_tests.cmake` lists every suite as one `add_test` entry. Add a suite with one more entry in the same file:

```
add_test(NAME my_format.tgf.test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/src/format/my_format/my_format.tgf" test
	"${PROJECT_SOURCE_DIR}/tests/format/my_format/my_format.tgf.test")
```
