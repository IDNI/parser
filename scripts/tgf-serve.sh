#!/bin/bash
# Serve the TGF WASM REPL for browser testing.
# Prerequisites: build the target first: ./dev tgf-wasm
#
# Usage:
#   ./dev tgf-serve              # serve on port 8088
#   ./dev tgf-serve 3000         # serve on custom port

set -euo pipefail

TOP="$(cd "$(dirname "$0")/.." && pwd)"
WASM_DIR="$TOP/build/emscripten/js/tgf"
PORT="${1:-8088}"

if [ ! -f "$WASM_DIR/index.html" ]; then
	echo "Error: $WASM_DIR/index.html not found." >&2
	echo "Build first: ./dev tgf-wasm" >&2
	exit 1
fi

exec node "$TOP/js/tau-wasm-terminal/serve.js" "$WASM_DIR" "$PORT"
