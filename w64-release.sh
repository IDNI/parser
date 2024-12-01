#!/bin/bash

# requires mingw-w64

./build.sh Release -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake $@
