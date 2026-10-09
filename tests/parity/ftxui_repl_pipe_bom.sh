#!/bin/sh
# ftxui_repl_pipe_bom.sh - The FTXUI REPL drops a leading UTF-8 byte order
# mark from piped input, on a one-line and on a two-line first command.
#
# Usage:
#   ftxui_repl_pipe_bom.sh [<emulator>] <repl_ftxui_async_fixture>
#
# A cross build passes its emulator (wine) before the fixture.
#
# Returns 0 if both cases pass, non-zero otherwise.

set -e

if [ $# -lt 1 ]; then
	echo "Usage: ftxui_repl_pipe_bom.sh [<emulator>] <repl_ftxui_async_fixture>" >&2
	exit 2
fi
BOM="$(printf '\357\273\277')"
STATUS=0

# check <name> <input> <expected evaluated text> <command>...
check() {
	name="$1" input="$2" expected="$3"
	shift 3
	out="$(printf '%s' "$input" | "$@" | tr -d '\r')" || true
	if printf '%s' "$out" | grep -q "$BOM"; then
		echo "FAIL: $name: the output keeps the byte order mark" >&2
		STATUS=1
	fi
	if ! printf '%s\n' "$out" | grep -qx "evaluated: $expected"; then
		echo "FAIL: $name: no line 'evaluated: $expected'" >&2
		STATUS=1
	fi
	printf '%s\n' "$out" >&2
}

# the hex mode of the fixture sees the mark that the checks below send
printf '%shello\n' "$BOM" | "$@" --hex | grep -q '^ef bb bf ' || { echo "FAIL: the hex mode shows no byte order mark" >&2; STATUS=1; }

check "one line" "${BOM}hello
quit
" "hello" "$@"

# the fixture asks for more lines after "incomplete", and then evaluates
# the whole buffer, the first line included
check "two lines" "${BOM}incomplete
more
quit
" "incomplete" "$@"

if [ "$STATUS" -eq 0 ]; then
	echo "PASS: ftxui_repl_pipe_bom"
fi
exit "$STATUS"
