# tgf JSON API

`tgf <grammar> repl --json` turns the TGF REPL into a request and
response API on stdin and stdout. One JSON object goes on each line.
`tgf <grammar> parse --json`, `grammar --json` and `gen --json` run one
command and print one response.

The client never parses text that a human reads. Every response is one
line, compact JSON, with no ANSI color codes. Every response carries
`state`, the current grammar file and start symbol:
`{"grammar":"g.tgf","start":"start"}`. An `eval` response carries
it once, on the top object.

Two JSON Schema files describe this API:
`src/format/tgf_api.json/tgf_api.schema.json` covers every request and
response, and `src/format/json/report.schema.json` covers the report
tree. The parse tree uses `src/format/ast.json/ast.schema.json`.

## Request forms

Two forms reach the same command handlers.

The `eval` form takes REPL text:

    {"id":1,"cmd":"eval","src":"set trim a, b . get trim"}

A structured form names the command and its fields:

    {"id":2,"cmd":"set","option":"trim","value":["a","b"]}
    {"id":3,"cmd":"parse","input":"1+2"}

`id` is optional and is echoed in the response. A structured request
becomes canonical REPL text and runs through the same path as `eval`.

Structured fields:

| cmd | fields |
|---|---|
| `parse` | `input` (string) |
| `parse file`, `load` | `file` (string) |
| `start`, `internal-grammar`, `unreachable` | `symbol` (string, optional) |
| `help` | `command` (string, optional) |
| `get` | `option` (string, optional) |
| `set` | `option`, `value` (bool, string, string array, or array of string arrays) |
| `toggle`, `enable`, `disable` | `option` |
| `add`, `delete` | `option`, `value` (array) |
| `grammar`, `reload`, `version`, `license`, `quit`, `clear` | none |

An empty array for `set` becomes `set <option>`, which clears a list or
tree path option. Every option name is checked against the
option table, every symbol against `[A-Za-z_][A-Za-z0-9_]*`, every
`help` argument against the command names, and `error-verbosity` against
its three values.

## Response shape

At start the server writes one `hello` line:

    {"hello":{"protocol":1,"version":"...","grammar":"g.tgf",
      "fixed_grammar":false,"start":"start","options":{...},
      "report":{...}},
     "state":{"grammar":"g.tgf","start":"start"}}

`options` holds all 24 REPL option values. `report` holds the diagnostic
report of the grammar load.

An `eval` response carries one entry per statement:

    {"id":1,"status":"ok","results":[
      {"cmd":"set","status":"ok","result":{...},"report":{...}},
      {"cmd":"get","status":"ok","result":{...},"report":{...}}],
     "state":{"grammar":"g.tgf","start":"start"},
     "report":{...}}

A structured request carries one result:

    {"id":2,"cmd":"set","status":"ok","result":{...},
     "state":{"grammar":"g.tgf","start":"start"},"report":{...}}

An error before a command runs (invalid JSON, missing `cmd`, a missing or
wrongly typed field, an unknown command, a failed check) has no `result`:

    {"id":null,"status":"error",
     "state":{"grammar":"g.tgf","start":"start"},"report":{...}}

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
| `start` | `{"start":"...","changed":true}` |
| `unreachable` | `{"symbol":"...","productions":["..."],"production_ids":[{...}]}` |
| `load`, `reload` | `{"grammar":"...","loaded":true}` |
| `help` | `{"command":"...","text":"..."}` |
| `version` | `{"version":"..."}` |
| `license` | `{"license":"..."}` |
| `quit` | `{}` with status `quit` |
| `clear` | `{}` |
| `get` | `{"options":{"name":value,...}}` or `{"option":"name","value":v}` |
| `set`, `toggle`, `enable`, `disable`, `add`, `delete` | same as `get` for that option |

Option values are a bool, an array of strings, an array of string arrays
(tree paths), or a string for `error-verbosity`. Option names are the
long names from `tgf_repl.tgf`. `derive-char-classes` (`dcc`) is a bool
option: true scans one-character rules as character classes.

`input` is present when `print-input` is on. `terminals` is present
when `print-terminals` is on. `tml_rules` and `tml_facts` carry text when
their option is on. `tree` is present when `print-graphs` is on.

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

## Example

    $ tgf tests/fixtures/tiny.tgf repl --json
    {"hello":{"protocol":1,...},"state":{"grammar":"tests/fixtures/tiny.tgf","start":"start"}}
    {"id":1,"cmd":"set","option":"trim","value":["a","b"]}
    {"id":1,"cmd":"set","status":"ok","result":{"option":"trim","value":["a","b"]},"state":{...},"report":{"nodes":[]}}
    {"id":2,"cmd":"parse","input":"123"}
    {"id":2,"cmd":"parse","status":"ok","result":{"ambiguous":{"trees":1,"nodes":[]},"terminals":"123","tree":{"symbol":"start",...}},"state":{...},"report":{"nodes":[]}}
    {"id":3,"cmd":"quit"}
    {"id":3,"cmd":"quit","status":"quit","result":{},"state":{...},"report":{"nodes":[]}}
