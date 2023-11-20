[back to index](../README.md#overview-of-types)

# lits

```
template <typename C = char, typename T = C> struct lits;
```

`lits` represents a sequence of literals to match an input to satisfy a part of a production rule.

`lits` has the same API as `std::vector<lit<C, T>>`.
As an addition it has a Boolean `neg` attribute.

## attribute

### bool neg;

`neg` states if the sequence is negated or not.

Negated literal sequence satisfies a part of a production rule if it does not match the input.

## constructors

Use `std::vector` constructor.

```
lits empty,
    hello = { lit('h'), lit('e'), lit('l'), lit('l'), lit('o') },
    hello_copy1(hello.begin(), hello.end()),
    hello_copy2(hello),
    etc(3, lit('.'));
```

## methods

Use `std::vector` methods.

Additionally there is a negation operator.

### lits<C, T> operator~(const lits<C, T>& x);

Negates the literal sequence.

```
lits nl = { lit('\n') },
    not_nl = ~nl;
```
