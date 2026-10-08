[back to index](../README.md#tgf-tool)

# tgf tool

This library provides a command line tool `tgf` which takes a TGF file and allows to do several things:
- run tgf parser REPL UI
- generate a parser in C++
- run a parser for a given input string
- display information about a grammar

Usage:
```
tgf <tgf file> [ <global options> ] [ <command> [ <command options> ] ]
```

Global options (apply to any command):

```
        --help             -h      detailed information about options
        --version          -v      print version and exit
        --license          -L      print license and exit
```

where command can be one of:
```
repl       runs REPL UI (this is a default command if no command provided)
gen        generate a parser code
parse      parse an input string, file or stdin
grammar    show information about grammar
test       run tester (requires a .tgf.test file)
```

Options for `repl` command:
```
tgf <tgf file> [ repl ] [ <options> ]
        --auto-disambiguate
                                   enables auto-disambiguation (default: true)
        --char-classes             character classes of the grammar, an entry is a class or class as name
        --colors           -c      use terminal colors (default: true)
        --derive-char-classes
                                   scan rules that match one character as character classes (default: true)
        --dont-trim-terminals-of
                                   nonterminals whose terminals are kept
        --dynamic                  default values for a @dynamic nonterminal, each entry written as name=value
        --enabled-productions
                           -G      comma-separated enabled productions
        --error-verbosity  -v      parse error verbosity: basic, detailed, root-cause (default: detailed)
        --evaluate         -e      run a REPL command and quit
        --grammar          -g      print the grammar (default: false)
        --help             -h      detailed information about options (default: false)
        --highlight                highlight types, each entry written as type: nonterminal
        --highlight-auto           highlight by name and content heuristics (default: false)
        --init-stdin               read one init line before the first hello, used by tgf serve (default: false)
        --inline                   tree paths to inline, each written as a > b
        --inline-char-classes
                                   inline character classes (default: false)
        --json             -J      print and read JSON (default: false)
        --legacy-repl      -X      use the legacy terminal REPL (default: false)
        --measure          -m      measure parse time and counters, turns every measure flag on (default: false)
        --nodisambig-list          nonterminals whose ambiguity is kept
        --print-ambiguity  -a      print ambiguous nodes (default: true)
        --print-graphs     -p      print the parsed graph (default: true)
        --print-tml-facts          print the forest as TML facts (default: false)
        --print-tml-rules          print the forest as TML rules (default: false)
        --session                  session id of a tgf serve session
        --start            -s      starting literal (default: start)
        --trim                     nonterminals to trim from the tree
        --trim-children            nonterminals whose children are trimmed
        --trim-children-terminals
                                   nonterminals whose children and terminals are trimmed
        --trim-terminals           trim all terminals (default: false)
        --variant                  the selected profile of enabled productions
        --variants                 profiles of enabled productions, each entry written as name=guard
```

Options for `gen` command:
```
tgf <tgf file> gen [ <options> ]
        --auto-disambiguate
                           -a      enables auto-disambiguation (default: true)
        --char-classes             character classes of the grammar, an entry is a class or class as name
        --char-type        -C      input character type (e.g. char, char32_t) (default: char)
        --decoder          -d      decoder function
        --derive-char-classes
                                   scan rules that match one character as character classes (default: true)
        --dont-trim-terminals-of
                                   nonterminals whose terminals are kept
        --dynamic          -D      default values for a @dynamic nonterminal, each entry written as name=value
        --enabled-productions
                           -G      comma-separated enabled productions
        --encoder          -e      encoder function
        --header-only      -H      keep the parser in a single header (default: true)
        --help             -h      detailed information about options (default: false)
        --highlight                highlight types, each entry written as type: nonterminal
        --highlight-auto           highlight by name and content heuristics (default: false)
        --inline                   tree paths to inline, each written as a > b
        --inline-char-classes
                                   inline character classes (default: false)
        --json             -J      print and read JSON (default: false)
        --name             -n      name of the generated parser struct
        --namespace        -N      namespace for the generated parser
        --nodisambig-list  -A      nonterminals whose ambiguity is kept
        --output           -o      output file for the parser
        --output-dir       -O      output directory
        --start            -s      starting literal (default: start)
        --terminal-type    -T      terminal character type (e.g. char, char32_t) (default: char)
        --treemr                   add a matcher() member built on idni::treemr::matcher_for (default: false)
        --trim                     nonterminals to trim from the tree
        --trim-children            nonterminals whose children are trimmed
        --trim-children-terminals
                                   nonterminals whose children and terminals are trimmed
        --trim-terminals           trim all terminals (default: false)
        --utf8             -U      shorthand: --char-type char --terminal-type char32_t with utf8 recoders (default: false)
        --variant                  the selected profile of enabled productions
        --variants                 profiles of enabled productions, each entry written as name=guard
```
For more information about decoder and encoder functions see [recoders](recoders.md)


Options for `parse` command:
```
tgf <tgf file> parse [ <options> ]
        --auto-disambiguate
                                   enables auto-disambiguation (default: true)
        --char-classes             character classes of the grammar, an entry is a class or class as name
        --colors           -c      use terminal colors (default: true)
        --derive-char-classes
                           -d      scan rules that match one character as character classes (default: true)
        --dont-trim-terminals-of
                                   nonterminals whose terminals are kept
        --dynamic                  default values for a @dynamic nonterminal, each entry written as name=value
        --enabled-productions
                           -G      comma-separated enabled productions
        --error-verbosity  -v      parse error verbosity: basic, detailed, root-cause (default: detailed)
        --grammar          -g      print the grammar (default: false)
        --help             -h      detailed information about options (default: false)
        --highlight                highlight types, each entry written as type: nonterminal
        --highlight-auto           highlight by name and content heuristics (default: false)
        --inline                   tree paths to inline, each written as a > b
        --inline-char-classes
                                   inline character classes (default: false)
        --input            -i      parse input from a file or STDIN if -
        --input-expression -e      parse input from a provided string
        --json             -J      print and read JSON (default: false)
        --measure          -m      measure parse time and counters, turns every measure flag on (default: false)
        --nodisambig-list          nonterminals whose ambiguity is kept
        --print-ambiguity  -a      print ambiguous nodes (default: true)
        --print-graphs     -p      print the parsed graph (default: true)
        --print-input      -I      print the input (default: false)
        --print-terminals  -t      print the parsed terminals (default: true)
        --print-tml-facts          print the forest as TML facts (default: false)
        --print-tml-rules          print the forest as TML rules (default: false)
        --start            -s      starting literal (default: start)
        --tree-path        -P      parse tree path: bintree or forest (default: bintree)
        --trim                     nonterminals to trim from the tree
        --trim-children            nonterminals whose children are trimmed
        --trim-children-terminals
                                   nonterminals whose children and terminals are trimmed
        --trim-terminals           trim all terminals (default: false)
        --variant                  the selected profile of enabled productions
        --variants                 profiles of enabled productions, each entry written as name=guard
```

`parse -e` and `repl -e` take their value as typed: the tool does not trim it and does not read quotes or escapes in it.

Options for `grammar` command:
```
tgf <tgf file> grammar [ <options> ]
        --auto-disambiguate
                                   enables auto-disambiguation (default: true)
        --char-class-report
                           -C      print the derived character class report (default: false)
        --char-classes             character classes of the grammar, an entry is a class or class as name
        --colors           -c      use terminal colors (default: true)
        --derive-char-classes
                                   scan rules that match one character as character classes (default: true)
        --dont-trim-terminals-of
                                   nonterminals whose terminals are kept
        --dynamic                  default values for a @dynamic nonterminal, each entry written as name=value
        --enabled-productions
                           -G      comma-separated enabled productions
        --grammar          -g      print the grammar (default: true)
        --help             -h      detailed information about options (default: false)
        --highlight                highlight types, each entry written as type: nonterminal
        --highlight-auto           highlight by name and content heuristics (default: false)
        --inline                   tree paths to inline, each written as a > b
        --inline-char-classes
                                   inline character classes (default: false)
        --json             -J      print and read JSON (default: false)
        --measure          -m      measure parse time and counters, turns every measure flag on (default: false)
        --nodisambig-list          nonterminals whose ambiguity is kept
        --nullable         -N      report nullable recursive productions (default: false)
        --start            -s      starting literal (default: start)
        --trim                     nonterminals to trim from the tree
        --trim-children            nonterminals whose children are trimmed
        --trim-children-terminals
                                   nonterminals whose children and terminals are trimmed
        --trim-terminals           trim all terminals (default: false)
        --variant                  the selected profile of enabled productions
        --variants                 profiles of enabled productions, each entry written as name=guard
```

Options for `test` command:
```
tgf <tgf file> test [ <options> ]
        --auto-disambiguate
                                   enables auto-disambiguation (default: true)
        --char-classes             character classes of the grammar, an entry is a class or class as name
        --derive-char-classes
                                   scan rules that match one character as character classes (default: true)
        --dont-trim-terminals-of
                                   nonterminals whose terminals are kept
        --dynamic                  default values for a @dynamic nonterminal, each entry written as name=value
        --enabled-productions
                           -G      comma-separated enabled productions
        --help             -h      detailed information about options (default: false)
        --highlight                highlight types, each entry written as type: nonterminal
        --highlight-auto           highlight by name and content heuristics (default: false)
        --inline                   tree paths to inline, each written as a > b
        --inline-char-classes
                                   inline character classes (default: false)
        --nodisambig-list          nonterminals whose ambiguity is kept
        --start                    starting literal (default: start)
        --trim                     nonterminals to trim from the tree
        --trim-children            nonterminals whose children are trimmed
        --trim-children-terminals
                                   nonterminals whose children and terminals are trimmed
        --trim-terminals           trim all terminals (default: false)
        --variant                  the selected profile of enabled productions
        --variants                 profiles of enabled productions, each entry written as name=guard
```

See [`TGF test files`](tgf.test.md) for the `.tgf.test` file format that this command runs.

## REPL commands

When running `tgf` tool without any command or with command `repl` it will run REPL UI.

It provides several commands which can help to inspect, test and debug TGF grammars.

Multiple commands can be provided in a single input line if using `.` as a command separator.

### quit

`quit` or `q` exits the REPL UI.

### help

`help` or `h` displays a help message about available commands.

`help command` or `h command` displays a detailed help message for the command.

### version

`version` or `v` prints actual git commit hash id at the compile time of this tool.

### clear

`clear` or `cls` clears a terminal.

### load

`load "filepath"` or `l "filepath"` loads TGF grammar from a filepath.

Usage:
```
load "csv.tgf"
l "csv.tgf"
```

### reload

`reload` or `r` reloads the TGF file provided as an argument or the last one loaded by `load` command.

### grammar

`grammar` or `g` prints content of the loaded TGF file.

### internal-grammar

`internal-grammar` or `ig` or `i` prints internal grammar loaded into the parser. It prints all reachable productions from a starting symbol set by grammar, settings or it is `start` by default. If a symbol is provided, this command prints reachable productions from the provided symbol.

Usage:
```
# print internal grammar (reachable rules from starting symbol)
internal-grammar
ig
i

# print reachable rules from a symbol
ig expr
i number
```

### unreachable

`unreachable symbol` or `u symbol` prints all productions which are not reachable from the symbol.

Usage:
```
unreachable expr   # show rules unreachable from nonterminal expr
u number           # show rules unreachable from nonterminal number
u                  # show rules unreachable from starting symbol (start by default)
```

### start

`start` prints what is the current starting symbol.

`start symbol` - sets a new starting symbol for parsing and for other commands.

Usage:
```
start       # prints current start symbol
start expr  # sets expr as a starting symbol
```

### parse

`parse source` or `parse "source"` or `p` is a usable short for `parse`.

Runs parser with the "source" as an input.

Usage:
```
parse 1+1          # runs parser with input string: 1+1
p 3*2              # runs parser with input string: 3*2
p "4*5\n3-2"       # runs parser with input string: 4*5\n3-2
```

### parse file

`parse file "filepath"` or `pf "filepath"` or `f "filepath"` runs parser with the content of the file as an input.

Usage:
```
# runs parser with file input.txt content as an input
parse file "input.txt"
pf "input.txt"
f "input.txt"
```

## REPL option commands

### get

`get` prints all options and their values.

`get option` prints option and its value.

Usage:
```
get                  # get all settings
get status           # get option status and its value
get inline           # get option inline and its values
get error-verbosity  # get option error-verbosity and its values
```

### set

`set option value` or `set option = value` sets an option to contain a value.

Usage:
```
set status off
set trim white_space, comment
set inline chars, expr > block > expr
set error-verbosity = detailed
```

### add

`add option value` adds a value into a list option.

Usage:
```
add trim white_space.
add inline expr > block > expr.
```

### del

`del option value` deletes a value from a list option.

Usage:
```
del trim white_space
del inline expr > block > expr.
```

### toggle

`toggle option` toggles boolean option value / flips true to false and false to true.

Usage:
```
toggle status
```

### enable

`enable option` enables a boolean option value / sets value to true.

Usage:
```
enable measure
```

### disable

`disable option` disables a boolean option value / sets its value to false.

Usage:
```
disable colors
```

## REPL options

All options can be accessed by `get` and `set` commands.
A flag option can be additionally changed by `enable`, `disable` and `toggle` commands.
A list option can be additionally updated by `add` and `del` commands.

### grammar options

```
  auto-disambiguate         enables auto-disambiguation                                                   flag
  nodisambig-list           nonterminals whose ambiguity is kept                                          list
  trim-terminals            trim all terminals                                                            flag
  inline-char-classes       inline character classes                                                      flag
  derive-char-classes       scan rules that match one character as character classes                      flag
  trim                      nonterminals to trim from the tree                                            list
  trim-children             nonterminals whose children are trimmed                                       list
  trim-children-terminals   nonterminals whose children and terminals are trimmed                         list
  dont-trim-terminals-of    nonterminals whose terminals are kept                                         list
  inline                    tree paths to inline, each written as a > b                                   list
  enabled-productions       comma-separated enabled productions                                           list
  start                     starting literal                                                              text
  char-classes              character classes of the grammar, an entry is a class or class as name        list
  highlight-auto            highlight by name and content heuristics                                      flag
  highlight                 highlight types, each entry written as type: nonterminal                      list
  dynamic                   default values for a @dynamic nonterminal, each entry written as name=value   list
  variants                  profiles of enabled productions, each entry written as name=guard             list
  variant                   the selected profile of enabled productions                                   text
```

### parse options

```
  gc                        enable garbage collection                                                     flag
  gc-lag                    garbage collection lag                                                        number
  max-length                maximum input length (0 = unlimited)                                          number
  measure                   measure parse time and counters, turns every measure flag on                  flag
  measure-each-pos          measure parse time at each position                                           flag
  measure-forest            measure forest building time                                                  flag
  measure-preprocess        measure forest preprocess time                                                flag
  measure-scopes            record timed scopes in the report                                             flag
  measure-counters          record parse counters and memory in the report                                flag
  debug                     print debug diagnostics                                                       flag
  error-verbosity           parse error verbosity: basic, detailed, root-cause                            text
  tree-path                 parse tree path: bintree or forest                                            text
```

### repl options

```
  print-terminals           print the parsed terminals                                                    flag
  status                    print status after each command                                               flag
  print-ambiguity           print ambiguous nodes                                                         flag
  print-graphs              print the parsed graph                                                        flag
  print-tml-rules           print the forest as TML rules                                                 flag
  print-tml-facts           print the forest as TML facts                                                 flag
  print-input               print the input                                                               flag
```

### ui options

```
  colors                    use terminal colors                                                           flag
```
