[back to index](../README.md#tgf-tool)

# tgf tool

This library provides a command line tool `tgf` which takes a TGF file and allows to do several things:
- run tgf parser REPL UI
- generate a parser in C++
- run a parser for a given input string
- display information about a grammar

Usage:
```
tgf <tgf file> [ <command> [ <command options> ] ]
```

where command can be one of:
```
repl       runs REPL UI (this is a default command if no command provided)
gen        generate a parser code
parse      parse an input string, file or stdin
grammar    prints grammar
```

Options for `repl` command:
```
tgf <tgf file> [ repl ] [ <options> ]
        --help             -h      detailed information about options
        --start            -s      starting literal
        --grammar          -g      prints grammar
        --print-ambiguity  -a      prints ambiguity info (true by default)
        --print-graphs     -p      prints parsed graph (true by default)
        --error-verbosity  -v      parse error is more verbose
        --tml-facts        -f      prints parsed graph in tml facts
        --tml-rules        -r      prints parsed graph in tml rules
        --evaluate         -e      run REPL command with input to evaluate and quit
```

Options for `gen` command:
```
tgf <tgf file> gen [ <options> ]
        --decoder          -d      decoder function
        --encoder          -e      encoder function
        --help             -h      detailed information about options
        --namespace        -N      namespace to wrap the generated parser
        --name             -n      name of the generated parser struct
        --output-dir       -O      path to a directory to write output file
        --output           -o      output file
        --start            -s      starting literal
```
For more information about decoder and encoder functions see [recoders](recoders.md)


Options for `parse` command:
```
tgf <tgf file> parse [ <options> ]
        --help             -h      detailed information about options
        --input            -i      parse input from file or STDIN if -
        --input-expression -e      parse input from a provided string
        --start            -s      starting literal
        --grammar          -g      prints grammar
        --print-ambiguity  -a      prints ambiguity info (true by default)
        --print-input      -I      prints input
        --terminals        -t      prints all parsed terminals serialized
        --error-verbosity  -v      parse error is more verbose
        --print-graphs     -p      prints parsed graph (true by default)
        --tml-facts        -f      prints parsed graph in tml facts
        --tml-rules        -r      prints parsed graph in tml rules
```

Options for `grammar` command:
```
tgf <tgf file> grammar [ <options> ]
        --help             -h      detailed information about options
        --start            -s      starting literal
        --grammar          -g      prints grammar (enabled by default)
```

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

`add option value` adds a value into a list of values (symbol list, treepath list).

Usage:
```
add trim white_space.
add inline expr > block > expr.
```

### del

`del option value` deletes a value from a list of values (symbol list, treepath list).

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
enable measure-parsing
```

### disable

`disable option` disables a boolean option value / sets its value to false.

Usage:
```
disable colors
```

## REPL options

All options can be accessed by `get` and `set` commands.

### boolean options

These options can be additionally changed by `enable`, `disable` and `toggle` commands.

```
  status                 show status                        on/off
  colors                 use term colors                    on/off
  print-ambiguity        prints ambiguous nodes             on/off
  print-terminals        prints parsed terminals            on/off
  print-graphs           prints parsed graphs               on/off
  print-rules            prints parsed forest as TML rules  on/off
  print-facts            prints parsed forest as TML facts  on/off
  measure-parsing        measures parsing time              on/off
  measure-each-pos       measures parsing time of each pos  on/off
  measure-forest         measures forest building time      on/off
  measure-preprocess     measures forest preprocess time    on/off
  trim-terminals         trim terminals                     on/off
  inline-char-classes    inline character classes           on/off
```

### symbol list options

These options can be additionally updated by `add` and `del` commands.

```
  nodisambig-list        list of nodes to keep ambiguous    symbol1, symbol2...
  trim                   list of nodes to trim              symbol1, symbol2...
  trim-children          list of nodes to trim children     symbol1, symbol2...
```

### treepaths list option

This option can be additionally updated by `add` and `del` commands.

```
  inline                 list of tree paths to inline       symbol1 > symbol2.
```

### error verbosity option

```
  error-verbosity        parse errors verbosity             basic/detailed/root-cause
```