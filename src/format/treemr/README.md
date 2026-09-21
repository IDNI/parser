# treemr (Tree Match/Replace) — a path/pattern DSL for parse trees

`treemr` matches a compact textual **pattern** against a parse tree
(an `lcrs_tree<pnode_type<C,T>>` produced by `idni::parser`) and gives three
operations over it. The naming follows a regular expression library: `match`
is the anchored operation, `search` is the unanchored one.

| operation   | question it answers                                  | entry points |
|-------------|------------------------------------------------------|--------------|
| **match**   | does `root` itself match the pattern?                | `match`      |
| **search**  | does the pattern match somewhere below `root`?        | `search`, `search_all` |
| **replace** | rewrite every match into something new               | `replace`, `replace_if`, `replace_until`, `replace_fixpoint`, `trim`, `trim_top` |

The pattern language is itself a TGF grammar (`treemr.tgf`). A pattern
string compiles into a tree-type-independent `compiled_pattern` IR, then
runs against any tree through a small adapter.

Everything lives in `namespace idni::treemr`.

```cpp
#include "format/treemr/treemr.h"   // pulls in the .tmpl.h
using namespace idni;
using namespace idni::treemr;
```

---

## Quick start

```cpp
// A tiny CSV-ish grammar and an input parsed into a tree.
nonterminals<char> nts;
auto g    = tgf<char>::from_string(nts,
                " start => csv. csv => row. "
                " row => cell (',' cell)*. cell => digit+. "
                " @use char class digit.").value();
parser<char> p(g);
auto res  = p.parse("12,34", 5);
tref root = res.get_shaped_tree2();

// Compile a pattern and ask questions about the tree.
auto m = matcher_for<char, char>(nts, "row > cell ',' cell").value();

bool hit = m.search(root);                // search: true
auto sel = m.search_all(root);            // every match, with captures
```

`matcher_for` returns a `diagnostics::result<...>`. On a DSL syntax error it
carries a `diagnostics::code::parse_error`, and no exception is thrown.
Check `has_value()`, or call `.value()` in a test.

---

## The pattern DSL

A pattern is a single **sibling sequence**: a left-to-right list of *slots*
that match consecutive children, optionally with edges that descend into a
node's own children.

### Atoms

An atom matches one node.

