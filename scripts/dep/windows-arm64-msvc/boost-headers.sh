#!/bin/bash
# The Boost headers package for windows-arm64-msvc.
# scripts/dep/common/boost-headers.sh holds the shared recipe.

set -u

DEP_FILE_TARGET=windows-arm64-msvc
DEP_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

source "$(dirname "${BASH_SOURCE[0]}")/../common/boost-headers.sh"
