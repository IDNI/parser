#!/bin/bash
# Build and cache the native Linux FTXUI package in the LOCAL store.
#
#   ./dev dep-ftxui -DTAU_BUILD_JOBS=5
#   ./dev dep-ftxui -DTAU_DEP_MODE=consumer -DTAU_BUILD_JOBS=5
#
# The source is pinned to one immutable commit. Nothing here moves a tag.
# Producer mode builds into a staging entry and publishes it. Consumer mode
# only looks up an existing entry and never clones, configures, or builds.
# Only the native, cross-toolchain, macOS and MSVC tuples are produced; any
# other target or host is rejected.

set -u

DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

DEP_FTXUI_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
FTXUI_DEFAULT_REPO="https://github.com/ArthurSonzogni/FTXUI.git"
# Full commit for tag v6.1.9. Never build the moving tag.
FTXUI_DEFAULT_COMMIT="5cfed50702f52d51c1b189b5f97f8beaf5eaa2a6"
# FTXUI's libraries declare cxx_std_17. Pin it so the standard is an input.
FTXUI_CXX_STANDARD=17

_dep_ftxui_compiler_id() {
	dep_compiler_id "$DEP_FTXUI_CXX"
}

_dep_ftxui_field_block() {
	local build_helper publish_helper manifest store
	local recipe_hash build_hash publish_hash manifest_hash store_hash
	build_helper="${__devrc_dir}/dep-build"
	publish_helper="${__devrc_dir}/devrc"
	manifest="${__devrc_dir}/../cmake/tau-manifest.cmake"
	store="${__devrc_dir}/../cmake/tau-store.cmake"
	recipe_hash="$(dep_sha256 "$DEP_FTXUI_RECIPE")" || return 1
	build_hash="$(dep_sha256 "$build_helper")" || return 1
	publish_hash="$(dep_sha256 "$publish_helper")" || return 1
	manifest_hash="$(dep_sha256 "$manifest")" || return 1
	store_hash="$(dep_sha256 "$store")" || return 1
	printf '%s\n' \
		"dep=ftxui" \
		"target=${DEP_FTXUI_TARGET:-native}" \
		"repo=${FTXUI_REPO}" \
		"commit=${FTXUI_COMMIT}" \
		"recipe_hash=${recipe_hash}" \
		"helper_build_hash=${build_hash}" \
		"provenance.publish_helper_hash=${publish_hash}" \
		"provenance.manifest_writer_hash=${manifest_hash}" \
		"provenance.store_writer_hash=${store_hash}" \
		"configure_args=${_DEP_FTXUI_CONFIGURE_ARGS[*]}" \
		"cxx_standard=${FTXUI_CXX_STANDARD}" \
		"cflags=${DEP_FTXUI_CFLAGS}" \
		"cxxflags=${DEP_FTXUI_CXXFLAGS}" \
		"cmake_path=${DEP_FTXUI_CMAKE}" \
		"cmake_version=$("$DEP_FTXUI_CMAKE" --version | head -n 1)" \
		"generator=Ninja" \
		"ninja_path=${DEP_FTXUI_NINJA}" \
		"ninja_version=$("$DEP_FTXUI_NINJA" --version)" \
		"compiler_id=$(_dep_ftxui_compiler_id)" \
		"compiler_version=$(dep_compiler_version "$DEP_FTXUI_CXX")" \
		"target_triple=$(dep_compiler_triple "$DEP_FTXUI_CXX" "${DEP_FTXUI_TARGET:-native}")" \
		"os=$(uname -s)" \
		"arch=$(uname -m)" \
		"os_release_hash=$(dep_os_release_hash)" \
		"libc_version=$(dep_libc_version)" \
		"build_type=Release" \
		"pic=ON" \
		"lto=OFF" \
		"sanitizer=OFF" \
		"deps=none"
}

# Producer callback. $1 is the staging prefix. The source tree and build tree
# live beside it, outside the package prefix, and are removed before success.
_dep_ftxui_producer() {
	local staging_prefix="$1"
	local staging work build head
	staging="$(dirname "$staging_prefix")"
	work="${staging}/work"
	build="${work}/build"
	rm -rf "$work"
	mkdir -p "$work" "$staging_prefix"
	git init -q "$work" || { rm -rf "$work"; return 1; }
	git -C "$work" remote add origin "$FTXUI_REPO" || { rm -rf "$work"; return 1; }
	git -C "$work" fetch -q --depth 1 origin "$FTXUI_COMMIT" \
		|| { rm -rf "$work"; return 1; }
	git -C "$work" checkout -q FETCH_HEAD || { rm -rf "$work"; return 1; }
	head="$(git -C "$work" rev-parse HEAD)"
	if [ "$head" != "$FTXUI_COMMIT" ]; then
		echo "dep-ftxui: checkout is ${head}, expected ${FTXUI_COMMIT}" >&2
		rm -rf "$work"
		return 1
	fi
	# The recorded flags travel in the configure arguments. Clear the ambient
	# ones so the environment cannot add a second, unrecorded value.
	env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
		"$DEP_FTXUI_CMAKE" -S "$work" -B "$build" \
		"${_DEP_FTXUI_CONFIGURE_ARGS[@]}" \
		"${_DEP_FTXUI_TOOLCHAIN_ARGS[@]}" \
		-DCMAKE_C_COMPILER="$DEP_FTXUI_CC" \
		-DCMAKE_CXX_COMPILER="$DEP_FTXUI_CXX" \
		-DCMAKE_INSTALL_PREFIX="$staging_prefix" \
		|| { rm -rf "$work"; return 1; }
	env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
		"$DEP_FTXUI_CMAKE" --build "$build" -- -j "$FTXUI_JOBS" \
		|| { rm -rf "$work"; return 1; }
	env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
		"$DEP_FTXUI_CMAKE" --install "$build" \
		|| { rm -rf "$work"; return 1; }
	# The pkg-config file is generated with the staging prefix baked in. Derive
	# every directory from the file's own location instead, so a relocated
	# package resolves itself.
	local pc="$staging_prefix/lib/pkgconfig/ftxui.pc"
	if [ -f "$pc" ]; then
		dep_sed_inplace "$pc" \
			-e 's|^prefix=.*|prefix=${pcfiledir}/../..|' \
			-e 's|^libdir=.*|libdir=${prefix}/lib|' \
			-e 's|^includedir=.*|includedir=${prefix}/include|'
	fi
	# Install the FTXUI license at its own destination, or fail.
	if [ ! -f "$staging_prefix/share/licenses/FTXUI/LICENSE" ]; then
		if [ -f "$work/LICENSE" ]; then
			mkdir -p "$staging_prefix/share/licenses/FTXUI"
			cp "$work/LICENSE" "$staging_prefix/share/licenses/FTXUI/LICENSE"
		else
			echo "dep-ftxui: no FTXUI license found to install" >&2
			rm -rf "$work"
			return 1
		fi
	fi
	rm -rf "$work"
	return 0
}

