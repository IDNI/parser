[back to index](../README.md#classes-and-structs)

# lit

```
template <typename C = char, typename T = C> struct lit;
```

`lit` object represents a literal of a grammar.

## constructors

### lit(T t);

Creates a terminal literal where T is a template type of the terminal.

```
lit l_dot = lit('.'), l_comma = lit(',');
```

### lit(size_t n, nonterminals<C, T>* nts);

Creates a non-terminal literal.

```
size_t nt_identifier = nts.get("identifier");
lit l_identifier = lit(nt_identifier, &nts);
```

### lit();

Creates a null literal.

```
lit l_null = lit();
```

Null literal can be used as a body of a production rule or as one of its disjuncted bodies to satisfy a production rule when there is no sequence matched.

```
whitespace => ' ' | '\n' | '\r' | '\t' | null.

program    => statements.
statements => statement statements | null.
```

## methods

### bool nt() const;

Returns true if the literal is nonterminal or false if it is terminal.

Before accessing a value of the literal by calling `n()` or `t()` it is required to determine if it is a nonterminal or a terminal first.

```
nonterminals<char> nts;
lit<char> l_identifier = nts("identifier"),
          l_dot('.'),
          l_null();

// l_identifier.nt() == true
// l_dot.nt()        == false
// l_null.nt()       == false
```

### size_t n() const;

Returns the id of an non-terminal if the literal is non-terminal.

Before calling this method one has to be sure that it is a non-terminal. Use `nt()` to find out.

```
nonterminals<char> nts;
size_t ntid_identifier = nts.get("identifier");
lit<char> l_identifier = nts("identifier");

// l_identifier.nt() == true
// l_identifier.n()  == ntid_identifier
```

### T t() const;

Returns the terminal character (or terminal of a type T) if the literal is terminal.

Before calling this method one has to be sure that it is a terminal. Use `nt()` to find out.

```
lit<char> l_dot('.');

// l_dot.nt() == false
// l_dot.t()  == '.'
```

### bool is_null() const;

Returns true if the literal is null.

```
lit<char> l_identifier = nts("identifier"),
          l_dot('.'),
          l_null();

// l_identifier.is_null() == false
// l_dot.is_null()        == false
// l_null.is_null()       == true
```

### std::vector\<T> to_terminals() const;

Returns a vector of terminals. It would have no elements if is non-terminal or null or it would have a single terminal element if the literal is terminal.

```
lit<char> l_identifier = nts("identifier"),
          l_dot('.'),
          l_null();

// l_identifier.to_terminals() == { }
// l_dot.to_terminals()        == { '.' }
// l_null.to_terminals()       == { }
```

### std::basic_string\<C> to_string(const std::basic_string\<C>& nll = {}) const;

Returns the literal as a string. `nll` is a string returned when the literal is null. If the literal is a terminal it returns the string containing just the terminal (escaped) in apostrophes ('). If the literal is a non-terminal it returns the name of the non-terminal literal.

```
lit<char> l_identifier = nts("identifier"),
          l_dot('.'),
          l_null();

// l_identifier.to_string() == "identifier"
// l_dot.to_string()        == "'.'"
// l_null.to_string()       == ""
// l_null.to_string("NULL") == "NULL"
```

### std::string to_std_string(const std::basic_string\<C>& nll = {}) const;

Returns the literal as a standard string (basic_string\<char>). This is useful for printing literals into standard output.

This method works in a same way as the previous method `to_string()` and converts the result into a `std::string` if it isn't already.
