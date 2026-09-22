#!/bin/bash

# This helper script clones tau-lang and compiles it with current parser code.
# Then it runs tau-lang tests to check if parser changes do not break tau-lang.
#
# This script accepts an optional argument: BUILD_TYPE.
# It can be any build type devrc recognises (debug, release, relwithdebinfo,
# coverage), case-insensitively. "release" is default if no argument is
# provided.

set -euo pipefail

DEV_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TAUDIR="./tau-lang"

source "${DEV_ROOT}/scripts/devrc"

# canonicalize the case, then let devrc's own parsing recognize every build
# type it knows about instead of hand-rolling a second, shorter list
BUILD_TYPE_ARG="${1:-Release}"
[[ $# -ge 1 ]] && shift
case "$(echo "${BUILD_TYPE_ARG}" | tr '[:upper:]' '[:lower:]')" in
	debug)          BUILD_TYPE_ARG="Debug" ;;
	release)        BUILD_TYPE_ARG="Release" ;;
	relwithdebinfo) BUILD_TYPE_ARG="RelWithDebInfo" ;;
	coverage)       BUILD_TYPE_ARG="Coverage" ;;
	*)
		echo "Invalid build type: ${BUILD_TYPE_ARG}"
		echo "Valid build types are: debug release relwithdebinfo coverage"
		exit 1
		;;
esac
dev_entry "${BUILD_TYPE_ARG}" "$@"
BUILD_TYPE="${BUILD_TYPE,,}"
TAU_BRANCH="${TAU_BRANCH:-main}"

# clone tau
if [ ! -d "$TAUDIR" ]; then
	echo "Cloning tau-lang (${TAU_BRANCH})"
	git clone --branch "${TAU_BRANCH}" https://github.com/IDNI/tau-lang "$TAUDIR"
else
	echo "tau-lang already cloned"
fi

# enter tau directory
cd $TAUDIR

# initialize submodule to prevent init when building
git_submodules_init

# remove content of the submodule to be replaced by current code
rm -rf external/parser/*

# copy current parser source to place where tau expects it
cp -r ../cmake ../scripts ../src ../CMakeLists.txt ../VERSION ../LICENSE.md ../README.md \
	external/parser

echo "Building with preset ${BUILD_TYPE} (TAU_BUILD_JOBS=$TAU_BUILD_JOBS)"
./dev preset "${BUILD_TYPE}-tests" run -DTAU_BUILD_JOBS=$TAU_BUILD_JOBS
