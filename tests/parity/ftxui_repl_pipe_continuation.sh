#!/bin/sh
# ftxui_repl_pipe_continuation.sh - In pipe mode the FTXUI REPL shows the
# continuation prompt before each line that continues an incomplete input,
# and the prompt before each new input.
#
# Usage:
#   ftxui_repl_pipe_continuation.sh [<emulator>] <repl_ftxui_async_fixture>
#
# A cross build passes its emulator (wine) before the fixture.
#
# Returns 0 if the output has the expected prompts, non-zero otherwise.

set -e

if [ $# -lt 1 ]; then
	echo "Usage: ftxui_repl_pipe_continuation.sh [<emulator>] <repl_ftxui_async_fixture>" >&2
	exit 2
fi
STATUS=0

# the fixture asks for more lines after "incomplete"
out="$(printf 'incomplete\nmore\nquit\n' | "$@" | tr -d '\r')" || true
for line in "test> incomplete" "...> more" "test> quit"; do
	if ! printf '%s\n' "$out" | grep -qxF "$line"; then
		echo "FAIL: no line '$line'" >&2
		STATUS=1
	fi
done
if printf '%s\n' "$out" | grep -qF "...> quit"; then
	echo "FAIL: a new input after a complete one shows the continuation prompt" >&2
	STATUS=1
fi
printf '%s\n' "$out" >&2

if [ "$STATUS" -eq 0 ]; then
	echo "PASS: ftxui_repl_pipe_continuation"
fi
exit "$STATUS"
