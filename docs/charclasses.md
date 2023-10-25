[back to index](../README.md#overview-of-types)

# charclasses

Library provides `<cctype>`-based character class functions.

Functions can be used independently on the rest of the library by including just `charclasses.h`.

Note that all these functions are declared in a namespace `idni::charclasses`.

Functions are templated with a `C` template which can be either `char` or `char32_t`.

`char` templated functions wrap `<cctype>` functions of the same name from a global namespace.

For `char32_t` library provides simplified implementations so far so they can be used. But be aware that it is not a proper Unicode classification.
Though it is possible to implement proper classification with custom character class functions.

`<cctype>`-based character class functions:
```
template <typename C> bool isalnum(C c);
template <typename C> bool isalpha(C c);
template <typename C> bool isblank(C c);
template <typename C> bool iscntrl(C c);
template <typename C> bool isdigit(C c);
template <typename C> bool isgraph(C c);
template <typename C> bool islower(C c);
template <typename C> bool isprint(C c);
template <typename C> bool ispunct(C c);
template <typename C> bool isspace(C c);
template <typename C> bool isupper(C c);
template <typename C> bool isxdigit(C c);
```

Additionally there is also `iseof()` function:
```
template <typename C> bool iseof(C c);
			// returns c == std::char_traits<C>::eof();
```
