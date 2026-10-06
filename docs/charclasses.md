[back to index](../README.md#classes-and-structs)

# charclasses

The library gives character class functions like the functions of `<cctype>`.
All functions are in the namespace `idni::charclasses`.
To use them without the rest of the library, include only `charclasses.h`.

Each function is a template on the character type `C`.
`C` is `char` or `char32_t`.

Character class functions:
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

Three more functions work for each character type:
```
template <typename C> bool iseof(C c);   // c == std::char_traits<C>::eof()
template <typename C> bool isany(C c);   // !iseof(c)
template <typename C> bool isascii(C c); // c <= 0x7F
```

## char

The `char` functions call the `<cctype>` function of the same name.
They cast the character to `unsigned char` before the call.

## char32_t

The `char32_t` functions use real Unicode classes.
Each function reads one bit from a lookup table in
`charclasses_unicode.generated.h`.
A code point above U+10FFFF has no class, so each function returns false.
The end of file value is above U+10FFFF, so it also has no class.

The table uses these Unicode properties:

| function | Unicode property |
|---|---|
| `isalpha` | Alphabetic |
| `isupper` | Uppercase |
| `islower` | Lowercase |
| `isdigit` | General Category Nd, all decimal digits and not only 0 to 9 |
| `isxdigit` | `isdigit`, A to F, a to f, and the fullwidth A to F and a to f |
| `isalnum` | `isalpha` or `isdigit` |
| `isspace` | White_Space |
| `isblank` | U+0009 (TAB) or General Category Zs |
| `iscntrl` | General Category Cc |
| `ispunct` | General Category P, and the ASCII symbols that `<cctype>` counts |
| `isgraph` | Not `iscntrl`, not `isspace`, not U+0000, and not in the categories Cn, Cs, Cf, Zl or Zp |
| `isprint` | `isgraph` or General Category Zs |

For ASCII, the `char32_t` functions give the same result as the `char`
functions in the C locale.
The ASCII symbols in `ispunct` are ``$ + < = > ^ ` | ~``.
TAB is blank, but it is not printable.

## Unicode tables

The `char32_t` functions read the tables in
`src/utility/charclasses_unicode.generated.h`.
The script `tools/unicode/gen_charclasses.py` writes this header from the
Unicode Character Database.
Do not edit the header by hand.
Change the script and run it again.

To write the header again, run this command from the parser root:
```
python3 tools/unicode/gen_charclasses.py
```

The script downloads `UnicodeData.txt`, `DerivedCoreProperties.txt`,
`PropList.txt` and `ReadMe.txt` of Unicode 17.0.0 into
`~/.cache/idni-ucd/<version>/`.
The script stops with an error when a file is missing.
It also stops when the version in `ReadMe.txt`, `PropList.txt` or
`DerivedCoreProperties.txt` is not the version that the script asks for.
It needs Python 3 and no other package.

Arguments:
- `--unicode-version <version>` downloads the files of a different Unicode version.
- `--cache-dir <dir>` keeps the downloaded files in a different folder.
- `--ucd-dir <dir>` reads the four files from a local folder and downloads nothing.
- `--output <file>` writes the header to a different file.
- `--selftest` checks the tables against the data files and the ASCII `<cctype>` classes.
  It writes no file.
