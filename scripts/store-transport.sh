#!/bin/bash
# Export a LOCAL store's current entries to a portable archive and import them
# into a fresh store namespace.
#
#   store-transport.sh export <store-root> <out.tar> <dep>/<id>...
#   store-transport.sh import <archive> <dest-store> [<dep>/<id>...]
#   store-transport.sh verify <store-root> <dep>/<id>...
#
# The archive is a POSIX tar of <dep>/<id>/{manifest.json,prefix}. It is the
# portable stand-in for the durable tier: an OCI image layer is a tar, and a
# release asset is usually a tarball, so modes, symlink targets, and long paths
# travel the same way.
#
# Import extracts to a scratch tree, checks that every extracted mode matches
# the archive and that every regular file under a bin/ directory is executable,
# then publishes each entry through the store's verified no-replace rename. It
# never writes into a live entry. A missing, malformed, or conflicting entry
# fails closed.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST_MODULE="${SCRIPT_DIR}/../cmake/tau-manifest.cmake"
STORE_MODULE="${SCRIPT_DIR}/../cmake/tau-store.cmake"
[ -f "$MANIFEST_MODULE" ] || { echo "store-transport: missing ${MANIFEST_MODULE}" >&2; exit 2; }
[ -f "$STORE_MODULE" ] || { echo "store-transport: missing ${STORE_MODULE}" >&2; exit 2; }

_token() { printf '%s' "$$-$(date +%s)-${RANDOM}"; }

usage() {
	echo "usage: store-transport.sh export <store-root> <out.tar> <dep>/<id>..." >&2
	echo "       store-transport.sh import <archive> <dest-store> [<dep>/<id>...]" >&2
	echo "       store-transport.sh verify <store-root> <dep>/<id>..." >&2
	exit 2
}

# A path is one <dep>/<id> pair.
_check_entry() {
	local entry="$1" dep id
	dep="${entry%%/*}"
	id="${entry#*/}"
	if [ "$dep" = "$entry" ] || [ -z "$dep" ] || [ -z "$id" ] \
			|| [ "${id#*/}" != "$id" ] || [ "$dep" = "." ] || [ "$dep" = ".." ]; then
		echo "store-transport: not a <dep>/<id> entry: '${entry}'" >&2
		return 1
	fi
	case "$id" in
		*[!0-9a-f]*|"") echo "store-transport: bad id in '${entry}'" >&2; return 1 ;;
	esac
	string_len="${#id}"
	[ "$string_len" -eq 64 ] || { echo "store-transport: id is not 64 hex: '${entry}'" >&2; return 1; }
	return 0
}

# Reject an archive member that escapes the extraction root or is not under a
# <dep>/<id>/ prefix.
_check_archive_members() {
	local archive="$1"
	python3 - "$archive" <<'PY'
import os, sys, tarfile
archive = sys.argv[1]
bad = []
with tarfile.open(archive) as t:
	for m in t.getmembers():
		name = m.name
		if name.startswith("/") or name.startswith("../") or "/../" in name or name == "..":
			bad.append(name); continue
		parts = [p for p in name.split("/") if p not in ("", ".")]
		# the entry root is <dep>/<id>; everything else is below it
		if len(parts) < 2:
			bad.append(name); continue
		id_part = parts[1]
		if len(id_part) != 64 or any(c not in "0123456789abcdef" for c in id_part):
			bad.append(name); continue
for name in bad[:5]:
	print(f"store-transport: unsafe archive member: {name}", file=sys.stderr)
sys.exit(1 if bad else 0)
PY
}

# Compare every extracted regular file's mode with the archive, then require
# every regular file under a bin/ directory to be executable. The manifest
# records the mode too (schema 3), but the archive is the independent cross-
# check the transport owns: a manifest is only as good as the writer that
# scanned the prefix the archive was made from.
_check_import_modes() {
	local archive="$1" root="$2"
	shift 2
	python3 - "$archive" "$root" "$@" <<'PY'
import os, stat, sys, tarfile
archive, root = sys.argv[1], sys.argv[2]
entries = sys.argv[3:]
failed = []
with tarfile.open(archive) as t:
	for m in t.getmembers():
		if not m.isfile():
			continue
		parts = [p for p in m.name.split("/") if p not in ("", ".")]
		if not parts:
			continue
		path = os.path.join(root, *parts)
		if not os.path.isfile(path):
			failed.append(f"missing after extract: {m.name}")
			continue
		actual = stat.S_IMODE(os.stat(path).st_mode)
		expected = m.mode & 0o7777
		if actual != expected:
			failed.append(f"mode mismatch {m.name}: archive {expected:o} extracted {actual:o}")
for entry in entries:
	prefix = os.path.join(root, entry, "prefix")
	for dirpath, _dirnames, filenames in os.walk(prefix):
		if os.path.basename(dirpath) != "bin":
			continue
		for name in filenames:
			path = os.path.join(dirpath, name)
			if os.path.isfile(path) and not os.access(path, os.X_OK):
				failed.append(f"bin file not executable: {os.path.relpath(path, root)}")
for line in failed[:10]:
	print(f"store-transport: {line}", file=sys.stderr)
sys.exit(1 if failed else 0)
PY
}

