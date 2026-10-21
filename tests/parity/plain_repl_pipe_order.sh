#!/bin/sh
# plain_repl_pipe_order.sh - The plain REPL with stdin from a pipe and stdout
# to a file writes each answer before the next prompt.
#
# Usage:
#   plain_repl_pipe_order.sh [<emulator>] <repl_ftxui_async_fixture>
#
# A cross build passes its emulator (wine) before the fixture.
#
# Returns 0 if the output has the expected order, non-zero otherwise.

set -e

if [ $# -lt 1 ]; then
	echo "Usage: plain_repl_pipe_order.sh [<emulator>] <repl_ftxui_async_fixture>" >&2
	exit 2
fi

# a file on stdout gives std::cout a full buffer
out_file="$(mktemp)"
trap 'rm -f "$out_file"' EXIT
printf 'a\nb\nquit\n' | "$@" --plain > "$out_file" || true
out="$(tr -d '\r' < "$out_file")"
expected="$(printf 'test> a\nevaluated: a\ntest> b\nevaluated: b\ntest> quit\nQuit.')"
if [ "$out" != "$expected" ]; then
	echo "FAIL: the answers and the prompts are out of order" >&2
	printf '%s\n' "$out" >&2
	exit 1
fi
echo "PASS: plain_repl_pipe_order"
