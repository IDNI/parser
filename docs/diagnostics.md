[back to index](../README.md#classes-and-structs)

# diagnostics

```
namespace idni::diagnostics;
```

`diagnostics` is a unified reporting facility used across the parser library. It bundles diagnostic messages (errors, warnings, infos) with metrics (timed scopes, counters, memory) into a single tree-shaped `report`. `result<T>` wraps a value with a report, mimicking `std::expected`.

The whole module lives in `src/utility/diagnostics.h` (declarations) and `src/utility/diagnostics.tmpl.h` (definitions); it is included by `parser`, `tgf`, the code generator, and most utilities.

Most call sites work with `result<T>` rather than a bare `report`, so it appears first below.


## result\<T\>

```
template <typename T> struct idni::diagnostics::result;
```

Wraps a value of type `T` and a `report`. Models `std::expected`-like semantics: either a value is present (success) or it is absent and the report carries the diagnostics.

Move-only. Construction with a name resets the report root (no automatic timing).

Thread-safety: `result<T>` inherits the same constraint as the report it
embeds — see [`report` thread-safety](#thread-safety) below. Separate
`result<T>` instances are independent; a single instance is not internally
synchronized.

```
diagnostics::result<grammar<char>> r;
{
    auto _ = r.open_if(measure, "grammar load");
    auto _p = r.open_if(measure, "tgf parse");
    // ... parsing ...
    r.emplace(std::move(g));
}
return r;
```


### constructors

```
result();                                // empty
explicit result(std::string_view name);  // empty, reset report root to `name`
explicit result(T value);                // success, no diagnostics
explicit result(report rep);             // failure (or pre-populated report)
result(result&& other) noexcept;
result& operator=(result&& other) noexcept;
```

The name-constructor only calls `report::reset(name)` — use `open` / `open_if` for timed scopes.

The value-taking constructor is disabled when `T = std::string` (it would
clash with the `std::string_view` name-constructor). For
`result<std::string>`, use `emplace` or `operator=` instead.


### bool has_value() const;
### explicit operator bool() const;
### bool has_error() const;

Status queries. `has_error()` is `true` if the report contains any error node.


### T& value() &;
### const T& value() const&;
### T&& value() &&;
### T& operator*() & requires !std::is_pointer_v\<T\>;
### const T& operator*() const& requires !std::is_pointer_v\<T\>;
### T&& operator*() && requires !std::is_pointer_v\<T\>;
### T* operator->() requires !std::is_pointer_v\<T\>;
### const T* operator->() const requires !std::is_pointer_v\<T\>;

Dereference observers (mirror `std::expected`; omitted for pointer `T` — use `value()` / implicit conversion).

### T& emplace(T&& v);
### template \<typename... Args\> T& emplace(Args&&... args);
### template \<typename U = T\> result& operator=(U&& v);
### void clear_value();

Set the success payload in place. `emplace` / `operator=` match
`std::expected`; if the report already has an error, the new value is
silently dropped (see invariant below). In debug builds (`DBG(...)`), an
assertion fires so the misuse is loud. Prefer `emplace` at the end of a
scope; `return result<T>(std::move(x))` when the report is empty.


### operator T() const requires std::is_pointer_v\<T\>;
### bool operator==(std::nullptr_t) const requires std::is_pointer_v\<T\>;
### bool operator!=(std::nullptr_t) const requires std::is_pointer_v\<T\>;

For pointer-valued results, an implicit conversion to `T` returns the held pointer (or `nullptr` if no value) and `== nullptr` / `!= nullptr` check presence directly. Lets `result<Forest*>` be used wherever `Forest*` is.


### report& report() &;
### const report& report() const&;
### report&& report() &&;

Access the embedded diagnostics tree. The `&&` overload moves the report out — useful for `append` or `forward_as`.


### void error(code c, std::string_view msg, int64_t primary = 0, std::initializer_list<attr> extra = {});
### void error(code c, std::string_view msg, std::initializer_list<attr> extra);
### void warning(std::string_view msg, int64_t primary = 0, std::initializer_list<attr> extra = {});
### void warning(std::string_view msg, std::initializer_list<attr> extra);
### void info(std::string_view msg, int64_t primary = 0, std::initializer_list<attr> extra = {});
### void info(std::string_view msg, std::initializer_list<attr> extra);

Push a message (same names as `report::error` / `warning` / `info`). Each has a primary-form (with `int64_t primary`) and a shorthand that takes only the attribute list. `error` enforces the *errored result carries no value* invariant.


### void print(std::ostream& os) const;
### void print(const sink& s) const;
### void print(const sinks& s) const;
### void print() const;

Emit diagnostics when `nodes()` is non-empty. Unified overloads route **all bands** to one stream or sink; `print(sinks)` keeps per-band routing.


### bool print_and_ok() const;
### bool print_and_ok(std::ostream& os) const;
### bool print_and_ok(const sink& s) const;
### bool print_and_ok(const sinks& s) const;

Same emission as `print`, then returns `has_value()`. The no-argument overload uses default routing (info → `cout`, errors/warnings → `cerr`). Boundary idiom: `if (r.print_and_ok()) { use r.value(); }`.


### scope_guard open_if(bool enable, report::key name, code tag = code::info_micros, int64_t value = 0);
### scope_guard open_if(bool enable, std::string_view name, code tag = code::info_micros, int64_t value = 0);
### scope_guard open(report::key name, code tag = code::info_micros, int64_t value = 0);
### scope_guard open(std::string_view name, code tag = code::info_micros, int64_t value = 0);

Open a child scope on this result's report.

* `open_if(enable, name, …)` — same as `open` when `enable` is `true`; returns a no-op guard otherwise (no report nodes, no timer reads). Does not skip the wrapped work. Prefer the `report::key` overload at hot paths when the name is pre-interned.
* `open(name, tag, value)` — general parent scope. `info_micros` (default) auto-times (wall-clock, `std::chrono::steady_clock`) via `scope_guard`; other tags push `value` untouched.


### template \<typename F\> auto step(bool enable, report::key name, F&& f) -> decltype(f());
### template \<typename F\> auto step(bool enable, std::string_view name, F&& f) -> decltype(f());

Same shape as [`report::step`](#reportstep) — forwards to the underlying report. Use it on `result<T>` for the common case of "run this lambda inside an optional timed scope".


### void append(report&& child);

Append a foreign `report` under whichever scope is currently open on this result.


### template <typename U> result& merge(result<U>&& child);
### template <typename U> result& operator<<(result<U>&& child);

Append the child's report into this one, drop its value, and enforce the value/error invariant. `operator<<` is a stream-style shorthand that returns `*this`, so merges chain: `r << child_a << child_b;`.


### template \<typename U\> std::optional\<U\> merge_take(result\<U\>&& child);

Take the child's value (if any), merge its report into this result, return the value. Replaces the manual extract-then-`merge` pattern.


### bool is_well_formed() const;

Sanity check: `has_value() || has_error()`. A well-formed result is either a value or an error — never an empty/clean result with no errors.


### std::expected comparison

| `std::expected<T,E>` | `result<T>` |
|--------------------|-------------|
| `has_value()`, `operator bool` | same |
| `value()`, `operator*` | same (+ `operator->` for non-pointer `T`) |
| `emplace`, `operator=(U&&)` | same |
| `error()` → `unexpected<E>` | errors live in `report()` (tree, not single `E`) |
| — | `print` / `print_and_ok`, scopes, `merge`, `merge_take` |


## free functions

### result\<T\> fail(report rep);

Builds a failed result carrying a pre-populated report.


### result\<T\> error(code c, std::string_view msg, int64_t primary = 0, std::initializer_list<attr> extra = {});

Builds a failed result with a single error message.


### result\<T\> forward_as(result\<U\>&& src, T value);

Forwards `src`'s diagnostics into a new `result<T>` whose value is `value` on success or absent on failure. The value/error invariant still applies, so when `src` carries an error `value` is dropped. Used to change the value type while preserving diagnostics.


### T exit_on_fail(result\<T\>&& r, int exit_code = 1, std::ostream& os = std::cerr);

Prints diagnostics (if any) and either returns the value or calls
`std::exit(exit_code)`. Used at process boundaries (CLI entry points,
examples) where errors should terminate. All bands — including the info
band — route to `os` (default `std::cerr`).

```
nonterminals<> nts;
grammar g = diagnostics::exit_on_fail(
    tgf<>::from_file(nts, "arithmetic.tgf"));
parser p(g);
```


## report

```
struct idni::diagnostics::report;
```

A tree of nodes. Each node carries a [`code`](#code) tag, an interned name key, an integer value, optional attributes, and a parent index. Errors, warnings, infos, timed scopes, counters, and memory readings are all stored as nodes — only the tag distinguishes them.


### constructor

```
report();
```

Creates an empty report. No root scope is pushed until `reset()` is called or a `result<T>` is reset with a name.


### void reset(std::string_view root_name = "root");
### void reset(key root);

Drops all nodes and pushes a new root scope. Dynamic strings (`intern_dynamic`) persist across `reset()` — only `clear()` drops them.


### void clear();

Drops all nodes, the interned string table, and the open-scope stack.


### key intern(const char* s);
### key intern(std::string_view s);
### key intern_dynamic(std::string_view s);
### std::string_view str(key id) const;

Interns a string and returns its `key`. `str(id)` resolves a key back to its string view. Empty strings map to `report::none` (key 0). Keys are `int_t`: positive ids are immutable built-in labels from `parser_strings::dict`, while unknown/user labels are report-local dynamic strings with negative ids.

<a id="thread-safety"></a>
Thread-safety: separate `report` objects can be used concurrently because built-in labels are immutable and dynamic labels are report-local. A single `report` object is not internally synchronized; concurrent mutation of the same report still requires external locking.


### scope_guard open(key name, code tag = code::info_micros, int64_t value = 0);
### scope_guard open(std::string_view name, code tag = code::info_micros, int64_t value = 0);

Opens a parent scope. Tag-driven:

* `code::info_micros` (the default) — a wall-clock RAII timer
  (`std::chrono::steady_clock`) runs from ctor to dtor; elapsed
  microseconds become the node's value on close.
* any other tag — no timer; the caller's `value` is final unless overwritten via `set_node_value`.

Returns a `scope_guard` that pops the scope when destroyed.

```
auto _ = report_.open(keys().parsing);   // timed; parser inherits keys
auto _ = report_.open(name, code::info_kb, kilobytes); // untimed
```


### scope_guard open_if(bool enable, key name, code tag = code::info_micros, int64_t value = 0);
### scope_guard open_if(bool enable, std::string_view name, code tag = code::info_micros, int64_t value = 0);

Like `open(name, tag, value)` but conditional. When `enable` is `false` returns a no-op guard, so the non-measuring path has no report node allocation and no `clock()` timing.


<a id="reportstep"></a>
### auto step(bool enable, key name, F&& f) -> decltype(f());
### auto step(bool enable, std::string_view name, F&& f) -> decltype(f());

Convenience wrapper: `auto _ = open_if(enable, name); return f();`. Used at every leaf scope in the parser pipeline.

```
ret = report_.step(po.measure_scopes, keys().build_forest, [&] {
    return build_forest_impl();
});
```


### void count(key name, int64_t v);
### void count(key name, size_t v);
### void count(std::string_view name, int64_t v);
### void count(std::string_view name, size_t v);

Records a dimensionless counter as a leaf node with tag `code::info_count`.


### void kb(key name, int64_t kilobytes);
### void kb(std::string_view name, int64_t kilobytes);

Records a memory reading in kilobytes (`code::info_kb`). Negative values trip a debug assertion.


### void error(code c, key msg, int64_t primary = 0, std::initializer_list<attr> extra = {});
### void error(code c, std::string_view msg, int64_t primary = 0, std::initializer_list<attr> extra = {});
### void warning(key msg, int64_t primary = 0, std::initializer_list<attr> extra = {});
### void warning(std::string_view msg, int64_t primary = 0, std::initializer_list<attr> extra = {});
### void info(key msg, int64_t primary = 0, std::initializer_list<attr> extra = {});
### void info(std::string_view msg, int64_t primary = 0, std::initializer_list<attr> extra = {});

Push a diagnostic message. For errors, `c` must satisfy `is_error(c)` (top two bits `00`, range `0x0000`–`0x3FFF`). `primary` is a context-dependent integer (a location, a count, etc.); `format_message` renders it as `loc=N` for errors with a non-empty message (including `loc=0`) and as `value=N` otherwise when nonzero. `extra` is an optional list of `attr{key, value}` pairs.

```
r.error(code::parse_error, "unexpected token", offset,
        {{r.intern("line"), line}, {r.intern("col"), col}});
```


### void append(report&& other);

Merges another report into this one under the current scope. Parent links and attribute offsets are remapped; report-local dynamic keys are copied into the destination.


### bool has_error() const;

`true` if any error node is present.


### std::string format_message(size_t node_idx) const;

Formats a single message node as `"<text> (key=value ...)"` when attributes or `loc`/`value` are present — extras are grouped in parentheses after the message (no severity prefix). Returns empty if the node is not a message.


### void print(const sinks& s) const;
### void print(std::ostream& os) const;
### void print(const sink& s) const;
### void print() const;
### std::ostream& to_json(std::ostream& os, bool print_names = false) const;
### friend std::ostream& operator<<(std::ostream& os, const report& r);

Output the report.

* `print(os)` / `print(sink)` — all bands to one destination (common case).
* `print(const sinks&)` — per-band routing (errors/warnings flat, info tab-indented). Partial literals are fine; empty sinks drop their band.
* `print()` — `{ .error = &std::cerr, .warning = &std::cerr, .info = &std::cout }`.
* `to_json(ostream&, print_names=false)` — full tree as JSON for tooling. With `print_names=true` each node carries an extra `"tag_name"` field with the symbolic `code` label (e.g. `"parse_error"`).
* `operator<<` — plain `print(os)` on one stream (no color).


### void set_node_value(int32_t idx, int64_t v);

Overwrite the value of a node at a known index (no-op if out of range). Used by `scope_guard` to stamp elapsed time on close.


### const std::vector<node>& nodes() const;

Direct access to the underlying node vector. Useful for custom report rendering.


## report::scope_guard

RAII handle returned by `report::open` / `report::open_if`. On destruction:

* if the scope was timed, the elapsed **wall-clock microseconds**
  (`std::chrono::steady_clock` via `measures::steady_timer`) are written
  into the node's value;
* the top of the open-scope stack is popped.

Move-constructible only — no move-assignment. The reason is LIFO:
`pop_scope` only closes the top of the scope stack, so re-binding a guard
to a different scope would mis-pop. Move-construct is enough for
returning a guard from a factory; assignment would need a stricter
contract and no caller needs it. Guards must therefore be destroyed in
LIFO order; out-of-order destruction trips a debug assertion (release
builds silently keep the outer scope as current). A default-constructed
guard is a no-op (used by `open_if` when timing is disabled and by
moved-from instances).


## sink and sinks

```
struct idni::diagnostics::sink;
struct idni::diagnostics::sinks { sink error; sink warning; sink info; };
```

A `sink` is one output slot for a single severity band. Three sinks bundle together in a `sinks` aggregate (errors, warnings, info-band) that `report::print` consumes.

### sink::sink();
### sink::sink(std::ostream* os);
### template <typename F> sink::sink(F&& f);
### explicit operator bool() const noexcept;
### void operator()(std::string_view line) const;

* default ctor — empty sink; lines written to it are dropped silently.
* `ostream*` ctor — each line is written to `*os` followed by `'\n'`. Passing `nullptr` yields an empty sink.
* callable ctor — any `F` that takes `std::string_view`. Lines are forwarded as-is (no trailing newline appended).
* `operator()` writes a single line to the sink.

```
// route everything to one stream
r.report().print(cerr);
// or per-band when bands must diverge:
r.report().print({ .error = &cerr, .warning = &cerr, .info = &cout });

// collect lines into a vector for testing
std::vector<std::string> lines;
diagnostics::sink collect = [&](std::string_view l) {
    lines.emplace_back(l);
};
r.report().print({ .error = collect, .warning = collect, .info = collect });

// silence info, keep errors visible
r.report().print({ .error = &cerr, .warning = &cerr, .info = {} });
```


## code

```
enum class idni::diagnostics::code : uint16_t;
```

The discriminator for every node:

Top two bits of the `uint16_t` value (`c & 0xC000`) encode severity:

| Prefix | Range | Meaning |
|--------|-------|---------|
| `00` | `0x0000`–`0x3FFF` | error codes (`unknown=0`, `invalid_argument`, `io_error`, `parse_error`, `runtime_error`, `unknown_char_class=17`, …) |
| `01` | `0x4000`–`0x7FFF` | warnings (`warning` = `0x4000`) |
| `10` | `0x8000`–`0xBFFF` | info and metrics (`info` = `0x8000`, `info_micros` = `0x8001` — time in µs, auto-scaled on print to µs/ms/s; `info_kb` = `0x8002`; `info_count` = `0x8003`) |
| `11` | `0xC000`–`0xFFFF` | unused — do not emit |

Predicates:

```
constexpr bool is_error(code c);       // 00 band
constexpr bool is_warning(code c);     // 01 band
constexpr bool is_info(code c);        // 10 band (includes metrics)
constexpr bool is_metric(code c);      // info_micros / info_kb / info_count
constexpr bool uses_micros_scale(code c);
```


## attr

```
struct attr { int_t key = 0; int64_t value = 0; };
```

Key-value attribute carried alongside a message node. `key` is an interned name in the same table as scope/message names. `int_t` is the project-wide signed integer type defined in [`defs.h`](../src/defs.h).


## parser-specific helpers

Parser counters and scope names live in `src/parser_strings.h` under `idni::parser_strings`:

* `names` — `constexpr std::string_view` display labels (`"grammar load"`, `"parse"`, `"build forest"`, …).
* `messages` — static message prefixes (`cannot open file: `, …).
* `keys` — immutable positive `int_t` ids for built-in labels; `parser<C,T>` privately inherits `keys`.
* `dict` / `dict_size` / `dict_reset` — read-only lookup for built-in static labels (`dict_reset` is a compatibility no-op).
* `counters` — plain struct of `size_t` counters (one field per `PARSER_DIAG_COUNTERS` entry); incremented in the parsing hot loop when `TAU_PARSER_MEASURE_COUNTERS` is defined.
* `flush_counters(report&, const counters&, const keys&)` — pushes every non-zero counter into the report at end of parse.

Report keys are `int_t`: positive ids resolve through the immutable `parser_strings::dict` static-label table, negative ids are report-local dynamic strings (`intern_dynamic`).


## interaction with the parser

`parser<C, T>::result` carries a `report` populated automatically during parsing. Enable timing with `parse_options::measure_scopes` and counters with `measure_counters`; sub-scopes around forest/preprocess steps are further gated by `measure_forest` and `measure_preprocess`. After a parse:

```
auto r = p.parse(input);
if (!r.found) std::cerr << r.parse_error.to_str() << '\n';
r.report().print({                         // routed output
    .error = &std::cerr, .warning = &std::cerr, .info = &std::cout });
r.report().to_json(std::cout);             // JSON for tooling
```

See [`measure`](measure.md) for the underlying timer primitive.
