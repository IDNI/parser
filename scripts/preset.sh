#!/bin/bash

DEV_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"
devrc_entry "$@"

PRESET="${DEV_POSITIONAL[0]:-release}"
RUN=""
if [[ ${DEV_POSITIONAL[1]:-} == "run" ]]; then
	RUN="run"
fi

preset_binary_dir() {
	case "$1" in
		release-mingw*|*-mingw-packages*)
			echo "build/release-mingw"
			;;
		debug-mingw*)
			echo "build/debug-mingw"
			;;
		relwithdebinfo*)
			echo "build/relwithdebinfo"
			;;
		emscripten*)
			echo "build/emscripten"
			;;
		release-clang|release-*-clang|release-make-clang \
			|release-ninja-clang)
			echo "build/release-clang"
			;;
		debug-clang|debug-*-clang|debug-make-clang \
			|debug-ninja-clang)
			echo "build/debug-clang"
			;;
		debug*)
			echo "build/debug"
			;;
		release*)
			echo "build/release"
			;;
		all)
			echo "build/release"
			;;
		*)
			echo "build"
			;;
	esac
}

echo "Preset: ${PRESET}"
echo "Run: ${RUN}"
if [[ ${#DEV_CMAKE[@]} -gt 0 ]]; then
	echo "CMake args: ${DEV_CMAKE[*]}"
fi

TAU_BUILD_JOBS="${TAU_BUILD_JOBS:-1}"

#LOGGING_OPTIONS="--debug-output --log-level=VERBOSE"
LOGGING_OPTIONS=""

echo "Configuring with preset ${PRESET}"
cmake ${LOGGING_OPTIONS} --fresh --preset ${PRESET} "${DEV_CMAKE[@]}"

echo "Building with preset ${PRESET}"
echo "Parallel jobs: ${TAU_BUILD_JOBS}"
cmake --build --preset ${PRESET} "${DEV_CMAKE[@]}" -- -j ${TAU_BUILD_JOBS}
STATUS=$?
if [ $STATUS -ne 0 ]; then
	echo "Build failed"
	exit $STATUS
fi
echo "Preset '${PRESET}' build completed"

if [[ $PRESET == *package* ]]; then
	BUILD_DIR="$(preset_binary_dir "${PRESET}")"
	echo "Creating packages in ${BUILD_DIR} (Release)"
	( cd "${BUILD_DIR}" && cpack -C Release )
	STATUS=$?
	if [ $STATUS -ne 0 ]; then
		echo "Packaging failed"
		exit $STATUS
	fi
	echo "Packages created in ${BUILD_DIR}/packages"
elif [[ $RUN == "run" ]]; then
	if [[ $PRESET == *test* ]] || [[ $PRESET == *-all ]] \
			|| [[ $PRESET == "all" ]]; then
		echo "Running tests"
		ctest --preset ${PRESET} -j ${TAU_BUILD_JOBS} \
			--output-on-failure "${DEV_CMAKE[@]}"
		STATUS=$?
		if [ $STATUS -ne 0 ]; then
			echo "Running tests failed"
			exit $STATUS
		fi
		echo "All tests passed"
	else
		BUILD_DIR="$(preset_binary_dir "${PRESET}")"
		TGF_ARGS=("${DEV_PROGRAM[@]}")
		if [[ ${TGF_ARGS[0]:-} == -- ]]; then
			TGF_ARGS=("${TGF_ARGS[@]:1}")
		fi
		./${BUILD_DIR}/tgf "${TGF_ARGS[@]}"
	fi
fi
