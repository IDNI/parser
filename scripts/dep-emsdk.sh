#!/bin/env bash

# Exit immediately on a failed command: a missing unzip, a failed download,
# or a failed move.
set -euo pipefail

# This script lives in the parser's own scripts/, so devrc is its sibling.
DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

dep_entry "$@"

EMSDK_SRC="emsdk-main"
EMSDK_DIR="$(dep_shared_prefix)/emsdk"

echo "EMSDK_DIR: ${EMSDK_DIR}"

dep_done_if_exists "${EMSDK_DIR}/upstream/emscripten/emcc" \
	"emsdk download and install"

EMSDK_PARENT="$(dirname "${EMSDK_DIR}")"
EMSDK_BASE="$(basename "${EMSDK_DIR}")"

mkdir -p "${EMSDK_PARENT}"
cd "${EMSDK_PARENT}"

# Download the emsdk
curl -L https://github.com/emscripten-core/emsdk/archive/refs/heads/main.zip -o emsdk.zip
unzip -q emsdk.zip
mkdir -p "$EMSDK_BASE"
# dotglob: the archive carries dotfiles (.github, .gitignore, .circleci)
# that a bare * would not move.
(shopt -s dotglob; mv "$EMSDK_SRC"/* "$EMSDK_BASE")
rmdir "$EMSDK_SRC"
rm emsdk.zip

# Install the latest emsdk version
cd "$EMSDK_BASE"
./emsdk install latest
./emsdk activate latest
