[back to index](../README.md#classes-and-structs)
[full reference](diagnostics.md)

# diagnostics — quick start

```
namespace idni::diagnostics;
```

`idni::diagnostics` is a unified facility for **diagnostic messages** (errors, warnings, infos) and **metrics** (timed scopes, counters, memory). Every function that does something worth measuring or reporting returns a `result<T>` — a value together with a `report` tree.

This page is example-driven. For the full API, see the [diagnostics reference](diagnostics.md).


## 1. Include and namespace

```cpp
#include "utility/diagnostics.h"
using namespace idni::diagnostics;
```


## 2. Timed scopes

Open a scope; it closes when the guard goes out of scope. With the default `info_micros` tag, elapsed microseconds are recorded automatically.

```cpp
void do_work() {
    report r;
    r.reset("my_operation");                     // root scope

    {
        auto _ = r.open("parse");                // timed child scope
        // ... expensive parsing ...
    }   // guard destroys → elapsed µs written to "parse" node

    {
        auto _ = r.open("evaluate");
        // ... evaluation ...
    }

    std::cout << r;  // prints the tree
}
```

Output:

```
my_operation:       1234.56 ms
	parse:           800.12 ms
	evaluate:        430.44 ms
```


## 3. Conditional measurement

When you don't want diagnostic overhead, don't call `open` / `open_if`. Use the report only for error/warning/info messages — those have no runtime cost beyond the call itself. To compile out timed scopes and counters entirely, gate them behind preprocessor macros.

At runtime, use `open_if`: when `enable` is false there is no report node allocation and no timer read — only a trivial no-op guard on the stack. The wrapped work still runs. `open_if` exists on both `report` and `result<T>`.

```cpp
void maybe_measure(report& r, bool enable_timing) {
    auto _ = r.open_if(enable_timing, "expensive_step");
    // ... work ...
}
```

For a single function-call section, `step()` (on `report`) is shorter:

```cpp
auto result = r.step(enable, "compute", [&] {
    return heavy_computation();
});
```

Anti-pattern — do not roll your own conditional with `std::optional`:

```cpp
// avoid this
auto _ = enable ? std::optional{r.open("compute")} : std::nullopt;
// use this
auto _ = r.open_if(enable, "compute");
```


## 4. Counters and memory

```cpp
void process(report& r) {
    r.count("records_processed", 1024);           // dimensionless counter
    r.count("allocations", alloc_count);

    long mem = idni::measures::current_rss_kb(); // process RSS in KB
    r.kb("memory_rss", mem);
}
```


## 5. Diagnostic messages

Errors, warnings, and infos are just nodes with message tags. Errors carry an error `code`; warnings and infos don't need one.

```cpp
void validate(report& r, const input& in) {
    if (in.empty()) {
        r.error(code::invalid_input_stream,
            "input is empty");
        return;
    }
    if (in.suspicious())
        r.warning("input looks unusual");

    r.info("validation passed");
}
```

Attach structured attributes to messages:

```cpp
r.error(code::parse_error, "unexpected token", offset,
    {{r.intern("line"), line_num},
     {r.intern("col"),  col_num}});
```

Grammar-specific codes include `unknown_char_class` (undefined character class name). See the [error `code` table](diagnostics.md#code) in the reference.


## 6. The `result<T>` class

`result<T>` is the main diagnostics container. It holds a value **and** a `report`. Use `open` / `open_if` for timed scopes; the name constructor only resets the report root.

### 6.1 Success path

```cpp
result<int> compute(int x, bool measure) {
    result<int> r;
    {
        auto _ = r.open_if(measure, "compute");
        auto _m = r.open_if(measure, "multiply");
        int value = x * 42;
        r.emplace(value);
    }
    return r;
}
```

### 6.2 Using the result

```cpp
auto r = compute(10);
if (r.print_and_ok(std::cerr)) {        // print diagnostics, return has_value()
    std::cout << "result: " << r.value() << '\n';
}
```

For full control of where each band lands, call the report's `print` with sinks:

```cpp
r.print(cerr);   // all bands to one stream
// per-band routing when needed:
r.report().print({ .error = &cerr, .warning = &cerr, .info = &cout });
```

### 6.3 Pointer results

For pointer-valued results, `result<T*>` supports `nullptr` comparison:

```cpp
result<Forest*> r = build_forest();
if (r != nullptr) {
    traverse(r.value());
}
```


## 7. Error propagation

### 7.1 Logging an error

`error()` pushes an error and **drops the value** — a failed result never carries a value.

```cpp
result<std::string> load_file(const char* path) {
    result<std::string> r("load_file");  // resets root only; no timing

    if (!file_exists(path)) {
        r.error(code::io_error, "file not found");
        return r;   // no value, error logged
    }

    r.emplace(read_all(path));
    return r;
}
```

### 7.2 Factory: `error<T>()` and `fail()`

```cpp
if (bad_input)
    return error<Grammar>(code::invalid_argument, "bad input");

report rep;
rep.error(code::internal_error, "unreachable");
return fail<Forest>(std::move(rep));
```

### 7.3 Forwarding diagnostics: `forward_as<T>()`

Convert a child result into a different type while preserving
diagnostics — including any warnings or info nodes the child accumulated
on the way to success:

```cpp
result<Forest> build_forest(result<Grammar>&& g) {
    if (!g) return forward_as<Forest>(std::move(g), Forest{});
    Forest f = make_forest(g.value());
    return forward_as<Forest>(std::move(g), std::move(f));
}
```

Both branches go through `forward_as`, so g's report (warnings, infos,
timed sub-scopes) ends up in the returned `result<Forest>` whether the
build succeeds or not. The failure-branch `Forest{}` is dropped by the
value/error invariant.

### 7.4 Terminate on failure: `exit_on_fail<T>()`

At program boundaries, bail out cleanly:

```cpp
int main(int argc, char** argv) {
    nonterminals<char> nts;
    grammar<char> g = exit_on_fail(
        tgf<char>::from_file(nts, argv[1]));
    // ...
}
```

Writes messages to `stderr` and calls `std::exit(1)` on failure. Returns the value otherwise.


## 8. Merging child reports

Collect diagnostics from a sub-operation into a parent result:

```cpp
result<int> parent_op() {
    result<int> r("parent");
    result<int> child = child_op();   // has its own report
    r << std::move(child);            // merge child's report into r
    r.emplace(42);
    return r;
}
```

`r << std::move(child)` (or `r.merge(std::move(child))`) appends the child's
report; the child's value is discarded. To keep the value:

```cpp
if (auto v = r.merge_take(std::move(child)))
    r.emplace(*v + 1);
```

Merges chain:

```cpp
r << std::move(a) << std::move(b);
```

To attach the child's subtree under a named child scope of `r`, open that
scope first:

```cpp
{
    auto _ = r.open("child_work");
    r << std::move(child);   // child's subtree nests under "child_work"
}
```

With no scope open, the child attaches under the report root.


## 9. Printing

`report::print` is the single primitive. One pre-order tree walk
emits each node to a per-band sink:

- **errors** → indent 0, formatted as `<text> (attrs… loc=…)` when extras exist (no `error:` prefix)
- **warnings** → indent 0, formatted as `<text> (attrs… value=…)` when extras exist (no `warning:` prefix)
- **info band** (`info`, `info_micros`, `info_count`, `info_kb`)
  → tab-indented per tree depth, formatted as `<label>: <value>`

```cpp
report r;
// ... populate ...

// Defaults: error+warning → std::cerr, info → std::cout.
r.print();

// All bands to one stream.
r.print(std::cerr);

// Per-sink routing — partial literals are fine.
r.print({.error = &std::cerr, .warning = &std::cerr,
         .info  = &std::cout});

// Boost.Log (or any other logger): wrap each level in a lambda.
r.print({
    .error   = [&](auto s){ BOOST_LOG_SEV(lg, error)   << s; },
    .warning = [&](auto s){ BOOST_LOG_SEV(lg, warning) << s; },
    .info    = &info_stream,
});

// Everything to one stream (errors/warnings still flat at col 0;
// info-band keeps the tree shape).
std::cout << r;

// Machine-readable JSON.
r.to_json(std::cout);
```

Color is on by default: error lines are red, warnings yellow, info-band
lines use a zebra pattern. The global `idni::TC` (term_colors) gates the
escape sequences — call `TC.set(false)` to suppress them, e.g. for stable
test snapshots or non-TTY logs. The tgf CLI exposes this through its
`--colors` option.

Each sink is invoked once per emitted line; the line never contains a
trailing newline (the sink appends one for stream targets, the
callable wrapper does so for boost::log records). An empty sink
silently drops its band.

To filter on the caller side instead of routing, walk `r.nodes()`
with the `is_error` / `is_warning` / `is_info` / `is_metric`
predicates.


## 10. Putting it all together

A complete function that parses, measures, and reports. Timed scopes
(`open`, `open_if`) and the `step` convenience all live on `result<T>`
directly; counters and `kb` readings still go through the underlying
report (reach it via `r.report()`).

```cpp
result<Value> parse_and_evaluate(const char* input, bool g_measure) {
    result<Value> r;
    auto& rep = r.report();
    {
        auto _ = r.open_if(g_measure, "parse_and_evaluate");
        Grammar g;
        {
            auto _p = r.open_if(g_measure, "parse");
            if (!parse(input, g)) {
                r.error(code::parse_error, "syntax error");
                return r;
            }
        }
        Value v = r.step(g_measure, "evaluate", [&] {
            return evaluate(g);
        });
        rep.count("nodes_visited", v.nodes);
        rep.kb("result_size_kb", sizeof(v) / 1024);
        r.emplace(std::move(v));
    }
    return r;
}
```


## Further reading

- [diagnostics reference](diagnostics.md) — full API
- [measure](measure.md) — underlying timer primitive (`idni::measures::timer`)
- [`parser.h`](../src/parser.h) — how the parser populates reports during parsing
