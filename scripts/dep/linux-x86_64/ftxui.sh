#!/bin/bash
# The FTXUI package for linux-x86_64.
# scripts/dep/common/ftxui.sh holds the shared recipe.

set -u

DEP_FILE_TARGET=linux-x86_64
DEP_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

source "$(dirname "${BASH_SOURCE[0]}")/../common/ftxui.sh"
