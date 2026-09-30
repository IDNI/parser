#!/bin/bash
# The parser SDK and tgf packages for wasm32-emscripten.
# scripts/dep/common/parser-sdk.sh holds the shared recipe.

set -u

DEP_FILE_TARGET=wasm32-emscripten
DEP_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

source "$(dirname "${BASH_SOURCE[0]}")/../common/parser-sdk.sh"
