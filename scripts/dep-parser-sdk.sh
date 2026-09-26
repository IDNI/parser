#!/bin/bash
# Build and cache the native Linux parser packages in the LOCAL store.
#
#   ./dev dep-parser-sdk -DTAU_PARSER_PACKAGE=sdk -DTAU_BUILD_JOBS=5
#   ./dev dep-parser-sdk -DTAU_PARSER_PACKAGE=sdk -DTAU_PARSER_LTO=ON -DTAU_BUILD_JOBS=5
#   ./dev dep-parser-sdk -DTAU_PARSER_PACKAGE=sdk -DTAU_PARSER_LTO=ON \
#       -DTAU_PARSER_LTO_FAT=OFF -DTAU_BUILD_JOBS=5
#   ./dev dep-parser-sdk -DTAU_PARSER_PACKAGE=tgf -DTAU_BUILD_JOBS=5
#
# With -DTAU_PARSER_LTO=ON the default is fat LTO objects
# (-ffat-lto-objects), so a consumer that does not itself link with -flto can
# still link the archive. -DTAU_PARSER_LTO_FAT=OFF produces thin objects.
#   ./dev dep-parser-sdk -DTAU_PARSER_PACKAGE=sdk -DTAU_DEP_MODE=consumer
#
# Two packages share this script because they share the parser source tree and
# the identity of that tree:
#   sdk  the parser SDK: headers, libtauparser.a, tauparserConfig.cmake and the
#        package it exports. TAU_PARSER_INSTALL=ON, tgf not built.
#   tgf  the host tgf tool only (bin/tgf). A target build consumes this package
#        so it never compiles tgf itself. Keyed by the host triple and the
#        parser identity.
#
# The source is the local parser tree, so the identity records both the source
# commit and a content hash of the tree; uncommitted local changes travel in the
# tree hash. Nothing here reads the Tau commit, the Tau tree, or TAU_BAS.
#
# Only the native, cross-toolchain, macOS and MSVC tuples are produced; any
# other target or host is rejected.

set -u

DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

DEP_PARSER_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
PARSER_SOURCE_DEFAULT="${DEV_ROOT}"
FTXUI_DEP_SCRIPT="${DEV_ROOT}/scripts/dep-ftxui.sh"
UNORDERED_DENSE_DEP_SCRIPT="${DEV_ROOT}/scripts/dep-unordered-dense.sh"
PARSER_CXX_STANDARD=17

# SHA-256 over the working-tree content of every tracked and untracked,
# non-ignored file, in sorted order. Generated build output is gitignored and so
# stays out. This is the parser tree content hash of the plan.
#
# Excluded, because each is hashed into the id separately or is provenance and
# cannot change the produced bytes:
#   scripts/devrc               the publish-side driver: accepts, records, places,
#                               and points at a finished tree; provenance
#   scripts/dep-*.sh            the narrow build module (dep-build) is
#                               helper_build_hash; a dependency producer only
#                               moves its own package id or the recorded
#                               dependency id
#   cmake/tau-manifest.cmake    the manifest writer, provenance (runs after
#   cmake/tau-store.cmake       install); the store writer likewise
_dep_parser_tree_hash() {
	local src="$1" value digest
	local exclude='^(scripts/devrc|scripts/dep-.*\.sh|cmake/tau-manifest\.cmake|cmake/tau-store\.cmake)$'
	if command -v sha256sum > /dev/null 2>&1; then
		digest="sha256sum"
	elif command -v shasum > /dev/null 2>&1; then
		digest="shasum -a 256"
	else
		echo "dep-parser-sdk: neither sha256sum nor shasum is available" >&2
		return 1
	fi
	# BSD tools have no -z/--zero; NUL records become newline records for the
	# sort and back for xargs, which is exact for every path without a newline.
	value="$(cd "$src" && git ls-files -z --cached --others --exclude-standard \
		| tr '\0' '\n' | grep -vE "$exclude" | LC_ALL=C sort \
		| tr '\n' '\0' | xargs -0 -n 100 $digest | $digest | awk '{print $1}')"
	if [ -z "$value" ]; then
		echo "dep-parser-sdk: parser tree hash is empty" >&2
		return 1
	fi
	printf '%s' "$value"
}

