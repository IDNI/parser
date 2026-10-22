[back to index](../README.md#classes-and-structs)

# charclasses

The library gives character class functions like the functions of `<cctype>`.
All functions are in the namespace `idni::charclasses`.
To use them without the rest of the library, include only `charclasses.h`.

Each function is a template on the character type `C`.
`C` is `char`, `char32_t` or another integral type.
Another integral type, such as `unsigned int`, converts to `char32_t` and
gets the `char32_t` answer.

There are two sets of the twelve character class functions.
The plain set is in `idni::charclasses`.
The Unicode set has the same names and is in `idni::charclasses::unicode`.
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

## Plain classes

The plain classes give the answer of `<cctype>` in the C locale.

- The `char` functions call the `<cctype>` function of the same name.
  They cast the character to `unsigned char` before the call.
- The `char32_t` functions give the same answer for ASCII.
  A code point above U+007F is in no plain class.
  For example, `isdigit<char32_t>` is false for the code point U+0663.

## Unicode classes

The Unicode classes in `idni::charclasses::unicode` use real Unicode classes.

- The `char` functions give the same answer as the plain `char` functions.
- The `char32_t` functions read one bit from a lookup table in
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

For ASCII, the table gives the same result as `<cctype>` in the C locale.
The ASCII symbols in `ispunct` are ``$ + < = > ^ ` | ~``.
TAB is blank, but it is not printable.

## Unicode properties

`src/utility/unicode_properties.h` gives more properties of one code point,
in the namespace `idni::charclasses::unicode`.
Each function takes a `char32_t`.
A value above U+10FFFF has no property.

| function | result |
|---|---|
| `decimal_digit_value(cp)` | The value 0 to 9 when Numeric_Type is Decimal, else no value |
| `has_digit_value(cp)` | True when Numeric_Type is Decimal or Digit, for example U+00B2 |
| `is_letter(cp)` | True for the General Category Lu, Ll, Lt, Lm or Lo. A mark is no letter |
| `has_numeric_value(cp)` | True when Numeric_Type is Decimal, Digit or Numeric, for example U+00BD |
| `is_cased(cp)` | True for the property Cased |
| `is_case_ignorable(cp)` | True for the property Case_Ignorable |
| `to_lower(cp)` | The simple lowercase mapping, or `cp` |
| `to_lower_full(cp, out)` | Appends the unconditional full lowercase mapping to `out`, else the simple one |
| `to_lower_full(text, out)` | Appends the full lowercase mapping of each code point of `text` to `out`, with the Final_Sigma rule |
| `to_upper(cp)` | The simple uppercase mapping, or `cp` |
| `to_upper_full(cp, out)` | Appends the unconditional full uppercase mapping to `out`, else the simple one. U+00DF gives `SS` |
| `to_upper_full(text, out)` | Appends the full uppercase mapping of each code point of `text` to `out` |

The section [Python string classes](#python-string-classes) uses these functions for the rules of the Python `str` methods.
The one code point form of `to_lower_full` has no mapping that needs the
context. The text form has the one rule of the full mapping that needs
the context and no language: U+03A3 gives U+03C2 at the end of a word.
The rules for one language, such as Turkish or Lithuanian, are not here.
No uppercase mapping needs the context, so both forms of `to_upper_full`
give the same mapping for each code point.

The tables are Unicode 17.0.0. A program that must match another Unicode
version can differ for the code points that are new or changed after that
version.

## Python string classes

`src/utility/python_charclasses.h` gives the character classes of the CPython `str` methods,
in the namespace `idni::charclasses::python`.
Each class function takes one `char32_t` and returns `true` when the Python method returns `True` for a string of that one code point.
A value above U+10FFFF is in no class.
The functions use the tables of the Unicode properties.
`unicode::decimal_digit_value` gives the value of the digits that Python `int()` reads.

| function | Python method | result |
|---|---|---|
| `isdecimal(c)` | `str.isdecimal` | True when Numeric_Type is Decimal |
| `isdigit(c)` | `str.isdigit` | True when Numeric_Type is Decimal or Digit, for example U+00B2 |
| `isnumeric(c)` | `str.isnumeric` | True when Numeric_Type is Decimal, Digit or Numeric, for example U+00BD |
| `isalpha(c)` | `str.isalpha` | True for the General Category Lu, Ll, Lt, Lm or Lo |
| `isalnum(c)` | `str.isalnum` | `isalpha` or `isnumeric` |
| `isspace(c)` | `str.isspace` | True for White_Space and for the Bidi_Class WS, B and S |
| `isprintable(c)` | `str.isprintable` | True for U+0020, and for no General Category Cc, Cf, Cs, Co, Cn, Zl, Zp or Zs |
| `lower(s, out)` | `str.lower` | Appends the full lowercase mapping of `s` to `out`, with the Final_Sigma rule |
| `upper(s, out)` | `str.upper` | Appends the full uppercase mapping of `s` to `out` |
| `lower(text)` | `str.lower` | The lowercase of the UTF-8 `text` as a `result<std::string>`. Invalid UTF-8 gives `code::invalid_utf8` with the byte offset in the label `offset` |
| `upper(text)` | `str.upper` | The uppercase of the UTF-8 `text` as a `result<std::string>`, with the same error as `lower(text)` |

## Names in a grammar

A grammar names a plain class with its short name, for example `digit`.
It names a Unicode class with the prefix `unicode_`, for example
`unicode_digit`.
See [character class functions](char_class_fns.md) for the full list.

## Unicode tables

The Unicode classes read the tables in
`src/utility/charclasses_unicode.generated.h`, and the Unicode properties
read the tables in `src/utility/unicode_properties.generated.h`.
The script `tools/unicode/gen_charclasses.py` writes both headers from the
Unicode Character Database.
Do not edit the headers by hand.
Change the script and run it again.

To write the header again, run this command from the parser root:
```
python3 tools/unicode/gen_charclasses.py
```

The script downloads `UnicodeData.txt`, `DerivedCoreProperties.txt`,
`PropList.txt`, `SpecialCasing.txt`, `extracted/DerivedNumericType.txt` and
`ReadMe.txt` of Unicode 17.0.0 into `~/.cache/idni-ucd/<version>/`.
The script stops with an error when a file is missing.
It also stops when the version in `ReadMe.txt`, `PropList.txt`,
`DerivedCoreProperties.txt`, `SpecialCasing.txt` or `DerivedNumericType.txt`
is not the version that the script asks for.
It needs Python 3 and no other package.

Arguments:
- `--unicode-version <version>` downloads the files of a different Unicode version.
- `--cache-dir <dir>` keeps the downloaded files in a different folder.
- `--ucd-dir <dir>` reads the six files from a local folder and downloads nothing.
  The folder keeps `DerivedNumericType.txt` in its subfolder `extracted`.
- `--output <file>` writes the header of the classes to a different file.
- `--properties-output <file>` writes the header of the properties to a different file.
- `--selftest` checks the tables against the data files and the ASCII `<cctype>` classes.
  It writes no file.
