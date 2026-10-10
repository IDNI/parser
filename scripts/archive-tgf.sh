#!/bin/bash
# Archive current tgf binary to build/archive, overwriting the last one. The
# copy keeps the name of the binary, so tgf.exe stays runnable on Windows.
set -euo pipefail
TOP="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${1:-$TOP/build/release/tgf}"
if [ ! -f "$SRC" ]; then
	echo "No tgf binary at $SRC" >&2; exit 1
fi
DIR="$TOP/build/archive"
mkdir -p "$DIR"
cp "$SRC" "$DIR/$(basename "$SRC")"
echo "Archived to $DIR/$(basename "$SRC")"
