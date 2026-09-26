#!/bin/bash
# The remote tier of the dependency store: one private OCI artifact per store
# entry, tagged <dep>-<id>. Configure's read-on-a-miss path and this script's
# tag/has/pull/push/local-entries subcommands reach it through TAU_STORE_REMOTE.
#
#   store-remote.sh tag <dep> <id>
#   store-remote.sh has <store-root> <dep>/<id>
#   store-remote.sh pull <store-root> <dep>/<id>
#   store-remote.sh push <store-root> <dep>/<id>
#   store-remote.sh local-entries <store-root>
#
# <store-root> is the shared prefix that holds store/ (TAU_SHARED_PREFIX).
# TAU_STORE_REMOTE names the remote, e.g. ghcr.io/<owner>/<repo>. When it is
# unset the remote is absent: has/pull/push report a miss and nothing changes.
# The payload is store-transport.sh's export of one entry; a pull imports it
# through the same verified no-replace publish a local import uses, so a
# remote package is verified before use.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TRANSPORT="${SCRIPT_DIR}/store-transport.sh"

# OCI tags are at most 128 characters. A <dep>-<id> that fits is used as is;
# one that does not is replaced by a deterministic hash of the pair, and the
# full <dep>/<id> travels in an annotation.
store_remote_tag() {
	# Built on its own line: `local` expands every word before it assigns, so a
	# same-line reference would read the caller's unset `dep` under `set -u`.
	local dep="$1" id="$2" tag hash
	tag="${dep}-${id}"
	if [ "${#tag}" -le 128 ]; then
		printf '%s' "$tag"
		return 0
	fi
	hash="$(printf '%s-%s' "$dep" "$id" | _store_remote_sha256_stdin)"
	printf '%s-%s' "${dep:0:32}" "${hash:0:32}"
}

store_remote_configured() {
	[ -n "${TAU_STORE_REMOTE:-}" ]
}

store_remote_ref() {
	local dep="$1" id="$2"
	printf '%s:%s' "$TAU_STORE_REMOTE" "$(store_remote_tag "$dep" "$id")"
}

# Exit 0 only when the remote answers for the entry's tag. Any other outcome,
# including an unreachable remote, is a miss; the caller then builds.
store_remote_has() {
	local store_root="$1" entry="$2" dep id
	store_remote_configured || return 1
	dep="${entry%%/*}"; id="${entry##*/}"
	oras manifest fetch "$(store_remote_ref "$dep" "$id")" >/dev/null 2>&1
}

# Pull one entry and import it. The tar is staged and imported by
# store-transport.sh, which verifies the manifest and every file mode.
store_remote_pull() {
	local store_root="$1" entry="$2" dep id ref tmp tar
	store_remote_configured || return 1
	dep="${entry%%/*}"; id="${entry##*/}"
	ref="$(store_remote_ref "$dep" "$id")"
	tmp="$(mktemp -d "${TMPDIR:-/tmp}/tau-remote.XXXXXX")" || return 1
	if ! oras pull "$ref" -o "$tmp" >&2; then
		rm -rf "$tmp"
		return 1
	fi
	# oras names a layer after the file pushed, so the fixed basename below is
	# the pull target.
	tar="$tmp/store-entry.tar"
	if [ ! -f "$tar" ]; then
		echo "store-remote: ${ref} carried no store-entry.tar" >&2
		rm -rf "$tmp"
		return 1
	fi
	if ! "$TRANSPORT" import "$tar" "$store_root" "$entry" >&2; then
		rm -rf "$tmp"
		return 1
	fi
	rm -rf "$tmp"
	return 0
}

store_remote_push() {
	local store_root="$1" entry="$2" dep id ref tmp tar
	store_remote_configured || return 1
	dep="${entry%%/*}"; id="${entry##*/}"
	ref="$(store_remote_ref "$dep" "$id")"
	# Verify before export: a push must not publish bytes the store would
	# refuse to hand back.
	if ! "$TRANSPORT" verify "$store_root" "$entry" >&2; then
		return 1
	fi
	tmp="$(mktemp -d "${TMPDIR:-/tmp}/tau-remote.XXXXXX")" || return 1
	tar="$tmp/store-entry.tar"
	if ! "$TRANSPORT" export "$store_root" "$tar" "$entry" >&2; then
		rm -rf "$tmp"
		return 1
	fi
	if ! oras push \
			--artifact-type "application/vnd.tau.store.entry.v1" \
			--annotation "dev.tau.store.entry=${entry}" \
			"$ref" "$tar" >&2; then
		rm -rf "$tmp"
		return 1
	fi
	rm -rf "$tmp"
	return 0
}

# Every complete local entry, one <dep>/<id> per line.
store_remote_local_entries() {
	local store_root="$1" dep_dir entry id
	[ -d "${store_root}/store" ] || return 0
	for dep_dir in "${store_root}/store"/*; do
		[ -d "$dep_dir" ] || continue
		for entry in "${dep_dir}"/*; do
			[ -d "$entry" ] || continue
			id="${entry##*/}"
			[ "${#id}" -eq 64 ] || continue
			case "$id" in *[!0-9a-f]*) continue ;; esac
			[ -f "${entry}/manifest.json" ] || continue
			[ -d "${entry}/prefix" ] || continue
			printf '%s/%s\n' "${dep_dir##*/}" "$id"
		done
	done | LC_ALL=C sort
}

_store_remote_sha256_stdin() {
	if command -v sha256sum > /dev/null 2>&1; then
		sha256sum | awk '{print $1}'
	else
		shasum -a 256 | awk '{print $1}'
	fi
}

cmd="${1:-}"
case "$cmd" in
tag)
	[ "$#" -eq 3 ] || { echo "usage: store-remote.sh tag <dep> <id>" >&2; exit 2; }
	store_remote_tag "$2" "$3"
	;;
has)
	[ "$#" -eq 3 ] || { echo "usage: store-remote.sh has <store-root> <dep>/<id>" >&2; exit 2; }
	store_remote_has "$2" "$3"
	;;
pull)
	[ "$#" -eq 3 ] || { echo "usage: store-remote.sh pull <store-root> <dep>/<id>" >&2; exit 2; }
	store_remote_pull "$2" "$3"
	;;
push)
	[ "$#" -eq 3 ] || { echo "usage: store-remote.sh push <store-root> <dep>/<id>" >&2; exit 2; }
	store_remote_push "$2" "$3"
	;;
local-entries)
	[ "$#" -eq 2 ] || { echo "usage: store-remote.sh local-entries <store-root>" >&2; exit 2; }
	store_remote_local_entries "$2"
	;;
*)
	echo "usage: store-remote.sh <tag|has|pull|push|local-entries> ..." >&2
	exit 2
	;;
esac
