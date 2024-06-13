[back to index](../README.md#classes-and-structs)

# terminals extraction from forest

Sometimes it is required to quickly collect all terminals which have been parsed under a node.

For example you could have a non-terminal called identifier. When you reach such a node you don't care about the structure bellow. You can just flatten terminals of the non-terminal into a string as a parsed name of the identifier.

## functions

### std::basic_ostream\<T>& terminals_to_stream(std::basic_ostream\<T>& os, const pforest& f, const pnode& r);

```
template <typename C = char, typename T = C>
std::basic_ostream<T>& terminals_to_stream(std::basic_ostream<T>& os,
	const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& r);
```

Flattens terminals of a subforest of a node `r` of a forest `f` into an ostream `os`

### std::basic_string\<T> terminals_to_str(const pforest& f, const pnode& r);

```
template <typename C = char, typename T = C>
std::basic_string<T> terminals_to_str(const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& r);
```

Flattens terminals of a subforest of a node `r` of a forest `f` into a `std::basic_string<T>`

### int_t terminals_to_int(const pforest& f, const pnode& r);

```
template <typename C = char, typename T = C>
int_t terminals_to_int(const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& r);
```

Flattens terminals of a subforest of a node `r` of a forest `f` into an `int_t`

Example:
```
const char* g_tgf =
	" integer    => ['-'] digit+. "
	" string     => '"' printable+ '"'. "
	" start      => integer | string. ";
nonterminals nts;
grammar g(tgf<>::from_string(nts, g_tgf));
parser p(g);

auto f = p.parse("-245");
if (!p.found()) {
	cerr << "parse_error: " << p.get_error().to_str() << endl;
	return;
}

auto cb_enter = [&r, &out_of_range, &f, this](const auto& n) {
	if (!n.first.nt()) return; // skip if terminal
	string nt = nts.get(n.first.n());
	else if (nt == "integer") {
		bool out_of_range;
		int_t i = terminals_to_int(*f, out_of_range);
		if (out_of_range) cerr << "integer is out of range" << endl;
		else cout << "parsed integer: " << i;
	} else if (nt == "str")
		cout << "parsed string: " << terminals_to_str(*f, n);
};

f->traverse(cb_enter); // traverse f and run cb_enter callbeck when entering each node
```

Program would produce:
```
parsed integer: -245
```
