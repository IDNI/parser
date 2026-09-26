#!/bin/bash
# Hermetic self-check for dep_ensure in external/parser/scripts/devrc.
#
# Sources devrc, points TAU_SHARED_PREFIX at a random build-local scratch
# directory, and drives dep_ensure with a file-based fake producer. The real
# ~/.tau is never read or written.

set -u
# The remote read is off for this check; the fake producer is the only source.
unset TAU_STORE_REMOTE

HERE="$(cd "$(dirname "$0")" && pwd)"
PARSER_ROOT="$(cd "$HERE/.." && pwd)"
DEV_ROOT="$PARSER_ROOT"
# shellcheck source=/dev/null
source "$PARSER_ROOT/scripts/devrc"

fail() {
	echo "tau-dep-ensure self-check: $*" >&2
	exit 1
}

SCRATCH="$(mktemp -d "${TMPDIR:-/tmp}/tau-dep-ensure.XXXXXX")" || exit 1
cleanup() { rm -rf "$SCRATCH"; }
trap cleanup EXIT
export TAU_SHARED_PREFIX="$SCRATCH/shared"
export TMPDIR="$SCRATCH/tmp"
mkdir -p "$TMPDIR"

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

fake_producer_fail() {
	printf 'x\n' >> "$COUNTER"
	local prefix="$1"
	shift
	mkdir -p "$prefix"
	printf 'partial\n' > "$prefix/partial.txt"
	return 7
}

fake_producer_exit() {
	printf 'x\n' >> "$COUNTER"
	local prefix="$1"
	shift
	mkdir -p "$prefix"
	printf 'partial\n' > "$prefix/partial.txt"
	exit 9
}

fake_producer_path() {
	printf 'x\n' >> "$COUNTER"
	local prefix="$1"
	shift
	mkdir -p "$prefix"
	printf '%s/secret\n' "$HOME" > "$prefix/leak.txt"
}

fake_producer_foreign_path() {
	printf 'x\n' >> "$COUNTER"
	local prefix="$1"
	shift
	mkdir -p "$prefix"
	printf '/home/%s\n' 'runner/foreign-secret' > "$prefix/foreign.txt"
}

bridge_id() {
	local file out
	file="$(mktemp "${TMPDIR:-/tmp}/tau-dep-id.XXXXXX")" || return 1
	printf '%s\n' "$1" > "$file"
	out="$(cmake -P "$PARSER_ROOT/cmake/tau-manifest.cmake" input-id-file "$file" 2>/dev/null)"
	rm -f "$file"
	printf '%s' "$out"
}

BLOCK="$(printf 'dep=demo\nsource=abc')"
BLOCK_OTHER="$(printf 'dep=demo\nsource=xyz')"
BLOCK_SEMI="$(printf 'dep=demo\nnote=a;b')"
FAIL_BLOCK="$(printf 'dep=faildep\nsource=abc')"
EXIT_BLOCK="$(printf 'dep=exitdep\nsource=abc')"
CONC_BLOCK="$(printf 'dep=concurrent\nsource=abc')"
PATH_BLOCK="$(printf 'source=abc')"
FOREIGN_BLOCK="$(printf 'source=foreign')"
STALE_BLOCK="$(printf 'dep=staledep\nsource=abc')"

