#!/bin/bash
# The unordered_dense package for windows-x86_64-mingw.
# scripts/dep/common/unordered-dense.sh holds the shared recipe.

set -u

DEP_FILE_TARGET=windows-x86_64-mingw
DEP_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

source "$(dirname "${BASH_SOURCE[0]}")/../common/unordered-dense.sh"
