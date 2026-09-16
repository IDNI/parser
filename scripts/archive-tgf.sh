#!/bin/bash
# Archive current tgf binary to build/archive/tgf, overwriting the last one.
set -euo pipefail
TOP="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${1:-$TOP/build/release/tgf}"
if [ ! -f "$SRC" ]; then
	echo "No tgf binary at $SRC" >&2; exit 1
fi
DIR="$TOP/build/archive"
mkdir -p "$DIR"
cp "$SRC" "$DIR/tgf"
echo "Archived to $DIR/tgf"
