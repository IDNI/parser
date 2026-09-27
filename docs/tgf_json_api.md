# tgf JSON API

`tgf <grammar> repl --json` turns the TGF REPL into a request and
response API on stdin and stdout. One JSON object goes on each line.
`tgf <grammar> parse --json`, `grammar --json` and `gen --json` run one
command and print one response.

Each message ends with one `\n`, on every platform. A request line may
end with `\r\n`, and CR counts as whitespace wherever the JSON reader
takes whitespace. A request line is limited to 16 MiB; a longer line is
answered with an error response, its tail is dropped up to the next
newline, and the loop goes on.

The client never parses text that a human reads. Every response is one
line, compact JSON, with no ANSI color codes. Every response carries
`state`, the current grammar file, start symbol and character class
nonterminals:
`{"grammar":"g.tgf","start":"start","char_classes":[7,9]}`. An `eval`
response carries it once, on the top object. `char_classes` lists the
nonterminal ids that the local printer colors as character classes; a
client colors a name in the list with the character class color and the
rest with the nonterminal color. The list is empty with no grammar. The
exception is an answer the server builds itself: a server error such as
an unknown session or a line over the limit, and the answer to
`end-session`. Those carry no `state`.

Two JSON Schema files describe this API:
`src/format/tgf_api.json/tgf_api.schema.json` covers every request and
response, and `src/format/json/report.schema.json` covers the report
tree. The parse tree uses `src/format/ast.json/ast.schema.json`.

## Request forms

Two forms reach the same command handlers.

The `eval` form takes REPL text:

    {"id":1,"cmd":"eval","src":"set trim a, b . get trim"}

