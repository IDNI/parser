#!/bin/bash
# Build and optionally serve the TGF WASM REPL in the browser.
#
# Usage:
#   ./dev tgf-wasm          # build only
#   ./dev tgf-wasm serve    # build + serve
#
# Prerequisites: ./dev dep-emsdk.sh (run once)

set -euo pipefail

TOP="$(cd "$(dirname "$0")/.." && pwd)"
source "$TOP/scripts/devrc"

BUILD_DIR="$TOP/build/emscripten"

# --- activate emsdk --------------------------------------------------------
EMSDK_DIR="$(dep_shared_prefix)/emsdk"
if [ -f "$EMSDK_DIR/emsdk_env.sh" ]; then
	source "$EMSDK_DIR/emsdk_env.sh" 2>/dev/null
elif ! command -v emcc &>/dev/null; then
	echo "Emscripten not found. Run './dev dep-emsdk.sh' first." >&2
	exit 1
fi

# --- ensure xterm.js is installed ------------------------------------------
if [ ! -d "$TOP/js/tau-wasm-terminal/node_modules" ]; then
	if ! command -v npm &>/dev/null; then
		echo "npm not found. Install Node.js/npm, then re-run." >&2
		exit 1
	fi
	echo "Installing xterm.js packages..."
	npm ci --prefix "$TOP/js/tau-wasm-terminal"
fi

# --- configure + build ------------------------------------------------
export TAU_BUILD_JOBS="${TAU_BUILD_JOBS:-4}"
echo "Building tgf_standalone (jobs=$TAU_BUILD_JOBS)..."
(cd "$TOP" && "$TOP/dev" preset release-wasm-tests-browser -DTAU_BUILD_JOBS="$TAU_BUILD_JOBS")

OUT="$BUILD_DIR/js/tgf/tgf_standalone.js"
echo "Built: $OUT ($(du -h "$OUT" | cut -f1))"

# --- serve (optional) ------------------------------------------------------
for arg in "$@"; do
	case "$arg" in
		serve) SERVE=1 ;;
	esac
done

if [ "${SERVE:-0}" = "1" ]; then
	(cd "$TOP" && "$TOP/dev" tgf-serve)
fi
