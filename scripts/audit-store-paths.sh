#!/bin/bash
# Audit a package prefix (or any tree) for absolute build, staging, store, and
# home paths baked into its artifacts.
#
#   audit-store-paths.sh <path>...
#
# Each <path> is scanned as a whole tree. A store root is audited entry by entry
# by passing the <root>/<dep>/<id>/prefix paths, so a gate can cover the current
# set without failing on immutable superseded entries. Every file that carries
# an absolute home or /tmp path, or a ".staging-" component, is reported with
# the number of distinct paths it holds. Exits 1 when any file carries one.
#
# Compiler-runtime and system paths (/usr, /lib, ...) are out of scope: they are
# not build, staging, store, or home paths. A mkstemp template such as
# /tmp/foo_XXXXXX is not a baked path and is ignored.
set -u

if [ "$#" -lt 1 ]; then
	echo "usage: audit-store-paths.sh <path>..." >&2
	exit 2
fi

# A baked path is a build, staging, store, or home path. Any home counts, not
# just the current user's: a path under another account, a vendored tarball, or
# a macOS runner still leaks the build environment. The name component and the
# trailing slash stop a mkstemp template such as /tmp/foo_XXXXXX from matching.
# The path must not be preceded by a path or word character: that is what keeps
# a Boost Spirit qi header path, whose middle component is the word home, from
# reading as a home path under another account. PCRE provides the lookbehind;
# without it the extended pattern is used and the false positive returns, so
# prefer a grep with -P.
if echo x | grep -qP 'x' 2>/dev/null; then
	grep_mode=-aoP
	pattern='(?<![A-Za-z0-9_/])(/home/[A-Za-z0-9_.+-]+|/Users/[A-Za-z0-9_.+-]+|/root|/tmp/[A-Za-z0-9_.+-]+)/[A-Za-z0-9_./+-]*'
else
	grep_mode=-aoE
	pattern='(/home/[A-Za-z0-9_.+-]+|/Users/[A-Za-z0-9_.+-]+|/root|/tmp/[A-Za-z0-9_.+-]+)/[A-Za-z0-9_./+-]*'
fi
staging='\.staging-[A-Za-z0-9_-]+'

found=0
scan_file() {
	local file="$1"
	local matches count
	matches="$(grep "${grep_mode}" "${pattern}|${staging}" "${file}" 2>/dev/null \
		| grep -v 'XXXXXX$' | sort -u)"
	count="$(printf '%s\n' "${matches}" | grep -c . )"
	if [ "${count}" -gt 0 ]; then
		printf '%s\t%s\n' "${count}" "${file}"
		printf '%s\n' "${matches}" | head -3 | sed 's/^/    /' >&2
		found=1
	fi
}

# BSD find/sort have no -z for sort; a scan reports every file it finds and
# the order it visits them in is irrelevant, so nothing is sorted here.
scan_tree() {
	local root="$1"
	local file
	while IFS= read -r -d '' file; do
		scan_file "${file}"
	done < <(find "${root}" -type f -print0 2>/dev/null)
}

for path in "$@"; do
	if [ ! -d "${path}" ]; then
		echo "audit-store-paths: not a directory: '${path}'" >&2
		exit 2
	fi
	scan_tree "${path}"
done

if [ "${found}" -ne 0 ]; then
	echo "audit-store-paths: absolute paths found" >&2
	exit 1
fi
echo "audit-store-paths: clean"
