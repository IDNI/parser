#!/bin/bash
set -euo pipefail

DEV_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

[[ $# -ge 1 ]] || {
	echo "usage: ./dev specialize <file>.tgf [cmake-args...]"
	exit 1
}

TGF_FILE="$(realpath "$1")"
shift
dev_entry "$@"
BASENAME="$(basename "$TGF_FILE" .tgf)"
EXE_NAME="${BASENAME}_grammar"
BUILD_DIR="$(preset_binary_dir "${BUILD_TYPE,,}")"

cmake -B "${DEV_ROOT}/${BUILD_DIR}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
	-DTAU_PARSER_BUILD_TGF=ON \
	-DTAU_PARSER_SPECIALIZE="${TGF_FILE}" \
	"${DEV_CMAKE[@]}" \
	"${DEV_ROOT}"

cmake --build "${DEV_ROOT}/${BUILD_DIR}" \
	--target "${EXE_NAME}" -j "${TAU_BUILD_JOBS}"

echo "Built: ${BUILD_DIR}/${EXE_NAME}"
echo "Note: re-specializing replaces the CMake target;"
echo "      old *_grammar binaries remain until ./dev clean"
