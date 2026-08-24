#!/bin/bash

# requires mingw-w64

DEV_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

dev_reject_build_type w64-debug Debug "$@"
./dev build Debug -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake "$@"
