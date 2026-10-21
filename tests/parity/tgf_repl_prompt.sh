#!/bin/sh
# tgf_repl_prompt.sh - The prompt of the plain tgf REPL without a grammar
# names the missing grammar.
#
# Usage:
#   tgf_repl_prompt.sh [<emulator>] <tgf>
#
# A cross build passes its emulator (wine) before tgf.
#
# Returns 0 if the prompt is as expected, non-zero otherwise.

set -e

if [ $# -lt 1 ]; then
	echo "Usage: tgf_repl_prompt.sh [<emulator>] <tgf>" >&2
	exit 2
fi

home="$(mktemp -d)"
trap 'rm -rf "$home"' EXIT
out="$(cd "$home" && printf 'quit\n' \
	| HOME="$home" "$@" repl -X -c false | tr -d '\r')"
case "$out" in
*"[ (no grammar loaded) ] tgf> quit"*) ;;
*)
	echo "FAIL: the prompt does not name the missing grammar" >&2
	printf '%s\n' "$out" >&2
	exit 1
	;;
esac
echo "PASS: tgf_repl_prompt"