dep_entry "$@"

case "${DEP_TARGET:-native}" in
	native|w64|wasm|darwin-arm64|darwin-x86_64|win-msvc-x64) ;;
	*)
		echo "dep-ftxui: unsupported target '${DEP_TARGET}'" >&2
		exit 2
		;;
esac
dep_require_target_host dep-ftxui "${DEP_TARGET:-native}"

mode="$(dep_var TAU_DEP_MODE producer)"
case "$mode" in
	producer|consumer) ;;
	*)
		echo "dep-ftxui: -DTAU_DEP_MODE must be producer or consumer, got '${mode}'" >&2
		exit 2
		;;
esac

FTXUI_REPO="$(dep_var FTXUI_REPO "$FTXUI_DEFAULT_REPO")"
FTXUI_COMMIT="$(dep_var FTXUI_COMMIT "$FTXUI_DEFAULT_COMMIT")"
case "$FTXUI_COMMIT" in
	*[!0-9a-f]*|"")
		echo "dep-ftxui: FTXUI_COMMIT must be 40 lowercase hex characters, got '${FTXUI_COMMIT}'" >&2
		exit 2
		;;
esac
if [ "${#FTXUI_COMMIT}" -ne 40 ]; then
	echo "dep-ftxui: FTXUI_COMMIT must be 40 lowercase hex characters, got '${FTXUI_COMMIT}'" >&2
	exit 2
fi
FTXUI_JOBS="$(dep_jobs)"

DEP_FTXUI_CMAKE="$(command -v "${CMAKE:-cmake}")" \
	|| { echo "dep-ftxui: cmake not found" >&2; exit 2; }
DEP_FTXUI_NINJA="$(command -v ninja)" \
	|| { echo "dep-ftxui: ninja not found" >&2; exit 2; }
DEP_FTXUI_CC="$(dep_var TAU_DEP_CC "")"
DEP_FTXUI_CXX="$(dep_var TAU_DEP_CXX "")"
if [ -z "$DEP_FTXUI_CC" ] || [ -z "$DEP_FTXUI_CXX" ]; then
	echo "dep-ftxui: no compiler; pass -DTAU_DEP_CC and -DTAU_DEP_CXX" >&2
	exit 2
fi
DEP_FTXUI_CFLAGS="$(dep_var TAU_DEP_CFLAGS "")"
DEP_FTXUI_CXXFLAGS="$(dep_var TAU_DEP_CXXFLAGS "")"
DEP_FTXUI_TARGET="${DEP_TARGET:-native}"
DEP_FTXUI_TOOLCHAIN="$(dep_var TAU_DEP_TOOLCHAIN "")"
_DEP_FTXUI_TOOLCHAIN_ARGS=()
if dep_target_needs_toolchain "$DEP_FTXUI_TARGET"; then
	if [ -z "$DEP_FTXUI_TOOLCHAIN" ]; then
		echo "dep-ftxui: ${DEP_FTXUI_TARGET} needs -DTAU_DEP_TOOLCHAIN" >&2
		exit 2
	fi
	_DEP_FTXUI_TOOLCHAIN_ARGS=(-DCMAKE_TOOLCHAIN_FILE="$DEP_FTXUI_TOOLCHAIN")
fi

_DEP_FTXUI_CONFIGURE_ARGS=(
	-G Ninja
	-DCMAKE_BUILD_TYPE=Release
	-DCMAKE_CXX_STANDARD="$FTXUI_CXX_STANDARD"
	-DCMAKE_CXX_STANDARD_REQUIRED=ON
	-DCMAKE_CXX_EXTENSIONS=OFF
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON
	-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF
	-DCMAKE_C_FLAGS="$DEP_FTXUI_CFLAGS"
	-DCMAKE_CXX_FLAGS="$DEP_FTXUI_CXXFLAGS"
	-DFTXUI_BUILD_EXAMPLES=OFF
	-DFTXUI_BUILD_DOCS=OFF
	-DFTXUI_BUILD_TESTS=OFF
	-DFTXUI_BUILD_TESTS_FUZZER=OFF
	-DFTXUI_ENABLE_INSTALL=ON
)

block="$(_dep_ftxui_field_block)" || {
	echo "dep-ftxui: cannot build the identity field block" >&2
	exit 1
}

out=""
if ! dep_ensure out "$mode" ftxui "$block" _dep_ftxui_producer; then
	echo "dep-ftxui: ${mode} failed" >&2
	exit 1
fi

echo "dep-ftxui: package prefix: ${out}"
