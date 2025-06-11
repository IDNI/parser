#!/bin/bash

./packages.sh \
        -DTAU_PARSER_WINDOWS_ZIP_PACKAGE=ON \
        -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake \
        $@

./packages.sh \
        -DTAU_PARSER_WINDOWS_PACKAGE=ON \
        -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake \
        $@
