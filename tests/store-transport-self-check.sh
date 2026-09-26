#!/bin/bash
# Hermetic self-check for scripts/store-transport.sh.
#
# Builds a tiny store in a random scratch namespace, exports it, imports it into
# a fresh namespace, and verifies it. Then it forces each failure: a lost
# executable bit, a truncated archive, a content corruption, and a missing
# expected entry. Exits zero and prints nothing on success.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARSER_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TRANSPORT="${PARSER_ROOT}/scripts/store-transport.sh"
MANIFEST="${PARSER_ROOT}/cmake/tau-manifest.cmake"
STORE_MODULE="${PARSER_ROOT}/cmake/tau-store.cmake"
CMAKE="${CMAKE:-cmake}"

for f in "$TRANSPORT" "$MANIFEST" "$STORE_MODULE"; do
	[ -f "$f" ] || { echo "store-transport self-check: missing $f" >&2; exit 1; }
done

TMP_ROOT="${TMPDIR:-/tmp}"
ROOT="$(mktemp -d "${TMP_ROOT}/tau-transport-self.XXXXXX")" || exit 1
fail() { echo "store-transport self-check: $*" >&2; rm -rf "$ROOT"; exit 1; }

block_file="${ROOT}/block"
printf 'dep=demo\nsource=abc\n' > "$block_file"
id="$("$CMAKE" -P "$MANIFEST" input-id-file "$block_file")" || fail "cannot compute id"
[ -n "$id" ] || fail "empty id"

make_store() {
	local shared="$1"
	local entry="${shared}/store/demo/${id}"
	mkdir -p "${entry}/prefix/bin" "${entry}/prefix/lib"
	printf '#!/bin/sh\necho ok\n' > "${entry}/prefix/bin/tool"
	chmod 755 "${entry}/prefix/bin/tool"
	printf 'data\n' > "${entry}/prefix/lib/data"
	chmod 644 "${entry}/prefix/lib/data"
	"$CMAKE" -P "$MANIFEST" manifest "${entry}/manifest.json" "$id" \
		"$block_file" "${entry}/prefix" || fail "cannot write manifest"
}

src="${ROOT}/src"; make_store "$src"
entry="demo/${id}"
tar="${ROOT}/store.tar"
"$TRANSPORT" export "$src" "$tar" "$entry" >/dev/null 2>&1 || fail "export failed"

# round trip
dst="${ROOT}/dst"
"$TRANSPORT" import "$tar" "$dst" "$entry" >/dev/null 2>&1 || fail "import failed"
"$TRANSPORT" verify "$dst" "$entry" >/dev/null 2>&1 || fail "verify failed"
[ -x "${dst}/store/demo/${id}/prefix/bin/tool" ] || fail "imported bin/tool is not executable"
[ -f "${dst}/store/demo/${id}/prefix/lib/data" ] || fail "imported lib/data is missing"

# a lost executable bit is rejected (the digest is unchanged)
modefix="${ROOT}/modefix"; mkdir -p "$modefix"
tar -xf "$tar" -C "$modefix" || fail "cannot extract for modefix"
chmod 644 "${modefix}/demo/${id}/prefix/bin/tool"
tar --format=posix -cf "${ROOT}/modefix.tar" -C "$modefix" "$entry" || fail "cannot re-tar modefix"
if "$TRANSPORT" import "${ROOT}/modefix.tar" "${ROOT}/modefix-dst" "$entry" >/dev/null 2>&1; then
	fail "a lost executable bit was accepted"
fi

# a truncated archive is rejected (cut into the member data, not just padding)
size="$(wc -c < "$tar")"
head -c $((size / 2)) "$tar" > "${ROOT}/trunc.tar"
if "$TRANSPORT" import "${ROOT}/trunc.tar" "${ROOT}/trunc-dst" "$entry" >/dev/null 2>&1; then
	fail "a truncated archive was accepted"
fi

# a content corruption is rejected by the manifest digest
contentfix="${ROOT}/contentfix"; mkdir -p "$contentfix"
tar -xf "$tar" -C "$contentfix" || fail "cannot extract for contentfix"
printf 'x' >> "${contentfix}/demo/${id}/prefix/lib/data"
tar --format=posix -cf "${ROOT}/contentfix.tar" -C "$contentfix" "$entry" || fail "cannot re-tar contentfix"
if "$TRANSPORT" import "${ROOT}/contentfix.tar" "${ROOT}/contentfix-dst" "$entry" >/dev/null 2>&1; then
	fail "a content corruption was accepted"
fi

# a missing expected entry fails closed
if "$TRANSPORT" import "$tar" "${ROOT}/missing-dst" "$entry" \
		"demo/0000000000000000000000000000000000000000000000000000000000000000" >/dev/null 2>&1; then
	fail "a missing expected entry was accepted"
fi

rm -rf "$ROOT"
echo "store-transport self-check passed" >&2
exit 0