# 1. a cold producer builds once and exposes the package
out=""
dep_ensure out producer demo "$BLOCK" fake_producer || fail "cold producer failed"
[ "$(calls)" -eq 1 ] || fail "cold producer calls=$(calls), expected 1"
[ -f "$out/file.txt" ] || fail "cold producer did not expose the package"
case "$out" in
	"$TAU_SHARED_PREFIX"/store/demo/*/prefix) ;;
	*) fail "output prefix is not store-based: $out" ;;
esac

# 2. a warm hit never calls the producer
out_warm=""
dep_ensure out_warm producer demo "$BLOCK" fake_producer || fail "warm hit failed"
[ "$(calls)" -eq 1 ] || fail "warm hit called the producer (calls=$(calls))"
[ "$out_warm" = "$out" ] || fail "warm hit returned a different prefix"

# 3. a consumer hit never calls the producer
out_consumer=""
dep_ensure out_consumer consumer demo "$BLOCK" fake_producer || fail "consumer hit failed"
[ "$(calls)" -eq 1 ] || fail "consumer hit called the producer (calls=$(calls))"
[ "$out_consumer" = "$out" ] || fail "consumer hit returned a different prefix"

# 4. a changed field creates a new id and calls the producer again
out_changed=""
dep_ensure out_changed producer demo "$BLOCK_OTHER" fake_producer || fail "changed field failed"
[ "$(calls)" -eq 2 ] || fail "changed field did not call the producer (calls=$(calls))"
[ "$out_changed" != "$out" ] || fail "changed field reused the old entry"

# 5. a consumer miss fails and does not call the producer
before="$(calls)"
dep_ensure unused_consumer consumer missing "$BLOCK" fake_producer \
	&& fail "a consumer miss succeeded"
[ "$(calls)" -eq "$before" ] || fail "a consumer miss called the producer"

# 6. a semicolon field value survives and gives a distinct id
id_plain="$(bridge_id "$BLOCK")"
id_semi="$(bridge_id "$BLOCK_SEMI")"
[ "$id_plain" != "$id_semi" ] || fail "a semicolon value did not change the id"
out_semi=""
dep_ensure out_semi producer demo "$BLOCK_SEMI" fake_producer || fail "semicolon block failed"
[ "$(calls)" -eq 3 ] || fail "semicolon block did not call the producer"
[ -f "$out_semi/file.txt" ] || fail "semicolon block did not expose the package"
before="$(calls)"
out_semi_warm=""
dep_ensure out_semi_warm producer demo "$BLOCK_SEMI" fake_producer \
	|| fail "semicolon warm hit failed"
[ "$(calls)" -eq "$before" ] || fail "semicolon warm hit called the producer"
[ "$out_semi_warm" = "$out_semi" ] || fail "semicolon warm hit returned a different prefix"

# 7. a producer failure leaves no final entry and removes its staging
before="$(calls)"
dep_ensure unused_fail producer faildep "$FAIL_BLOCK" fake_producer_fail \
	&& fail "a producer failure succeeded"
[ "$(calls)" -eq $((before + 1)) ] || fail "the failing producer was not called"
fail_id="$(bridge_id "$FAIL_BLOCK")"
if ls -d "$TAU_SHARED_PREFIX/store/faildep/"*.staging-* >/dev/null 2>&1; then
	fail "a failed producer left a staging entry"
fi
if ls -d "$TAU_SHARED_PREFIX/store/faildep/"*.claim >/dev/null 2>&1; then
	fail "a failed producer left a claim"
fi
lookup="$(cmake -P "$PARSER_ROOT/cmake/tau-store.cmake" lookup \
	"$TAU_SHARED_PREFIX" faildep "$fail_id" 2>/dev/null)" \
	|| fail "lookup after a failed producer errored"
[ "$lookup" = "miss" ] || fail "a failed producer left a final entry"

# 7b. a producer that calls exit is cleaned up by the EXIT trap
before="$(calls)"
dep_ensure unused_exit producer exitdep "$EXIT_BLOCK" fake_producer_exit \
	&& fail "a producer that exited was accepted"
[ "$(calls)" -eq $((before + 1)) ] || fail "the exiting producer was not called"
exit_id="$(bridge_id "$EXIT_BLOCK")"
if ls -d "$TAU_SHARED_PREFIX/store/exitdep/"*.staging-* >/dev/null 2>&1; then
	fail "an exiting producer left a staging entry"
fi
if ls -d "$TAU_SHARED_PREFIX/store/exitdep/"*.claim >/dev/null 2>&1; then
	fail "an exiting producer left a claim"
fi
lookup="$(cmake -P "$PARSER_ROOT/cmake/tau-store.cmake" lookup \
	"$TAU_SHARED_PREFIX" exitdep "$exit_id" 2>/dev/null)" \
	|| fail "lookup after an exiting producer errored"
[ "$lookup" = "miss" ] || fail "an exiting producer left a final entry"
if ls "$TMPDIR/"tau-dep-block.* >/dev/null 2>&1; then
	fail "an exiting producer left a block temp file"
fi

# 7c. a failing or empty shared-prefix resolver is rejected before any path
before="$(calls)"
( dep_shared_prefix() { return 1; }
  dep_ensure _r producer demo "$BLOCK" fake_producer \
	2>"$SCRATCH/resolver.err" && exit 1
  exit 0 ) || fail "a failing resolver was accepted"
grep -q "TAU_SHARED_PREFIX" "$SCRATCH/resolver.err" \
	|| fail "a failing resolver was not reported"
( dep_shared_prefix() { printf '%s' ""; }
  dep_ensure _r producer demo "$BLOCK" fake_producer \
	2>"$SCRATCH/resolver2.err" && exit 1
  exit 0 ) || fail "an empty resolver was accepted"
grep -q "TAU_SHARED_PREFIX" "$SCRATCH/resolver2.err" \
	|| fail "an empty resolver was not reported"
if grep -q "/store/" "$SCRATCH/resolver2.err"; then
	fail "a root-level store path was attempted"
fi
[ "$(calls)" -eq "$before" ] || fail "a rejected resolver ran a producer"

# 8. common output variable names are set on the caller
dep_ensure prefix producer demo "$BLOCK" fake_producer || fail "out-var prefix failed"
[ "$prefix" = "$out" ] || fail "out-var prefix was not set to the store prefix"
dep_ensure id producer demo "$BLOCK" fake_producer || fail "out-var id failed"
[ "$id" = "$out" ] || fail "out-var id was not set"
dep_ensure status producer demo "$BLOCK" fake_producer || fail "out-var status failed"
[ "$status" = "$out" ] || fail "out-var status was not set"

# 9. invalid arguments fail
dep_ensure out bogus demo "$BLOCK" fake_producer && fail "an invalid mode was accepted"
dep_ensure 1bad producer demo "$BLOCK" fake_producer && fail "an invalid out-var was accepted"
dep_ensure _tau_dep_ensure_reserved producer demo "$BLOCK" fake_producer \
	&& fail "a reserved out-var was accepted"
dep_ensure out producer && fail "too few arguments were accepted"

# 10. a stale claim fails closed, is not removed, and runs no producer
stale_id="$(bridge_id "$STALE_BLOCK")"
mkdir -p "$TAU_SHARED_PREFIX/store/staledep/$stale_id.claim"
before="$(calls)"
if TAU_DEP_CLAIM_TIMEOUT=1 TAU_DEP_CLAIM_POLL=1 \
		dep_ensure unused_stale producer staledep "$STALE_BLOCK" fake_producer \
		2>"$SCRATCH/stale.err"; then
	fail "a stale claim did not fail closed"
fi
[ -d "$TAU_SHARED_PREFIX/store/staledep/$stale_id.claim" ] \
	|| fail "a stale claim was removed"
[ "$(calls)" -eq "$before" ] || fail "a stale claim ran a producer"
grep -q "timed out" "$SCRATCH/stale.err" || fail "the stale claim error was not reported"

# 10b. a claim owned by a dead pid on this host is reclaimed: the owner file and
# the claim directory go away and the producer runs.
sleep 0 & dead_pid=$!
wait "$dead_pid" 2>/dev/null || true
kill -0 "$dead_pid" 2>/dev/null \
	&& fail "the dead-owner pid ${dead_pid} is still alive"
RECLAIM_BLOCK="$(printf 'dep=reclaimdep\nsource=abc')"
reclaim_id="$(bridge_id "$RECLAIM_BLOCK")"
mkdir -p "$TAU_SHARED_PREFIX/store/reclaimdep/$reclaim_id.claim"
printf '%s %s\n' "$(uname -n)" "$dead_pid" \
	> "$TAU_SHARED_PREFIX/store/reclaimdep/$reclaim_id.claim/owner"
before="$(calls)"
if ! TAU_DEP_CLAIM_TIMEOUT=5 TAU_DEP_CLAIM_POLL=1 \
		dep_ensure out_reclaim producer reclaimdep "$RECLAIM_BLOCK" fake_producer \
		2>"$SCRATCH/reclaim.err"; then
	fail "a claim owned by a dead pid was not reclaimed"
fi
[ "$(calls)" -eq $((before + 1)) ] \
	|| fail "the producer did not run after reclaiming a dead-owner claim"
[ -f "$out_reclaim/file.txt" ] || fail "the reclaimed package was not exposed"
if [ -e "$TAU_SHARED_PREFIX/store/reclaimdep/$reclaim_id.claim" ]; then
	fail "a reclaimed claim was left behind"
fi

# 10c. a claim left by a producer subshell that died while its parent shell
# lives is reclaimed too: dep_ensure records $BASHPID, so a live parent never
# keeps a dead producer's claim alive.
RECLAIM_SUB_BLOCK="$(printf 'dep=reclaimsub\nsource=abc')"
reclaim_sub_id="$(bridge_id "$RECLAIM_SUB_BLOCK")"
mkdir -p "$TAU_SHARED_PREFIX/store/reclaimsub/$reclaim_sub_id.claim"
( printf '%s %s\n' "$(uname -n)" "${BASHPID:-$(sh -c 'echo $PPID')}" \
	> "$TAU_SHARED_PREFIX/store/reclaimsub/$reclaim_sub_id.claim/owner" )
before="$(calls)"
if ! TAU_DEP_CLAIM_TIMEOUT=5 TAU_DEP_CLAIM_POLL=1 \
		dep_ensure out_reclaim_sub producer reclaimsub "$RECLAIM_SUB_BLOCK" \
		fake_producer 2>"$SCRATCH/reclaim-sub.err"; then
	fail "a claim left by a dead producer subshell was not reclaimed"
fi
[ "$(calls)" -eq $((before + 1)) ] \
	|| fail "the producer did not run after a dead producer subshell"
[ -f "$out_reclaim_sub/file.txt" ] \
	|| fail "the subshell-reclaimed package was not exposed"
if [ -e "$TAU_SHARED_PREFIX/store/reclaimsub/$reclaim_sub_id.claim" ]; then
	fail "a subshell-reclaimed claim was left behind"
fi

# 11. concurrent cold start: two processes, exactly one producer
before="$(calls)"
( dep_ensure _c1 producer concurrent "$CONC_BLOCK" fake_producer
  printf '%s\n' "$_c1" > "$SCRATCH/c1" ) &
p1=$!
( dep_ensure _c2 producer concurrent "$CONC_BLOCK" fake_producer
  printf '%s\n' "$_c2" > "$SCRATCH/c2" ) &
p2=$!
wait "$p1" || fail "the first concurrent call failed"
wait "$p2" || fail "the second concurrent call failed"
[ "$(calls)" -eq $((before + 1)) ] \
	|| fail "a concurrent cold start ran the producer $(( $(calls) - before )) times"
[ -s "$SCRATCH/c1" ] && [ -s "$SCRATCH/c2" ] || fail "a concurrent call produced no prefix"
[ "$(cat "$SCRATCH/c1")" = "$(cat "$SCRATCH/c2")" ] \
	|| fail "concurrent calls returned different prefixes"

# 12. corruption fails without a rebuild and keeps the CMake diagnostics
printf 'corrupted\n' > "$out/file.txt"
before="$(calls)"
if dep_ensure unused_corrupt producer demo "$BLOCK" fake_producer \
		2>"$SCRATCH/err.txt"; then
	fail "a corrupt entry succeeded"
fi
[ "$(calls)" -eq "$before" ] || fail "a corrupt entry triggered a rebuild (calls=$(calls))"
grep -q "output record mismatch" "$SCRATCH/err.txt" \
	|| fail "the integrity diagnostic was suppressed"

# 13. a producer that bakes an absolute home path fails closed and publishes
# nothing, so a consumer can never resolve a package with one.
path_id="$(bridge_id "$PATH_BLOCK")"
before="$(calls)"
if dep_ensure unused_path producer pathdep "$PATH_BLOCK" fake_producer_path \
		2>"$SCRATCH/path.err"; then
	fail "a producer that baked a home path succeeded"
fi
[ "$(calls)" -eq $((before + 1)) ] || fail "the path producer was not called"
if ls -d "$TAU_SHARED_PREFIX/store/pathdep/"*.staging* >/dev/null 2>&1; then
	fail "a gate failure left a staging entry"
fi
lookup="$(cmake -P "$PARSER_ROOT/cmake/tau-store.cmake" lookup \
	"$TAU_SHARED_PREFIX" pathdep "$path_id" 2>/dev/null)" \
	|| fail "lookup after a gate failure errored"
[ "$lookup" = "miss" ] || fail "a gate failure left a final entry"
grep -q "absolute build, staging, store, or home path" "$SCRATCH/path.err" \
	|| fail "the gate failure was not reported"

# 14. a producer that bakes a foreign home path also fails closed
foreign_id="$(bridge_id "$FOREIGN_BLOCK")"
before="$(calls)"
if dep_ensure unused_foreign producer foreigndep "$FOREIGN_BLOCK" \
		fake_producer_foreign_path 2>"$SCRATCH/foreign.err"; then
	fail "a producer that baked a foreign home path succeeded"
fi
[ "$(calls)" -eq $((before + 1)) ] || fail "the foreign path producer was not called"
lookup="$(cmake -P "$PARSER_ROOT/cmake/tau-store.cmake" lookup \
	"$TAU_SHARED_PREFIX" foreigndep "$foreign_id" 2>/dev/null)" \
	|| fail "lookup after a foreign gate failure errored"
[ "$lookup" = "miss" ] || fail "a foreign gate failure left a final entry"
grep -q "absolute build, staging, store, or home path" "$SCRATCH/foreign.err" \
	|| fail "the foreign gate failure was not reported"

echo "tau-dep-ensure self-check passed" >&2
exit 0
