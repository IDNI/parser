#!/bin/sh
# tgf_wasm_cli_paths.sh - run the real tgf.js program (not require() of the
# module) from host paths that live under /tmp.
#
# Usage:
#   tgf_wasm_cli_paths.sh <node> <tgf.js> <grammar_fixture>
#
# Use /tmp directly so TMPDIR cannot move these cases outside the host mount.
set -eu

NODE=$1
TGF_JS=$2
GRAMMAR=$3

[ -x "$NODE" ] || { echo "tgf_wasm_cli_paths: node not found: $NODE" >&2; exit 1; }
[ -f "$TGF_JS" ] || { echo "tgf_wasm_cli_paths: tgf.js not found: $TGF_JS" >&2; exit 1; }
[ -f "$GRAMMAR" ] || { echo "tgf_wasm_cli_paths: grammar not found: $GRAMMAR" >&2; exit 1; }

WORK=$(mktemp -d /tmp/tgf-cli-paths.XXXXXX) || exit 1
trap 'rm -rf "$WORK"' EXIT INT TERM

cp "$GRAMMAR" "$WORK/tiny.tgf"
mkdir -p "$WORK/out"

fail() { echo "tgf_wasm_cli_paths: $*" >&2; exit 1; }

# A loaded grammar prints its productions, so an empty result or a help text
# cannot pass.
expect_grammar() {
	label=$1 out=$2
	printf '%s\n' "$out" | grep -q 'start(' \
		|| fail "$label: grammar did not load; output: $out"
}

# 1. absolute host grammar inside a temporary directory under /tmp
out=$("$NODE" "$TGF_JS" "$WORK/tiny.tgf" grammar 2>&1) \
	|| fail "absolute /tmp grammar exited non-zero: $out"
expect_grammar "absolute /tmp grammar" "$out"

# 2. working directory inside a temporary directory under /tmp, relative grammar
out=$(cd "$WORK" && "$NODE" "$TGF_JS" tiny.tgf grammar 2>&1) \
	|| fail "relative grammar from a /tmp cwd exited non-zero: $out"
expect_grammar "relative grammar from a /tmp cwd" "$out"

# 3. ordinary existing absolute path outside /tmp keeps working
out=$("$NODE" "$TGF_JS" "$GRAMMAR" grammar 2>&1) \
	|| fail "ordinary absolute grammar exited non-zero: $out"
expect_grammar "ordinary absolute grammar" "$out"

# 4. gen writes its output into a directory under /tmp
out=$("$NODE" "$TGF_JS" "$WORK/tiny.tgf" gen --output-dir "$WORK/out" 2>&1) \
	|| fail "gen --output-dir under /tmp exited non-zero: $out"
[ -s "$WORK/out/tiny_parser.generated.h" ] \
	|| fail "gen wrote no tiny_parser.generated.h under /tmp"

echo "tgf_wasm_cli_paths: OK" >&2
exit 0
