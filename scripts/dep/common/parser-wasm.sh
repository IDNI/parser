#!/bin/bash
# Publish the compiled wasm build as a LOCAL store package.
# scripts/dep/wasm32-emscripten/parser-wasm.sh sources this file.
#
#   ./dev dep-parser-wasm -DTAU_DEP_TARGET=wasm32-emscripten \
#     -DTAU_DEP_MODE=producer \
#     -DTAU_PARSER_WASM_BUILD_DIR=build/release-wasm \
#     -DTAU_PARSER_WASM_CMAKE_CACHE=build/release-wasm/CMakeCache.txt
#
# The producer does not build anything: it publishes the artifacts the wasm
# preset already wrote (a build with the browser page enabled), so the entry
# holds the bytes the browser gate serves. Consumer mode only looks up the
# entry and never compiles wasm; it takes either the identity from a
# CMakeCache.txt or a recorded entry via -DTAU_PARSER_WASM_ENTRY=parser-wasm/<id>.
#
# The producer reads the identity from the CMakeCache.txt, so the id cannot
# drift from the build: the compiler, the build-type flags, the thread option
# and the dependency prefixes (and so their ids). The source tree hash covers
# the per-target flags the cache does not record. Writer and scanner hashes
# are provenance, not id inputs.
set -u

DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

DEP_RECIPE_COMMON="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

# Content hash of the parser tree. The skip set matches .dockerignore plus the
# build outputs generated in the source tree (doctest.h, version_license.h),
# so the hash covers the source and stays deterministic without a .git dir.
_dep_parser_wasm_tree_hash() {
	local src="$1" value digest
	local exclude='^\./(scripts/(dep/[^/]+/parser-wasm\.sh|devrc|dep-build)|cmake/(tau-manifest\.cmake|tau-store\.cmake))$'
	if command -v sha256sum > /dev/null 2>&1; then
		digest="sha256sum"
	else
		digest="shasum -a 256"
	fi
	value="$(cd "$src" && find . \
		\( -name .git -o -name .local -o -name .claude -o -name node_modules \
			-o -name __pycache__ -o -name tau-lang -o -name build -o -name 'build-*' \
			-o -name doctest.h -o -name version_license.h -o -name '.*_history' \) -prune -o \
		-type f -print0 \
		| LC_ALL=C sort -z \
		| grep -zvE "$exclude" \
		| xargs -0 -r $digest | dep_sha256_stdin)"
	[ -n "$value" ] || { echo "dep-parser-wasm: parser tree hash is empty" >&2; return 1; }
	printf '%s' "$value"
}

# One cache value, empty when the key is absent. The value may hold spaces.
_parser_wasm_cache_value() {
	local cache="$1" key="$2" line
	line="$(grep -m1 "^${key}:" "$cache" 2>/dev/null)" || { printf ''; return 0; }
	printf '%s' "${line#*=}"
}

# The build-type flags the configure used: base flags plus the type override.
_parser_wasm_flags() {
	local cache="$1" lang="$2" type="$3"
	printf '%s' "$(_parser_wasm_cache_value "$cache" "CMAKE_${lang}_FLAGS") $(_parser_wasm_cache_value "$cache" "CMAKE_${lang}_FLAGS_${type}")" \
		| sed -e 's/^ *//' -e 's/ *$//'
}

# The store id of a dependency prefix recorded in the cache.
_parser_wasm_prefix_id() {
	local cache="$1" key="$2" prefix
	prefix="$(_parser_wasm_cache_value "$cache" "$key")"
	[ -n "$prefix" ] || return 1
	basename "$(dirname "$prefix")"
}

# The emscripten cache does not record the compiler; derive emcc from the
# toolchain file it does record.
_parser_wasm_emcc() {
	local cache="$1" toolchain d1 d2 d3 d4
	toolchain="$(_parser_wasm_cache_value "$cache" CMAKE_TOOLCHAIN_FILE)"
	[ -n "$toolchain" ] || return 1
	d1="$(dirname "$toolchain")"
	d2="$(dirname "$d1")"
	d3="$(dirname "$d2")"
	d4="$(dirname "$d3")"
	printf '%s/emcc' "$d4"
}