_dep_parser_compiler_id() {
	dep_compiler_id "$DEP_PARSER_CXX"
}

# Resolve a dependency package through its own script and print its prefix. The
# dependency mode follows ours, so a consumer run never builds a dependency.
_dep_parser_resolve_dep() {
	local script="$1" mode="$2" out
	if ! out="$(bash "$script" -DTAU_DEP_MODE="$mode" -DTAU_BUILD_JOBS="$DEP_PARSER_JOBS" \
			-DTAU_DEP_CC="$DEP_PARSER_CC" -DTAU_DEP_CXX="$DEP_PARSER_CXX" \
			-DTAU_DEP_CFLAGS="$DEP_PARSER_CFLAGS" -DTAU_DEP_CXXFLAGS="$DEP_PARSER_CXXFLAGS" \
			-DTAU_DEP_TARGET="$DEP_PARSER_TARGET" -DTAU_DEP_TOOLCHAIN="$DEP_PARSER_TOOLCHAIN")"; then
		return 1
	fi
	printf '%s\n' "$out" | sed -n 's/.*package prefix: //p' | tail -n 1
}

_dep_parser_field_block() {
	local dep="$1" ftxui_id="$2" unordered_dense_id="$3"
	local build_helper publish_helper manifest store
	local recipe_hash build_hash publish_hash manifest_hash store_hash
	build_helper="${__devrc_dir}/dep-build"
	publish_helper="${__devrc_dir}/devrc"
	manifest="${__devrc_dir}/../cmake/tau-manifest.cmake"
	store="${__devrc_dir}/../cmake/tau-store.cmake"
	recipe_hash="$(dep_sha256 "$DEP_PARSER_RECIPE")" || return 1
	build_hash="$(dep_sha256 "$build_helper")" || return 1
	publish_hash="$(dep_sha256 "$publish_helper")" || return 1
	manifest_hash="$(dep_sha256 "$manifest")" || return 1
	store_hash="$(dep_sha256 "$store")" || return 1
	printf '%s\n' \
		"dep=${dep}" \
		"target=${DEP_PARSER_TARGET:-native}" \
		"parser_commit=${PARSER_COMMIT}" \
		"parser_tree_hash=${PARSER_TREE_HASH}" \
		"recipe_hash=${recipe_hash}" \
		"helper_build_hash=${build_hash}" \
		"provenance.publish_helper_hash=${publish_hash}" \
		"provenance.manifest_writer_hash=${manifest_hash}" \
		"provenance.store_writer_hash=${store_hash}" \
		"ftxui_package_id=${ftxui_id}" \
		"unordered_dense_package_id=${unordered_dense_id}" \
		"configure_args=${_DEP_PARSER_CONFIGURE_ARGS[*]}" \
		"cxx_standard=${PARSER_CXX_STANDARD}" \
		"cflags=${DEP_PARSER_CFLAGS}" \
		"cxxflags=${DEP_PARSER_CXXFLAGS}" \
		"cmake_path=${DEP_PARSER_CMAKE}" \
		"cmake_version=$("$DEP_PARSER_CMAKE" --version | head -n 1)" \
		"generator=Ninja" \
		"ninja_path=${DEP_PARSER_NINJA}" \
		"ninja_version=$("$DEP_PARSER_NINJA" --version)" \
		"compiler_id=$(_dep_parser_compiler_id)" \
		"compiler_version=$(dep_compiler_version "$DEP_PARSER_CXX")" \
		"target_triple=$(dep_compiler_triple "$DEP_PARSER_CXX" "${DEP_PARSER_TARGET:-native}")" \
		"os=$(uname -s)" \
		"arch=$(uname -m)" \
		"os_release_hash=$(dep_os_release_hash)" \
		"libc_version=$(dep_libc_version)" \
		"build_type=Release" \
		"pic=ON" \
		"lto=${PARSER_LTO}" \
		"lto_fat=${PARSER_LTO_FAT_EFFECTIVE}" \
		"sanitizer=OFF" \
		"file_prefix_map=parser-src;parser-build;staging"
}

