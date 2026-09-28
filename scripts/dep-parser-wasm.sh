#!/bin/bash
# Publish the compiled wasm build as a LOCAL store package.
#
#   ./dev dep-parser-wasm \
#     -DTAU_DEP_MODE=producer \
#     -DTAU_PARSER_WASM_BUILD_DIR=build/release-wasm \
#     -DTAU_PARSER_FTXUI_ID=<id> -DTAU_PARSER_UNORDERED_DENSE_ID=<id> \
#     -DTAU_DEP_CC=<emcc> -DTAU_DEP_CXX=<emcc> \
#     -DTAU_DEP_CFLAGS=<flags> -DTAU_DEP_CXXFLAGS=<flags> \
#     -DTAU_DEP_TARGET=wasm32-emscripten
#
# The producer does not build anything: it publishes the artifacts the wasm
# preset already wrote (a -browser build, so the browser page files exist),
# so the entry holds the bytes the browser gate serves. Consumer mode only
# looks up the entry and never compiles wasm.
#
# The id hashes the source tree and the build-affecting toolchain inputs: the
# wasm-target FTXUI and unordered_dense package ids, the emcc compiler and the
# recorded flags. Writer and scanner hashes are provenance, not id inputs.
set -u

DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

TAU_PARSER_WASM_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

# Content hash of the parser working tree. The recipe and the publish helpers
# are excluded: each is hashed on its own or cannot change the package bytes.
_dep_parser_wasm_tree_hash() {
	local src="$1" value digest
	local exclude='^(scripts/(dep-parser-wasm\.sh|devrc|dep-build)|cmake/(tau-manifest\.cmake|tau-store\.cmake))$'
	if command -v sha256sum > /dev/null 2>&1; then
		digest="sha256sum"
	else
		digest="shasum -a 256"
	fi
	value="$(cd "$src" && git ls-files -z --cached --others --exclude-standard \
		| grep -zvE "$exclude" \
		| LC_ALL=C sort -z | xargs -0 -r $digest | dep_sha256_stdin)"
	[ -n "$value" ] || { echo "dep-parser-wasm: parser tree hash is empty" >&2; return 1; }
	printf '%s' "$value"
}

_dep_parser_wasm_field_block() {
	local recipe_hash tree_hash threads
	local build_helper publish_helper manifest store
	local build_hash publish_hash manifest_hash store_hash
	# The browser page and tgf_standalone are built with pthreads, so the
	# pthread variant is a different package.
	threads="OFF"
	case " ${TAU_PARSER_WASM_CXXFLAGS:-} " in *" -pthread "*) threads="ON" ;; esac
	build_helper="${__devrc_dir}/dep-build"
	publish_helper="${__devrc_dir}/devrc"
	manifest="${__devrc_dir}/../cmake/tau-manifest.cmake"
	store="${__devrc_dir}/../cmake/tau-store.cmake"
	recipe_hash="$(dep_sha256 "$TAU_PARSER_WASM_RECIPE")" || return 1
	tree_hash="$(_dep_parser_wasm_tree_hash "$TAU_PARSER_WASM_TREE")" || return 1
	build_hash="$(dep_sha256 "$build_helper")" || return 1
	publish_hash="$(dep_sha256 "$publish_helper")" || return 1
	manifest_hash="$(dep_sha256 "$manifest")" || return 1
	store_hash="$(dep_sha256 "$store")" || return 1
	printf '%s\n' \
		"dep=parser-wasm" \
		"parser_tree_hash=${tree_hash}" \
		"compiler_id=$(dep_compiler_id "$TAU_PARSER_WASM_CXX")" \
		"compiler_version=$(dep_compiler_version "$TAU_PARSER_WASM_CXX")" \
		"target_triple=wasm32" \
		"cflags=${TAU_PARSER_WASM_CFLAGS}" \
		"cxxflags=${TAU_PARSER_WASM_CXXFLAGS}" \
		"threads=${threads}" \
		"ftxui_package_id=${TAU_PARSER_FTXUI_ID}" \
		"unordered_dense_package_id=${TAU_PARSER_UNORDERED_DENSE_ID}" \
		"recipe_hash=${recipe_hash}" \
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
		tgf_node.js tgf_node.wasm syntax_highlighter.js syntax_highlighter.wasm
		js/tgf/tgf_standalone.js js/tgf/tgf_standalone.wasm
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

dep_entry "$@"

mode="$(dep_var TAU_DEP_MODE producer)"
case "$mode" in
	producer|consumer) ;;
	*) echo "dep-parser-wasm: -DTAU_DEP_MODE must be producer or consumer" >&2; exit 2 ;;
esac

TAU_PARSER_WASM_BUILD_DIR="$(dep_var TAU_PARSER_WASM_BUILD_DIR "")"
TAU_PARSER_FTXUI_ID="$(dep_var TAU_PARSER_FTXUI_ID "")"
TAU_PARSER_UNORDERED_DENSE_ID="$(dep_var TAU_PARSER_UNORDERED_DENSE_ID "")"
TAU_PARSER_WASM_TREE="$(dep_var TAU_PARSER_WASM_TREE "$DEV_ROOT")"

TAU_PARSER_WASM_CC="$(dep_var TAU_DEP_CC "")"
TAU_PARSER_WASM_CXX="$(dep_var TAU_DEP_CXX "")"
TAU_PARSER_WASM_CFLAGS="$(dep_var TAU_DEP_CFLAGS "")"
TAU_PARSER_WASM_CXXFLAGS="$(dep_var TAU_DEP_CXXFLAGS "")"
TAU_PARSER_WASM_TARGET="$(dep_var TAU_DEP_TARGET "")"

if [ "$mode" = "producer" ] && [ -z "$TAU_PARSER_WASM_BUILD_DIR" ]; then
	echo "dep-parser-wasm: -DTAU_PARSER_WASM_BUILD_DIR is required" >&2
	exit 2
fi
[ -n "$TAU_PARSER_FTXUI_ID" ] || { echo "dep-parser-wasm: -DTAU_PARSER_FTXUI_ID is required" >&2; exit 2; }
[ -n "$TAU_PARSER_UNORDERED_DENSE_ID" ] || { echo "dep-parser-wasm: -DTAU_PARSER_UNORDERED_DENSE_ID is required" >&2; exit 2; }
[ -n "$TAU_PARSER_WASM_CXX" ] || { echo "dep-parser-wasm: no compiler; pass -DTAU_DEP_CXX" >&2; exit 2; }
[ -n "$TAU_PARSER_WASM_CC" ] || { echo "dep-parser-wasm: no compiler; pass -DTAU_DEP_CC" >&2; exit 2; }
[ "$TAU_PARSER_WASM_TARGET" = "wasm32-emscripten" ] || {
	echo "dep-parser-wasm: target must be wasm32-emscripten, got '${TAU_PARSER_WASM_TARGET}'" >&2
	exit 2
}

block="$(_dep_parser_wasm_field_block)" || {
	echo "dep-parser-wasm: cannot build the identity field block" >&2
	exit 1
}

out=""
if ! dep_ensure out "$mode" parser-wasm "$block" _dep_parser_wasm_producer \
		"$TAU_PARSER_WASM_BUILD_DIR"; then
	echo "dep-parser-wasm: ${mode} failed" >&2
	exit 1
fi

# Record the entry as used, so store eviction keeps it by last use.
if [ -n "$out" ] && [ -d "$out" ]; then
	date +%s > "$(dirname "$out")/.last-used"
fi

echo "dep-parser-wasm: package prefix: ${out}"
