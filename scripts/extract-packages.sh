#!/bin/bash

# Copy the packages and the API documentation out of the image that
# `./dev docker packages` builds. They land where a local build puts them:
# build/release/packages, build/release-w64/packages and _site.

set -euo pipefail

IMAGE="${1:-parser:packages}"

CID=$(docker create "$IMAGE")
trap 'docker rm "$CID" > /dev/null' EXIT

mkdir -p build/release
docker cp "$CID:/parser/build/release/packages" build/release
# The image has the Windows packages only when W64_PACKAGES=yes.
mkdir -p build/release-w64
docker cp "$CID:/parser/build/release-w64/packages" build/release-w64 2> /dev/null \
	|| rmdir --ignore-fail-on-non-empty build/release-w64
# The documentation target runs doxygen in the source root.
docker cp "$CID:/parser/_site" .
