[back to index](../README.md#classes-and-structs)

# character class functions

A production rule can have many single terminals. You can give them as one function. The function takes a character and returns `true` if the character satisfies the rule.

For example, instead of a following rule:
```
digit => '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'.
```
you can create a function and pass it to the grammar when instantiated:
```
nonterminals nts;
char_class_fns cc;
prods ps, start(nts("start")), digit(nts("digit"));
ps(start, digit);
cc(nts.get("digit"), [](char c) { return (c >= '0') && (c <= '9'); });
grammar g(nts, ps, start, cc);
```

# char_class_fn

```
using char_class_fn = std::function<bool(T)>;
```

`char_class_fn` is a type used for character class functions.

Character class function takes a terminal character and returns `true` if a character is a part of a character class.

# char_class_fns

`char_class_fns` is a container for character class functions.

You give it to the constructor of a `grammar`.

## methods

### void operator()(size_t nt, const char_class_fn<T>& fn);

This operator adds a new char class function.

```
nonterminals<char> nts;
size_t binary_digit = nts("binary_digit");
char_class_fns<char> cc;
cc(binary_digit, [](char c) { return c == '0' || c == '1'; });
```

### bool is_fn(size_t nt) const;

Checks if the non-terminal with the id `nt` is a character class function.

```
cc.is_fn(binary_digit);
```

### bool is_eof_fn(size_t nt) const;

Checks if the non-terminal with the id `nt` is the `eof` character class function.

```
cc.is_eof_fn(1);
```


## predefined character class functions

The library has common [character classes](charclasses.md). A helper function returns a `char_class_fns` container with one or more of these predefined functions.

### char_cc_fns\<T> predefined_car_classes(const std::vector\<std::string>& cc_fn_names, nonterminals<C, T>& nts);

`nts` is a reference to `nonterminals`. `cc_fn_names` is a vector of names of functions we want to use. Any of the following function names is available:

```
// { function name, predefined function used }

{ "eof",       charclasses::iseof<T>    },
{ "any",       charclasses::isany<T>    },
{ "ascii",     charclasses::isascii<T>  },
{ "alnum",     charclasses::isalnum<T>  },
{ "alpha",     charclasses::isalpha<T>  },
{ "blank",     charclasses::isblank<T>  },
{ "cntrl",     charclasses::iscntrl<T>  },
{ "digit",     charclasses::isdigit<T>  },
{ "graph",     charclasses::isgraph<T>  },
{ "lower",     charclasses::islower<T>  },
{ "printable", charclasses::isprint<T>  },
{ "punct",     charclasses::ispunct<T>  },
{ "space",     charclasses::isspace<T>  },
{ "upper",     charclasses::isupper<T>  },
{ "xdigit",    charclasses::isxdigit<T> },
{ "unicode_alnum",     charclasses::unicode::isalnum<T>  },
{ "unicode_alpha",     charclasses::unicode::isalpha<T>  },
{ "unicode_blank",     charclasses::unicode::isblank<T>  },
{ "unicode_cntrl",     charclasses::unicode::iscntrl<T>  },
{ "unicode_digit",     charclasses::unicode::isdigit<T>  },
{ "unicode_graph",     charclasses::unicode::isgraph<T>  },
{ "unicode_lower",     charclasses::unicode::islower<T>  },
{ "unicode_printable", charclasses::unicode::isprint<T>  },
{ "unicode_punct",     charclasses::unicode::ispunct<T>  },
{ "unicode_space",     charclasses::unicode::isspace<T>  },
{ "unicode_upper",     charclasses::unicode::isupper<T>  },
{ "unicode_xdigit",    charclasses::unicode::isxdigit<T> }
```

The names without a prefix are the plain classes. They answer as `<cctype>`
in the C locale, also for `char32_t`.
The names with the prefix `unicode_` are the Unicode classes.
For `char32_t` they use the Unicode tables.
For `char` they give the same answer as the plain class.

### char_cc_fns\<T> predefined_char_classes(const std::vector\<std::string>& cc_fn_names, const std::map\<std::string, std::string>& renames, nonterminals<C, T>& nts);

`renames` maps a predefined class to its new name.
The grammar then uses only the nonterminal of the new name for the class.
A renamed class loads also when `cc_fn_names` does not list it.
A class in `cc_fn_names` and in `renames` loads only under its new name.
All renames apply at the same time, so two classes can swap their names.
```
predefined_char_classes({ "space" }, { { "digit", "d" } }, nts);
predefined_char_classes({}, { { "digit", "alpha" }, { "alpha", "digit" } }, nts);
```
These are errors, and each error names the name:
- Two renames give the same new name.
- A new name is the name of a class that keeps its own name, for example `{ "digit" }` with `{ { "alpha", "digit" } }`.
- A name in `cc_fn_names` occurs two times.
- A class is not a predefined class.

`char_class_fns::renamed` keeps the source class of each renamed class, by its new name.
Only the parser generator reads it.

Example:
```
nonterminals nts;
char_cc_fns cc = predefined_char_classes({ "alpha", "digit", "space" }, nts);
prods ps;
// ... populate ps with production rules ...
grammar g(nts, ps, nts("start"), cc);
```