# Producer callback. $1 is the staging prefix, $2 the FTXUI prefix, $3 the
# unordered_dense prefix. The parser is configured and built out of tree; the
# generated parser files land in the source tree, which the install reads.
_dep_parser_producer() {
	local staging_prefix="$1" ftxui_prefix="$2" unordered_dense_prefix="$3"
	local staging work build target component prefix_map
	staging="$(dirname "$staging_prefix")"
	work="${staging}/work"
	build="${work}/build"
	# The staging work tree is random per build, so it cannot enter the identity.
	# The real paths go into the compiler flags; the identity records the fixed
	# placeholder each role maps to. MSVC has no flag-encoded path map, so it
	# relies on the install-time rewrite below instead.
	prefix_map=""
	if [ "$(dep_compiler_id "$DEP_PARSER_CXX")" != "MSVC" ]; then
		prefix_map="-ffile-prefix-map=${build}=parser-build -ffile-prefix-map=${work}=parser-src -ffile-prefix-map=${PARSER_SOURCE}=parser-src -ffile-prefix-map=${staging_prefix}=staging -ffile-prefix-map=${staging}=staging"
	fi
	rm -rf "$work"
	mkdir -p "$work"
	if [ "$PARSER_PACKAGE" = "tgf" ]; then
		target=tgf
		component=tgf
	else
		target=tauparser_static
		component=""
	fi
	env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
		"$DEP_PARSER_CMAKE" -S "$PARSER_SOURCE" -B "$build" \
		"${_DEP_PARSER_CONFIGURE_ARGS[@]}" \
		"${_DEP_PARSER_HOST_ARGS[@]}" \
		"-DCMAKE_C_COMPILER=$DEP_PARSER_CC" \
		"-DCMAKE_CXX_COMPILER=$DEP_PARSER_CXX" \
		"-DCMAKE_CXX_FLAGS=${PARSER_CONFIG_CXX_FLAGS} ${prefix_map}" \
		"-DTAU_BUILD_JOBS=$DEP_PARSER_JOBS" \
		"-DTAU_PARSER_FTXUI_PREFIX=$ftxui_prefix" \
		"-DTAU_PARSER_UNORDERED_DENSE_PREFIX=$unordered_dense_prefix" \
		-DCMAKE_INSTALL_PREFIX="$staging_prefix" \
		|| { rm -rf "$work"; return 1; }
	env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
		"$DEP_PARSER_CMAKE" --build "$build" --target "$target" -- -j "$DEP_PARSER_JOBS" \
		|| { rm -rf "$work"; return 1; }
	if [ -n "$component" ]; then
		env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
			"$DEP_PARSER_CMAKE" --install "$build" --component "$component" \
			|| { rm -rf "$work"; return 1; }
	else
		env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
			"$DEP_PARSER_CMAKE" --install "$build" \
			|| { rm -rf "$work"; return 1; }
	fi
	# GCC's LTO metadata keeps the compiler working directory for inlined
	# template locations and does not apply -fdebug-prefix-map there, so a few
	# byte-length-prefixed copies of the build tree survive. Rewrite those exact
	# bytes to an equal-length placeholder: the value is diagnostic only, and an
	# equal length keeps the LTO records valid. Do this only for the archive,
	# which is the only installed file that can carry LTO IR.
	local archive
	for archive in "$staging_prefix/lib/libtauparser.a" \
			"$staging_prefix/lib/tauparser.lib"; do
		[ -f "$archive" ] || continue
		if ! python3 - "$archive" "$build" "$staging_prefix" "$PARSER_SOURCE" \
			"$work" "$staging" <<'PY'
import sys
path, *olds = sys.argv[1:]
data = open(path, 'rb').read()
for old in sorted(olds, key=len, reverse=True):
	if not old:
		continue
	b = old.encode()
	if b in data:
		data = data.replace(b, b'@' * len(b))
open(path, 'wb').write(data)
PY
		then
			echo "dep-parser-sdk: cannot rewrite LTO build paths in ${archive}" >&2
			rm -rf "$work"
			return 1
		fi
	done
	# The parser install puts LICENSE.md under share/doc; also record it as the
	# package license, like the other dependencies.
	if [ -f "$PARSER_SOURCE/LICENSE.md" ]; then
		mkdir -p "$staging_prefix/share/licenses/${DEP_PARSER_NAME}"
		cp "$PARSER_SOURCE/LICENSE.md" \
			"$staging_prefix/share/licenses/${DEP_PARSER_NAME}/LICENSE.md"
	else
		echo "dep-parser-sdk: no LICENSE.md in the parser source" >&2
		rm -rf "$work"
		return 1
	fi
	rm -rf "$work"
	return 0
}

