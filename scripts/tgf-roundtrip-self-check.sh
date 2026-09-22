#!/bin/bash
# Self-check: regenerate every committed parser from its own grammar with
# the just-built tgf tool and fail if the output differs from what's committed.
set -euo pipefail
TOP="$(cd "$(dirname "$0")/.." && pwd)"
TGF="${1:-$TOP/build/release/tgf}"

# grammar|output_dir|name|utf8(0/1)
ENTRIES=(
	"src/format/tgf/tgf.tgf|src/format/tgf|tgf_parser|0"
	"src/format/json/json.tgf|src/format/json|json_parser|1"
	"src/tgf/tgf_repl.tgf|src/tgf|tgf_repl_parser|1"
	"src/format/treemr/treemr.tgf|src/format/treemr|treemr_parser|1"
	"src/format/tgf.test/tgf.test.tgf|src/format/tgf.test|tgf_test_parser|1"
)

TMP="$(mktemp -d)"
trap "rm -rf '$TMP'" EXIT

checked=0
skipped=0

for entry in "${ENTRIES[@]}"; do
	IFS='|' read -r grammar outdir name utf8 <<< "$entry"
	hdr_committed="$TOP/$outdir/$name.generated.h"
	cpp_committed="$TOP/$outdir/$name.generated.cpp"

	if [ ! -f "$TOP/$grammar" ] || [ ! -f "$hdr_committed" ] || [ ! -f "$cpp_committed" ]; then
		echo "roundtrip: skip $name (grammar or committed output missing)"
		skipped=$((skipped + 1))
		continue
	fi

	gendir="$TMP/$name"
	mkdir -p "$gendir"
	# cd to TOP so the committed relative path lands in the generated
	# file's header comment, matching the committed output exactly
	args=("$grammar" gen --name "$name" --output-dir "$gendir" --header-only false)
	[ "$utf8" = "1" ] && args+=(--utf8)

	if ! ( cd "$TOP" && "$TGF" "${args[@]}" ) >"$TMP/$name.log" 2>&1; then
		echo "roundtrip: FAIL $name: generator exited non-zero"
		cat "$TMP/$name.log"
		exit 1
	fi

	for ext in h cpp; do
		gen_file="$gendir/$name.generated.$ext"
		committed_file="$TOP/$outdir/$name.generated.$ext"
		if ! diff -q "$committed_file" "$gen_file" >/dev/null 2>&1; then
			echo "roundtrip: FAIL $name.generated.$ext differs from committed output"
			diff -u "$committed_file" "$gen_file" | head -40
			exit 1
		fi
	done

	checked=$((checked + 1))
done

echo "roundtrip: checked $checked, skipped $skipped"
