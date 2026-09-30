#!/bin/bash
# The FTXUI package for wasm32-emscripten.
# scripts/dep/common/ftxui.sh holds the shared recipe.

set -u

DEP_FILE_TARGET=wasm32-emscripten
DEP_RECIPE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"

# The listener-eof patch changes the package, so its hash is part of the id.
_dep_ftxui_target_fields() {
	printf 'emscripten_patch_hash=%s\n' \
		"$(dep_sha256 "${__devrc_dir}/../cmake/ftxui-emscripten-listener-eof.patch")"
}

# The same patch that cmake/ftxui.cmake applies on the FetchContent path.
_dep_ftxui_target_patch() {
	local work="$1"
	(cd "$work" && "$DEP_FTXUI_CMAKE" \
		-DPATCH="${__devrc_dir}/../cmake/ftxui-emscripten-listener-eof.patch" \
		-P "${__devrc_dir}/../cmake/ftxui-apply-patch.cmake")
}

source "$(dirname "${BASH_SOURCE[0]}")/../common/ftxui.sh"