dep_entry "$@"

case "${DEP_TARGET:-native}" in
	native|w64|wasm|darwin-arm64|darwin-x86_64|win-msvc-x64) ;;
	*)
		echo "dep-parser-sdk: unsupported target '${DEP_TARGET}'" >&2
		exit 2
		;;
esac
dep_require_target_host dep-parser-sdk "${DEP_TARGET:-native}"

mode="$(dep_var TAU_DEP_MODE producer)"
case "$mode" in
	producer|consumer) ;;
	*)
		echo "dep-parser-sdk: -DTAU_DEP_MODE must be producer or consumer, got '${mode}'" >&2
		exit 2
		;;
esac

PARSER_PACKAGE="$(dep_var TAU_PARSER_PACKAGE sdk)"
case "$PARSER_PACKAGE" in
	sdk)  DEP_PARSER_NAME="parser-sdk" ;;
	tgf)  DEP_PARSER_NAME="tgf" ;;
	*)
		echo "dep-parser-sdk: -DTAU_PARSER_PACKAGE must be sdk or tgf, got '${PARSER_PACKAGE}'" >&2
		exit 2
		;;
esac

PARSER_SOURCE="$(dep_var PARSER_SOURCE "$PARSER_SOURCE_DEFAULT")"
PARSER_LTO="$(dep_var TAU_PARSER_LTO OFF)"
case "$PARSER_LTO" in
	ON|OFF) ;;
	*)
		echo "dep-parser-sdk: -DTAU_PARSER_LTO must be ON or OFF, got '${PARSER_LTO}'" >&2
		exit 2
		;;
esac
PARSER_LTO_FAT="$(dep_var TAU_PARSER_LTO_FAT ON)"
case "$PARSER_LTO_FAT" in
	ON|OFF) ;;
	*)
		echo "dep-parser-sdk: -DTAU_PARSER_LTO_FAT must be ON or OFF, got '${PARSER_LTO_FAT}'" >&2
		exit 2
		;;
esac
# Fat LTO objects only exist with LTO on. With LTO off the recorded value is
# OFF, because no LTO object is produced.
if [ "$PARSER_LTO" = "ON" ]; then
	PARSER_LTO_FAT_EFFECTIVE="$PARSER_LTO_FAT"
else
	PARSER_LTO_FAT_EFFECTIVE=OFF
fi
if [ ! -d "$PARSER_SOURCE/.git" ] && [ ! -f "$PARSER_SOURCE/.git" ]; then
	echo "dep-parser-sdk: PARSER_SOURCE is not a git work tree: '${PARSER_SOURCE}'" >&2
	exit 2
fi
PARSER_SOURCE="$(cd "$PARSER_SOURCE" && pwd)"

PARSER_COMMIT_DEFAULT="$(git -C "$PARSER_SOURCE" rev-parse HEAD)"
PARSER_COMMIT="$(dep_var PARSER_COMMIT "$PARSER_COMMIT_DEFAULT")"
case "$PARSER_COMMIT" in
	*[!0-9a-f]*|"")
		echo "dep-parser-sdk: PARSER_COMMIT must be 40 lowercase hex characters, got '${PARSER_COMMIT}'" >&2
		exit 2
		;;
