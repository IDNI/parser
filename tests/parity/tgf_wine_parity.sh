#!/bin/bash
# Parity test: compare native tgf and wine tgf.exe output.
# Usage: tests/tgf_wine_parity.sh <native_tgf> <mingw_tgf.exe> <grammar_file>
set -e

NATIVE="$1"
WINE_EXE="$2"
GRAMMAR="$3"

# Silence wine's fixme/winediag startup chatter so it doesn't pollute the diff.
export WINEDEBUG=-all

echo "=== tgf wine parity: $GRAMMAR ==="

# Test grammar subcommand
NATIVE_OUT=$(mktemp)
WINE_OUT=$(mktemp)
trap "rm -f $NATIVE_OUT $WINE_OUT" EXIT

# Strip CR so the Windows build's CRLF line endings don't break the diff.
"$NATIVE" "$GRAMMAR" grammar 2>&1 | tr -d '\r' > "$NATIVE_OUT"
wine "$WINE_EXE" "$GRAMMAR" grammar 2>&1 | tr -d '\r' > "$WINE_OUT"

if diff -q "$NATIVE_OUT" "$WINE_OUT" >/dev/null; then
    echo "grammar: OK"
else
    echo "grammar: MISMATCH"
    diff "$NATIVE_OUT" "$WINE_OUT" | head -20
    exit 1
fi

# Test parse subcommand with a known input. Pass it via -e (argv) rather than
# stdin: piping into a wine process truncates the input, which is a wine
# plumbing quirk unrelated to tgf parse parity.
"$NATIVE" "$GRAMMAR" parse -e '{"a":1}' 2>&1 | tr -d '\r' > "$NATIVE_OUT"
wine "$WINE_EXE" "$GRAMMAR" parse -e '{"a":1}' 2>&1 | tr -d '\r' > "$WINE_OUT"

if diff -q "$NATIVE_OUT" "$WINE_OUT" >/dev/null; then
    echo "parse:   OK"
else
    echo "parse:   MISMATCH"
    exit 1
fi

echo "parity: PASS"
