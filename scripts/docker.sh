#!/bin/bash

set -euo pipefail

CMD="${1:-help}"
PROGRESS="auto" # "auto" or "plain"

build() {
	echo "Building: '${*}'"
	docker buildx build --progress="${PROGRESS}" "$@" .
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
		echo "  linux              - build and run the suite on Linux"
		echo "  w64-deps           - build the w64-deps image (wine)"
		echo "  linux-mingw64-wine - cross-build for Windows, run the suite under wine"
		echo "  wasm-deps          - build the wasm-deps image (emsdk + node)"
		echo "  wasm-node          - build the wasm-node image and run the node suite"
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
	"linux")
		build --target linux -t parser:linux "${@:2}"
		;;
	"w64-deps")
		build --target w64-deps -t parser:w64-deps "${@:2}"
		;;
	"linux-mingw64-wine")
		build --target linux-mingw64-wine -t parser:mingw64-wine "${@:2}"
		;;
	"wasm-deps")
		build --target wasm-deps -t parser:wasm-deps "${@:2}"
		;;
	"wasm-node")
		build --target wasm-node -t parser:wasm-node "${@:2}"
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
