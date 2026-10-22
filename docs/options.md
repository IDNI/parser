[back to index](../README.md#classes-and-structs)
[quick start](options_quickstart.md)

# options

```
namespace idni;
```

The options repository keeps the options of a program in one place:

- One declared catalog. A program declares each option once, with its name, its owner, its default value and its help text.
- One value per option, read and written by name. A later write replaces the value.
- A bound field. An option can live in a field of a struct. A write by name then goes into the field.
- One name for every surface. The command line, the REPL, the environment and an api use the same name.

The code is in `src/utility/options.h` (the repository) and `src/utility/options_types.h` (the plain types). The tests are in `tests/doctest_options.cpp`.

```
options_repository& repo = options();
if (auto r = repo.declare(my_option_set); r.has_error())
	r.report().print(std::cerr);
repo.set("count", 3);
std::size_t n = repo.get<std::size_t>("count");
```


## option_value

```
using option_list  = std::vector<std::string>;
using option_value = std::variant<bool, std::size_t, std::string, option_list>;
```

The value of one option. The index of the alternative is the kind of the option.


## option_kind

```
enum class option_kind : std::size_t { flag, numeric, text, list };

option_kind kind_of(const option_value& v);
option_kind kind_of(const option_spec& s);
```

The kind of an option is the index of its alternative in `option_value`. `kind_of(s)` gives the kind of the fallback of the spec `s`.

| kind | type | text form |
|------|------|-----------|
| `flag` | `bool` | `true`, `on`, `1`, `yes`, `y`, `t` and `false`, `off`, `0`, `no`, `n`, `f`, in any case |
| `numeric` | `std::size_t` | decimal digits only |
| `text` | `std::string` | the text, or a TGF string in double quotes |
| `list` | `option_list` | entries split at the commas, each entry bare or a TGF string in double quotes |


## option_spec

```
struct option_spec {
	std::string name;
	std::string owner;
	option_value fallback;
	std::string help;
	std::vector<option_surface> surfaces = {};
	bool from_env = true;
	bool raw = false;
};
```

One option. The fields are:

- `name` - the one name of the option on every surface.
- `owner` - a label for a group of options. `reset()` and a program filter use it.
- `fallback` - the default value. The type of `fallback` gives the kind.
- `help` - the text of the option in the help.
- `surfaces` - where the option shows on the command line. An option with no surface is not on the command line.
- `from_env` - `load_env()` reads the option only when this is true.
- `raw` - a text option keeps the text as given: no quotes and no trim.


## option_surface

```
struct option_surface {
	std::string command;
	char short_name = '\0';
	std::optional<option_value> fallback = std::nullopt;
};
```

Where an option shows on the command line: the command, a short letter, and an optional default of that command.

- `command` - the command that takes the option. The surface of the options before the command is `args_config::global_surface`, usually the empty name.
- `short_name` - the letter of `-x`. No letter when it is `'\0'`.
- `fallback` - the default of the option for this command. It has the kind of the option. The default applies only to an option that no field binds.
- A default on the global surface `""` is an error, `option_global_default`, because the fallback of the spec already applies there.


## command_spec

```
struct command_spec {
	std::string name;
	std::string help;
};
```

One command of the program and its text in the help.


## option_set

```
struct option_set {
	std::vector<option_spec> options = {};
	std::vector<command_spec> commands = {};
};
```

Everything one program declares. A library declares its own set, and a program declares its set next to it.


## options_repository

```
struct idni::options_repository;
```

One value per option. A later write replaces the value. A repository cannot be copied.

The repository finds the value of an option in this order:

1. The field, when a field binds the option.
2. The stored value, when a write stored one.
3. The default of the current command, when a surface of that command has one. The current command is the command of the last `parse_args()`.
4. The fallback of the spec.


### diagnostics::result\<void\> declare(const option_set& set);

Declares the options and the commands of `set`.

- A name that is new is declared.
- A name declared with an equal spec is fine. The surfaces are not compared, and the declared ones stay.
- A different spec under a declared name fails, and nothing of `set` is declared.
- A command declared again with an equal spec is fine. A different command under a declared name fails.

A surface with a default on the global surface `""` is an error.

Errors: `option_already_exists`, `command_already_exists`, `option_short_name_taken`, `option_command_default_kind`, `option_global_default`.


### diagnostics::result\<void\> add_surface(std::string_view name, option_surface surface);

Adds a surface to a declared option. A program uses it to put an option that a library declared on its own commands.

- An unknown name is an error.
- A short letter already used on that command is an error.
- An equal surface that the option has already changes nothing.
- A default on the global surface `""` is an error.

Errors: `option_not_found`, `option_short_name_taken`, `option_command_default_kind`, `option_global_default`.


### template \<typename T\> T get(std::string_view name) const;

The value of `name` as `T`. `T` is `bool`, `std::size_t`, `std::string` or `option_list`.

A wrong name or type is a programming error, caught by a DBG assert only. A release build gives `T{}`.


### option_value value(std::string_view name) const;

The value of `name`, of the option's own kind. A wrong name is a programming error, caught by a DBG assert only.


### template \<typename T\> void set(std::string_view name, T value);

The typed write of a program. It takes a literal or an `int` too, so a caller needs no cast:

```
repo.set("name", "abc");
repo.set("count", 3);
```

A wrong name, a value of another kind, a negative number and a codec error are programming errors, caught by a DBG assert only.


### diagnostics::result\<void\> set(std::string_view name, option_value value);

Stores a value of the option's own kind. An unknown name, a value of another kind and a codec failure are errors, and nothing changes.

Errors: `option_not_found`, `option_value_kind`, and the error of the codec.


### diagnostics::result\<void\> set_text(std::string_view name, std::string_view text);

Reads `text` with the kind of the option and stores the value. A REPL, an api or a binding uses it.

- The text loses the spaces around it, for every kind.
- A text or a list entry may be a TGF string in double quotes. The quotes keep the spaces and the commas inside them.
- A `raw` text option keeps the text as given.

An unknown name or a bad value is an error, and nothing changes.

Errors: `option_not_found`, `option_bad_value`, and the error of the codec.


### diagnostics::result\<std::string\> get_text(std::string_view name) const;

The text of the value of `name`. `set_text()` reads the text back as the same value. A text or a list entry prints in double quotes when its bare form reads back differently. A `raw` text option gives its text as it is.

Errors: `option_not_found`.


### template \<typename T\> diagnostics::result\<void\> bind(std::string_view name, T& field, option_hook after = {});

Binds a declared option to its field. The field type is the type of the option.

- The value of the option is the value of the field from now on.
- A stored value goes into the field, and the hook runs.
- With no stored value, the field keeps what it holds.
- The hook `after` runs after each write.
- The owner unbinds before the field ends.

An unknown name, a field of another type and a second bind of one option are programming errors, caught by a DBG assert only.


### template \<typename T, typename Codec\> requires option_codec\<Codec, T\> diagnostics::result\<void\> bind(std::string_view name, T& field, Codec codec, option_hook after = {});

Binds with a codec, when the field type is not the option type. See [codec](#codec). The rules of `bind()` above apply.

Errors: the error of the codec, when a stored value does not fit the field.


### void unbind(std::string_view name);

The owner calls it in its destructor. The last value of the field stays in the repository, so a later `bind()` puts it into the next field.

An owner can outlive its repository, as a static grammar outlives the thread local repository of `options()`. So the owner registers a callback with `add_detach()`, and the destructor of the repository runs every callback. The owner then forgets the repository, and its own destructor unbinds nothing. `remove_detach()` drops the callback when the owner unbinds first.

An option that is not bound is a programming error, caught by a DBG assert only.


### void reset(std::string_view owner);

Gives every option of `owner` its default value: the fallback of the spec. A bound field gets the fallback written, and its hook runs. An unbound option loses its stored value.


### diagnostics::result\<void\> reset_option(std::string_view name);

Gives the option `name` its default, as `reset()` does for each option of an owner.

Errors: `option_not_found`.


### std::vector\<std::string\> names() const;

The names of the declared options, in the order of the declarations.


### std::shared_ptr\<const option_spec\> find(std::string_view name) const;

The declared option `name`, or null when no option has the name. The spec stays valid after a later `declare()`.


### diagnostics::result\<void\> load_env(std::string_view prefix);

Writes each option from its environment variable. The variable is the prefix as given plus the name in upper case, with `-` as `_`. A caller passes a prefix such as `TGF_`, so the option `max-line` reads `TGF_MAX_LINE`.

- An option with `from_env` false is not read.
- An option with no variable keeps its value.
- A bad value is an error. The error names the variable first, then the detail. The other variables are still read.

Errors: `option_env_bad_value`, followed by `option_bad_value` or the error of the codec.


### diagnostics::result\<parsed_args\> parse_args(int argc, char\*\* argv, const args_config& cfg = {});

Reads the command line and stores its values. Nothing is stored on an error, and no field is written until every value of argv fits its field.

The command line has this form:

```
<program> [ <global options> ] [ <command> [ <command options> ] ] [ <inputs> ]
```

- `--name value` and `--name=value` give a value. `-x value` gives a value by the short letter.
- `-abc` sets the three short flags `a`, `b` and `c`. A letter that needs a value must be the last letter of its group.
- A flag with no value is true. A flag takes the next word only when that word is a flag word, such as `off`.
- A numeric word such as `-5` is a value, not an option.
- A list given again appends. Every other kind keeps the last value.
- The options before the command use the surface `cfg.global_surface`. The options after the command use the surface of the command.
- An input is `-`, a file that exists, or a URL. A URL is `<scheme>://<rest>`, or `<host>:<port>` with a numeric port.
- With no command, the command is `cfg.default_command`, or `cfg.default_command_when_inputs` when argv names inputs.
- An unknown option before the command goes to the default command. With no default command, it is an error.

Most errors carry the help of the program as an info node of the report.

A program reads `get<bool>("help")` after it and calls `help()` itself.

Errors: `option_not_found`, `option_missing_argument`, `option_bad_value`, `option_unknown_command`, `option_second_command`, `option_no_such_input`, and the error of a codec.


### args_config

```
struct args_config {
	std::string name;
	std::string help_header;
	std::string description;
	std::string default_command;
	std::string default_command_when_inputs;
	std::string global_surface;
};
```

The program data that `parse_args()` and `help()` need.

- `name` - the name of the program.
- `help_header` - the first word of the help. When it is empty, the help uses `name`.
- `description` - the line under the header of the program help.
- `default_command` - the command when argv names no command.
- `default_command_when_inputs` - the command when argv names inputs: files or URLs.
- `global_surface` - the surface of the options given before the command.


### parsed_args

```
struct parsed_args {
	std::string command;
	bool command_given = false;
	std::vector<std::string> inputs;
	std::vector<std::pair<std::string, option_value>> values;
};
```

What one command line gave that is no option, and the options it gave.

- `command` - the command, given or default. Empty when no command applies.
- `command_given` - true when argv named the command.
- `inputs` - the positional arguments: `-`, the files and the URLs.
- `values` - the options argv gave, with their values. There is one entry per option, at the place of its first use, with its final value. A program writes them again after it loads a grammar, so a directive of the grammar does not undo a value of the user.


### void help(std::ostream& os, std::string_view command = {}) const;

Prints the help of `command`, or of the program when `command` is empty. It uses the `args_config` of the last `parse_args()` call.

The help of the program shows the header, the description, the options of the global surface and the commands. The help of a command shows its options. Each row shows the long name, the short letter, the help text and the default. The default of a command row is the value for that command.

```
tool <file> [ options ] <command> [ <command options> ]
a test tool

Options:
	--help		-h	show the help (default: false)
	--verbose	-v	print more (default: false)
Commands:
	run			run the tool
```


### effective (private)

```
option_value effective(std::size_t i, std::string_view command) const;
option_value effective(std::size_t i) const;
```

The value of the option at index `i`: the field of a bound option, else the stored value, else the default of `command`, else the default of the option. The second form uses the command of the last `parse_args()`. Every read goes through it.


<a id="codec"></a>
## codec

```
template <typename C, typename T>
concept option_codec = requires(const C& c, const T& field, const option_value& v) {
	{ c.to_value(field) } -> std::convertible_to<option_value>;
	{ c.from_value(v) } -> std::same_as<diagnostics::result<T>>;
};
```

A codec converts between a field and the value of its option. A program needs one when the field type is not one of the four option types.

- `to_value` gives the value of the field. It has the kind of the option.
- `from_value` gives the field value, or an error for a bad value. On an error the field does not change, and the hook does not run.

This codec binds a `std::set<int>` to a list option:

```
struct int_set_codec {
	option_value to_value(const std::set<int>& field) const {
		option_list out;
		for (int x : field) out.push_back(std::to_string(x));
		return out;
	}
	diagnostics::result<std::set<int>> from_value(const option_value& v) const {
		diagnostics::result<std::set<int>> r;
		std::set<int> out;
		if (const auto* list = std::get_if<option_list>(&v))
			for (const auto& s : *list) {
				int x = 0;
				const char* end = s.data() + s.size();
				auto [p, ec] = std::from_chars(s.data(), end, x);
				if (ec != std::errc{} || p != end) return r.with_error(
					diagnostics::code::invalid_argument, "not a number");
				out.insert(x);
			}
		return r.with_value(std::move(out));
	}
};

std::set<int> ids;
auto bound = repo.bind("ids", ids, int_set_codec{});
auto written = repo.set_text("ids", "1, 2");   // ids is { 1, 2 }
auto bad = repo.set_text("ids", "x");          // "not a number", ids stays { 1, 2 }
repo.unbind("ids");
```

The parser has two codecs of its own in `src/parser_options.h`: `error_verbosity_codec` and `tree_path_codec`.


## hook

```
using option_hook = std::function<void()>;
```

A hook runs after each write of a bound option. Every write runs it: `set()`, `set_text()`, `load_env()`, `parse_args()`, `reset()`, `reset_option()`, and the `bind()` that puts a stored value into the field.

The hook runs once per write. It does not run on a failed write.


## options() and options_scope

```
options_repository& options();

struct options_scope {
	explicit options_scope(options_repository& r);
	~options_scope();
};
```

`options()` gives the current repository of the calling thread. The first call on a thread creates one that lives as long as the thread. Each thread has its own repository, and nothing passes a repository to a library.

`options_scope` makes the repository `r` current for a scope. Its destructor restores the previous one. Scopes nest. A server session or a test that needs values of its own uses a scope:

```
options_repository mine;
options_scope scope(mine);
options().set("count", 5);       // writes to mine
```

A repository and the fields it binds belong to one thread: the thread that created the repository. A DBG assert catches a write, a bind, an unbind or a reset on another thread. A DBG assert also catches a scope that ends out of order.


## errors

Each error is a message of `parser_strings::messages` in `src/parser_strings.h`. The attributes `label::name` and `label::value` name the option and the value.

| message | text | code | given by |
|---------|------|------|----------|
| `option_already_exists` | Option already exists | `invalid_argument` | `declare()`: a different spec under a declared name |
| `command_already_exists` | Command already exists | `invalid_argument` | `declare()`: a different command under a declared name |
| `option_short_name_taken` | Two options use the same short name on one command | `invalid_argument` | `declare()`, `add_surface()`: a short letter that another option uses on the same command. The error names both options |
| `option_command_default_kind` | The default of a command has another kind than the option | `type_error` | `declare()`, `add_surface()`: a surface default of another kind |
| `option_global_default` | A default on the global surface does not apply | `invalid_argument` | `declare()`, `add_surface()`: a surface default on the global surface `""` |
| `option_value_kind` | The value has another kind than the option | `type_error` | `set()` of an `option_value` |
| `option_not_found` | Unknown option | `not_found` | `add_surface()`, `set()`, `set_text()`, `get_text()`, `reset_option()` |
| `option_not_found` | Unknown option | `invalid_argument` | `parse_args()`: an option that the surface does not have |
| `option_bad_value` | The option does not take the value | `invalid_argument` | `set_text()`, `load_env()`, `parse_args()`: a text that does not read as the kind. A bad TGF string adds the escape error |
| `option_env_bad_value` | The option does not take the value of the environment variable | `invalid_argument` | `load_env()`: names the variable, before the detail of the bad value or the codec error |
| `option_missing_argument` | Missing argument for option | `invalid_argument` | `parse_args()`: an option with no value after it, or a short letter that needs a value inside a group |
| `option_unknown_command` | Unknown command | `invalid_argument` | `parse_args()`: a default command that is not declared |
| `option_second_command` | Only one command is allowed | `invalid_argument` | `parse_args()`: a second command after the first |
| `option_no_such_input` | Invalid command, or no such file or URL | `invalid_argument` | `parse_args()`: a word that is no command, no file and no URL |

A codec gives its own errors. `parse_args()` and `load_env()` keep them in the report.
