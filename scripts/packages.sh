#!/bin/bash

set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/devrc"

./dev build Release \
	-DTAU_PARSER_BUILD_TGF=ON \
	-DTAU_PARSER_BUILD_EXAMPLES=ON \
	"$@"
run_cpack ./build-Release
