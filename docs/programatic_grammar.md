[back to index](../README.md)

# creating grammars programatically

Library provides many operators for `lit`, `lits`, `conjs`, `disjs`, `prod`, `prods` objects to enable simple creation of grammars.

`prods` is a basic element for these expressions and thus it can represent also only a part of the grammar or rule.

## example

Let's say we want a parser for integers.

First we need `nonterminals` container for mapping nonterminal names and their ids and we need a `prods` to represent grammar productions.
```
nonterminals nts;
prods ps;
```

We will also use a predefined character class function `digit`.
```
auto cc = predefined_char_classes({ "digit" }, nts);
```

Then we use `prods` to create literals we want to reference to when building the grammar:
```
prods nll(lit()),                                     // null literal
        minus('-'),                                   // '-' terminal literal
        start(nts("start")), integer(nts("integer")), // nonterminals
        digits(nts("digits")), digit(nts("digit"));
```

Now we define the grammar using operators. `|` is for disjunction and `+` is for concatenating literals into a sequence:
```
ps(start,      integer);
ps(integer,    ('-' + digits) | digits);
ps(digits,     digit digits | nll)
```

Now we can instantiate `grammar`
```
grammar g(nts, ps, start, cc);
```

Additionally we can use conjunction `&` and negation `~` operators:
```
prods alpha(nts("alpha")), alnum(nts("alnum")), chars(nts("chars")),
	chars_rest(nts("chars_rest")), keyword(nts("keyword")),
	identifier(nts("identifier"));
ps(chars,         alpha + chars_rest);
ps(chars_rest,    alnum + chars_rest | nll);
ps(keyword,       prods("if") | "else"); // keyword    => "if" | "else".
ps(identifier,    chars & ~keyword);     // identifier => chars & ~keyword.
```