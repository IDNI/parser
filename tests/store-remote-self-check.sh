#!/bin/bash
# Hermetic self-check for the remote store bridge: the <dep>-<id> tag naming
# and the "remote miss builds" rule. A fake oras on PATH stands in for the
# registry, so nothing reaches the network and ~/.tau is never touched.
#
# The check sources devrc, points TAU_SHARED_PREFIX at a random scratch
# directory and drives dep_ensure with a file-based fake producer. One store
# entry is built locally, exported with the real store-transport.sh and handed
# to the fake oras as the remote payload.

set -u
# The case controls TAU_STORE_REMOTE itself; an ambient value must not leak in.
unset TAU_STORE_REMOTE

HERE="$(cd "$(dirname "$0")" && pwd)"
PARSER_ROOT="$(cd "$HERE/.." && pwd)"
DEV_ROOT="$PARSER_ROOT"
# shellcheck source=/dev/null
source "$PARSER_ROOT/scripts/devrc"

fail() {
	echo "store-remote self-check: $*" >&2
	exit 1
}

# dep_ensure's stderr and the fake oras log hold the reason a remote pull or
# import gave up; print both so a failure names it and not only the symptom.
fail_remote() {
	local log
	for log in "$SCRATCH/remote.err" "$ORAS_LOG"; do
		echo "store-remote self-check: --- ${log} ---" >&2
		[ -f "$log" ] && cat "$log" >&2
	done
	fail "$*"
}

# A TMPDIR ending in / leaves // on macOS, and Git Bash reports /tmp and short
# names where CMake sees C:/...; either one breaks the string compare, so
# normalize the path before translating it.
to_cmake_path() {
	local path
	path="$(cd "$1" && pwd)"
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -m -l "$path"
	else
		printf '%s' "$path"
	fi
}

# SCRATCH stays the POSIX form: PATH and the shell's own paths need it, and a
# converted C:/... path holds a colon that splits the :-separated PATH. Only a
# path compared against CMake's output takes the converted form.
SCRATCH="$(mktemp -d "${TMPDIR:-/tmp}/tau-store-remote.XXXXXX")" || exit 1
SCRATCH_CMAKE="$(to_cmake_path "$SCRATCH")"
cleanup() { rm -rf "$SCRATCH"; }
trap cleanup EXIT
export TMPDIR="$SCRATCH/tmp"
mkdir -p "$TMPDIR" "$SCRATCH/bin"

# ── the fake oras ───────────────────────────────────────────────────────
export ORAS_LOG="$SCRATCH/oras.log"
: > "$ORAS_LOG"
cat > "$SCRATCH/bin/oras" <<'SH'
#!/bin/sh
printf '%s\n' "$*" >> "$ORAS_LOG"
sub="${1:-}"
case "$sub" in
	manifest)
		exit 1
		;;
	pull)
		shift
		dir=""
		while [ "$#" -gt 0 ]; do
			case "$1" in
				-o) dir="$2"; shift 2 ;;
				*) shift ;;
			esac
		done
		[ -n "$dir" ] || exit 1
		if [ -n "${FAKE_ORAS_PULL_SRC:-}" ] && [ -f "$FAKE_ORAS_PULL_SRC" ]; then
			mkdir -p "$dir"
			cp "$FAKE_ORAS_PULL_SRC" "$dir/store-entry.tar"
			exit 0
		fi
		exit 1
		;;
	push)
		exit 0
		;;
esac
exit 1
SH
chmod +x "$SCRATCH/bin/oras"
export PATH="$SCRATCH/bin:$PATH"

# ── a producer and its counter ──────────────────────────────────────────
COUNTER="$SCRATCH/producer.calls"
: > "$COUNTER"
calls() { wc -l < "$COUNTER" | tr -d ' '; }
fake_producer() {
	printf 'x\n' >> "$COUNTER"
	local prefix="$1"
	shift
	mkdir -p "$prefix"
	printf 'hello\n' > "$prefix/file.txt"
}

BLOCK="$(printf 'dep=demo\nsource=remote')"
ID="$(printf '%s\n' "$BLOCK" > "$SCRATCH/block"
	cmake -P "$PARSER_ROOT/cmake/tau-manifest.cmake" input-id-file "$SCRATCH/block")"

