[back to index](../README.md#classes-and-structs)

# parser::input

`parser<C, T>::input` is a wrapper for an input data pointer, an input stream or a file (by filedescriptor). It provides an API to access the input data.

It is mostly used internally by `parser` but it is also passed to an input decoder function (can be configured in `parser::options`).

## methods

### C cur();

Returns the current character.

### size_t pos();

Returns the position of the current character.

### bool next();

Moves to the next character. Increments position and updates the current character.

### bool eof();

Returns `true` if the input reached the end of file or a `size` stated in `parse()` call.

The end is a position. Every byte before it is input, also a NUL and a
byte 0xFF. Only an `eof` value that a caller sets to a character value
(see `parse_options::eof`) also ends the input at that character.

## example

See a decoder `utf8_to_u32_conv` in [recoders](recoders.md) which is using `in.cur()` and `in.next()` to read multibytes of various lengths. The decoder function reads 1 to 4 chars (UTF-8) from the input and converts them into a single `char32_t` (Unicode).
