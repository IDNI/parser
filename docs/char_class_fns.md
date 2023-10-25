[back to index](../README.md#overview-of-types)

# character class functions

In a case you need a production rule with many single terminals you can specify them by a function which accepts a character and returns `true` if a terminal character satisfies the production rule.

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

It can be passed to a `grammar` when instantiated.

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

## predefined character class functions

Library provides common [character classes](charclasses.md) and it offers a helper function which can return a `char_class_fns` container with one or more of these predefined functions.

### char_cc_fns\<T> predefined_car_classes(const std::vector\<std::string>& cc_fn_names, nonterminals<C, T>& nts);

`nts` is a reference to `nonterminals`. `cc_fn_names`` is a vector of names of functions we want to use. Any of the following function names is available:

```
// { function name, predefined function used }

{ "eof",       charclasses::iseof<T>    },
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
{ "xdigit",    charclasses::isxdigit<T> }
```

It is used like this:
```
nonterminals nts;
char_cc_fns cc = predefined_char_classes({ "alpha", "digit", "space" }, nts);
prods ps;
// ... populate ps with production rules ...
grammar g(nts, ps, nts("start"), cc);
```