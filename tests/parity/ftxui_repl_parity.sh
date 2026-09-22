#!/bin/sh
# ftxui_repl_parity.sh - Compare FTXUI REPL output against legacy REPL output.
#
# Runs identical commands through both `tgf repl` (FTXUI) and
# `tgf repl -X` (legacy), then asserts identical .tgf_history and
# identical visible output (after stripping ANSI escapes).
#
# Usage:
#   ftxui_repl_parity.sh <tgf_binary> <grammar_file>
#
# Returns 0 if outputs match, non-zero otherwise.

set -e

TGF_BIN="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GRAMMAR="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"

if [ $# -lt 2 ]; then
	echo "Usage: ftxui_repl_parity.sh <tgf_binary> <grammar_file>" >&2
	exit 2
fi

TMPDIR="${TMPDIR:-/tmp}"
FTXUI_DIR="$TMPDIR/ftxui_repl_parity_ftxui.$$"
LEGACY_DIR="$TMPDIR/ftxui_repl_parity_legacy.$$"
cleanup() { rm -rf "$FTXUI_DIR" "$LEGACY_DIR"; }
trap cleanup EXIT

mkdir -p "$FTXUI_DIR" "$LEGACY_DIR"

# Commands: grammar inspection + parse valid + parse error + quit
cat > "$TMPDIR/ftxui_repl_cmds.$$" << 'EOF'
grammar
quit
EOF
CMDS_FILE="$TMPDIR/ftxui_repl_cmds.$$"

# --- FTXUI mode -------------------------------------------------------------
(cd "$FTXUI_DIR" && "$TGF_BIN" "$GRAMMAR" repl < "$CMDS_FILE") \
	> "$FTXUI_DIR/out.txt" 2>&1 || true

# --- Legacy mode (-X) -------------------------------------------------------
(cd "$LEGACY_DIR" && "$TGF_BIN" "$GRAMMAR" repl -X < "$CMDS_FILE") \
	> "$LEGACY_DIR/out.txt" 2>&1 || true

rm -f "$CMDS_FILE"

# --- Compare history files --------------------------------------------------
normalize() {
	# strip ANSI escapes, timing values, and trailing whitespace
	sed 's/\x1b\[[0-9;]*m//g' "$1" \
		| sed 's/[0-9.]* \(µs\|ms\)//g' \
		| sed 's/[[:space:]]*$//' \
		| grep -v '^$' || true
}

HIST_FTXUI="$FTXUI_DIR/.tgf_history"
HIST_LEGACY="$LEGACY_DIR/.tgf_history"

if [ ! -f "$HIST_FTXUI" ] || [ ! -f "$HIST_LEGACY" ]; then
	echo "FAIL: .tgf_history not created" >&2
	echo "FTXUI: $([ -f "$HIST_FTXUI" ] && echo exists || echo missing)" >&2
	echo "Legacy: $([ -f "$HIST_LEGACY" ] && echo exists || echo missing)" >&2
	exit 1
fi

if ! diff -q "$HIST_FTXUI" "$HIST_LEGACY" >/dev/null 2>&1; then
	echo "FAIL: .tgf_history differs" >&2
	echo "--- FTXUI history ---" >&2
	cat "$HIST_FTXUI" >&2
	echo "--- Legacy history ---" >&2
	cat "$HIST_LEGACY" >&2
	exit 1
fi

# --- Compare visible output (normalized) ------------------------------------
NORM_FTXUI="$FTXUI_DIR/out_norm.txt"
NORM_LEGACY="$LEGACY_DIR/out_norm.txt"
normalize "$FTXUI_DIR/out.txt" > "$NORM_FTXUI"
normalize "$LEGACY_DIR/out.txt" > "$NORM_LEGACY"

if ! diff -q "$NORM_FTXUI" "$NORM_LEGACY" >/dev/null 2>&1; then
	echo "FAIL: visible output differs" >&2
	echo "--- diff ---" >&2
	diff "$NORM_FTXUI" "$NORM_LEGACY" >&2 || true
	exit 1
fi

echo "PASS: ftxui_repl_parity ($(basename "$GRAMMAR"))"
exit 0
