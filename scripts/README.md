# Scripts

Helpers for building and testing Tau Parser Library.

Run any script from the project root via `./dev`:

```bash
./dev <SCRIPT> [SCRIPT_OPTIONS...]
```

`./dev help` lists available scripts (everything in `scripts/*.sh`).

## How `./dev` handles arguments

[`dev`](../dev) runs `scripts/*.sh`. CMake drivers [`build.sh`](build.sh) and
[`preset.sh`](preset.sh) source [`devrc`](devrc). `build_entry` and
`preset_entry` call `dev_entry` internally; use `dev_entry` when you only need
parsed args and `TAU_BUILD_JOBS`. `test_entry` runs a legacy build + `ctest`;
pass CMake `-D` flags as arguments (e.g. `-DTAU_PARSER_BUILD_TESTS=ON`).

When embedded in **tau-lang**, the same `devrc` file is sourced from
`external/parser/scripts/` (tau sets `DEV_ROOT` to the parser submodule path).

All of the following may appear **in any order** on the command line
(`normalize_args` in `devrc` is the single parser):

| Token | Effect |
|-------|--------|
| `-D…` | CMake definition (via `DEV_CMAKE`) |
| `Debug` / `Release` / `RelWithDebInfo` / `Coverage` | legacy build type |
| `-v` | verbose build |
| `--target NAME` | build only this target |
| `-G NAME` | sets `GENERATOR` (not `DEV_CMAKE`); legacy build defaults to Ninja; preset uses preset generator unless `-G` is passed |
| preset name (e.g. `release-tests`) | preset to configure |
| `run` | after preset build: run tests or `tgf` |
| `--` | start of program args |

Examples:

```bash
./dev preset release-tests run -DTAU_PARSER_BUILD_TESTS=ON
./dev preset -DTAU_PARSER_BUILD_TESTS=ON release-tests run
./dev build -v Debug --target test_parser -DTAU_PARSER_BUILD_TESTS=ON
```

Use `--` to pass arguments to a program run by a script (e.g. `tgf`):

```bash
./dev preset release-tgf run -- --help
```

## Parallel build jobs (`TAU_BUILD_JOBS`)

`./dev` sets **`TAU_BUILD_JOBS`** (and `CMAKE_BUILD_PARALLEL_LEVEL`) for every
script. Resolution order:

1. `-DTAU_BUILD_JOBS=N` on the command line (any position)
2. `TAU_BUILD_JOBS` already in the environment
3. Half of detected logical CPU cores (auto)

Examples:

```bash
./dev preset release-tests run -DTAU_BUILD_JOBS=14
TAU_BUILD_JOBS=14 ./dev preset release-tests run
export TAU_BUILD_JOBS=10
./dev release
```

`-DTAU_BUILD_JOBS` is stripped before `cmake --preset` (CMake rejects it there);
the value is applied via the exported environment. Other `-D` options are passed
to CMake as usual.

CMake configure also reads `$ENV{TAU_BUILD_JOBS}` when the cache value is `0`
(see [`CMakeLists.txt`](../CMakeLists.txt)). A machine-local
[`CMakeLocalLists.txt`](../CMakeLocalLists.txt) may override this.

## Dual build directories

Two layouts coexist on purpose:

| Path | Used by | Example |
|------|---------|---------|
| `build-${BUILD_TYPE}` | [`build.sh`](build.sh), `debug`, `release`, `packages`, … | `build-Release`, `build-Debug` |
| `build/<lowercase>` | [`preset.sh`](preset.sh), `cmake --preset` | `build/release`, `build/debug` |

| CMake `BUILD_TYPE` | Legacy (`./dev build`) | Preset `binaryDir` |
|--------------------|------------------------|---------------------|
| `Release` | `build-Release` | `build/release` |
| `Debug` | `build-Debug` | `build/debug` |
| `RelWithDebInfo` | `build-RelWithDebInfo` | `build/relwithdebinfo` |
| MinGW Release | `build-Release` + toolchain `-D` | `build/release-mingw` |
| Emscripten | `build-Release` + `-D` | `build/emscripten` |

Legacy wrappers are unchanged. Prefer presets for new work; use the matching
`build/…` tree when running `cpack` after a preset build.

## Cleaning

- `clean [all]` — remove debugging artifacts; with `all`, also remove `build`,
  `build-*`, and preset trees under `build/`.

## Building

Building scripts run CMake with different options. Extra `-D` options from
`./dev` are passed through to CMake.

- `build [<BUILD_TYPE> [<CMAKE_OPTIONS>]]` — legacy tree (`build-Release`, …)
- `debug`, `release`, `relwithdebinfo` — shorthand for `build` with matching type
- `w64-debug`, `w64-release` — Windows cross-build (legacy tree + mingw toolchain)

### CMake presets

`preset [<PRESET>] [run] [<CMAKE_OPTIONS>]` — configure (fresh), build, and
optionally test or run `tgf` via
[`CMakePresets.json`](../CMakePresets.json).