The `src` text is TGF source, so a path in a `parse file` or `load`
command takes TGF string escapes: one `\` is written `\\`. The
structured form takes the path as a plain JSON string and needs no
escape.

A structured form names the command and its fields:

    {"id":2,"cmd":"set","option":"trim","value":["a","b"]}
    {"id":3,"cmd":"parse","input":"1+2"}
    {"id":4,"cmd":"load","name":"g.tgf","source":"start => 'a'."}
    {"id":5,"cmd":"hello"}

`load` takes either `file`, the path of a grammar file, or `name` and
`source`, where `name` is a label and `source` is the grammar text.
`hello` asks for a fresh `hello` object.

`id` is optional and is echoed in the response. A structured request
becomes canonical REPL text and runs through the same path as `eval`,
except `load` with `source` and `hello`, which the front end answers
directly.

Structured fields:

| cmd | fields |
|---|---|
| `parse` | `input` (string) |
| `parse file` | `file` (string) |
| `load` | `file` (string), or `name` and `source` (strings) |
| `start`, `internal-grammar`, `unreachable` | `symbol` (string, optional) |
| `help` | `command` (string, optional) |
| `get` | `option` (string, optional) |
| `set` | `option`, `value` (bool, string, string array, or array of string arrays) |
| `toggle`, `enable`, `disable` | `option` |
| `add`, `delete` | `option`, `value` (array) |
| `grammar`, `reload`, `version`, `license`, `quit`, `clear`, `hello` | none |

An empty array for `set` becomes `set <option>`, which clears a list or
tree path option. Every option name is checked against the
option table, every symbol against `[A-Za-z_][A-Za-z0-9_]*`, every
`help` argument against the command names, and `error-verbosity` against
its three values.

## Response shape

At start the server writes one `hello` line:

    {"hello":{"protocol":1,"version":"...","grammar":"g.tgf",
      "fixed_grammar":false,"start":"start","options":{...},
      "directives":{...},"report":{...}},
     "state":{"grammar":"g.tgf","start":"start","char_classes":[7]}}

`options` holds all 24 REPL option values. `directives` holds what the
grammar file says in its `@` directives, or `null` with no grammar. Its
shape is below. `report` holds the diagnostic
report of the grammar load. When the evaluator has a session id (set by
`--session <id>`), `hello` also carries `session`, the id a client uses
to attach again. The request `{"cmd":"hello"}` answers with a fresh
`hello` object in the same shape, with the request `id`, `cmd` and
`status` echoed:

    {"id":5,"cmd":"hello","status":"ok","hello":{...},
     "state":{"grammar":"g.tgf","start":"start","char_classes":[7]}}

An `eval` response carries one entry per statement:

    {"id":1,"status":"ok","results":[
      {"cmd":"set","status":"ok","result":{...},"report":{...}},
      {"cmd":"get","status":"ok","result":{...},"report":{...}}],
     "state":{"grammar":"g.tgf","start":"start","char_classes":[7]},
     "report":{...}}

A structured request carries one result:

    {"id":2,"cmd":"set","status":"ok","result":{...},
     "state":{"grammar":"g.tgf","start":"start","char_classes":[7]},
     "report":{...}}

An error before a command runs (invalid JSON, missing `cmd`, a missing or
wrongly typed field, an unknown command, a failed check) has no `result`:

    {"id":null,"status":"error",
     "state":{"grammar":"g.tgf","start":"start","char_classes":[7]},
     "report":{...}}

`status` is one of `ok`, `error`, `incomplete` and `quit`. `incomplete`
means the request text ends inside a command; the client sends the full
text again. `quit` means the loop stops after the response.

The top `status` of an `eval` response is `quit` when a statement has
`quit`, and the loop stops. Else it is `error` when a statement has
`error`. Else it is `ok`. A request text that ends inside a command
gives the top `status` `incomplete` and no entries.

## The report

A report is a nested tree. Each node carries its `children`, not a
parent index; `children` is absent when a node has none.

    {"nodes":[{"tag":32771,"message":"Time.","key":"parse","value":120,
      "attrs":[{"key":"name","value":"tok"}],"children":[...]}]}

A text label attribute holds a JSON string; a numeric attribute holds a
number. `message` is the code name and is present in API output.

## Command data

| Command | `result` data |
|---|---|
| `parse`, `parse file` | `{"input"?,"ambiguous"?,"terminals"?,"tml_rules"?,"tml_facts"?,"tree"?}` |
| `grammar` | `{"file":"...","source":"..."}` |
| `internal-grammar` | `{"start":"...","productions":["..."],"production_ids":[{...}]}` |
| `start` | `{"start":"..." or null,"changed":true}` |
| `unreachable` | `{"symbol":"...","productions":["..."],"production_ids":[{...}]}` |
| `load`, `reload` | `{"grammar":"...","loaded":true,"directives":{...}}` |
| `help` | `{"command":"...","text":"..."}` |
| `version` | `{"version":"..."}` |
| `license` | `{"license":"..."}` |
| `quit` | `{}` with status `quit` |
| `clear` | `{}` |
| `get` | `{"options":{"name":value,...}}` or `{"option":"name","value":v}` |
| `set`, `toggle`, `enable`, `disable`, `add`, `delete` | same as `get` for that option |

Option values are a bool, an array of strings, an array of string arrays
(tree paths), a string for `error-verbosity`, or a symbol for `start`.
Option names are the long names from `tgf_repl.tgf`.
`derive-char-classes` (`dcc`) is a bool option: true scans one-character
rules as character classes. `start` is the start symbol; `set start foo`
does the same as the `start foo` command. `start` is `null` when no
start symbol is set.

`input` is present when `print-input` is on. `terminals` is present
when `print-terminals` is on. `tml_rules` and `tml_facts` carry text when
their option is on. `tree` is present when `print-graphs` is on.

`source` of `grammar` is the exact content of the grammar file, byte for
byte, on every platform.

`ambiguous` is present when `print-ambiguity` is on:

    {"ambiguous":{"trees":2,"nodes":[
      {"symbol":"A","id":5,"range":[0,1],
       "alternatives":[{"children":[<ast node>,...]},
                       {"children":[<ast node>,...]}]}]}}

`trees` is the number of parse trees, the count the text prints as
`# n trees:`. `symbol`, `id` and `range` name the ambiguous nonterminal.
Each alternative holds its child AST nodes, in the text's order.

