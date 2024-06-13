[back to index](../README.md#classes-and-structs)

# grammar

```
template <typename C = char, typename T = C> struct grammar;
```

`grammar` is an object required by `parser`. It contains grammar's production rules and grammar's character class functions.

## constructor

```
grammar(nonterminals<C, T>& nts,
	const prods<C, T>& ps,
	const prods<C, T>& start,
	const char_class_fns<T>& cc_fns);
```
`grammar` requires a reference to nonterminals `nts`.

It optionally accepts programatically created production rules `prods`, a starting nonterminal `start` and character class functions `cc_fns`.

```
nonterminals<char> nts;

grammar<char> g_empty(nts);

prods<char> greeting(nts("greeting")), ps;
ps(nts("start"), greeting);
ps(greeting,     prods<char>('H') + 'i');
grammar<char> g_start(nts, ps);

grammar<char> g_greeting(nts, ps, greeting);

char_class_fns cc = predefined_char_classes({ "alnum", "space" }, nts);
grammar<char> g_cc_fns(nts, ps, greeting, cc);
```

## methods

### size_t size() const;

Returns a number of production rules in a grammar.

Note that disjunctions of `prods` are split into separate rules by `grammar` so if using any disjunction this number would be greater than the size of `prods` provided when the `grammar` was instantiated.

```
nonterminals<char> nts;
prods<char> ps;
ps(nts("start"), prods<char>("Hello") | "Hi");
// ps.size() == 1
//     start => "Hello" | "Hi".

grammar<char> g(nts, ps);
// g.size() == 2
//     start => "Hello". start => "Hi".
```

### lit<C, T> operator()(const size_t& p) const;

Returns a head literal of a production rule with index `p`.

```
nonterminals<char> nts;
prods<char> ps, greeting(nts("greeting"));
ps(nts("start"), greeting);
ps(greeting, prods<char>("Hello") | "Hi");
grammar<char> g(nts, ps);
// g(0) == nts("greeting")
// g(1) == g(2) == nts("start")
```

### const std::vector<lits<C, T>>& operator[](const size_t& p) const;

Returns a body (vector of conjuncted literal sequences) of a production rule with index `p`.

```
nonterminals<char> nts;
prods<char> ps, greeting(nts("greeting"));
ps(nts("start"), greeting);
ps(greeting, prods<char>("Hello") | "Hi");
grammar<char> g(nts, ps);
// g[0] == { lits<char>(nts("greeting"), nts) }
// g[1] == { lits<char>("Hello") }
// g[2] == { lits<char>("Hi") }
```

### size_t len(const size_t& p, const size_t& c) const;

Returns length (number of literals) of a conjunction with index `c` of a production rule with index `p`.

```
nonterminals<char> nts;
prods<char> ps, start(nts("start")),
	alpha(nts("alpha")), alnum(nts("alnum")), digit(nts("digit"));
ps(nts("start"), (alpha + digit) & (alnum + digit));
char_cc_fns cc = predefined_char_classes({ "alpha", "alnum", "digit" }, nts);
grammar<char> g(nts, ps, start, cc);
// g.len(0, 0) == 2 // alpha and digit
// g.len(0, 1) == 2 // alnum and digit
```


### bool nullable(lit<C, T> l) const;

Returns `true` if a literal `l` is nullable.

```
nonterminals<char> nts;
lit<char> l_start(nts("start")), l_eol(nts("eol"));
prods<char> ps, start(l_start), eol(l_eol), nl('\n'), nul(lit<char>());
ps(start, eol | nul);
ps(eol,   nl);
grammar<char> g(nts, ps);
// g.nullable(l_start) == true
// g.nullable(l_eol) == false
```

### bool conjunctive(size_t p) const;

Returns true if a production rule with index `p` has a conjunction, ie. there exists multiple literal sequences which all must match.


### bool char_class_check(lit<C, T> l, T ch) const;

Runs a character class function by a name provided as a literal `l` for a `ch` terminal character and returns its result which is true if the terminal is member of the character class.


### size_t get_char_class_production(lit<C, T> l, T ch);

Does the same check as `char_class_check(l, ch)`. If the terminal `ch` is a member of the character class `l` it calls `add_char_class_production` to add a new production rule: `l => ch`.

This is used by `parser` to dynamically add terminal characters into character class nonterminal while parsing.

It returns production rule index or `(size_t) -1` if the check fails, ie. character `ch` is not a member of the character class.


### size_t add_char_class_production(lit<C, T> l, T ch);

Adds a new production rule: `l => ch` and returns index of it.


### const std::set<size_t>& prod_ids_of_literal(const lit<C, T>& l);

Returns indexes of all production rules for a nonterminal `l`.


### const lit<C, T>& start_literal() const;

Returns the starting nonterminal literal.


### bool is_cc_fn(const size_t& p) const;

Returns true if the production rule with index `p` is a character class function.


### bool is_eof_fn(const size_t& p) const;

Returns true if the production rule with index `p` is the `eof` character class.


### std::ostream& print_production(std::ostream& os, const production& p) const;

Prints a production rule with index `p` into ostream `os`.


### lit<C, T> nt(size_t n);

Returns a literal of a nonterminal with id `n`.

```
nonterminals nts;
prods<char> ps;
ps(nts("start"), prods<char>('.'));
grammar<char> g(nts, ps);
g.nt(0) == nts("start");
```

### lit<C, T> nt(const std::basic_string<C>& s);

Returns a literal of a nonterminal named `s`. It is added into `nonterminals` if it's not contained already.

```
nonterminals nts;
prods<char> ps;
ps(nts("start"), prods<char>('.'));
grammar<char> g(nts, ps);
g.nt("start") == nts("start");
g.nt("hi") == nts("hi");
```
