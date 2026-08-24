#!/bin/bash

set -euo pipefail

for PACKAGE_FLAG in TAU_PARSER_WINDOWS_ZIP_PACKAGE TAU_PARSER_WINDOWS_PACKAGE; do
	./dev packages \
		"-D${PACKAGE_FLAG}=ON" \
		-DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake \
		"$@"
done