```bash
./dev preset release-tests run
./dev preset release-all run
./dev preset all
./dev preset release-examples
./dev preset release-packages
./dev preset release-mingw-packages
./dev preset relwithdebinfo
./dev preset release-measure
./dev preset release-tgf run -- --version
./dev preset emscripten
```

Default preset name is `release` if omitted.

#### Preset reference

| Preset | Builds | `binaryDir` |
|--------|--------|-------------|
| `release`, `debug` | header-only library (default) | `build/release`, `build/debug` |
| `release-tests`, `debug-tests` | + tests | same |
| `release-tgf`, `debug-tgf` | + TGF | same |
| `release-tgf-clang`, `debug-tgf-clang` | + TGF (clang) | `build/release-clang`, `build/debug-clang` |
| `release-clang`, `debug-clang` (+ `-make-`, `-ninja-`) | header-only (default) | `build/release-clang`, `build/debug-clang` |
| `release-examples`, `debug-examples` | + examples | same |
| `release-packages` | TGF + examples (Linux cpack) | `build/release` |
| `release-all`, `debug-all`, `all` | everything (`TAU_PARSER_BUILD_ALL`) | `build/release` / `build/debug` (`all` → `build/release`) |
| `release-static`, `release-shared` | static/shared + install | `build/release` |
| `release-measure`, `debug-measure` | + instrumentation | same |
| `debug-asan` | + address sanitizer | `build/debug` |
| `relwithdebinfo` (+ `-tests`, `-tgf`, …) | RelWithDebInfo variants | `build/relwithdebinfo` |
| `release-mingw`, `debug-mingw` | Windows cross-compile | `build/release-mingw`, … |
| `release-mingw-packages`, `release-mingw-packages-zip` | MinGW + cpack (NSIS or ZIP) | `build/release-mingw` |
| `emscripten` | Emscripten (`EMSCRIPTEN_DIR` in preset) | `build/emscripten` |

Hidden building blocks (`_build-tests`, `_build-packages`, `_build-all`, …) are
composed by the public presets above; see [`CMakePresets.json`](../CMakePresets.json).

#### `run` and packaging presets

- Presets whose name contains **`package`** run `cpack -C Release` after build
  (`release-packages`, `release-mingw-packages`, `release-mingw-packages-zip`).
- **`run`** — `ctest` when the name contains `test`, ends with `-all`, or is
  `all` (not used on packaging presets); otherwise runs `tgf` (args after `--`).

```bash
./dev preset release-packages
# configure, build, cpack → build/release/packages

./dev packages
# legacy: still uses build-Release/packages
```

## Building release packages

- `packages` — legacy: `build-Release` + `cpack`
- `w64-packages` — legacy: Windows NSIS and ZIP via mingw + `packages`
- Preset alternative: `./dev preset release-packages`, `./dev preset release-mingw-packages`

## Emscripten

- `get-emsdk` — download emsdk into `external/emsdk`
- `js-debug`, `js-release` — legacy `build-Debug` / `build-Release` + Emscripten `-D`
- Preset: `./dev preset emscripten` (sets `EMSCRIPTEN_DIR` in the preset)

## TGF

- `tgf` — legacy release build + `-DTAU_PARSER_BUILD_TGF=ON`
- Preset: `release-tgf` / `debug-tgf`, or `./dev preset release-tgf run -- …`
- Clang: `release-tgf-clang` / `debug-tgf-clang`, or `./dev preset release-tgf-clang run -- --help`

## Specialize

`specialize <FILE.tgf> [<BUILD_TYPE>] [<CMAKE_OPTIONS>]` — build a grammar-baked
REPL binary from a `.tgf` file.

```bash
./dev specialize tests/fixtures/tiny.tgf
# → build/release/tiny_grammar

./dev specialize tests/fixtures/tau.tgf Debug
# → build/debug/tau_grammar
```

The resulting executable behaves like `tgf <file>.tgf` (same REPL, same
`parse`/`grammar`/`get`/`set`/… commands), but the parser is generated and
compiled in: no `.tgf` or `.generated.h` is needed at runtime. `load` and
`reload` print a "not available" message in a specialized binary.

| Item | Default | Override |
|------|---------|----------|
| Output name | `<basename>_grammar` | (not exposed in script) |
| Build type | `Release` | first positional: `Debug`, `RelWithDebInfo`, … |
| Build dir | `build/release` (preset layout) | follows the build type |

Mechanism: the script invokes the main CMake build with
`-DTAU_PARSER_SPECIALIZE=<absolute-path>` and the `*_grammar` target;
[`cmake/specialize.cmake`](../cmake/specialize.cmake) runs `tgf <file>.tgf gen`
at build time and emits an executable from
[`cmake/specialize/specialized_main.cpp.in`](../cmake/specialize/specialized_main.cpp.in).

Editing the `.tgf` file triggers a CMake reconfigure on the next build (via
`CMAKE_CONFIGURE_DEPENDS`), which keeps the embedded grammar text in sync with
the regenerated parser.

Re-running `./dev specialize bar.tgf` replaces the active CMake target;
previously built `*_grammar` executables remain in the build tree until
`./dev clean`.

## Testing

- `test-with-tau` — clone `tau-lang`, build against current parser, run its tests
