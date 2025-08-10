# Scripts

Helpers for building and testing Tau Parser Library.

These scripts are also available as `./dev <SCRIPT> <SCRIPT_OPTIONS>` from project's root directory.

## Cleaning

- `clean [all]` removes debugging files. If `all` is provided, it also removes build directories as well.

## Building

Building scripts run cmake with different options. Additional script options provided to `./dev <script>` are passed as additional arguments to cmake. 

- `build [<BUILD_TYPE> [<CMAKE_OPTIONS>]]` builds Tau Parser Library
- `debug [<CMAKE_OPTIONS>]`, `release [<CMAKE_OPTIONS>]` and `relwithdebinfo [<CMAKE_OPTIONS>]` call `build` script with `Debug`, `Release`, `RelWithDebInfo` BUILD_TYPE respectively
- `w64-debug [<CMAKE_OPTIONS>]` and `w64-release [<CMAKE_OPTIONS>]` build for windows using mingw-w64

### Build directories

`debug` and `w64-debug` builds are using `build-Debug` directory.

`release` and `w64-release` builds are using `build-Release` directory.

`relwithdebinfo` build is using `build-RelWithDebInfo` directory.

## Building release packages

- `packages [<CMAKE_OPTIONS>]` builds packages for linux
- `w64-packages [<CMAKE_OPTIONS>]` builds packages for windows

Packages are created in `build-Release/packages` directory.

## Emscripten

- `get-emsdk` downloads and installs emsdk into `external/emsdk` directory

## TGF

- `tgf [<CMAKE_OPTIONS>]` builds TGF tool for generating parsers from grammars in TGF files

## Testing

- `test-with-tau` script clones `tau-lang`, compiles it with current parser code and runs its release tests