# ── 1. tag naming ───────────────────────────────────────────────────────
tag="$("$PARSER_ROOT/scripts/store-remote.sh" tag demo "$ID")" \
	|| fail "tag naming failed"
[ "$tag" = "demo-${ID}" ] || fail "tag is '${tag}', expected 'demo-${ID}'"

long_dep="$(printf 'd%.0s' $(seq 1 100))"
long_tag="$("$PARSER_ROOT/scripts/store-remote.sh" tag "$long_dep" "$ID")" \
	|| fail "long tag naming failed"
[ "${#long_tag}" -le 128 ] || fail "long tag is ${#long_tag} chars, over 128"
[ "$long_tag" != "${long_dep}-${ID}" ] || fail "an over-long tag was not shortened"
expr "x${long_tag}" : 'x[A-Za-z0-9_][A-Za-z0-9._-]*$' > /dev/null \
	|| fail "the shortened tag '${long_tag}' is not a valid OCI tag"

# ── 2. a remote read on a miss: the producer is not called ──────────────
export TAU_SHARED_PREFIX="$SCRATCH_CMAKE/source-store"
source_out=""
dep_ensure source_out producer demo "$BLOCK" fake_producer \
	|| fail "the source producer failed"
[ "$(calls)" -eq 1 ] || fail "the source producer ran $(calls) times, expected 1"
"$PARSER_ROOT/scripts/store-transport.sh" export "$TAU_SHARED_PREFIX" \
	"$SCRATCH/remote.tar" "demo/${ID}" >&2 \
	|| fail "the payload export failed"
[ -f "$SCRATCH/remote.tar" ] || fail "the payload export wrote no tar"

export TAU_STORE_REMOTE="ghcr.io/self-check/tau-store"
export FAKE_ORAS_PULL_SRC="$SCRATCH/remote.tar"
export TAU_SHARED_PREFIX="$SCRATCH_CMAKE/remote-store"
before="$(calls)"
remote_out=""
if ! dep_ensure remote_out producer demo "$BLOCK" fake_producer \
		2>"$SCRATCH/remote.err"; then
	fail_remote "a remote-hit read failed"
fi
[ "$(calls)" -eq "$before" ] \
	|| fail_remote "a remote hit called the producer ($(calls) calls)"
[ -f "$remote_out/file.txt" ] || fail "a remote hit exposed no package"
case "$remote_out" in
	"$TAU_SHARED_PREFIX"/store/demo/*/prefix) ;;
	*) fail "the remote prefix is not store-based: $remote_out (shared prefix: $TAU_SHARED_PREFIX)" ;;
esac
grep -q "^pull " "$ORAS_LOG" || fail_remote "a remote hit never called oras pull"

# ── 3. a remote miss builds: the producer is called ─────────────────────
unset FAKE_ORAS_PULL_SRC
export TAU_SHARED_PREFIX="$SCRATCH_CMAKE/miss-store"
before="$(calls)"
miss_out=""
if ! dep_ensure miss_out producer demo "$BLOCK" fake_producer \
		2>"$SCRATCH/miss.err"; then
	fail "a remote miss did not build"
fi
[ "$(calls)" -eq $((before + 1)) ] \
	|| fail "a remote miss did not call the producer"
[ -f "$miss_out/file.txt" ] || fail "a remote miss exposed no package"
grep -q "is not on ${TAU_STORE_REMOTE}; building" "$SCRATCH/miss.err" \
	|| fail "a remote miss was not reported"

# ── 4. no remote: oras is never invoked and the producer builds ─────────
unset TAU_STORE_REMOTE
export TAU_SHARED_PREFIX="$SCRATCH_CMAKE/local-store"
: > "$ORAS_LOG"
before="$(calls)"
local_out=""
if ! dep_ensure local_out producer demo "$BLOCK" fake_producer \
		2>"$SCRATCH/local.err"; then
	fail "a local build failed"
fi
[ "$(calls)" -eq $((before + 1)) ] || fail "a local build did not call the producer"
[ -f "$local_out/file.txt" ] || fail "a local build exposed no package"
[ ! -s "$ORAS_LOG" ] || fail "oras ran with TAU_STORE_REMOTE unset"

echo "store-remote self-check passed" >&2
exit 0
