[back to index](../README.md#overview-of-types)

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

### std::unique_ptr<parser<C, T>::forest> parse(..., size_t size, int_type eof);

```
std::unique_ptr<parser<C, T>::forest> parse(const C* data,
	size_t size, size_t max_length, int_type eof);
```
```
std::unique_ptr<parser<C, T>::forest> parse(int filedescriptor,
	size_t max_length, int_type eof);
```
```
std::unique_ptr<parser<C, T>::forest> parse(std::basic_istream<C>& is,
	size_t max_length, int_type eof);
```

`parse` accepts an input `data` pointer, a `filedescriptor` of an input file or an input stream `is`

Other two parameters are optional:
- `size` length of the data
- `max_length` to specify a maximum length of the parsed input or 0 if not limiting (defaults to 0)
- `eof`, ie end of file character (defaults to `std::char_traits<C>::eof()`)

`parse` returns unique pointer to the parsed forest `std::unique_ptr<parser<C, T>::forest>`

```
parser p(g); // g is a grammar created before

// parse data from a data pointer
auto f1 = p.parse("1+2/1", 5);

// parse up to 5 characters from a file (by provided file descriptor)
int fd = open("input.txt", O_RDONLY);
auto f2 = p.parse(fd, 5);
close(fd);

// parse up to 10 characters from an input stream
// terminating if an end of line reached
std::ifstream is("input.txt");
auto f3 = p.parse(is, 10, '\n');
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