### `production_ids`

Every `productions` array is paired with a `production_ids` array, one
entry per string, in the same order. The strings stay as they are; the
ids carry what the text shows as `sym(5)` and `G0`:

    {"index":0,"head":2,"body":[[3]],"guard":null,
     "conjunctive":false}

- `index`: the production index, the `G0` of the text.
- `head`: the nonterminal id of the head.
- `body`: one array for each conjunct. Each array holds one entry per
  literal: the nonterminal id, or `null` for a terminal.
- `guard`: the guard name, or `null`. The text shows it as
  `# guarded: name`.
- `conjunctive`: true when the production has more than one conjunct.
  The text shows it as `# conjunctive`.

The arrays appear on `internal-grammar`, `unreachable`,
`grammar --json` and the `internal_grammar` of
`parse --json --grammar`.

### `directives`

`hello`, and the `result` of `load` and `reload`, carry a `directives`
object with what the grammar file says in its `@` directives. It is a
read-only snapshot from load time, so a later `set` does not change it.
The live values stay in `options`. With no grammar the value is `null`.

    {"use_char_classes":["digit"],"start":"start",
     "enable_productions":["comma","crlf"],
     "disable_productions":[],
     "enabled_productions":["comma","crlf","no_header"],
     "trim":["DQUOTE","SEP"],"trim_children":[],
     "trim_children_terminals":[],"trim_terminals":[],
     "trim_all_terminals":false,
     "inline":[["TEXTDATA"]],"inline_char_classes":true,
     "ambiguous":["expr","term"],
     "highlight":[{"type":"string","patterns":["escaped"]}],
     "highlight_auto":false,
     "dynamic":{"tok":["a","b"]}}

- `use_char_classes`: the names of `@use char classes`.
- `start`: the symbol of `@start`, or `null`.
- `enable_productions` and `disable_productions`: the guard names as
  written after `@enable` and `@disable`; the `productions` keyword is
  not a guard name.
- `enabled_productions`: the production guard names that are on after
  the load.
- `trim`, `trim_children`, `trim_children_terminals`: the names of the
  matching `@trim` lists.
- `trim_terminals`: the names after `except children of`.
- `trim_all_terminals`: true for `@trim all terminals`.
- `inline`: the tree paths of `@inline`, each a list of names.
- `inline_char_classes`: true for `@inline char classes`.
- `ambiguous`: the names of `@ambiguous`.
- `highlight`: the type and patterns of each `@highlight` entry.
- `highlight_auto`: true for `@highlight auto`.
- `dynamic`: the default values of each `@dynamic` nonterminal.

## The tree

The `tree` value is one AST JSON node, the bare root node.

    {"symbol":"expr","id":5,"range":[0,3],"children":[
      {"symbol":"digit","id":7,"range":[0,1],"children":[
        {"symbol":"","range":[0,1],"text":"1"}]}]}

A node field:

- `symbol`: the nonterminal name. The only required field. A terminal
  leaf has `""`; a TGF name is never empty.
- `id`: the nonterminal number, an integer. Present only on a
  nonterminal node; a terminal leaf has none. The text tree shows it as
  `symbol(5)`.
- `range`: `[start, end]` offsets. For a UTF-8 grammar the unit is one
  code point, not a byte.
- `children`: absent when the node has none.
- `text`: only on a terminal leaf. It holds the terminal characters. A
  nonterminal leaf has no `text`.
- `data` and `data_type`: occur together; `data` indexes the `pools` of a
  tau document. tau uses `pools`; tgf uses `text`.

A null node is skipped. The schema is
`src/format/ast.json/ast.schema.json`.

## One-shot CLI forms

- `repl --json` runs the request loop.
- `repl --json --session <id>` sets the session id that the `hello`
  line reports.
