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
# reading as a home path under another account.
# Git Bash and MSVC spell the home directory as /c/Users/<user>,
# C:/Users/<user>, or C:\Users\<user>; all three are home paths.
# Emscripten bakes its virtual home, /home/web_user, into every JS file that
# carries its file system; it is not a build path, so it and anything below it
# are exempt, while every other home still counts.
# Two Boost comments name an example path: framework.ipp writes
# /tmp/mylogsink.xml and parameter_cache.hpp writes /home/user/.boost_compute.
# Neither is a build path, so the exact literals are exempt too.
if echo x | grep -qP 'x' 2>/dev/null; then
	grep_mode=-aoP
	pattern='(?<![A-Za-z0-9_/\\])(/home/(?!web_user(?:$|[^A-Za-z0-9_.+-]))[A-Za-z0-9_.+-]+|/Users/[A-Za-z0-9_.+-]+|/[A-Za-z]/Users/[A-Za-z0-9_.+-]+|/root|/tmp/[A-Za-z0-9_.+-]+|[A-Za-z]:[\\/]Users[\\/][A-Za-z0-9_.+-]+)[\\/A-Za-z0-9_.+-]*'
	exclude='XXXXXX$|^/tmp/mylogsink\.xml$|^/home/user/\.boost_compute/'
	strip=''
else
	grep_mode=-aoE
	pattern='(^|[^A-Za-z0-9_/\\])(/home/[A-Za-z0-9_.+-]+|/Users/[A-Za-z0-9_.+-]+|/[A-Za-z]/Users/[A-Za-z0-9_.+-]+|/root|/tmp/[A-Za-z0-9_.+-]+|[A-Za-z]:[\\/]Users[\\/][A-Za-z0-9_.+-]+)[\\/A-Za-z0-9_.+-]*'
	exclude='XXXXXX$|^/home/web_user(/|$)|^/tmp/mylogsink\.xml$|^/home/user/\.boost_compute/'
	strip='^[^/A-Za-z]'
fi
staging='\.staging-[A-Za-z0-9_-]+'
regex="${pattern}|${staging}"

# One grep pass per tree, not one process per file: a store package holds
# thousands of files, and a process per file dominates the audit on Windows.
tmp="$(mktemp -d)" || exit 1
trap 'rm -rf "$tmp"' EXIT INT TERM

# Split grep's "path:match" lines, strip the ERE lookbehind copy, drop the
# excluded matches, and print "path<TAB>match". AUDIT_FILE marks a file whose
# own name carries a colon, so grep ran without -H and every line is a match.
cat > "$tmp/dedup.awk" <<'AWK'
BEGIN {
	root = ENVIRON["AUDIT_ROOT"]
	strip = ENVIRON["AUDIT_STRIP"]
	exclude = ENVIRON["AUDIT_EXCLUDE"]
	single = ENVIRON["AUDIT_FILE"]
}
{
	if (single != "") {
		path = single
		text = $0
	} else {
		i = index($0, ":")
		if (i == 0) next
		path = substr($0, 1, i - 1)
		text = substr($0, i + 1)
	}
	if (strip != "") sub(strip, "", text)
	if (text ~ exclude) next
	if (substr(path, 1, 2) == "./") path = substr(path, 3)
	print root "/" path "\t" text
}
AWK

# The sorted stream groups each path, so counting a group and printing its
# first three matches reproduces the per-file sort -u | head -3.
cat > "$tmp/print.awk" <<'AWK'
function flush() {
	if (count > 0) printf "%d\t%s\n", count, path
}
{
	i = index($0, "\t")
	next_path = substr($0, 1, i - 1)
	if (next_path != path) {
		flush()
		path = next_path
		count = 0
		shown = 0
	}
	count++
	if (shown < 3) {
		print "    " substr($0, i + 1) > "/dev/stderr"
		shown++
	}
}
END { flush() }
AWK

scan_tree() {
	local root="$1" prefix
	# find does not follow a symlink start point, so neither does the audit.
	[ -L "$root" ] && return 0
	prefix="${root%/}"
	while [ "$prefix" != "${prefix%/}" ]; do prefix="${prefix%/}"; done
	(
		cd "$root" || exit 1
		: > "$tmp/plain"
		: > "$tmp/colon"
		# Hold the two list files open: a reopen per file is slow on Git Bash.
		exec 8>> "$tmp/plain"
		exec 9>> "$tmp/colon"
		find . -type f -print0 2>/dev/null \
			| while IFS= read -r -d '' f; do
				case "$f" in
					*:*) printf '%s\0' "$f" >&9 ;;
					*)   printf '%s\0' "$f" >&8 ;;
				esac
			done
		exec 8>&- 9>&-
		{
			if [ -s "$tmp/plain" ]; then
				xargs -0 grep -H "${grep_mode}" -- "$regex" \
					< "$tmp/plain" 2>/dev/null \
					| AUDIT_ROOT="$prefix" AUDIT_STRIP="$strip" \
						AUDIT_EXCLUDE="$exclude" AUDIT_FILE= \
						awk -f "$tmp/dedup.awk"
			fi
			# A colon in the file name breaks the "path:match" split, so
			# grep those one file at a time without -H.
			if [ -s "$tmp/colon" ]; then
				while IFS= read -r -d '' f; do
					grep "${grep_mode}" -- "$regex" "$f" 2>/dev/null \
						| AUDIT_ROOT="$prefix" AUDIT_STRIP="$strip" \
							AUDIT_EXCLUDE="$exclude" AUDIT_FILE="$f" \
							awk -f "$tmp/dedup.awk"
				done < "$tmp/colon"
			fi
		} | sort -u -t $'\t' -k1,1 -k2,2 \
			| awk -f "$tmp/print.awk"
	)
}

: > "$tmp/out"
for path in "$@"; do
	if [ ! -d "$path" ]; then
		echo "audit-store-paths: not a directory: '${path}'" >&2
		exit 2
	fi
	scan_tree "$path" >> "$tmp/out"
done

if [ -s "$tmp/out" ]; then
	cat "$tmp/out"
	echo "audit-store-paths: absolute paths found" >&2
	exit 1
fi
echo "audit-store-paths: clean"
