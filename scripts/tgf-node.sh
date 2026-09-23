#!/bin/bash
# Run the TGF FTXUI REPL for WebAssembly in the terminal through Node.js.
#
# Usage:
#   ./dev tgf-node                # preloaded /tau.tgf
#   ./dev tgf-node mygrammar.tgf  # your own grammar file
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

# --- build tgf_standalone if missing ---------------------------------------
OUT="$BUILD_DIR/js/tgf/tgf_standalone.js"
if [ ! -f "$OUT" ]; then
	export TAU_BUILD_JOBS="${TAU_BUILD_JOBS:-4}"
	echo "Building tgf_standalone (jobs=$TAU_BUILD_JOBS)..."
	(cd "$TOP" && "$TOP/dev" preset release-tgf-emscripten -DTAU_BUILD_JOBS="$TAU_BUILD_JOBS")
fi

# the program has no default grammar, so pass the preloaded one
[ $# -eq 0 ] && set -- /tau.tgf

exec node "$TOP/js/tau-wasm-terminal/node.js" "$OUT" "$@"
