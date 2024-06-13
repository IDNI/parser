[back to index](../README.md#classes-and-structs)

# recoders

Recoders are functions required for parsing input data in a different character type (template type `C`) than is a type of terminals (template type `T`).

Decoder decodes input type `C` into terminal type `T`.

Encoder encodes terminal type `T` into input type `C`.

Both can be passed to a `parser` in [`parser::options`](parser_options.md).

### parser<C, T>::decoder_type

```
using decoder_type = std::function<std::vector<T>(parser<C, T>::input&)>
```

When parser calls a decoder it passes a reference to [`parser<C, T>::input`](docs/parser_input.md) as an argument. Decoder returns a vector of terminals produced by the input char. It is a conversion from a templated type `C` to a templated type `T`.

### parser<C, T>::encoder_type

```
using encoder_type  = std::function<std::basic_string<C>(const std::vector<T>&)>;
```

Encoder takes a vector of terminals and returns a string with char type of the input. It is a conversion from a templated type `T` to a templated type `C`.

## UTF-8 recoders

This library currently provides decoder and encoder for conversions between UTF-8 in `char` and Unicode in `char32_t`.

## utf8_to_u32_conv

Decoder for UTF-8 in `char` into Unicode in `char32_t`.

```
parser<char, char32_t>::decoder_type utf8_to_u32_conv =
	[](idni::parser<char, char32_t>::input& in) {
		std::vector<char32_t> r;
		idni::utf8char s[4] = { 0, 0, 0, 0 };
		char32_t ch;
		for (uint8_t p = 0; p != 3; ++p) {
			s[p] = in.cur();
			if (!idni::is_mb_codepoint(s[p], p))
				return idni::peek_codepoint(s, p + 1, ch),
					in.next(), r = { ch };
			if (!in.next()) return r;
		}
		return in.next(), r;
	};
```

## u32_to_utf8

Encoder for Unicode in `char32_t` into UTF-8 in `char`.

```
parser<char, char32_t>::encoder_type u32_to_utf8_conv =
	[](const std::vector<char32_t>& ts) {
		std::stringstream ss;
		for (auto t : ts) ss << idni::to_string(t);
		return ss.str();
	};
```