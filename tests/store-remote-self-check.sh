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

SCRATCH="$(mktemp -d "${TMPDIR:-/tmp}/tau-store-remote.XXXXXX")" || exit 1
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
export TAU_SHARED_PREFIX="$SCRATCH/source-store"
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
export TAU_SHARED_PREFIX="$SCRATCH/remote-store"
before="$(calls)"
remote_out=""
if ! dep_ensure remote_out producer demo "$BLOCK" fake_producer \
		2>"$SCRATCH/remote.err"; then
	fail "a remote-hit read failed"
fi
[ "$(calls)" -eq "$before" ] \
	|| fail "a remote hit called the producer ($(calls) calls)"
[ -f "$remote_out/file.txt" ] || fail "a remote hit exposed no package"
case "$remote_out" in
	"$TAU_SHARED_PREFIX"/store/demo/*/prefix) ;;
	*) fail "the remote prefix is not store-based: $remote_out" ;;
esac
grep -q "^pull " "$ORAS_LOG" || fail "a remote hit never called oras pull"

# ── 3. a remote miss builds: the producer is called ─────────────────────
unset FAKE_ORAS_PULL_SRC
export TAU_SHARED_PREFIX="$SCRATCH/miss-store"
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
export TAU_SHARED_PREFIX="$SCRATCH/local-store"
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
