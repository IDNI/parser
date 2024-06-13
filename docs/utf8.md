[back to index](../README.md#classes-and-structs)

# UTF-8

`parser` library provides own implementation of UTF-8 support so it does not depend on other Unicode librarires.

## types

Library defines `utf8char` and `utf8string` based on unsigned char:

```
typedef unsigned char utf8char;
typedef std::basic_string<utf8char> utf8string;
```

## functions

### bool is_mb_codepoint(utf8char ch, uint8_t p = 0);

Checks if character is a begining of a multibyte codepoint or if it is a part of a multibyte codepoint.


### size_t peek_codepoint(const utf8char* s, size_t l, char32_t &ch);

Converts const utf8char* sequence s of 1-4 utf8 code units into codepoint `ch`.
- `s` is a string of unsigned chars containing utf8 text
- `l` is a size of the s string
- `ch` is a reference to a codepoint which is populated by the result

Returns a byte size of a codepoint (0 - 4 bytes) or `(size_t) -1` if illegal UTF-8 code unit.


### size_t codepoint_size(char32_t ch);

Returns a byte size (0 - 4) of a Unicode codepoint `ch` in bytes.


### size_t emit_codepoint(char32_t ch, utf8char *s);

Converts an Unicode codepoint `ch` to an UTF-8 sequence of `unsigned char`s into an address `s`. Be sure to have an allocated space of at least 4 bytes.

Returns a byte size of the codepoint `ch`.


### std::basic_ostream\<utf8char>& emit_codepoint(std::basic_ostream\<utf8char>& os, char32_t ch);

Converts `ch` unicode codepoint to a 1-4 utf8chars and puts them into a stream `os`

Returns the stream `os`.


### std::vector<utf8char>& emit_codepoint(std::vector<utf8char>& v, char32_t ch);

Converts `ch` unicode codepoint to 1 - 4 utf8chars and pushes them into a referenced vector `v`.

Returns the reference to the `v` vector


### std::basic_ostream<char32_t>& operator<<(std::basic_ostream<char32_t>& os, const char* s);

Stream `<<` operator converting a C string `s` into u32 before putting it into the u32 stream `os`.


### std::basic_ostream<char32_t>& operator<<(std::basic_ostream<char32_t>& os, const std::string& s);

Stream `<<` operator converting a std::string `s` into u32 before putting it into the u32 stream `os`.


## conversion functions

### conversions to utf8string

```
utf8string to_utf8string(int32_t v);
utf8string to_utf8string(char ch);
utf8string to_utf8string(const char* s);
utf8string to_utf8string(const std::string& s);
utf8string to_utf8string(char32_t ch);
utf8string to_utf8string(const std::u32string& str);
```

### conversions to u32string

```
std::u32string to_u32string(const std::u32string& str);
std::u32string to_u32string(const utf8string& str);
std::u32string to_u32string(const std::string& str);
```

### conversions to std::string

```
std::string to_string(const std::string& s);
std::string to_string(const utf8string& s);
std::string to_string(const std::u32string& s);
std::string to_string(const char32_t& s);
```

#### alternatively

```
std::string to_std_string(const std::string& s);
std::string to_std_string(const utf8string& s);
std::string to_std_string(const std::u32string& s);
std::string to_std_string(const char32_t& ch);
```

### templated conversions

templated conversions of chars and strings based on char or char32_t to strings based on char or char32_t. This is useful for specifying a result type during a compilation.

```
template <typename CharT>
typename std::basic_string<CharT> from_cstr(const char *);

template <typename CharT>
typename std::basic_string<CharT> from_cstr(const char32_t *);

template <typename CharT>
typename std::basic_string<CharT> from_str(const std::string&);

template <typename CharT>
typename std::basic_string<CharT> from_str(const std::u32string&);
```
