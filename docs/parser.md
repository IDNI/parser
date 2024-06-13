[back to index](../README.md#classes-and-structs)

# parser

## constructor

```
parser(grammar<C, T>& g, const options& o = {});
```

Instantiates a new parser with a given grammar `g` and options `o`.

```
parser parser_with_default_options(g); // g is a grammar created before

parser<>::options o;
o.binarize = true;
parser<> parser_with_binarization(g, o);
```

## methods

### parser<C, T>::result parse(..., parse_options po);

Depending on your input source there are several parse methods available:

```
parser<C, T>::result parse(const C* data, size_t size, parse_options po);
```
```
parser<C, T>::result parse(std::basic_istream<C>& is, parse_options po);
```
```
parser<C, T>::result parse(const std::string& filename, parse_options po);
```
```
parser<C, T>::result parse(int filedescriptor, parse_options po); // WIN32 only
```

`parse` accepts an input `data` pointer with its `size`, an input stream `is`, a `filename` or a windows `filedescriptor`.

Another argument is `parser::parse_options` and it's optional.
- `size` length of the data
- `max_length` to specify a maximum length of the parsed input or 0 if not limiting (defaults to 0)
- `eof`, ie end of file character (defaults to `std::char_traits<C>::eof()`)

`parse` method call returns `parse<C, T>::result`

```
parser p(g); // g is a grammar created before

// parse data from a data pointer
auto r1 = p.parse("1+2/1", 5);

// parse up to 5 characters from a file (by provided file descriptor)
auto r2 = p.parse("input.txt");

// parse up to 10 characters from an input stream
// terminating if an end of line reached
auto r3 = p.parse(is, { .max_length = 10, .eof = '\n' });
```

### bool found();

`found` method returns true if the last `parse` method call matched a starting literal production rule. This means that the parsed input was parsed fully and successfully and there exists at least one tree with a root being the starting literal. If this method returns false use `parser<C, T>::get_error()` to obtain more information.

```
auto f = p.parse("3/3"); // p is a parser created before
if (p.found()) std::out << "input parsed successfuly\n";
else std::cerr << "parse error\n";
```

### parser<C, T>::error get_error();

Returns `parser<C, T>::error` object containing information about a parsing error if any happened, ie. `found()` call returned `false`.

```
auto f = p.parse("3/"); // p is a parser created before
if (!p.found()) std::cerr << "parse error: " << p.get_error().to_str() << '\n';
```