- `repl --json --init-stdin` reads one init line before the `hello`
  line. The line is `{"init":{"grammar":{"name":..,"source":..}|null,
  "start":..,"options":{..}}}`. The loop applies the grammar from
  `name` and `source`, then the start symbol and the option values, and
  then writes its first `hello` with them. A bad init line puts its
  errors in the `report` of that `hello`, and the child goes on with
  what it could apply. `grammar` is `null` with no grammar and absent
  for a built-in grammar, which the child keeps. This form exists for
  the Windows child of `tgf serve`; a POSIX session child is forked and
  needs no init line.
- `repl --json --evaluate "<src>"` prints one `eval` response and exits.
  No `hello` line.
- `parse --json`, `grammar --json` and `gen --json` print one response
  `{"cmd":..,"status":..,"result":..,"state":{...},"report":..}` and
  exit. No `hello` line and no `id`.
- `parse --json --grammar` adds the internal grammar under
  `internal_grammar`, in the shape of the `internal-grammar` command.
- `parse --json --measure` writes no text and adds the bintree totals
  under `bintree_totals` as numbers, plus `chain_length_histogram` (the
  eight bins of the text), `top_hash_groups`
  (`[{"size":n,"triples":n}]`) and `largest_group_samples`
  (`[{"value":"...","range":[s,e],"left":...,"right":...,
  "stored_hash":"...","recomputed_hash":"..."}]`). A 64-bit hash is a
  string, because a JSON number cannot hold it exactly; `left` and
  `right` are a child hash or `null`.
- `grammar --json` data is
  `{"start":"...","productions":["..."],"production_ids":[{...}],
  "nullable":[{"symbol":..,"id":..,"index":..,"production":..}]}`.
  Without `--nullable` the `nullable` key is absent.
- `gen --json` data is `{"files":["..."]}`, the paths it wrote.

The exit code is `1` when the status is `error`, else `0`.

## No grammar

`tgf`, `tgf repl` and `tgf repl --json` start with an empty grammar
when no file is given. `hello.grammar`, `state.grammar`, `hello.start`
and `state.start` are `null`, `state.char_classes` is an empty list and
`hello.directives` is `null`; the other hello fields stay. Once a start
symbol is set, `hello.start`, `state.start`, the `start` option and the
`start` command give it, also with no grammar.

Commands that need productions report the error
`no grammar loaded, use load`: `parse`, `parse file`,
`internal-grammar`, `unreachable`, `grammar` and `reload`. `load`
installs a grammar. `get`, `set`, `toggle`, `enable`, `disable`, `add`,
`delete`, `help`, `version`, `license`, `quit` and `clear` work as
usual. `set start foo` stores the start symbol, and the later `load`
uses it.

The one-shot `tgf parse`, `tgf grammar` and `tgf gen` with no grammar
report the same error and exit 1. With `--json` they print one error
response.

## Server sessions

`tgf serve` runs each session with the evaluator option `no_server_paths`,
because a client must not name a path that the server reads. The session
protocol, the server answers, the limits and the session logs are in
`docs/tgf_serve.md`.

## Example

    $ tgf tests/fixtures/tiny.tgf repl --json
    {"hello":{"protocol":1,...},"state":{"grammar":"tests/fixtures/tiny.tgf","start":"start","char_classes":[7]}}
    {"id":1,"cmd":"set","option":"trim","value":["a","b"]}
    {"id":1,"cmd":"set","status":"ok","result":{"option":"trim","value":["a","b"]},"state":{...},"report":{"nodes":[]}}
    {"id":2,"cmd":"parse","input":"123"}
    {"id":2,"cmd":"parse","status":"ok","result":{"ambiguous":{"trees":1,"nodes":[]},"terminals":"123","tree":{"symbol":"start",...}},"state":{...},"report":{"nodes":[]}}
    {"id":3,"cmd":"quit"}
    {"id":3,"cmd":"quit","status":"quit","result":{},"state":{...},"report":{"nodes":[]}}
