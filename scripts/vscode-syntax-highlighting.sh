#!/bin/bash
# Build the syntax_highlighting WASM module and assemble the VS Code / Cursor
# TGF extension in place at editors/vscode/.
#
# Usage:
#   ./scripts/vscode-syntax-highlighting.sh
#
# Prerequisites: scripts/dep-emsdk.sh (run once), node and npm on PATH.

set -euo pipefail

TOP="$(cd "$(dirname "$0")/.." && pwd)"
source "${TOP}/scripts/devrc"
dep_entry "$@"

EMSDK_PREFIX="$(dep_shared_prefix)"
EMSCRIPTEN_BUILD="$TOP/build/release-wasm"
WASM_SRC="$EMSCRIPTEN_BUILD"
EXT_DIR="$TOP/editors/vscode"

SYNTAX_HIGHLIGHTING_TARGET="syntax_highlighting"

# --- activate emsdk --------------------------------------------------------
if [ -f "$EMSDK_PREFIX/emsdk/emsdk_env.sh" ]; then
	# shellcheck disable=SC1091
	source "$EMSDK_PREFIX/emsdk/emsdk_env.sh"
elif ! command -v emcc &>/dev/null; then
	echo "Emscripten not found. Run ./scripts/dep-emsdk.sh first." >&2
	exit 1
fi

# --- build wasm highlight target -------------------------------------------
if [ ! -f "$EMSCRIPTEN_BUILD/CMakeCache.txt" ]; then
	echo "Configuring emscripten build..."
	cmake -B "$EMSCRIPTEN_BUILD" --preset release-wasm \
		-DTAU_PARSER_BUILD_EMSCRIPTEN=ON
fi

JOBS="$(dep_jobs)"
echo "Building $SYNTAX_HIGHLIGHTING_TARGET (jobs=$JOBS)..."
cmake --build "$EMSCRIPTEN_BUILD" --target "$SYNTAX_HIGHLIGHTING_TARGET" -j "$JOBS"

# --- copy wasm output into the extension ------------------------------------
mkdir -p "$EXT_DIR/wasm"
cp "$WASM_SRC/syntax_highlighting.js" "$EXT_DIR/wasm/"
cp "$WASM_SRC/syntax_highlighting.wasm" "$EXT_DIR/wasm/"
echo "  wasm: syntax_highlighting.js + .wasm -> $EXT_DIR/wasm/"

# --- copy grammar files into the extension -----------------------------------
mkdir -p "$EXT_DIR/grammars"
node -e '
const fs = require("fs");
const path = require("path");
const top = process.argv[1];
const extDir = process.argv[2];
const grammars = JSON.parse(fs.readFileSync(path.join(extDir, "grammars.json"), "utf8"));
for (const [langId, entry] of Object.entries(grammars)) {
	const src = path.join(top, entry.path);
	if (!fs.existsSync(src)) {
		console.error(`Grammar source missing for ${langId}: ${src}`);
		process.exit(1);
	}
	const dst = path.join(extDir, "grammars", `${langId}.tgf`);
	fs.copyFileSync(src, dst);
	console.log(`  grammar: ${src} -> ${dst}`);
}
' "$TOP" "$EXT_DIR"

# --- compile the extension's TypeScript -------------------------------------
echo "Installing extension dependencies and compiling TypeScript..."
( cd "$EXT_DIR" && npm install && npx tsc -p . )

# --- smoke test --------------------------------------------------------------
echo "Running WASM smoke test..."
node "$TOP/tests/syntax_highlighting/test_syntax_highlighting.mjs" "$EXT_DIR"

echo ""
echo "Extension ready at: $EXT_DIR"
echo "To install, symlink:"
echo "  ~/.cursor/extensions/tgf        -> $EXT_DIR"
echo "  ~/.cursor-server/extensions/tgf -> $EXT_DIR"
echo "Remove an old tgf-syntax-highlighting link, else both extensions load."