_dep_parser_wasm_field_block() {
	local recipe_hash recipe_common_hash tree_hash build_type threads cflags cxxflags compiler
	local ftxui_id unordered_dense_id
	local build_helper publish_helper manifest store
	local build_hash publish_hash manifest_hash store_hash
	build_type="$(_parser_wasm_cache_value "$TAU_PARSER_WASM_CMAKE_CACHE" CMAKE_BUILD_TYPE)"
	[ -n "$build_type" ] || { echo "dep-parser-wasm: no CMAKE_BUILD_TYPE in the cache" >&2; return 1; }
	build_type="$(printf '%s' "$build_type" | tr '[:lower:]' '[:upper:]')"
	compiler="$(_parser_wasm_emcc "$TAU_PARSER_WASM_CMAKE_CACHE")" \
		|| { echo "dep-parser-wasm: no CMAKE_TOOLCHAIN_FILE in the cache" >&2; return 1; }
	cflags="$(_parser_wasm_flags "$TAU_PARSER_WASM_CMAKE_CACHE" C "$build_type")"
	cxxflags="$(_parser_wasm_flags "$TAU_PARSER_WASM_CMAKE_CACHE" CXX "$build_type")"
	threads="OFF"
	case " ${cxxflags} " in *" -pthread "*) threads="ON" ;; esac
	ftxui_id="$(_parser_wasm_prefix_id "$TAU_PARSER_WASM_CMAKE_CACHE" TAU_PARSER_FTXUI_PREFIX)" \
		|| { echo "dep-parser-wasm: no TAU_PARSER_FTXUI_PREFIX in the cache" >&2; return 1; }
	unordered_dense_id="$(_parser_wasm_prefix_id "$TAU_PARSER_WASM_CMAKE_CACHE" TAU_PARSER_UNORDERED_DENSE_PREFIX)" \
		|| { echo "dep-parser-wasm: no TAU_PARSER_UNORDERED_DENSE_PREFIX in the cache" >&2; return 1; }
	build_helper="${__devrc_dir}/dep-build"
	publish_helper="${__devrc_dir}/devrc"
	manifest="${__devrc_dir}/../cmake/tau-manifest.cmake"
	store="${__devrc_dir}/../cmake/tau-store.cmake"
	recipe_hash="$(dep_sha256 "$DEP_RECIPE")" || return 1
	recipe_common_hash="$(dep_sha256 "$DEP_RECIPE_COMMON")" || return 1
	tree_hash="$(_dep_parser_wasm_tree_hash "$TAU_PARSER_WASM_TREE")" || return 1
	build_hash="$(dep_sha256 "$build_helper")" || return 1
	publish_hash="$(dep_sha256 "$publish_helper")" || return 1
	manifest_hash="$(dep_sha256 "$manifest")" || return 1
	store_hash="$(dep_sha256 "$store")" || return 1
	printf '%s\n' \
		"dep=parser-wasm" \
		"parser_tree_hash=${tree_hash}" \
		"compiler_id=$(dep_compiler_id "$compiler")" \
		"compiler_version=$(dep_compiler_version "$compiler")" \
		"target_triple=wasm32" \
		"cflags=${cflags}" \
		"cxxflags=${cxxflags}" \
		"threads=${threads}" \
		"ftxui_package_id=${ftxui_id}" \
		"unordered_dense_package_id=${unordered_dense_id}" \
		"recipe_hash=${recipe_hash}" \
		"recipe_common_hash=${recipe_common_hash}" \
		"helper_build_hash=${build_hash}" \
		"provenance.publish_helper_hash=${publish_hash}" \
		"provenance.manifest_writer_hash=${manifest_hash}" \
		"provenance.store_writer_hash=${store_hash}"
}

# Publish exactly the files the node and browser gates read. A partial set
# must never reach the store, so every listed file is required.
_dep_parser_wasm_producer() {
	local staging_prefix="$1" build_dir="$2"
	local rel
	local files="tauparser.js tauparser.wasm tauparser.node.js tauparser.html
		tgf.js tgf.wasm
		syntax_highlighter.js syntax_highlighter.wasm
		js/tgf/tgf_standalone.js js/tgf/tgf_standalone.wasm
		js/tgf/tgf_standalone.data
		js/tgf/index.html js/tgf/index.mjs
		js/tgf/shell.css js/tgf/terminal.js js/tgf/sw.js
		js/tgf/vendor/xterm.js js/tgf/vendor/xterm.css
		js/tgf/vendor/addon-fit.js js/tgf/vendor/addon-webgl.js"
	if [ ! -d "$build_dir" ]; then
		echo "dep-parser-wasm: build directory not found: '$build_dir'" >&2
		return 1
	fi
	for rel in $files; do
		if [ ! -f "$build_dir/$rel" ]; then
			echo "dep-parser-wasm: package is missing $rel in '$build_dir'" >&2
			return 1
		fi
	done
	for rel in $files; do
		mkdir -p "$(dirname "$staging_prefix/$rel")" || return 1
		cp "$build_dir/$rel" "$staging_prefix/$rel" || return 1
	done
	return 0
}

