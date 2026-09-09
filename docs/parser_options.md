[back to index](../README.md#classes-and-structs)

# parser::options

`options` plain struct object can be provided to a `parser` to change its default behavior.

## attributes

### bool binarize;

If `binarize` is `true` `parser` applies binarization to ensure every forest node has at most 2 or less children nodes.

Binarization splits rules with more than two children nodes into multiple rules by adding new temporary production rules.

This adds additional intermediate `__temp` symbols into a resulting `forest`. Use `bool remove_binarization(graph&);` to remove these symbols.

Default value is false.

### bool incr_gen_forest;

If `incr_gen_forest` is true `parser` builds `forest`s incrementally as soon as any item is completed.

Default value is false.

### terminal_codec<C, T> codec;

Bundled decoder and encoder for converting between input character type `C` and terminal type `T`. See [recoders](recoders.md) for function signatures and built-in UTF-8 converters.

- `codec.decode` - decoder (`decoder_type`), default empty (no decoder)
- `codec.encode` - encoder (`encoder_type`), default empty (no encoder)

For `char` input and `char32_t` terminals, `idni::default_parser_options<C, T>()` pre-wires UTF-8 recoders.

### dynamic_grow_fn on_dynamic_grow;

`dynamic_grow_fn` is `std::function<void(input&, size_t, size_t, size_t)>`.

`parser` calls `on_dynamic_grow` for a child nonterminal registered in `dynamic_grow_nts`, at the moment its registered parent advances to a position strictly past that child's own matched span. The call passes the `input` being parsed, the child nonterminal's id, and its matched span as `from` and `to`. The span is `[from, to)`.

The trigger sits on the parent's progress, not on either nonterminal's own completion. The grammar shape this feature exists for is the reason. A left-recursive child, such as `chars => alpha | chars alpha`, completes at every prefix length of the run of letters it matches. Firing on the child's own completion would then grow the grammar once for every prefix of a name. A completed parent, by contrast, pins exactly one span: the one its own production actually consumed the child over. So `parser` waits for a registered parent to move past the child before it fires.

Read the matched text with `in.get_terminals(from, to)`. Grow the grammar from inside the hook with `grammar::add_dynamic_production_from`. A production added this way is visible at every later position of the same parse. See [grammar](grammar.md).

The hook can fire more than once for the same span. An ambiguous grammar completes one nonterminal through several Earley items. More than one of the parent's own predictors can reach the same child span. `add_dynamic_production_from` dedups by value, so a repeated grow with the same text is harmless. A callback that does something else must tolerate the repeat, or dedup on its own.

Two different registered children can grow the identical value, each through its own registered parent. `add_dynamic_production_from` does not add a second production for that repeat. Instead, it records the second child alongside the first, against the one pending value. Either parent can then confirm the value once it completes. Neither parent has to complete on its own.

Default value is unset. No hook runs by default.

Growing the grammar differs from confirming a value. A registered parent's own completion confirms every child span that parent's progress already grew during this parse. It does not grow anything by itself. A value that never gets confirmed this way still matched fine for the rest of the parse it grew in. It does not survive past that parse.

The parser decides every value the hook adds once, at the end of the parse. A parse that succeeds commits every confirmed value through `grammar::commit_dynamic`, into the `dynamic_context` the parse used (see `dynamic_context` below). It retires an unconfirmed value the same way a failed parse retires everything. A parse that fails retires every value the hook added during it, confirmed or not, through `grammar::rollback_dynamic`.

Earley completes an item for every reachable derivation, not only the one a parse ends up using. So the hook can fire, and the grammar can grow and even get confirmed, on a path whose parse later fails. The same holds for a derivation `auto_disambiguate` drops. A failed parse discards every such value.

Rollback retires a grown production rather than removing it, so `G` never shrinks. A char class production cached during the same parse survives untouched. A value that must outlive every parse, regardless of which `dynamic_context` it reads, belongs in `grammar::add_dynamic`. See [grammar](grammar.md).

### std::set<std::pair<size_t, size_t>> dynamic_grow_nts;

Pairs of a parent nonterminal id and a child nonterminal id. `on_dynamic_grow` fires for a pair's child once the pair's parent advances strictly past that child's span. This is not a set of single ids. A nonterminal grows the grammar only as the registered child of a registered pair, and only while its registered parent makes progress. See `on_dynamic_grow` above for why the trigger sits there.

A pair whose parent id equals its child id would confirm the child on its own completion. That is exactly the defect the parent/child split exists to avoid.

The `parser` constructor and `set_dynamic_grow` both drop such a pair. The constructor removes an invalid pair from the set and keeps the rest. A debug build asserts when it drops a pair, to flag the misuse. `set_dynamic_grow` rejects the whole call instead. It returns false and leaves the parser options unchanged.

Default value is empty. An empty set disables the hook itself. `parser` checks `dynamic_grow_nts.empty()` first. So no span ever grows the grammar, and no unused hook costs anything. This does not exempt a parse from deciding leftover state. A value can reach `grammar::add_dynamic_production_from` directly, outside any hook firing. Such a value stays pending and live until the next parse ends. That parse still resolves it, the same way it resolves any hook of its own. See `grammar::has_pending_dynamic` and `grammar::commit_dynamic` in [grammar](grammar.md).

A production `add_dynamic_production_from` appends always gets a larger index than any production declared while building the grammar. Nothing removes a statically declared production, not even `grammar::rollback_dynamic`. A static index therefore always stays smaller than a dynamic one. Among completions that cover one span, disambiguation picks the smallest production index. See `auto_disambiguate` in [grammar::options](grammar_options.md). So a value `on_dynamic_grow` adds never outranks a static alternative for the same text.

### dynamic_context<C>

`dynamic_context<C>` holds the values a `dynamic_grow_nts` hook confirmed across earlier parses:

```
template <typename C = char>
struct dynamic_context {
	std::map<size_t, std::set<std::basic_string<C>>> values = {};
};
```

`values` maps a nonterminal id to every value confirmed for it so far.

A parse commits every confirmed value into a `dynamic_context` at its own end, through `grammar::commit_dynamic`, or it rolls one back through `grammar::rollback_dynamic`. One of the two calls always runs, regardless of `dynamic_grow_nts`. Reading the container to make its values live again only happens when `dynamic_grow_nts` is not empty. An empty set has no pair to match a value against. A later parse that reads the same container therefore sees the values an earlier parse confirmed. See `on_dynamic_grow` above for what confirms a value, and [`grammar::commit_dynamic`](grammar.md) for how it lands here.

A `parser` owns one internally, used for a parse whose `parse_options::dynamic_ctx` is null. Two parsers never share this internal container. Two containers a host passes explicitly never see each other's confirmed values either.

### dynamic_context<C>* parse_options::dynamic_ctx;

A field of `parser<C, T>::parse_options`, the per-parse options struct a `parse` call accepts alongside the input. Names the `dynamic_context` a parse reads its confirmed values from and writes its own confirmed values into. Null uses the parser's own internal container.

Pass the same `dynamic_context` to two parsers that share one grammar to let one parser's confirmed values reach the other. Pass separate containers, or leave the field null on separate parsers, to keep them from seeing each other's growth.

```
parser<char> p1(g, popt), p2(g, popt); // g, popt set up dynamic_grow_nts

dynamic_context<char> ctx;
parser<char>::parse_options po;
po.dynamic_ctx = &ctx;

p1.parse(in1.c_str(), in1.size(), po); // p1's confirmed values land in ctx
p2.parse(in2.c_str(), in2.size(), po); // p2 reads and writes ctx too
```

### bool set_dynamic_grow(dynamic_grow_fn fn, std::set<std::pair<size_t, size_t>> nts);

Sets `on_dynamic_grow` and `dynamic_grow_nts` on a `parser` object already constructed. Use it when a caller cannot pass options at construction time. A generated singleton that builds its own options is the usual case. Call it between parses, the same restriction `add_dynamic` carries.

Returns false and changes nothing if any pair in `nts` has an equal parent and child id. This is the same check the `parser` constructor runs on `options::dynamic_grow_nts`. Returns true otherwise, after it sets both fields.

```
parser<char> p(g); // default options: the hook is unset

p.set_dynamic_grow(
	[&](parser<char>::input& in, size_t, size_t from, size_t to) {
		g.add_dynamic_production_from(type_name_l,
			in.get_terminals(from, to));
	},
	{ { def_id, chars_id } });
```

## constructor

### parser<C, T>::options();

## example
```
parser<char, char32_t>::options opts;

opts.binarize = true;
opts.codec.decode = utf8_to_u32_conv; // lib provided utf8 to u32
opts.codec.encode = u32_to_utf8_conv; // lib provided u32 to utf8
```
