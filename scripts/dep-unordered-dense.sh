#!/bin/bash
# Build and cache the native Linux ankerl/unordered_dense package in the LOCAL store.
#
#   ./dev dep-unordered-dense -DTAU_BUILD_JOBS=5
#   ./dev dep-unordered-dense -DTAU_DEP_MODE=consumer -DTAU_BUILD_JOBS=5
#
# The source is pinned to one immutable commit. Nothing here moves a tag.
# Producer mode builds into a staging entry and publishes it. Consumer mode only
# looks up an existing entry and never clones, configures, or builds.
#
# The library is header-only. The package is the upstream install: the ankerl
# headers plus unordered_denseConfig.cmake / unordered_denseTargets.cmake, which
# consumers reach with find_package(unordered_dense CONFIG). The upstream install
# omits its LICENSE, so the producer installs it.
#
# Only the native, cross-toolchain, macOS and MSVC tuples are produced; any
# other target or host is rejected.

set -u

DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

DEP_UNORDERED_DENSE_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
UNORDERED_DENSE_DEFAULT_REPO="https://github.com/martinus/unordered_dense.git"
# Full commit for tag v4.4.0. Never build the moving tag.
UNORDERED_DENSE_DEFAULT_COMMIT="231e48c9426bd21c273669e5fdcd042c146975cf"
# upstream sets cxx_std_17 on the interface. Pin it so the standard is an input.
UNORDERED_DENSE_CXX_STANDARD=17

_dep_unordered_dense_compiler_id() {
	dep_compiler_id "$DEP_UNORDERED_DENSE_CXX"
}

_dep_unordered_dense_field_block() {
	local build_helper publish_helper manifest store
	local recipe_hash build_hash publish_hash manifest_hash store_hash
	build_helper="${__devrc_dir}/dep-build"
	publish_helper="${__devrc_dir}/devrc"
	manifest="${__devrc_dir}/../cmake/tau-manifest.cmake"
	store="${__devrc_dir}/../cmake/tau-store.cmake"
	recipe_hash="$(dep_sha256 "$DEP_UNORDERED_DENSE_RECIPE")" || return 1
	build_hash="$(dep_sha256 "$build_helper")" || return 1
	publish_hash="$(dep_sha256 "$publish_helper")" || return 1
	manifest_hash="$(dep_sha256 "$manifest")" || return 1
	store_hash="$(dep_sha256 "$store")" || return 1
	printf '%s\n' \
		"dep=unordered_dense" \
		"target=${DEP_UNORDERED_DENSE_TARGET:-native}" \
		"repo=${UNORDERED_DENSE_REPO}" \
		"commit=${UNORDERED_DENSE_COMMIT}" \
		"recipe_hash=${recipe_hash}" \
		"helper_build_hash=${build_hash}" \
		"provenance.publish_helper_hash=${publish_hash}" \
		"provenance.manifest_writer_hash=${manifest_hash}" \
		"provenance.store_writer_hash=${store_hash}" \
		"configure_args=${_DEP_UNORDERED_DENSE_CONFIGURE_ARGS[*]}" \
		"cxx_standard=${UNORDERED_DENSE_CXX_STANDARD}" \
		"cflags=${DEP_UNORDERED_DENSE_CFLAGS}" \
		"cxxflags=${DEP_UNORDERED_DENSE_CXXFLAGS}" \
		"cmake_path=${DEP_UNORDERED_DENSE_CMAKE}" \
		"cmake_version=$("$DEP_UNORDERED_DENSE_CMAKE" --version | head -n 1)" \
		"generator=Ninja" \
		"ninja_path=${DEP_UNORDERED_DENSE_NINJA}" \
		"ninja_version=$("$DEP_UNORDERED_DENSE_NINJA" --version)" \
		"compiler_id=$(_dep_unordered_dense_compiler_id)" \
		"compiler_version=$(dep_compiler_version "$DEP_UNORDERED_DENSE_CXX")" \
		"target_triple=$(dep_compiler_triple "$DEP_UNORDERED_DENSE_CXX" "${DEP_UNORDERED_DENSE_TARGET:-native}")" \
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
_dep_unordered_dense_producer() {
	local staging_prefix="$1"
	local staging work build head
	staging="$(dirname "$staging_prefix")"
	work="${staging}/work"
	build="${work}/build"
	rm -rf "$work"
	mkdir -p "$work" "$staging_prefix"
	git init -q "$work" || { rm -rf "$work"; return 1; }
	git -C "$work" remote add origin "$UNORDERED_DENSE_REPO" || { rm -rf "$work"; return 1; }
	git -C "$work" fetch -q --depth 1 origin "$UNORDERED_DENSE_COMMIT" \
		|| { rm -rf "$work"; return 1; }
	git -C "$work" checkout -q FETCH_HEAD || { rm -rf "$work"; return 1; }
	head="$(git -C "$work" rev-parse HEAD)"
	if [ "$head" != "$UNORDERED_DENSE_COMMIT" ]; then
		echo "dep-unordered-dense: checkout is ${head}, expected ${UNORDERED_DENSE_COMMIT}" >&2
		rm -rf "$work"
		return 1
	fi
	# The recorded flags travel in the configure arguments. Clear the ambient
	# ones so the environment cannot add a second, unrecorded value.
	env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
		"$DEP_UNORDERED_DENSE_CMAKE" -S "$work" -B "$build" \
		"${_DEP_UNORDERED_DENSE_CONFIGURE_ARGS[@]}" \
		"${_DEP_UNORDERED_DENSE_TOOLCHAIN_ARGS[@]}" \
		-DCMAKE_C_COMPILER="$DEP_UNORDERED_DENSE_CC" \
		-DCMAKE_CXX_COMPILER="$DEP_UNORDERED_DENSE_CXX" \
		-DCMAKE_INSTALL_PREFIX="$staging_prefix" \
		|| { rm -rf "$work"; return 1; }
	env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
		"$DEP_UNORDERED_DENSE_CMAKE" --build "$build" -- -j "$UNORDERED_DENSE_JOBS" \
		|| { rm -rf "$work"; return 1; }
	env -u CPPFLAGS -u CXXFLAGS -u CFLAGS -u LDFLAGS \
		"$DEP_UNORDERED_DENSE_CMAKE" --install "$build" \
		|| { rm -rf "$work"; return 1; }
	# Install the upstream license, or fail: the upstream install omits it.
	if [ ! -f "$staging_prefix/share/licenses/unordered_dense/LICENSE" ]; then
		if [ -f "$work/LICENSE" ]; then
			mkdir -p "$staging_prefix/share/licenses/unordered_dense"
			cp "$work/LICENSE" "$staging_prefix/share/licenses/unordered_dense/LICENSE"
		else
			echo "dep-unordered-dense: no upstream license found to install" >&2
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
		echo "dep-unordered-dense: unsupported target '${DEP_TARGET}'" >&2
		exit 2
		;;
esac
dep_require_target_host dep-unordered-dense "${DEP_TARGET:-native}"

mode="$(dep_var TAU_DEP_MODE producer)"
case "$mode" in
	producer|consumer) ;;
	*)
		echo "dep-unordered-dense: -DTAU_DEP_MODE must be producer or consumer, got '${mode}'" >&2
		exit 2
		;;
