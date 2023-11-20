[back to index](../README.md#overview-of-types)

# prods

```
template <typename C = char, typename T = C> struct prods;
```

`prods` represents a grammar in DNF. It is a list of production rules.

It can also represent a single production rule or any element of a production rule since `prods` is used for expressions to make building of a [grammar programatically](programatic_grammar.md) more convenient.

`prods` has the same API as `std::vector<prod<C, T>>`.


## constructors

### prods();

Creates an empty `prods`.

```
prods ps;
```

### prods(const lit<C, T>& l);

Creates `prods` with a single `prod` rule with an empty head literal and with a single literal `l` as body of the rule. Such a `prods` represents a literal `lit` usable in expressions.

```
nonterminals nts;
prods plus('+'),             // prods representing '+' terminal
	start(nts("start")), // prods representing start non-terminal
	nll(lit());          // prods representing null literal
```

### prods(const std::basic_string<C>& s);

Same as previous constructor but instead of a single literal `l` in body of the rule it represents a sequence of terminal literals from the string `s`.

```
prods hello("hello")  // prods representing 'h' 'e' 'l' 'l' 'o' sequence
```

### prods(const std::vector<T>& v);

Same as previous constructor but terminal literals are taken from the vector `v`.

```
prods hi({ 'h', 'i' }); // prods representing 'h' 'i' sequence
```

## methods

### lit<C, T> to_lit() const;

`to_lit` returns the last literal from a body of the last rule.
This simplifies getting of a `lit` if `prods` represents a single `lit` element.

```
prods dot('.');
// dot.to_lit() == lit('.')
```

### disjs<C, T> to_disjs() const;

`to_disjs` returns a `disjs` (body) of the last rule `prod`.


### void operator()(const lit<C, T>& l);

Adds a `prod` rule with an empty head literal and with a body with a single literal `l`.

This is used by `prods(const lit<C, T>& l)` constructor.


### void operator()(const std::basic_string<C>& s);

Adds a `prod` rule with an empty head literal and with a body with a sequence of terminal literals. Sequence is provided as a string `s`.

This is used by `prods(const std::basic_string<C>& s)` constructor.


### void operator()(const std::vector<T>& v);

Adds a `prod` rule with an empty head literal and with a body with a sequence of terminal literals. Sequence is provided as a vector `v`.

This is used by `prods(const std::vector<T>& v)` constructor.


### void operator()(const lit<C, T>& l, const prods<C, T>& p);

Adds a rule `prod` with a head literal `l` and body `p` represented by `prods`.

```
nonterminals nts;
prods ps, greeting(nts("greeting")), hi("hi"), hello("hello");

ps(nts("start"),    greeting);           // start    => greeting.
ps(nts("greeting"), hi | hello);         // greeting => "hi" | "hello".
```


### void operator()(const prods<C, T>& l, const prods<C, T>& p);

Adds a rule `prod` with a head literal `l` (represented by `prods`) and body `p` (represented by `prods`)

```
nonterminals nts;
prods ps, start(nts("start")), greeting(nts("greeting")), hi("hi"), hello("hello");

ps(start,    greeting);           // start    => greeting.
ps(greeting, hi | hello);         // greeting => "hi" | "hello".
```


### bool operator==(const lit<C, T>& l) const;

This operator enables to compare `prods` to a literal `l`. This is usable when `prods` represents a single literal.

```
prods nl('\n');
// nl == lit('\n')
```