#!/bin/bash

set -euo pipefail

CMD="${1:-help}"
PROGRESS="auto" # "auto" or "plain"

build() {
	local git_args=()
	if git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
		git_args=(
			--build-arg "TAU_PARSER_GIT_DESCRIBED=$(git describe --tags --always)"
			--build-arg "TAU_PARSER_GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)"
			--build-arg "TAU_PARSER_GIT_COMMIT_HASH=$(git log -1 --format=%h)"
		)
	fi
	echo "Building: '${*}'"
	docker buildx build --progress="${PROGRESS}" ${git_args[@]+"${git_args[@]}"} "$@" .
}

run() {
	echo "Running docker run --rm -it ${*}"
	docker run --rm -it "$@"
}

case "${CMD}" in
	"help")
		echo "Usage: ./dev docker <ACTION> [DOCKER OPTIONS]"
		echo "Every action runs docker build. Extra options are appended."
		echo "Available actions:"
		echo "  base               - build the base image (system packages)"
		echo "  deps               - build the deps image (oras store client)"
		echo "  source             - build the source image"
		echo "  linux-resolve      - resolve the Linux store packages"
		echo "  linux-publish      - publish the Linux store packages"
		echo "  linux              - build and run the suite on Linux"
		echo "  w64-deps           - build the w64-deps image (wine)"
		echo "  linux-mingw64-wine-resolve - resolve the MinGW store packages"
		echo "  linux-mingw64-wine-publish - publish the MinGW store packages"
		echo "  linux-mingw64-wine - cross-build for Windows, run the suite under wine"
		echo "  wasm-deps          - build the wasm-deps image (emsdk + node)"
		echo "  wasm-resolve       - resolve the wasm store packages"
		echo "  wasm-publish       - publish the wasm store packages"
		echo "  wasm-build         - build the wasm artifacts and produce parser-wasm"
		echo "  wasm-node          - build the wasm-node image and run the node suite"
		echo "  wasm-npm           - pack the npm package into \$PARSER_NPM_DIR (default build/npm)"
		echo "  wasm-browser-deps  - build the wasm-browser-deps image (Chrome, npm packages)"
		echo "  wasm-browser       - build the wasm-browser image and run the browser suite"
		echo "  packages           - build the release packages"
		echo "  nightly            - build the nightly packages"
		echo "  tgf                - build the tgf image and run it"
		echo "  run                - run a container"
		echo "  bash               - run bash in a container"
		;;
	"base")
		build --target base -t parser:base "${@:2}"
		;;
	"deps")
		build --target deps -t parser:deps "${@:2}"
		;;
	"source")
		build --target source -t parser:source "${@:2}"
		;;
	"linux-resolve")
		build --target linux-resolve -t parser:linux-resolve "${@:2}"
		;;
	"linux-publish")
		build --target linux-publish -t parser:linux-publish "${@:2}"
		;;
	"linux")
		build --target linux -t parser:linux "${@:2}"
		;;
	"w64-deps")
		build --target w64-deps -t parser:w64-deps "${@:2}"
		;;
	"linux-mingw64-wine-resolve")
		build --target linux-mingw64-wine-resolve -t parser:mingw64-wine-resolve "${@:2}"
		;;
	"linux-mingw64-wine-publish")
		build --target linux-mingw64-wine-publish -t parser:mingw64-wine-publish "${@:2}"
		;;
	"linux-mingw64-wine")
		build --target linux-mingw64-wine -t parser:mingw64-wine "${@:2}"
		;;
	"wasm-deps")
		build --target wasm-deps -t parser:wasm-deps "${@:2}"
		;;
	"wasm-resolve")
		build --target wasm-resolve -t parser:wasm-resolve "${@:2}"
		;;
	"wasm-publish")
		build --target wasm-publish -t parser:wasm-publish "${@:2}"
		;;
	"wasm-build")
		build --target wasm-build -t parser:wasm-build "${@:2}"
		;;
	"wasm-node")
		build --target wasm-node -t parser:wasm-node "${@:2}"
		;;
	"wasm-npm")
		build --target wasm-npm \
			--output "type=local,dest=${PARSER_NPM_DIR:-build/npm}" "${@:2}"
		;;
	"wasm-browser-deps")
		build --target wasm-browser-deps -t parser:wasm-browser-deps "${@:2}"
		;;
	"wasm-browser")
		build --target wasm-browser -t parser:wasm-browser "${@:2}"
		;;
	"packages")
		build --target packages --build-arg RELEASE=yes -t parser:packages "${@:2}"
		;;
	"nightly")
		build --target packages --build-arg RELEASE=yes \
			--build-arg NIGHTLY=yes -t parser:nightly "${@:2}"
		;;
	"tgf")
		build --target packages -t parser:tgf "${@:2}" && run -t parser:tgf
		;;
	"run")
		run "${@:2}"
		;;
	"bash")
		run --entrypoint /bin/bash "${@:2}"
		;;
	*)
		echo "Unknown docker action: ${CMD}"
		exit 1
		;;
esac
