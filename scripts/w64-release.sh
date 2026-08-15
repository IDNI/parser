#!/bin/bash

# requires mingw-w64

DEV_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

dev_reject_build_type w64-release Release "$@"
./dev build Release -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake "$@"
