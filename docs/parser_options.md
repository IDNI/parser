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

### size_t gc_lag;

number of steps garbage collection lags behind parsing position n. should be greater than 0 and less than the size of the input for any collection activity. We cannot use % since in the streaming case, we don't know exact size in advance.

Default value is 1.

### decoder_type chars_to_terminals;

decoder function reading an input converting element or elements of a type `C` to a vector of elements of type `T` according to template type parameters `T` and `C`. More about these recorders [here](recoders.md).

Default value is 0, ie. no decoder function.

### encoder_type terminals_to_chars;

encoder function converting vector of terminals of a type `T` to a `std::basic_string\<C>` according to template type parameters `T` and `C`. More about these recoders [here](recoders.md).

Default value is 0, ie. no encoder function.

## constructor

### parser<C, T>::options();

## example
```
parser<char, char32_t>::options opts;

opts.binarize = true;
opts.gc_lag = 10;
opts.chars_to_terminals = utf8_to_u32_conv; // lib provided utf8 to u32
opts.terminals_to_chars = u32_to_utf8_conv; // lib provided u32 to utf8
```