esac

UNORDERED_DENSE_REPO="$(dep_var UNORDERED_DENSE_REPO "$UNORDERED_DENSE_DEFAULT_REPO")"
UNORDERED_DENSE_COMMIT="$(dep_var UNORDERED_DENSE_COMMIT "$UNORDERED_DENSE_DEFAULT_COMMIT")"
case "$UNORDERED_DENSE_COMMIT" in
	*[!0-9a-f]*|"")
		echo "dep-unordered-dense: UNORDERED_DENSE_COMMIT must be 40 lowercase hex characters, got '${UNORDERED_DENSE_COMMIT}'" >&2
		exit 2
		;;
esac
if [ "${#UNORDERED_DENSE_COMMIT}" -ne 40 ]; then
	echo "dep-unordered-dense: UNORDERED_DENSE_COMMIT must be 40 lowercase hex characters, got '${UNORDERED_DENSE_COMMIT}'" >&2
	exit 2
fi
UNORDERED_DENSE_JOBS="$(dep_jobs)"

DEP_UNORDERED_DENSE_CMAKE="$(command -v "${CMAKE:-cmake}")" \
	|| { echo "dep-unordered-dense: cmake not found" >&2; exit 2; }
DEP_UNORDERED_DENSE_NINJA="$(command -v ninja)" \
	|| { echo "dep-unordered-dense: ninja not found" >&2; exit 2; }
DEP_UNORDERED_DENSE_CC="$(dep_var TAU_DEP_CC "")"
DEP_UNORDERED_DENSE_CXX="$(dep_var TAU_DEP_CXX "")"
if [ -z "$DEP_UNORDERED_DENSE_CC" ] || [ -z "$DEP_UNORDERED_DENSE_CXX" ]; then
	echo "dep-unordered-dense: no compiler; pass -DTAU_DEP_CC and -DTAU_DEP_CXX" >&2
	exit 2
fi
DEP_UNORDERED_DENSE_CFLAGS="$(dep_var TAU_DEP_CFLAGS "")"
DEP_UNORDERED_DENSE_CXXFLAGS="$(dep_var TAU_DEP_CXXFLAGS "")"
DEP_UNORDERED_DENSE_TARGET="${DEP_TARGET:-native}"
DEP_UNORDERED_DENSE_TOOLCHAIN="$(dep_var TAU_DEP_TOOLCHAIN "")"
_DEP_UNORDERED_DENSE_TOOLCHAIN_ARGS=()
if dep_target_needs_toolchain "$DEP_UNORDERED_DENSE_TARGET"; then
	if [ -z "$DEP_UNORDERED_DENSE_TOOLCHAIN" ]; then
		echo "dep-unordered-dense: ${DEP_UNORDERED_DENSE_TARGET} needs -DTAU_DEP_TOOLCHAIN" >&2
		exit 2
	fi
	_DEP_UNORDERED_DENSE_TOOLCHAIN_ARGS=(
		-DCMAKE_TOOLCHAIN_FILE="$DEP_UNORDERED_DENSE_TOOLCHAIN")
fi

_DEP_UNORDERED_DENSE_CONFIGURE_ARGS=(
	-G Ninja
	-DCMAKE_BUILD_TYPE=Release
	-DCMAKE_CXX_STANDARD="$UNORDERED_DENSE_CXX_STANDARD"
	-DCMAKE_CXX_STANDARD_REQUIRED=ON
	-DCMAKE_CXX_EXTENSIONS=OFF
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON
	-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF
	-DCMAKE_C_FLAGS="$DEP_UNORDERED_DENSE_CFLAGS"
	-DCMAKE_CXX_FLAGS="$DEP_UNORDERED_DENSE_CXXFLAGS"
)

block="$(_dep_unordered_dense_field_block)" || {
	echo "dep-unordered-dense: cannot build the identity field block" >&2
	exit 1
}

out=""
if ! dep_ensure out "$mode" unordered_dense "$block" _dep_unordered_dense_producer; then
	echo "dep-unordered-dense: ${mode} failed" >&2
	exit 1
fi

echo "dep-unordered-dense: package prefix: ${out}"