cmd="${1:-}"; shift 2>/dev/null || true

case "$cmd" in
export)
	[ "$#" -ge 3 ] || usage
	store_root="$1"; out_tar="$2"; shift 2
	[ -d "${store_root}/store" ] || { echo "store-transport: no store at '${store_root}/store'" >&2; exit 2; }
	for entry in "$@"; do
		_check_entry "$entry" || exit 2
		[ -f "${store_root}/store/${entry}/manifest.json" ] || {
			echo "store-transport: missing manifest for '${entry}'" >&2; exit 1; }
		[ -d "${store_root}/store/${entry}/prefix" ] || {
			echo "store-transport: missing prefix for '${entry}'" >&2; exit 1; }
	done
	tar --format=posix -cf "$out_tar" -C "${store_root}/store" "$@" || {
		echo "store-transport: export failed" >&2; exit 1; }
	echo "store-transport: exported $# entries to ${out_tar}"
	;;
import)
	[ "$#" -ge 2 ] || usage
	archive="$1"; dest_store="$2"; shift 2
	[ -f "$archive" ] || { echo "store-transport: no archive '${archive}'" >&2; exit 2; }
	_check_archive_members "$archive" || exit 1
	mkdir -p "$dest_store"
	extract="$(mktemp -d "${TMPDIR:-/tmp}/tau-transport.XXXXXX")" || exit 1
	# shellcheck disable=SC2064
	trap "rm -rf '${extract}'" EXIT
	tar -xf "$archive" -C "$extract" || {
		echo "store-transport: extract failed" >&2; exit 1; }

	# The entry list is what the caller expects. With none, import everything the
	# archive holds. Either way, an expected entry that is absent fails closed.
	expected=("$@")
	if [ "${#expected[@]}" -eq 0 ]; then
		while IFS= read -r d; do
			id="$(basename "$d")"
			expected+=("$(basename "$(dirname "$d")")/${id}")
		done < <(find "$extract" -mindepth 2 -maxdepth 2 -type d -print | sort)
	fi
	for entry in "${expected[@]}"; do
		_check_entry "$entry" || exit 2
		[ -f "${extract}/${entry}/manifest.json" ] || {
			echo "store-transport: archive is missing entry '${entry}'" >&2; exit 1; }
	done
	_check_import_modes "$archive" "$extract" "${expected[@]}" || exit 1

	for entry in "${expected[@]}"; do
		dep="${entry%%/*}"; id="${entry##*/}"
		dep_dir="${dest_store}/store/${dep}"
		staging="${dep_dir}/${id}.staging-$(_token)"
		mkdir -p "$dep_dir" || exit 1
		mv "${extract}/${entry}" "$staging" || {
			echo "store-transport: cannot stage '${entry}'" >&2; rm -rf "$staging"; exit 1; }
		if ! cmake -P "$STORE_MODULE" publish "$dest_store" "$dep" "$id" "$staging" >/dev/null; then
			echo "store-transport: publish failed for '${entry}'" >&2
			rm -rf "$staging"
			exit 1
		fi
		echo "store-transport: imported ${entry}"
	done
	rm -rf "$extract"
	trap - EXIT
	;;
verify)
	[ "$#" -ge 2 ] || usage
	store_root="$1"; shift
	for entry in "$@"; do
		_check_entry "$entry" || exit 2
		dep="${entry%%/*}"; id="${entry##*/}"
		if ! out="$(cmake -P "$STORE_MODULE" lookup "$store_root" "$dep" "$id" 2>/dev/null)"; then
			echo "store-transport: verification failed for '${entry}'" >&2
			exit 1
		fi
		if [ "$(printf '%s\n' "$out" | sed -n '1p')" != "hit" ]; then
			echo "store-transport: no verified entry for '${entry}'" >&2
			exit 1
		fi
		echo "store-transport: verified ${entry}"
	done
	;;
*)
	usage
	;;
esac
