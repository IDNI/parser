#!/bin/bash
# Cache the Boost headers package in the LOCAL store.
#
#   ./dev dep-boost-headers -DTAU_BUILD_JOBS=8
#   ./dev dep-boost-headers -DTAU_DEP_MODE=consumer
#
# The source is the pinned Boost release archive and its published SHA-256.
# Producer mode downloads into a staging entry and publishes it. Consumer mode
# only looks up an existing entry and never downloads.
#
# The package is headers only, so no compiler, flags, cmake or ninja enter the
# identity and every target produces the same bytes. The store name is
# boost_headers, because tau-lang owns the boost store name.
#
# Only the native, cross-toolchain, macOS and MSVC tuples are produced; any
# other target or host is rejected.

set -u

DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

DEP_BOOST_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
BOOST_VERSION="1.86.0"
BOOST_UNDERSCORE="1_86_0"
# Official release archive and its published SHA-256, so a build gets the
# exact headers this project pins.
BOOST_URL_DEFAULT="https://archives.boost.io/release/${BOOST_VERSION}/source/boost_${BOOST_UNDERSCORE}.tar.gz"
BOOST_SHA256_DEFAULT="2575e74ffc3ef1cd0babac2c1ee8bdb5782a0ee672b1912da40e5b4b591ca01f"

_dep_boost_field_block() {
	local build_helper publish_helper manifest store
	local recipe_hash build_hash publish_hash manifest_hash store_hash
	build_helper="${__devrc_dir}/dep-build"
	publish_helper="${__devrc_dir}/devrc"
	manifest="${__devrc_dir}/../cmake/tau-manifest.cmake"
	store="${__devrc_dir}/../cmake/tau-store.cmake"
	recipe_hash="$(dep_sha256 "$DEP_BOOST_RECIPE")" || return 1
	build_hash="$(dep_sha256 "$build_helper")" || return 1
	publish_hash="$(dep_sha256 "$publish_helper")" || return 1
	manifest_hash="$(dep_sha256 "$manifest")" || return 1
	store_hash="$(dep_sha256 "$store")" || return 1
	printf '%s\n' \
		"dep=boost_headers" \
		"target=${BOOST_TARGET}" \
		"version=${BOOST_VERSION}" \
		"sha256=${BOOST_SHA256}" \
		"recipe_hash=${recipe_hash}" \
		"helper_build_hash=${build_hash}" \
		"provenance.publish_helper_hash=${publish_hash}" \
		"provenance.manifest_writer_hash=${manifest_hash}" \
		"provenance.store_writer_hash=${store_hash}"
}

_dep_boost_download() {
	local url="$1" archive="$2"
	echo "Downloading ${url}"
	if command -v curl > /dev/null 2>&1; then
		curl -fsSL -o "$archive" "$url"
	elif command -v wget > /dev/null 2>&1; then
		wget -qO "$archive" "$url"
	else
		echo "dep-boost-headers: neither curl nor wget is available" >&2
		return 1
	fi
}

_dep_boost_digest() {
	local archive="$1" actual
	if command -v sha256sum > /dev/null 2>&1; then
		actual="$(sha256sum "$archive" | awk '{print $1}')"
	elif command -v shasum > /dev/null 2>&1; then
		actual="$(shasum -a 256 "$archive" | awk '{print $1}')"
	else
		echo "dep-boost-headers: neither sha256sum nor shasum is available" >&2
		return 1
	fi
	printf '%s' "$actual"
}

# Producer callback. $1 is the staging prefix. The download lives in a
# temporary directory outside the package, so no archive path reaches the
# published tree.
_dep_boost_producer() {
	local staging_prefix="$1"
	local tmp archive actual
	tmp="$(mktemp -d)" || return 1
	archive="${tmp}/boost_${BOOST_UNDERSCORE}.tar.gz"
	if ! _dep_boost_download "$BOOST_URL" "$archive"; then
		rm -rf "$tmp"
		return 1
	fi
	if ! actual="$(_dep_boost_digest "$archive")"; then
		rm -rf "$tmp"
		return 1
	fi
	if [ "$actual" != "$BOOST_SHA256" ]; then
		echo "dep-boost-headers: Boost ${BOOST_VERSION} checksum mismatch" >&2
		echo "  expected: ${BOOST_SHA256}" >&2
		echo "  actual:   ${actual}" >&2
		rm -rf "$tmp"
		return 1
	fi
	if ! mkdir -p "${staging_prefix}/include" \
			"${staging_prefix}/share/licenses/boost"; then
		rm -rf "$tmp"
		return 1
	fi
	# --strip-components=1 drops the boost_${BOOST_UNDERSCORE}/ top level.
	if ! tar -xzf "$archive" -C "${staging_prefix}/include" --strip-components=1 \
			"boost_${BOOST_UNDERSCORE}/boost"; then
		rm -rf "$tmp"
		return 1
	fi
	if ! tar -xzf "$archive" -C "$tmp" --strip-components=1 \
			"boost_${BOOST_UNDERSCORE}/LICENSE_1_0.txt"; then
		rm -rf "$tmp"
		return 1
	fi
	if ! mv "${tmp}/LICENSE_1_0.txt" \
			"${staging_prefix}/share/licenses/boost/LICENSE_1_0.txt"; then
		rm -rf "$tmp"
		return 1
	fi
	rm -rf "$tmp"
	return 0
}

dep_entry "$@"

case "${DEP_TARGET:-$(dep_host_target)}" in
	linux-x86_64|linux-arm64|darwin-arm64|darwin-x86_64|wasm32-emscripten|windows-x86_64-mingw|windows-x86_64-msvc) ;;
	*)
		echo "dep-boost-headers: unsupported target '${DEP_TARGET}'" >&2
		exit 2
		;;
esac
dep_require_target_host dep-boost-headers "${DEP_TARGET:-$(dep_host_target)}"

mode="$(dep_var TAU_DEP_MODE producer)"
case "$mode" in
	producer|consumer) ;;
	*)
		echo "dep-boost-headers: -DTAU_DEP_MODE must be producer or consumer, got '${mode}'" >&2
		exit 2
		;;
esac

BOOST_URL="$(dep_var BOOST_URL "$BOOST_URL_DEFAULT")"
BOOST_SHA256="$(dep_var BOOST_SHA256 "$BOOST_SHA256_DEFAULT")"
BOOST_TARGET="${DEP_TARGET:-$(dep_host_target)}"

block="$(_dep_boost_field_block)" || {
	echo "dep-boost-headers: cannot build the identity field block" >&2
	exit 1
}

out=""
if ! dep_ensure out "$mode" boost_headers "$block" _dep_boost_producer; then
	echo "dep-boost-headers: ${mode} failed" >&2
	exit 1
fi

echo "dep-boost-headers: package prefix: ${out}"