esac
if [ "${#PARSER_COMMIT}" -ne 40 ]; then
	echo "dep-parser-sdk: PARSER_COMMIT must be 40 lowercase hex characters, got '${PARSER_COMMIT}'" >&2
	exit 2
fi
if [ "$PARSER_COMMIT" != "$PARSER_COMMIT_DEFAULT" ]; then
	echo "dep-parser-sdk: PARSER_COMMIT ${PARSER_COMMIT} is not the local parser HEAD ${PARSER_COMMIT_DEFAULT}" >&2
	exit 2
fi

PARSER_TREE_HASH="$(_dep_parser_tree_hash "$PARSER_SOURCE")" || exit 1
DEP_PARSER_JOBS="$(dep_jobs)"

DEP_PARSER_CMAKE="$(command -v "${CMAKE:-cmake}")" \
	|| { echo "dep-parser-sdk: cmake not found" >&2; exit 2; }
DEP_PARSER_NINJA="$(command -v ninja)" \
	|| { echo "dep-parser-sdk: ninja not found" >&2; exit 2; }
DEP_PARSER_CC="$(dep_var TAU_DEP_CC "")"
DEP_PARSER_CXX="$(dep_var TAU_DEP_CXX "")"
if [ -z "$DEP_PARSER_CC" ] || [ -z "$DEP_PARSER_CXX" ]; then
	echo "dep-parser-sdk: no compiler; pass -DTAU_DEP_CC and -DTAU_DEP_CXX" >&2
	exit 2
fi
DEP_PARSER_CFLAGS="$(dep_var TAU_DEP_CFLAGS "")"
DEP_PARSER_CXXFLAGS="$(dep_var TAU_DEP_CXXFLAGS "")"
DEP_PARSER_TARGET="${DEP_TARGET:-native}"
DEP_PARSER_TOOLCHAIN="$(dep_var TAU_DEP_TOOLCHAIN "")"
_DEP_PARSER_TOOLCHAIN_ARGS=()
if dep_target_needs_toolchain "$DEP_PARSER_TARGET"; then
	if [ -z "$DEP_PARSER_TOOLCHAIN" ]; then
		echo "dep-parser-sdk: ${DEP_PARSER_TARGET} needs -DTAU_DEP_TOOLCHAIN" >&2
		exit 2
	fi
	_DEP_PARSER_TOOLCHAIN_ARGS=(-DCMAKE_TOOLCHAIN_FILE="$DEP_PARSER_TOOLCHAIN")
fi
_DEP_PARSER_TARGET_ARGS=()
DEP_PARSER_HOST_CC="$(dep_var TAU_PARSER_HOST_C_COMPILER "")"
DEP_PARSER_HOST_CXX="$(dep_var TAU_PARSER_HOST_CXX_COMPILER "")"
_DEP_PARSER_HOST_ARGS=()
if [ -n "$DEP_PARSER_HOST_CC" ]; then
	_DEP_PARSER_HOST_ARGS+=("-DTAU_PARSER_HOST_C_COMPILER=$DEP_PARSER_HOST_CC")
fi
if [ -n "$DEP_PARSER_HOST_CXX" ]; then
	_DEP_PARSER_HOST_ARGS+=("-DTAU_PARSER_HOST_CXX_COMPILER=$DEP_PARSER_HOST_CXX")
fi

# Resolve the consumed dependency packages. The ids are part of this identity.
if ! FTXUI_PREFIX="$(_dep_parser_resolve_dep "$FTXUI_DEP_SCRIPT" "$mode")"; then
	echo "dep-parser-sdk: cannot resolve the ftxui package" >&2
	exit 1
fi
if ! UNORDERED_DENSE_PREFIX="$(_dep_parser_resolve_dep "$UNORDERED_DENSE_DEP_SCRIPT" "$mode")"; then
	echo "dep-parser-sdk: cannot resolve the unordered_dense package" >&2
	exit 1
fi
if [ -z "$FTXUI_PREFIX" ] || [ -z "$UNORDERED_DENSE_PREFIX" ]; then
	echo "dep-parser-sdk: a dependency package resolved to an empty prefix" >&2
	exit 1
