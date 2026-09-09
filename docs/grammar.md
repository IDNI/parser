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


### std::optional<size_t> add_dynamic_production_from(const lit<C, T>& l, const std::basic_string<C>& value);

Adds one pending alternative `value` to a `@dynamic` nonterminal `l`. It works like `add_dynamic` does for each of its values. It skips the full grammar rebuild that `add_dynamic` does. It only appends one production and inserts its index into `ntsm`, the same trade `add_char_class_production` makes. This makes it cheap enough to call while a parse using `l` runs. Call it from `parser::options::on_dynamic_grow` for that (see [parser::options](parser_options.md)).

It dedups against every value already committed. A repeat of a committed value returns `std::nullopt` and adds nothing.

A repeat of a value still pending in the current parse does not add a second production either. Instead, the method records the new (parent, child) pair alongside the pair already stored for that value. It returns the same production index the first call returned. This lets two registered pairs share one pending hook for the same value. Either registered parent can then confirm the value by completing over it. See [parser::options::dynamic_grow_nts](parser_options.md) for the meaning of a call made outside a hook.

If `value` was added and later retired, the method reuses that production's original index instead of appending a new one. The grammar holds at most one production per distinct `(l, value)` pair for the life of the process.

`compute_nullables()` is not called, so `l`'s nullability is not recomputed. An empty `value` would make `l` nullable, so this method refuses it and returns `std::nullopt` without adding anything. Use `add_dynamic` instead for a `@dynamic` nonterminal that must accept the empty string.

A value this method adds stays pending until the parse ends. `commit_dynamic` and `rollback_dynamic` then decide what happens to it. See below, and see [parser::options::dynamic_grow_nts](parser_options.md) for how a value gets confirmed in the first place.

A caller does not have to call this method from inside `on_dynamic_grow`. A caller may call it directly, between parses. `grammar::has_pending_dynamic()` reports whether such a call is still undecided. The value it adds stays pending and live until the next parse ends. `parser::parse` always resolves every pending hook by then, even when `parser::options::dynamic_grow_nts` has no pair registered for that parse. So a value added outside a parse survives only when a registered parent of the very next parse confirms it.

```
nonterminals<char> nts;
prods<char> ps, start(nts("start")), def(nts("def")),
	chars(nts("chars")), alpha(nts("alpha"));
char_class_fns<char> cc = predefined_char_classes<char>({ "alpha" }, nts);
ps(start, def);
ps(def, prods<char>("t ") + chars + "=" + "X");
ps(chars, alpha);
ps(chars, chars + alpha); // chars is left-recursive
grammar<char> g(nts, ps, start, cc);

lit<char> type_name_l = g.nt("type_name");
size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();

parser<char>::options popt;
popt.dynamic_grow_nts = { { def_id, chars_id } }; // def is the parent, chars the child
popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
	size_t from, size_t to)
{
	g.add_dynamic_production_from(type_name_l, in.get_terminals(from, to));
};
parser<char> p(g, popt);
std::string in = "t Point=X";
// p.parse(in.c_str(), in.size()).found == true
// a later parse can then match type_name against "Point"
```


### bool commit_dynamic(dynamic_context<C>& ctx);

Decides every value `add_dynamic_production_from` added since the last commit or rollback. For a confirmed value, `commit_dynamic` writes it into `ctx`'s values and keeps it linked into `ntsm`. A parse that reads the same `ctx` then still matches it. `commit_dynamic` retires an unconfirmed value the same way `rollback_dynamic` does. It returns `true` when it confirms at least one value.

`parser::parse` calls `commit_dynamic` at the end of a parse that succeeds. It passes the `dynamic_context` that parse used. `parser::parse` calls `rollback_dynamic` instead at the end of a parse that fails. One of these two calls always runs, even when `parser::options::dynamic_grow_nts` has no pair registered. A hook added outside `on_dynamic_grow` still gets decided at the end of the very next parse. See [parser::options::dynamic_grow_nts](parser_options.md) for how a value gets confirmed, and for the scope of `dynamic_context`.

`commit_dynamic` never writes into `opt.dynamic`, which holds only the alternatives a host adds through `add_dynamic`.


### void rollback_dynamic();

Retires every value `add_dynamic_production_from` added since the last commit or rollback, confirmed or not. A retired production keeps its place in `G`. Only its index leaves `ntsm`, and its value leaves the pending record. `predict()` reads `ntsm` fresh on every call, so a retired production stops matching from that point on. The grammar never shrinks.

A hook added outside a parse, through a direct `add_dynamic_production_from` call, stays pending and live until the next parse ends. That parse decides it like any other pending hook: `commit_dynamic` if a registered parent confirms it during that parse, `rollback_dynamic` otherwise.

A production `add_dynamic` has since claimed is exempt. See `add_dynamic` below.

A char class production `add_char_class_production` appends during the same parse survives a rollback. It is a value-neutral cache, not a hook production, so `rollback_dynamic` never touches it.

`parser::parse` calls `rollback_dynamic` at the end of a parse that fails. Earley completes every reachable derivation, not only the one a parse ends up using. So a value can grow the grammar on a path whose parse later fails. It can even get confirmed there. The same holds for a derivation `auto_disambiguate` drops. `rollback_dynamic` discards every such value.

A value that must outlive one parse belongs in `add_dynamic`, which never enters this list.

A retired value is not gone. If `add_dynamic_production_from` sees the same `(l, value)` pair again, it re-links the retired production's index into `ntsm` instead of appending a new one. So `G` holds at most one production per distinct value added through this method, for the life of the process.

```
parser<char> p(g, popt);
p.parse(failing_input.c_str(), failing_input.size());
// the parse above failed, so any value the hook added during it was
// retired at that parse's own end
p.parse(next_input.c_str(), next_input.size());
// this parse starts with none of that retired value's growth
```


### bool has_pending_dynamic() const;

`has_pending_dynamic` returns `true` while at least one added value is still undecided. Undecided means neither `commit_dynamic` nor `rollback_dynamic` has run for it. Every `parser::parse` call decides all of them by its own end. So this method normally reports `false` once a `parse` call ends.


### void add_dynamic(const std::basic_string<C>& nt, const std::vector<std::basic_string<C>>& values);

Adds a production rule for every value in `values` that a nonterminal named `nt` does not have yet. Each value becomes one terminal string alternative. `add_dynamic` skips a value already added and never removes a production.

One grammar object serves every parser that holds a reference to it. So `add_dynamic` changes the grammar for all of them. It is not per parser.

`add_dynamic` writes the grammar. A parse must not run at the same time. Call it before the first parse.

A value the hook already confirmed through `add_dynamic_production_from` and `commit_dynamic` stays live only for a parse that reads the same `dynamic_context`. Calling `add_dynamic` with that same value claims it for the grammar itself. The value then becomes permanent for every parser sharing this grammar. No `dynamic_context` choice a later parse makes changes this, and no `rollback_dynamic` call removes it.

`add_dynamic` records every value the caller gives it in `opt.dynamic`, including one the hook added first through `add_dynamic_production_from`. It records a value once, even across separate calls with the same value.

```
nonterminals<char> nts;
prods<char> ps, start(nts("start")), type_name(nts("type_name"));
ps(start, type_name);
grammar<char> g(nts, ps, start, {});
g.add_dynamic("type_name", { "u8", "u16" });
parser<char> p(g);
// p.parse("u8", 2).found == true
```


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
