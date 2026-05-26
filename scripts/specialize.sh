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
BUILD_TYPE="${DEV_POSITIONAL[0]:-Release}"
case "${BUILD_TYPE}" in
	Release)        BUILD_DIR="build/release" ;;
	Debug)          BUILD_DIR="build/debug" ;;
	RelWithDebInfo) BUILD_DIR="build/relwithdebinfo" ;;
	*)              BUILD_DIR="build-${BUILD_TYPE}" ;;
esac

cmake -B "${DEV_ROOT}/${BUILD_DIR}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
	-DTAU_PARSER_BUILD_TGF=ON \
	-DTAU_PARSER_SPECIALIZE="${TGF_FILE}" \
	"${DEV_CMAKE[@]}" "$@" \
	"${DEV_ROOT}"

cmake --build "${DEV_ROOT}/${BUILD_DIR}" \
	--target "${EXE_NAME}" -j "${TAU_BUILD_JOBS}"

echo "Built: ${BUILD_DIR}/${EXE_NAME}"
echo "Note: re-specializing replaces the CMake target;"
echo "      old *_grammar binaries remain until ./dev clean"