# Consumer mode with -DTAU_PARSER_WASM_ENTRY=parser-wasm/<id>: look the entry
# up directly and verify its manifest, without recomputing an id from a tree
# that may differ from the producer's.
_dep_parser_wasm_entry_lookup() {
	local entry="$1" dep id prefix lookup status
	dep="${entry%%/*}"
	id="${entry#*/}"
	if [ "$dep" != "parser-wasm" ] || [ -z "$id" ] || [ "$id" = "$entry" ]; then
		echo "dep-parser-wasm: -DTAU_PARSER_WASM_ENTRY must be parser-wasm/<id>, got '${entry}'" >&2
		return 1
	fi
	prefix="$(dep_shared_prefix)" || return 1
	lookup="$(cmake -P "${__devrc_dir}/../cmake/tau-store.cmake" lookup "$prefix" "$dep" "$id")" || {
		echo "dep-parser-wasm: cannot look up ${entry} in the LOCAL store" >&2
		return 1
	}
	status="$(printf '%s\n' "$lookup" | sed -n '1p')"
	if [ "$status" != "hit" ]; then
		echo "dep-parser-wasm: ${entry} is not in the LOCAL store" >&2
		return 1
	fi
	printf '%s' "$(printf '%s\n' "$lookup" | sed -n '2p')"
}

dep_entry "$@"

dep_require_file_target parser-wasm

mode="$(dep_var TAU_DEP_MODE producer)"
case "$mode" in
	producer|consumer) ;;
	*) echo "dep-parser-wasm: -DTAU_DEP_MODE must be producer or consumer" >&2; exit 2 ;;
esac

TAU_PARSER_WASM_BUILD_DIR="$(dep_var TAU_PARSER_WASM_BUILD_DIR "")"
TAU_PARSER_WASM_CMAKE_CACHE="$(dep_var TAU_PARSER_WASM_CMAKE_CACHE "")"
TAU_PARSER_WASM_TREE="$(dep_var TAU_PARSER_WASM_TREE "$DEV_ROOT")"
TAU_PARSER_WASM_ENTRY="$(dep_var TAU_PARSER_WASM_ENTRY "")"

out=""
if [ -n "$TAU_PARSER_WASM_ENTRY" ]; then
	[ "$mode" = "consumer" ] || {
		echo "dep-parser-wasm: -DTAU_PARSER_WASM_ENTRY is consumer-only" >&2
		exit 2
	}
	out="$(_dep_parser_wasm_entry_lookup "$TAU_PARSER_WASM_ENTRY")" || {
		echo "dep-parser-wasm: consumer failed" >&2
		exit 1
	}
else
	if [ "$mode" = "producer" ] && [ -z "$TAU_PARSER_WASM_BUILD_DIR" ]; then
		echo "dep-parser-wasm: -DTAU_PARSER_WASM_BUILD_DIR is required" >&2
		exit 2
	fi
	[ -n "$TAU_PARSER_WASM_CMAKE_CACHE" ] || {
		echo "dep-parser-wasm: -DTAU_PARSER_WASM_CMAKE_CACHE is required" >&2
		exit 2
	}
	[ -f "$TAU_PARSER_WASM_CMAKE_CACHE" ] || {
		echo "dep-parser-wasm: cache is not a file: '$TAU_PARSER_WASM_CMAKE_CACHE'" >&2
		exit 2
	}

	block="$(_dep_parser_wasm_field_block)" || {
		echo "dep-parser-wasm: cannot build the identity field block" >&2
		exit 1
	}

	if ! dep_ensure out "$mode" parser-wasm "$block" _dep_parser_wasm_producer \
			"$TAU_PARSER_WASM_BUILD_DIR"; then
		echo "dep-parser-wasm: ${mode} failed" >&2
		exit 1
	fi
fi

# Record the entry as used, so store eviction keeps it by last use.
if [ -n "$out" ] && [ -d "$out" ]; then
	date +%s > "$(dirname "$out")/.last-used"
fi

echo "dep-parser-wasm: package prefix: ${out}"