fi
FTXUI_PACKAGE_ID="$(basename "$(dirname "$FTXUI_PREFIX")")"
UNORDERED_DENSE_PACKAGE_ID="$(basename "$(dirname "$UNORDERED_DENSE_PREFIX")")"

# Thin LTO comes from CMake's IPO flags, which force -fno-fat-lto-objects. Fat
# LTO comes from the parser's own -flto=auto plus -ffat-lto-objects in the CXX
# flags, with CMake IPO off so its -fno-fat-lto-objects cannot override it.
PARSER_LTO_IPO="$PARSER_LTO"
PARSER_LTO_FAT_FLAGS=""
if [ "$PARSER_LTO_FAT_EFFECTIVE" = "ON" ]; then
	PARSER_LTO_IPO=OFF
	# AppleClang rejects -ffat-lto-objects; the package it produces is
	# thin, so the recorded lto_fat must say so or the id would lie.
	if [ "$(_dep_parser_compiler_id)" = "AppleClang" ]; then
		PARSER_LTO_FAT_EFFECTIVE=OFF
	else
		PARSER_LTO_FAT_FLAGS="-ffat-lto-objects"
	fi
fi
PARSER_CONFIG_CXX_FLAGS="$(printf '%s %s' "$DEP_PARSER_CXXFLAGS" \
	"$PARSER_LTO_FAT_FLAGS" | sed 's/^ *//; s/ *$//')"

if [ "$PARSER_PACKAGE" = "tgf" ]; then
	PARSER_BUILD_OPTIONS=(
		-DTAU_PARSER_BUILD_STATIC_LIBRARY=ON
		-DTAU_PARSER_BUILD_TGF=ON
	)
else
	PARSER_BUILD_OPTIONS=(
		-DTAU_PARSER_BUILD_STATIC_LIBRARY=ON
		-DTAU_PARSER_BUILD_TGF=OFF
	)
fi

_DEP_PARSER_CONFIGURE_ARGS=(
	-G Ninja
	-DCMAKE_BUILD_TYPE=Release
	-DCMAKE_CXX_STANDARD="$PARSER_CXX_STANDARD"
	-DCMAKE_CXX_STANDARD_REQUIRED=ON
	-DCMAKE_CXX_EXTENSIONS=OFF
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON
	-DCMAKE_INTERPROCEDURAL_OPTIMIZATION="$PARSER_LTO_IPO"
	-DTAU_PARSER_LTO="$PARSER_LTO"
	-DCMAKE_C_FLAGS="$DEP_PARSER_CFLAGS"
	-DCMAKE_CXX_FLAGS="$PARSER_CONFIG_CXX_FLAGS"
	-DTAU_PARSER_INSTALL=ON
	-DTAU_PARSER_BUILD_TESTS=OFF
	-DTAU_PARSER_BUILD_EXAMPLES=OFF
	-DTAU_PARSER_BUILD_DOC=OFF
	-DFETCHCONTENT_FULLY_DISCONNECTED=ON
	"${_DEP_PARSER_TOOLCHAIN_ARGS[@]}"
	"${_DEP_PARSER_TARGET_ARGS[@]}"
	"${PARSER_BUILD_OPTIONS[@]}"
)

block="$(_dep_parser_field_block "$DEP_PARSER_NAME" \
	"$FTXUI_PACKAGE_ID" "$UNORDERED_DENSE_PACKAGE_ID")" || {
	echo "dep-parser-sdk: cannot build the identity field block" >&2
	exit 1
}

out=""
if ! dep_ensure out "$mode" "$DEP_PARSER_NAME" "$block" _dep_parser_producer \
		"$FTXUI_PREFIX" "$UNORDERED_DENSE_PREFIX"; then
	echo "dep-parser-sdk: ${mode} failed for ${DEP_PARSER_NAME}" >&2
	exit 1
fi

echo "dep-parser-sdk: ${DEP_PARSER_NAME} package prefix: ${out}"
