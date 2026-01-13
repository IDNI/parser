#!/bin/bash

# This helper script clones tau-lang and compiles it with current parser code.
# Then it runs tau-lang tests to check if parser changes do not break tau-lang.
#
# This script accepts an optional argument: BUILD_TYPE.
# It can be one of "debug" or "release".
# "release" is default if no argument is provided

TAUDIR="./tau-lang"

# check the first argument if it contains "release" or "debug".
# if no argument is provided "release" is used
BUILD_TYPE="${1:-release}"
BUILD_TYPE=$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')
BUILD_TYPES=("release" "debug")
if [[ ! " ${BUILD_TYPES[@]} " =~ " ${BUILD_TYPE} " ]]; then
	echo "Invalid build type: ${BUILD_TYPE}"
	echo "Valid build types are: ${BUILD_TYPES[*]}"
	exit 1
fi

# clone tau
if [ ! -d "$TAUDIR" ]; then
	echo "Cloning tau-lang"
	git clone https://github.com/IDNI/tau-lang $TAUDIR
else
	echo "tau-lang already cloned"
fi

# enter tau directory
cd $TAUDIR

# initialize submodule to prevent init when building
git submodule status | while read -r LINE; do
	GIT_SUBMOD=$(echo $LINE | awk '{print $2}')
	if [[ $LINE == -* ]]; then
		echo "Initializing submodule $GIT_SUBMOD"
		git submodule update --init --recursive $GIT_SUBMOD
	else
		echo "Submodule ${GIT_SUBMOD} is already initialized"
	fi
done

# remove content of the submodule to be replaced by current code
rm -rf external/parser/*

# copy current parser source to place where tau expects it
cp -r ../cmake ../src ../CMakeLists.txt ../VERSION ../LICENSE.md ../README.md \
	external/parser

# build tau with tests and run them
./dev $BUILD_TYPE -DTAU_BUILD_TESTS=ON && ./dev test-$BUILD_TYPE
