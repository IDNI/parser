#!/bin/bash
# The unordered_dense package for linux-x86_64.
# scripts/dep/common/unordered-dense.sh holds the shared recipe.

set -u

DEP_FILE_TARGET=linux-x86_64
DEP_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

source "$(dirname "${BASH_SOURCE[0]}")/../common/unordered-dense.sh"
