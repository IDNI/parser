[back to index](../README.md#classes-and-structs)

# parser::options

`options` plain struct object can be provided to a `parser` to change its default behavior.

## attributes

### bool binarize;

If `binarize` is `true` `parser` applies binarization to ensure every forest node has at most 2 or less children nodes.

Binarization splits rules with more than two children nodes into multiple rules by adding new temporary production rules.

This adds additional intermediate `__temp` symbols into a resulting `forest`. Use `bool remove_binarization(graph&);` to remove these symbols.

Default value is false.

### bool incr_gen_forest;

If `incr_gen_forest` is true `parser` builds `forest`s incrementally as soon as any item is completed.

Default value is false.

### terminal_codec<C, T> codec;

Bundled decoder and encoder for converting between input character type `C` and terminal type `T`. See [recoders](recoders.md) for function signatures and built-in UTF-8 converters.

- `codec.decode` - decoder (`decoder_type`), default empty (no decoder)
- `codec.encode` - encoder (`encoder_type`), default empty (no encoder)

For `char` input and `char32_t` terminals, `idni::default_parser_options<C, T>()` pre-wires UTF-8 recoders.

## constructor

### parser<C, T>::options();

## example
```
parser<char, char32_t>::options opts;

opts.binarize = true;
opts.codec.decode = utf8_to_u32_conv; // lib provided utf8 to u32
opts.codec.encode = u32_to_utf8_conv; // lib provided u32 to utf8
```