| syntax        | kind       | matches |
|---------------|------------|---------|
| `name`        | NT name    | a nonterminal node named `name` |
| `%`           | wildcard   | any single node |
| `'c'`         | char lit   | a node whose collected terminal text is `c` |
| `"str"`       | string lit | a node whose collected terminal text is `str` |
| `( … )`       | capture    | a capture group (see [Captures](#captures)) |
| `( a \| b )`  | union      | alternation inside a capture |

A name follows `(alpha|_)(alnum|_)*`. Both literal kinds accept
`\a \b \f \n \r \t \v \\ \/`, `\xHH`, `\uHHHH`, and `\UHHHHHHHH`. A char
literal also accepts `\'`. A string literal also accepts `\"`.

### Quantifiers

A trailing quantifier makes a slot repeat, greedy and regex-style.

| quantifier | meaning        |
|------------|----------------|
| `*`        | zero or more   |
| `+`        | one or more    |
| `?`        | zero or one    |

```
digit+          # one or more digit siblings
%*              # any number of nodes
cell?           # an optional cell
```

### Edges — descending into a node

By default a sibling sequence matches **sibling** nodes. An edge attaches a
child sequence to an atom.

| edge | name            | meaning |
|------|-----------------|---------|
| `>`  | direct edge     | the right-hand sequence matches the atom's **direct children** |
| `>>` | descendant edge | the right-hand sequence matches **somewhere below** the atom, at any depth |

```
row > cell          # a row with a cell among its direct children
row > ^ cell        # a row whose FIRST child is a cell
row >> digit        # a digit anywhere beneath a row
% > digit           # any node with a direct digit child
```

The sequence after an edge is **unanchored by default**. It can match at any
position in the child list, so `row > ','` finds the `','` between two
cells. Pin it with an anchor. See [Anchors](#anchors).

**An edge slurps the tail.** The sequence after an edge consumes the *rest*
of the enclosing sequence. Those slots become children of the edge's atom.

```
row > cell ',' cell      # cell ',' cell are ALL children of row
```

Use parentheses to scope an edge to a single atom.

```
(row > cell) ',' cell    # `row > cell`, THEN a sibling ',', THEN a sibling cell
```

### Anchors

Within a sibling sequence, an anchor pins the match to one end of the child
list. A slot always matches **consecutive** siblings. Without an anchor, a
sequence can sit anywhere in the list, like an unanchored regex. So
`row > delim` matches a `delim` anywhere among the children of `row`.

| anchor | position                       | meaning |
|--------|---------------------------------|---------|
| `^`    | first slot of a sequence         | the sequence starts at the first child |
| `$`    | last slot of a sequence          | the sequence ends at the last child, or the candidate has no right sibling |
| `/`    | before the whole pattern         | the match root must be the root of the searched tree |
| `!`    | after an atom                    | the matched node must have no children |

```
row > ^ cell              # the first child of row is a cell
row > cell $               # the last child of row is a cell
row > (^ cell ',' cell $) # exactly two cells under row, nothing else
```

An anchor is an ordinary slot. The compiler requires `^` to sit first in its
sequence, and `$` to sit last. A pattern that breaks either rule fails to
compile.

#### A top-level `^` is a compile error

`^` and `$` normally pin a sequence to the ends of one child list. At the
top of a pattern, outside every edge, there is no child list: the tree
cannot walk a sibling list backwards, so a bare `^`, or a `^` propagated
from a first-slot group, is rejected at compile time.

```
^ row               # compile error: no parent child list to pin
```

A top-level `$` stays legal, because a right sibling is always reachable
from a candidate node. It requires the candidate itself to have no right
sibling.

```
row $               # matches a `row` candidate with no right sibling
```

An edge slurps the tail, so both anchors bind to the sequence introduced by
the edge, with or without parentheses.

```
rule > ^ digit+ $         # both anchors bind to the sequence after `>`
```

#### `/` — anchor the whole pattern to the tree root

`/` prefixes the entire pattern. It requires the match root to be the exact
node passed to `match`, `search`, `search_all`, or `replace`, not merely an
ancestor-free node somewhere inside the subtree.

```
/row                # matches only when `row` is itself the searched root
```

Because the searched root can only ever be one node, `search_all` returns
at most one result for a `/`-anchored pattern.

An `__AMB__` wrapper node groups the alternative parses of one span. When
the tree root is an `__AMB__` wrapper, each of its alternatives counts as
the root too. So a `/`-anchored pattern can still match inside an ambiguous
parse.

#### `!` — require a childless node

`!` follows an atom. It requires the matched node to have no children,
regardless of the atom's own kind.

```
cell!               # a childless `cell` node
%!                  # any childless node
'a'!                # a childless node whose collected text is "a"
(cell)!             # a capture whose matched node is a childless `cell`
```

The compiler rejects `!` on an atom that also carries a child edge, because
a node with a required child can never be childless. The grammar accepts
the combination on purpose, so the compiler gives a clear message instead
of a bare parse error.

```
a! > b              # compile error: a leaf atom cannot carry a child edge
```

#### Anchors inside a group

An anchor distributes over alternation. `(^ a | ^ b)` means the same as
`^ (a | b)`. A group anchors its enclosing sequence only when every
alternative carries the anchor.

An anchor on only some alternatives of a group is contradictory. The
compiler rejects it, for example `(^ a | b)`.

A `^` inside a group also needs the group to sit first in its enclosing
sequence. A `$` inside a group needs the group to sit last. `x > b (^ a)`
fails to compile because the group is not first.

A group that holds only an anchor, such as `(^)` or `($)`, is a compile
error. Every group is a capture, and an anchor-only group can never
capture a node. It would only shift the numbering of later captures.

### Captures

Every `( … )` is a **capture group**. The first node it matches is
recorded, then returned by `match` and the search operations. A
multi-slot capture records only its first node, not a range. Groups are
numbered by their opening parenthesis, left-to-right starting at **1**,
outer before inner, as in regexes (`0` is reserved for the match root).
Alternation `|` is allowed inside a group. The first alternative that
matches wins, and an unmatched optional group is recorded as `nullptr`.

```
row > (cell) ',' (cell)   # capture 1 = first cell, capture 2 = second cell
(digit | alpha)           # capture 1 = whichever branch matched
(cell (','))              # capture 1 = the cell, capture 2 = the ','
```

### Whitespace & comments

Whitespace between slots carries no meaning. `#` starts a comment that runs
to the end of the line, and a comment counts as whitespace.

```
( digit       # first alternative
| alpha )     # second alternative
```

### Grammar at a glance

```
pattern     = ['/'] sib_seq
sib_seq     = slot_chain
slot_chain  = simple_slot (ws simple_slot)* [ws edge_slot] | edge_slot
simple_slot = '^' | '$' | atom ['!'] [quantifier]
edge_slot   = atom ['!'] [quantifier] edge sib_seq          # edge slurps the tail
atom        = name | '%' | terminal | '(' alt_seq ')'
alt_seq     = sib_seq ('|' sib_seq)*
edge        = '>>' | '>'
quantifier  = '*' | '+' | '?'
```

The authoritative grammar is `treemr.tgf`.

---

## Compiling a pattern

Two layers exist, mirroring the header.

```cpp
// 1. DSL string -> tree-type-independent IR (no tree needed yet).
diagnostics::result<compiled_pattern> cp = treemr::compile("row > cell");

// 2. Bind the pattern to a concrete tree type via a factory.
auto m1 = matcher_for<char, char>(nts, std::move(cp).value()); // from compiled IR (infallible)
auto m2 = matcher_for<char, char>(nts, "row > cell");          // from string (result<>)
```

`matcher_for` builds a `matcher<pnode_type<C,T>>` bound to a `node_adapter`
produced by `parse_node_adapter<C,T>(nts)`. The adapter captures `nts` **by
reference**, so the `nonterminals` object must outlive the matcher.

### From a generated parser

`matcher_for` also accepts any object that satisfies the `nt_source`
concept, in place of a bare `nonterminals` table. Every `<x>_parser` struct
that the `tgf` tool generates satisfies it.

```cpp
auto m = matcher_for(my_parser::instance(), "row > cell ',' cell");
```

The `tgf` tool has a `--treemr` generator option, and it defaults to
`false`. With `--treemr true` the generated parser struct gains a
`matcher(pattern)` member, built on `matcher_for(*this, pattern)`.

```cpp
auto m = my_parser::instance().matcher("row > cell ',' cell");
```

Leave the option off for `treemr.tgf` itself. `treemr.h` includes the
generated parser of that grammar, so a `matcher()` member there needs
`treemr.h` in turn, and that closes a cycle.

---

## match

Anchored, boolean, like `std::regex_match`: `root` itself must be the
match root.

```cpp
auto m = matcher_for<char, char>(nts, "row > cell ',' cell").value();

m.match(root);          // true only if `root` itself is the match root
```

A second overload also fills in the captures.

```cpp
match_result mt;
m.match(root, mt);      // same test; mt.root and mt.captures fill in on success
```

---

## search

Unanchored, like `std::regex_search`: the pattern can match anywhere in
the subtree rooted at `root`.

```cpp
m.search(root);                     // true if the pattern matches ANYWHERE below root
match_result mt;
m.search(root, mt);                 // same test, and fills in mt on success
```

`search_all` returns every match, together with its captures, as a
`match_result`.

```cpp
struct match_result {
    tref  root;          // the matched node
    trefs captures;      // capture 1..N (nullptr for an unmatched optional group)
    tref  operator[](size_t i) const;  // [0] is root, [i] is captures[i - 1]
    size_t size() const;               // 0 if no match, else 1 + captures.size()
    explicit operator bool() const;    // true when root is not nullptr
};
```

```cpp
auto m   = matcher_for<char, char>(nts, "row > (cell) ',' (cell)").value();
auto sel = m.search_all(root);

for (const match_result& mt : sel) {
    tref whole  = mt[0];             // == mt.root  (the matched `row`)
    tref first  = mt[1];             // capture 1   (first cell)
    tref second = mt[2];             // capture 2   (second cell)
    // mt.captures.size() == 2 here
}
```

`search_all` returns one `match_result` per match site, so `(cell)`
against `"1,2,3"` gives three results, each with its own captured cell.

---

## replace

`replace` finds every node that matches the pattern, anchored at that
node, in one **post-order** (bottom-up) pass, and asks `fn` for its
replacement. A parent's own match test sees its already-rewritten
children, because children rewrite first. The original tree stays
untouched. `replace` returns a new root, because trees are immutable and
hash-consed.

```cpp
using replace_fn = std::function<tref(const match_result&)>;

tref replace(tref root, replace_fn fn,
        ambig_mode mode = ambig_mode::FORBID) const;
```

`fn` returns one of three things.

- the matched node itself, for no change.
- `nullptr`, to delete the matched node.
- any other node, to put in its place.

```cpp
// Swap the two captured cells of every `add` node: add(a, b) -> add(b, a)
auto m = matcher_for<char, char>(nts, "add > (%) (%)").value();
tref out = m.replace(root, [](const match_result& mt) {
    return parser<char>::tree::get(/* add value */,
        { mt.captures[1], mt.captures[0] });
});
```

**One call per distinct subtree.** `replace` caches its post-order pass by
subtree equality. Two structurally equal subtrees share a single callback
call and a single replacement, so `fn` must be pure. A node value carries
its source span, and the equality test compares that value, so a freshly
parsed tree never shares a callback call. Sharing starts once a
transformation rebuilds nodes without the original spans.

### Skipping subtrees: `replace_if`, `replace_until`

```cpp
tref replace_if(tref root, replace_fn fn, query_fn query,
        ambig_mode mode = ambig_mode::FORBID) const;

tref replace_until(tref root, replace_fn fn, query_fn query,
        ambig_mode mode = ambig_mode::FORBID) const;
```

`replace_if` leaves a subtree untouched, exactly as it is, when its own
root node fails `query`. `replace_until` leaves a subtree untouched when
its own root node satisfies `query`. Both names and both meanings match
the rewriter API already on the tree itself, in `utility/tree.h`.

```cpp
m.replace_if(root, fn, [](tref n)    { return !is_comment(n); }); // skip a comment subtree
m.replace_until(root, fn, [](tref n) { return  is_comment(n); }); // same skip, opposite predicate
```

### Deleting matches: `trim`, `trim_top`

```cpp
tref trim(tref root, ambig_mode mode = ambig_mode::FORBID) const;
tref trim_top(tref root, ambig_mode mode = ambig_mode::FORBID) const;
```

`trim` deletes every match, at any depth, including a match nested inside
another match. Its post-order pass deletes the inner match first. The
outer node's own match test then runs against the already-shrunk subtree,
so an outer node can start matching only after its inner match is gone.

`trim_top` deletes only a match with no matching ancestor. Its traversal
stops descending once it deletes a node, so a match nested inside another
match is never tested on its own. It disappears together with its
ancestor, and its own removal is never counted separately.

This difference is easy to get backwards. Take a childless `target` node
nested one level inside another `target` node, and the pattern
`target!` (a `target` with no children).

```
trim(root)          # deletes both: the inner match falls first, which leaves
                     # the outer node childless too, so it now matches as well
trim_top(root)       # deletes only the inner match: the outer node still has
                     # a child when it is tested, so it survives, now childless
```

### `replace_fixpoint`

```cpp
static constexpr size_t default_max_fixpoint_iters = 1000;

diagnostics::result<tref> replace_fixpoint(tref root, replace_fn fn,
        ambig_mode mode = ambig_mode::FORBID,
        size_t max_iters = default_max_fixpoint_iters) const;
```

`replace_fixpoint` repeats `replace` until one pass changes nothing, or
until it reaches `max_iters` passes. The default cap is
`default_max_fixpoint_iters` (1000 passes). Pass `0` for no cap.

Because nodes are hash-consed, "no change" is a pointer check between one
pass and the next. Reaching the cap without a fixed point is an error, not
a silent stop, so a rule set that never settles is caught rather than
truncated.

```cpp
auto fp = m.replace_fixpoint(root, fn);
if (fp.has_value()) root = fp.value();
```

---

## Ambiguity modes

A parse forest may contain `__AMB__` wrapper nodes. Each one groups the
alternative parses of the same span. The alternatives under one `__AMB__`
node carry no guaranteed order across runs. A search visits every node in
the tree, so a match root can sit strictly inside one alternative. The
`ambig_mode` decides how a pattern treats an `__AMB__` node. Every
operation takes an `ambig_mode` (default `FORBID`).

| mode     | behavior |
|----------|-----------|
| `FORBID` | the match root, and every node below it, must not be an `__AMB__` node |
| `ANY`    | a pattern that crosses `__AMB__` matches when at least one alternative satisfies it |
| `UNIQUE` | a pattern that crosses `__AMB__` matches when exactly one alternative satisfies it |
| `ALL`    | a pattern that crosses `__AMB__` matches only when every alternative satisfies it |

Under `ALL`, a capture holds one value that describes every alternative.
A capture that names the same node in each alternative keeps that node.
A capture that differs across alternatives becomes a new `__AMB__` node,
whose children are the distinct values, in alternative order. The
synthesized node carries the span `{0, 0}`, so a caller must not read a
source position from it.

`UNIQUE` takes its captures from the one alternative that satisfies the
pattern. It never merges or synthesizes a node.

A synthesized `__AMB__` node is not part of the parse forest. A caller
must not walk upward from it.

`FORBID` judges one match attempt, not the whole tree. An `__AMB__` node
in an unrelated branch does not block a match that never reaches it.

`FORBID` refuses a match whose match root sits directly under an
`__AMB__` node. `FORBID` also refuses a match whose match root has an
`__AMB__` node anywhere in its own subtree, even when the pattern never
descends that far.

A search starts its scan at the match root, never at the whole searched
tree. A match in one branch never fails because of an `__AMB__` node in
another branch.

`match` treats the given node as the root of the search. A root has no
parent, so the parent rule is satisfied there. The subtree rule still
applies to the given node itself.

```cpp
m.match(root, ambig_mode::ANY);
m.search(root, ambig_mode::UNIQUE);
m.search(root, ambig_mode::ALL);
```

---

## Matching against other tree types (adapters)

`matcher<NodeT>` is a template on the node type only. It holds one
`node_adapter` by value: a plain struct of two `std::function` members
that tell the engine how to recognize NTs and terminals on that node
type.

```cpp
struct node_adapter {
    std::function<bool(tref, std::string_view)> is_nt_fn;
    std::function<bool(tref, std::string_view)> is_terminal_fn; // optional
    std::function<tref(std::string_view, const trefs&)> make_node_fn; // optional

    bool is_nt(tref n, std::string_view s) const;
    bool is_terminal(tref n, std::string_view s) const;
};
```

`parse_node_adapter<C,T>(nts)` builds a `node_adapter` for an
`idni::parser` tree, and `matcher_for` uses it. To match against another
node type, build a `node_adapter` directly and construct the matcher with
it.

```cpp
node_adapter a;
a.is_nt_fn = [](tref n, std::string_view name){ /* ... */ return true; };
a.is_terminal_fn = [](tref n, std::string_view text){ /* ... */ return true; };
auto m = matcher<MyNode>(std::move(compiled), std::move(a));
```

When the pattern uses terminal literals (`'c'`, `"str"`), the adapter
must supply `is_terminal_fn`. Otherwise only `is_nt_fn` is required, and a
pattern with an empty `is_terminal_fn` simply fails to match rather than
crashing. `make_node_fn` builds a node named `nt` with the given
children, for a capture that `ALL` synthesizes. `parse_node_adapter`
always sets it, so a parse tree always gets a synthesized node. A
hand-built adapter can leave `make_node_fn` empty, and `ALL` then keeps
the first alternative's capture.

---

## `lcrs_tree<T>` forwarding

`lcrs_tree<T>` (`utility/tree.h`) carries the same operations as member
functions, for any matcher object that satisfies its `tree_matcher`
concept (a `match(tref)` and a `search(tref)`, each returning something
bool-like). A `treemr::matcher<T>` satisfies it.

```cpp
t.match(m);
t.search(m);
t.search_all(m);
t.replace(m, fn);
t.replace_if(m, fn, query);
t.replace_until(m, fn, query);
t.trim(m);
t.trim_top(m);
```

`utility/` does not depend on `format/treemr/`, so `tree_matcher` names
only the shape a matcher needs, never `treemr::matcher<T>` itself.

---

## Files

| file | purpose |
|------|---------|
| `treemr.h`                    | public API (operations, IR types, factories) |
| `treemr.tmpl.h`               | implementation (compiler + matching engine) |
| `treemr.tgf`                  | the pattern-language grammar |
| `treemr_parser.generated.h`   | generated parser for the DSL grammar |
| `treemr_parser.generated.cpp` | generated parser productions, for a non-header-only build |

C++ behavior is covered by `tests/doctest_treemr.cpp`.
