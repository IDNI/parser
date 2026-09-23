# This is a Dockerfile for building, testing, packaging and running
# the Tau Parser Library and TGF tool

# To build the image with a tag 'tgf' from ./Dockerfile:
#   docker build -t tgf .

# use --build-arg TESTS="no" to skip running tests
# use --build-arg BUILD_PRESET="debug" for building of the debugging version

# To build a single stage:
#   ./dev docker linux            or    ./dev docker linux-mingw64-wine

# To run tgf using the created image in interactive mode
# with a tgf file mounted to /grammar.tgf:
#   docker run --rm -it -v <tgf file>:/grammar.tgf tgf /grammar.tgf [<tgf options>]

# --rm flag is used to remove the container after it exits


FROM ubuntu:24.04 AS base

# Install dependencies
RUN apt-get update && apt-get install -y \
	bash wget git nsis rpm doxygen graphviz \
	cmake=3.28.3-1build7 \
	g++=4:13.2.0-7ubuntu1 \
	ninja-build=1.11.1-2 \
	clang-19=1:19.1.1-1ubuntu1~24.04.2 \
	mingw-w64=11.0.1-3build1 \
	python3-distutils-extra \
	python3-pexpect python3-pyte

# The presets name clang and clang++. The versioned package does not provide
# those names.
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 && \
	update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100

# Argument BUILD_PRESET=release/debug picks the CMake preset family
ARG BUILD_PRESET=release

# Argument NIGHTLY=yes is used to build nightly packages (works only if RELEASE=yes)
ARG NIGHTLY=no

# Copy source code
COPY ./ /parser

# # OR clone from git
# RUN git clone https://github.com/IDNI/parser /parser

WORKDIR /parser

RUN ./dev clean all

# if NIGHTLY is set to yes, then add .YYYY-MM-DD to the first line of the VERSION file
RUN if [ "$NIGHTLY" = "yes" ]; then \
	echo -n "$(head -n 1 VERSION)-$(date --iso)" > VERSION; \
fi
RUN echo "(BUILD) -- Building version: $(head -n 1 VERSION)"


# ------------------------------------------------------------
# Linux build and its test suite

FROM base AS linux

# Argument BUILD_PRESET=release/debug picks the CMake preset family
ARG BUILD_PRESET=release

# Argument TESTS=no is used to skip running tests
ARG TESTS=yes

# Argument BUILD_JOBS=N raises the parallelism. One job is the safe default.
ARG BUILD_JOBS=1

# Build tests and run them if TESTS is set to yes. Stop the build if they fail
RUN echo " (BUILD) -- Running tests: $TESTS"
# `run` on a -tests preset makes ./dev call ctest itself
RUN if [ "$TESTS" = "yes" ]; then \
	./dev preset ${BUILD_PRESET}-tests run -DTAU_BUILD_JOBS=${BUILD_JOBS} \
		|| exit 1; \
fi

# Argument TEST_GCC_BUILD=no skips the make and gcc check
ARG TEST_GCC_BUILD=yes

# The default build is ninja and clang. This proves make and gcc still work.
RUN if [ "$TESTS" = "yes" ] && [ "$TEST_GCC_BUILD" = "yes" ]; then \
	./dev preset release-make-gcc -DTAU_BUILD_JOBS=${BUILD_JOBS} && \
	rm -rf build/release-gcc; \
fi


# ------------------------------------------------------------
# Windows cross build, with the suite run under wine

FROM base AS linux-mingw64-wine

ARG TESTS=yes

# Argument BUILD_JOBS=N raises the parallelism. One job is the safe default.
ARG BUILD_JOBS=1

# WINEPREFIX keeps the wine configuration out of the home directory.
# WINEDEBUG drops wine's own noise, and not the output of a test.
# WINEARCH keeps the prefix 64-bit only, so wineboot skips the 32-bit setup.
ENV WINEPREFIX=/root/.wine-parser WINEDEBUG=-all WINEARCH=win64

RUN apt-get update && apt-get install -y --no-install-recommends wine

# Create the wine prefix once. A parallel test run against a missing prefix
# makes every wine process race to create the wineserver socket.
RUN wineboot --init && wineserver -w

# wine, not wine64: the Ubuntu package runs these 64-bit PE binaries on its
# own, and needs no i386 multiarch.
RUN echo " (BUILD) -- Running tests under wine: $TESTS" && \
	if [ "$TESTS" = "yes" ]; then \
		./dev preset release-mingw-tests run -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
	else \
		./dev preset release-mingw-tests -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
	fi

# The wine parity test lives in the native tree and registers itself once the
# cross-built tgf.exe is there, so the native suite runs it.
RUN if [ "$TESTS" = "yes" ]; then \
		echo " (BUILD) -- Running the native suite with the wine parity test" && \
		./dev preset release-tests run -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
	fi


# ------------------------------------------------------------
# documentation, packages, and the tgf entrypoint

FROM linux AS packages

# Argument BUILD_PRESET=release/debug picks the CMake preset family
ARG BUILD_PRESET=release

# Argument BUILD_JOBS=N raises the parallelism. One job is the safe default.
ARG BUILD_JOBS=1

# Argument RELEASE=yes is used to build release packages
ARG RELEASE=no

ARG NIGHTLY=no

# Argument DOCUMENTATION=no skips building of API documentation
ARG DOCUMENTATION=yes

RUN echo "(BUILD) -- Building packages: $RELEASE (nightly: $NIGHTLY)"

# Documentation
RUN if [ "$DOCUMENTATION" = "yes" ] || [ "$RELEASE" = "yes" ]; then \
	./dev preset ${BUILD_PRESET} -DTAU_BUILD_JOBS=${BUILD_JOBS} \
		-DTAU_PARSER_BUILD_DOC=ON; \
fi

# Linux packages
RUN if [ "$RELEASE" = "yes" ]; then \
	./dev preset release-packages -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
fi

# Windows packages. The mingw presets build in build/release-mingw, so they
# never share a cache with the native build.
RUN if [ "$RELEASE" = "yes" ]; then \
	./dev preset release-mingw-packages -DTAU_BUILD_JOBS=${BUILD_JOBS} && \
	./dev preset release-mingw-packages-zip -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
fi

# If tgf executable does not exist already, build it
RUN if [ ! -f ./build/${BUILD_PRESET}/tgf ]; then \
	./dev preset ${BUILD_PRESET}-tgf -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
fi

# Set the entrypoint to the tgf executable
WORKDIR /parser/build/${BUILD_PRESET}
ENTRYPOINT [ "./tgf" ]
CMD []
