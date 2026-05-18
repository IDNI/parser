#!/bin/bash

PRESET="${1}"
if [ -z "${PRESET}" ]; then
	PRESET="release"
fi

ARGS_START=2
RUN="${2}"
if [[ $RUN == "run" ]]; then
	ARGS_START=3
fi

echo "Preset: ${PRESET}"
echo "Run: ${RUN}"
echo "Args: ${@:$ARGS_START}"

# Set number of build jobs (0 to use half of available logical CPU cores)
TAU_BUILD_JOBS=0
for arg in "${@:$ARGS_START}"; do
	if [[ $arg == -DTAU_BUILD_JOBS=* ]]; then
		TAU_BUILD_JOBS="${arg#-DTAU_BUILD_JOBS=}"
		break
	fi
done
if [ $TAU_BUILD_JOBS -eq 0 ]; then
	TAU_BUILD_JOBS=$(cmake -P cmake/processor-counter.cmake 2>&1 || echo "1")
	TAU_BUILD_JOBS=$((TAU_BUILD_JOBS / 2))
	echo "TAU_BUILD_JOBS: ${TAU_BUILD_JOBS} (half of available cores)"
fi

export CMAKE_BUILD_PARALLEL_LEVEL=${TAU_BUILD_JOBS}

#LOGGING_OPTIONS="--debug-output --log-level=VERBOSE"
LOGGING_OPTIONS=""

echo "Configuring with preset ${PRESET}"
cmake ${LOGGING_OPTIONS} --fresh --preset ${PRESET} ${@:$ARGS_START}

echo "Building with preset ${PRESET}"
echo "Parallel jobs: ${TAU_BUILD_JOBS}"
cmake --build --preset ${PRESET} ${@:$ARGS_START} -- -j ${TAU_BUILD_JOBS}
STATUS=$?
if [ $STATUS -ne 0 ]; then
	echo "Build failed"
	exit $STATUS
fi
echo "Preset '${PRESET}' build completed"

if [[ $RUN == "run" ]]; then
	if [[ $PRESET == *test* ]]; then
		echo "Running tests"
		ctest --preset ${PRESET} -j ${TAU_BUILD_JOBS} --output-on-failure ${@:$ARGS_START}
		STATUS=$?
		if [ $STATUS -ne 0 ]; then
			echo "Running tests failed"
			exit $STATUS
		fi
		echo "All tests passed"
	else
		BUILD_DIR="build"
		if [[ $PRESET == release* ]]; then
			BUILD_DIR="build/release"
		fi
		if [[ $PRESET == debug* ]]; then
			BUILD_DIR="build/debug"
		fi
		if [[ $PRESET == relwithdebinfo* ]]; then
			BUILD_DIR="build/relwithdebinfo"
		fi
		if [[ $PRESET == coverage* ]]; then
			BUILD_DIR="build/coverage"
		fi
		for arg in "${@:$ARGS_START}"; do
			ARGS_START=$((ARGS_START + 1))
			if [[ $arg == -- ]]; then
				break
			fi
		done
		./${BUILD_DIR}/tgf ${@:$ARGS_START}
	fi
fi
