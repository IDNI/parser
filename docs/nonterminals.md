[back to index](../README.md#classes-and-structs)

# nontereminals

```
template <typename C = char, typename T = C> struct nonterminals;
```

`nonterminals` is a container for maping ids of non-terminal literals to their names.

Since this library sees nonterminals only as their ids this container provides some convenience to use string names and get string names back when using the library.

It is used by various parts of the library (`lit`, `prods`, `grammar`, `tgf`...).

## constructors

### nonterminals();

Create new empty container for non-terminals.

```
nonterminals nts;
nonterminals<char32_t> nts32;
nonterminals<char, char32_t> nts_utf8;
```

### nonterminals(const std::vector<std::basic_string<C>>& list);
### nonterminals(std::initializer_list<std::basic_string<C>> init_list);

Create a container and initialize it with the provided list.

```
nonterminals nts({ "start", "sym", "char" });
nonterminals<char32_t> nts32({ U"start", U"expr" });
nonterminals<char, char32_t> nts_utf8(std::vector<std::string>>{ "start", "a", "b" });
```


## methods

### size_t get(const std::basic_string\<C>& s);

Adds new name `s` of a non-terminal into the container and returns its id

```
size_t nt_identifier = nts.get("identifier"),
       nt_variable   = nts.get("variable");
```

### const std::basic_string\<C>& get(size_t nt) const;

returns a name of a non-terminal by a provided id `nt`

```
cout << nt_identifier << ": " << nts.get(nt_identifier) << "\n";
```

### lit<C, T> operator()(const std::basic_string\<C>& s);

this operator returns a non-terminal literal by it's name. If the name does not exist in the container yet it adds it.

```
lit l_identifier = nts("identifier");
```

### lit<C, T> operator()(size_t nt);

this operator returns a non-terminal literal by it's id.

```
lit l_identifier = nts(1);
```
