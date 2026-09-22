#!/bin/sh
# tgf_wasm_parity.sh - Compare native tgf output against tgf_node WASM output
# for a grammar file and a set of REPL commands.
#
# Usage:
#   tgf_wasm_parity.sh <tgf_binary> <grammar_file> [<cmd>...]
#
# Returns 0 if outputs match, non-zero otherwise.
#
# Prerequisites: tgf_node.js must be built (./dev preset emscripten).

set -e

TGF_BIN="$1"
GRAMMAR="$2"
shift 2

if [ $# -eq 0 ]; then
	set -- "grammar" "p x = 0 || y = 1" "p x = 2" "quit"
fi

TMPDIR="${TMPDIR:-/tmp}"
NATIVE_OUT="$TMPDIR/tgf_wasm_parity_native.$$.txt"
WASM_OUT="$TMPDIR/tgf_wasm_parity_wasm.$$.txt"
cleanup() { rm -f "$NATIVE_OUT" "$WASM_OUT"; }
trap cleanup EXIT

# --- Native tgf (pipe mode) ------------------------------------------------
{
	for cmd in "$@"; do
		echo "$cmd"
	done
} | "$TGF_BIN" "$GRAMMAR" repl -X 2>&1 | sed 's/\x1b\[[0-9;]*m//g' > "$NATIVE_OUT" || true

# --- WASM tgf_node ----------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
node "$SCRIPT_DIR/tgf_wasm_parity.js" "$GRAMMAR" "$@" > "$WASM_OUT" 2>/dev/null || {
	echo "WASM parity test: tgf_node failed" >&2
	cat "$WASM_OUT" >&2
	exit 1
}

# --- Compare ----------------------------------------------------------------
normalize() {
	sed 's/\x1b\[[0-9;]*m//g' "$1" \
		| sed '/^\[.*\] tgf>.*$/d' \
		| sed '/^=== .* ===$/d' \
		| sed '/^Unproductive nonterminal/d' \
		| sed '/^Syntax Error:/d' \
		| sed '/^\.\.\.expecting/d' \
		| sed 's/[0-9.]* \(µs\|ms\)//g' \
		| sed 's/[[:space:]]*$//' \
		| grep -v '^$' || true
}

NATIVE_NORM="$TMPDIR/tgf_wasm_parity_native_norm.$$.txt"
WASM_NORM="$TMPDIR/tgf_wasm_parity_wasm_norm.$$.txt"
normalize "$NATIVE_OUT" > "$NATIVE_NORM"
normalize "$WASM_OUT" > "$WASM_NORM"

if diff -q "$NATIVE_NORM" "$WASM_NORM" >/dev/null 2>&1; then
	echo "PASS: tgf_wasm_parity ($GRAMMAR)"
	exit 0
else
	echo "FAIL: tgf_wasm_parity ($GRAMMAR) - outputs differ" >&2
	echo "--- native ($NATIVE_OUT) -----------------------------------" >&2
	cat "$NATIVE_OUT" >&2
	echo "--- wasm ($WASM_OUT) ---------------------------------------" >&2
	cat "$WASM_OUT" >&2
	echo "--- diff ---------------------------------------------------" >&2
	diff "$NATIVE_NORM" "$WASM_NORM" >&2 || true
	exit 1
fi
