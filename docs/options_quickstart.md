[back to index](../README.md#classes-and-structs)
[full reference](options.md)

# options — quick start

```
namespace idni;
```

The options repository keeps every option of a program in one catalog. A program declares each option once. The command line, the environment, a REPL and an api then read and write the option by the same name. A value can live in a field of a struct, and a write by name goes into the field.

This page shows the steps of a program that switches from the old `cli` helper to the repository. For the full API, see the [options reference](options.md).


## 1. Include and namespace

```cpp
#include "utility/options.h"
using namespace idni;
```

`options()` gives the repository of the calling thread. The examples use it.


## 2. Declare a set

An `option_set` holds the options and the commands of a program. Each option has a name, an owner, a default value, a help text and its surfaces. The type of the default gives the kind: `bool`, `std::size_t`, `std::string` or `option_list`.

```cpp
const option_set tool_option_set = {
	.options = {
		{ "help", "cli", false, "show the help", { { "", 'h' } } },
		{ "verbose", "cli", false, "print more", { { "", 'v' } } },
		{ "count", "run", std::size_t{ 1 }, "how many times",
			{ { "run", 'n' } } },
		{ "name", "run", std::string("x"), "a name", { { "run", 's' } } },
		{ "tags", "run", option_list{}, "the tags", { { "run", 't' } } },
	},
	.commands = {
		{ "run", "run the tool" },
		{ "show", "show the state" },
	},
};

int main(int argc, char** argv) {
	auto declared = options().declare(tool_option_set);
	if (declared.has_error())
		return declared.report().print(std::cerr), 1;
	// ...
}
```

A second `declare()` of an equal spec is fine. A different spec under a declared name is an error, and nothing of the set is declared.


## 3. Bind the fields, unbind in the destructor

A struct of a library keeps its options in its own fields. `bind()` makes the field the place of the value. A write by name then goes into the field, and the owner reads the field directly.

```cpp
struct run_settings {
	std::size_t count = 1;
	std::string name = "x";
	diagnostics::report rep;

	run_settings() {
		rep.append(options().bind("count", count).report());
		rep.append(options().bind("name", name).report());
	}
	~run_settings() {
		options().unbind("count");
		options().unbind("name");
	}
};
```

The field type is the type of the option. A value that the repository stored before the bind goes into the field. The owner unbinds before the field ends. The last value of the field stays in the repository.


## 4. Add the surfaces

A surface puts an option on the command line. An option with no surface is not on the command line.

```cpp
{ "count", "run", std::size_t{ 1 }, "how many times",
	{ { "run" },                           // --count on run
	  { "show", 'n', std::size_t{ 3 } } } } // --count and -n on show, default 3
```

- The command name of the surface selects the command. The empty name is the surface of the options before the command.
- The letter is the short name: `-n`. Two options cannot use one letter on one command.
- The third field is a default for that command. It applies when the command line names that command, and only to an option that no field binds.

A program puts an option that a library declared on its own commands with `add_surface()`:

```cpp
auto added = options().add_surface("trim", { "run" });
```


## 5. Read the environment

`load_env()` reads one variable per option: the prefix plus the name in upper case, with `-` as `_`.

```cpp
auto env = options().load_env("TOOL_");      // TOOL_COUNT, TOOL_NAME, ...
if (env.has_error()) return env.report().print(std::cerr), 1;
```

An option declared with `from_env` false is not read. A bad value is an error that names the variable.


## 6. Parse the command line and print the help

`parse_args()` stores the values of argv. An `args_config` gives the program data.

```cpp
const args_config cfg{
	.name = "tool",
	.help_header = "tool <file>",
	.description = "a test tool",
	.default_command = "run",
};
auto parsed = options().parse_args(argc, argv, cfg);
if (!parsed.has_value()) return parsed.report().print(std::cerr), 1;
const parsed_args& args = parsed.value();

if (options().get<bool>("help")) {
	options().help(std::cout, args.command_given ? args.command : "");
	return 0;
}
for (const auto& input : args.inputs) std::cout << input << "\n";
```

- `args.command` is the command, given or default.
- `args.inputs` holds `-`, the files and the URLs.
- `args.values` holds the options of argv, in argv order. A program writes them again after it loads a grammar.

Nothing is stored on an error. The report of the error carries the help of the program.

The program writes its sources in order. A later write replaces the value, so the command line wins over the environment:

```cpp
auto env = options().load_env("TOOL_");
auto parsed = options().parse_args(argc, argv, cfg);
```


## 7. Read and write by name from a REPL

A REPL command such as `set count 4` takes text. `set_text()` reads the text with the kind of the option, and `get_text()` gives text that reads back as the same value.

```cpp
auto written = options().set_text("count", "4");
if (written.has_error()) written.report().print(std::cerr);

auto text = options().get_text("tags");     // "a, b, c"
if (text.has_value()) std::cout << text.value() << "\n";

for (const auto& name : options().names())
	if (auto spec = options().find(name); spec && spec->owner == "run")
		std::cout << name << "\t" << spec->help << "\n";
```

- `names()` lists the declared options in the order of the declarations.
- `find()` gives the spec, or null for an unknown name.
- An unknown name or a bad value is an error, and nothing changes.

A typed write and read of a program:

```cpp
options().set("count", 3);
options().set("name", "abc");
std::size_t n = options().get<std::size_t>("count");
```


## 8. Reset an owner or one option

```cpp
options().reset("run");                         // every option of the owner run
auto one = options().reset_option("count");     // one option
```

A reset writes the default of the spec into a bound field. An unbound option loses its stored value.


## 9. A codec for a field of another type

A field of a type that is not an option type needs a codec. The codec converts the field to the option value and back.

```cpp
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
auto bound = options().bind("ids", ids, int_set_codec{});
auto bad = options().set_text("ids", "x");    // "not a number", ids does not change
```

The option `ids` is a list option. The error of `from_value` goes into the result of the write.


## 10. A hook after a write

A hook runs after each write of a bound option. It does not run on a failed write.

```cpp
void apply_colors(bool on);

bool colors = true;                           // "colors" is a declared flag
auto bound = options().bind("colors", colors, [&] { apply_colors(colors); });
```

`set()`, `set_text()`, `load_env()`, `parse_args()`, `reset()` and `reset_option()` all run the hook.


## 11. A repository for a scope

Each thread has its own repository. A server session or a test that needs its own values makes a repository current for a scope:

```cpp
options_repository mine;
{
	options_scope scope(mine);
	auto declared = options().declare(tool_option_set);
	options().set("count", 5);                // writes to mine
}                                             // the previous repository is current again
```

A repository and the fields it binds belong to the thread that created the repository.


## 12. The switch table

| the old `cli` helper | options repository |
|-----------------|--------------------|
| `cli::option("count", 'n', 1).set_description("how many times")` | `option_spec{ "count", "run", std::size_t{ 1 }, "how many times", { { "run", 'n' } } }` |
| `cli::option::value`, a `std::variant<std::string, bool, int>` | `option_value`, a `std::variant<bool, std::size_t, std::string, option_list>` |
| an `int` option, `get<int>("port")` | a `std::size_t` option, `get<std::size_t>("port")` |
| `cli::options` of the program | options with a surface on `args_config::global_surface`, usually `""` |
| `cli::command("run", "run the tool")` | `command_spec{ "run", "run the tool" }` in `option_set::commands` |
| `command::add_option(o)` | a surface `{ "run" }` in the spec, or `add_surface("count", { "run" })` |
| `cli cl(name, argc, argv, cmds, dflt_cmd, opts)` | `options().declare(set)`, then `options().parse_args(argc, argv, cfg)` |
| `set_description(d)` | `args_config::description` |
| `set_help_header(h)` | `args_config::help_header` |
| `set_default_command(c)` | `args_config::default_command` |
| `set_default_command_when_files(c)` | `args_config::default_command_when_inputs` |
| `process_args()` and `status()` | the result of `parse_args()`: `has_value()` or `has_error()` |
| `get_processed_options()["help"].get<bool>()` | `options().get<bool>("help")` |
| `get_processed_command()` and `command::name()` | `parsed_args::command` |
| `command::ok()` | the result of `parse_args()` has a value: an unknown command is an error |
| `command::get<bool>("force")` | `options().get<bool>("force")` |
| `command::has("start")` | `options().find("start")` and the `surfaces` of the spec |
| `command::operator[]("count").set(argv)` | `options().set_text("count", text)` |
| `option::set(v)` | `options().set("count", v)` |
| `option::is_bool()`, `is_int()`, `is_string()` | `kind_of(*options().find(name))` is `option_kind::flag`, `numeric` or `text` |
| `option::short_name()`, `option::description()` | the `surfaces` and the `help` of `options().find(name)` |
| `get_files()` | `parsed_args::inputs` |
| `help()`, `help(cmd)` | `options().help(std::cout)`, `options().help(std::cout, "run")` |
| `error(msg, true)` | the report of the failed result, with the help as an info node |
| `set_output_stream(os)` | the stream argument of `help(os)` |
| `set_error_stream(os)` | the report of each result, printed to a stream of the program |
| a struct filled from `cmd.get<...>()` after the parse | `bind()` of each field, so each write goes into the field |
| no environment | `options().load_env("TOOL_")` |


## Further reading

- [options reference](options.md) - full API
- [diagnostics quick start](diagnostics_quickstart.md) - the `result<T>` and the report of each call
- [`options.h`](../src/utility/options.h) - the repository
- [`tgf_cli_options.h`](../src/tgf/tgf_cli_options.h) - the option set of the tgf tool
