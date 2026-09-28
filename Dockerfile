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


# Pinned by digest: an upstream retag of the floating tag invalidates this
# layer and every cached layer after it.
FROM ubuntu:24.04@sha256:224a1869083a311ef3f13648a154ba79832fbef6364d31493642ca03082da254 AS base

# Install dependencies
RUN apt-get update && apt-get install -y \
	bash wget git nsis rpm doxygen graphviz \
	curl ca-certificates \
	cmake=3.28.3-1build7 \
	g++=4:13.2.0-7ubuntu1 \
	ninja-build=1.11.1-2 \
	clang-19=1:19.1.1-1ubuntu1~24.04.2 \
	mingw-w64=11.0.1-3build1 \
	python3-distutils-extra \
	python3-pexpect python3-pyte ccache

# The presets name clang and clang++. The versioned package does not provide
# those names.
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 && \
	update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100

# ------------------------------------------------------------
# Dependency store client: oras reads a missing package from the remote
# store at configure time (cmake/parser-deps.cmake).

FROM base AS deps

ARG BUILD_JOBS=5

# Only the files dep-oras.sh runs, so a source change keeps this layer.
COPY ./dev /parser/
COPY ./scripts/devrc ./scripts/dep-build ./scripts/dep-oras.sh /parser/scripts/
COPY ./cmake/tau-resolve.cmake /parser/cmake/
WORKDIR /parser

RUN echo "(BUILD) -- Building the dependency store client: oras" && \
	./dev dep-oras


# ------------------------------------------------------------
# Source tree for every stage that builds the parser

FROM deps AS source

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
# linux-x86_64 store resolver: resolve the packages the build reads, so the
# test job finds them in the layer cache. The publish is a side stage, so an
# argument that only the publish uses never changes a resolve layer.

FROM source AS linux-resolve

# Argument BUILD_PRESET=release/debug picks the CMake preset family
ARG BUILD_PRESET=release

# Argument TESTS=no is used to skip running tests
ARG TESTS=yes

# Argument BUILD_JOBS=N raises the parallelism. One job is the safe default.
ARG BUILD_JOBS=1

# The remote store: configure reads a missing package from it before it
# builds one (cmake/parser-deps.cmake). The token is mounted only for the
# configure step; an empty value keeps the remote out of a local build.
ARG TAU_STORE_REMOTE=
ENV TAU_STORE_REMOTE=${TAU_STORE_REMOTE}

# A TESTS=no build (the nightly packages) never reads the store.
RUN --mount=type=secret,id=gh_token \
	if [ "$TESTS" = "yes" ]; then \
		scripts/with-gh-token ./dev preset ${BUILD_PRESET} --configure-only \
			-DTAU_PARSER_DEPS_FROM_STORE=ON -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
	fi

FROM linux-resolve AS linux-publish

# The trusted workflow turns this on: this stage publishes the packages the
# remote lacks, so the Linux ids match what every Docker consumer requests.
# Off for every other build.
ARG TAU_STORE_PUBLISH=OFF
RUN --mount=type=secret,id=gh_token \
	if [ "$TAU_STORE_PUBLISH" = "ON" ]; then \
		scripts/with-gh-token ./dev store-publish; \
	fi


# ------------------------------------------------------------
# Linux build and its test suite

FROM linux-resolve AS linux

# ccache keeps compiled objects in a cache mount, so a source change only
# recompiles what it touches. CI carries the mount across runs.
ENV CMAKE_C_COMPILER_LAUNCHER=ccache CMAKE_CXX_COMPILER_LAUNCHER=ccache \
	CCACHE_DIR=/root/.ccache CCACHE_MAXSIZE=1G

# Argument BUILD_PRESET=release/debug picks the CMake preset family
ARG BUILD_PRESET=release

# Argument TESTS=no is used to skip running tests
ARG TESTS=yes

# Argument BUILD_JOBS=N raises the parallelism. One job is the safe default.
ARG BUILD_JOBS=1

# Build tests and run them if TESTS is set to yes. Stop the build if they fail
RUN echo " (BUILD) -- Running tests: $TESTS"
# `run` on a -tests preset makes ./dev call ctest itself
RUN --mount=type=cache,target=/root/.ccache,sharing=locked \
	--mount=type=secret,id=gh_token \
	if [ "$TESTS" = "yes" ]; then \
	scripts/with-gh-token ./dev preset ${BUILD_PRESET}-tests run -DTAU_BUILD_JOBS=${BUILD_JOBS} \
		-DTAU_PARSER_DEPS_FROM_STORE=ON \
		|| exit 1; \
fi

# Argument TEST_GCC_BUILD=no skips the make and gcc check
ARG TEST_GCC_BUILD=yes

# The default build is ninja and clang. This proves make and gcc still work.
RUN --mount=type=cache,target=/root/.ccache,sharing=locked \
	if [ "$TESTS" = "yes" ] && [ "$TEST_GCC_BUILD" = "yes" ]; then \
	./dev preset release-make-gcc -DTAU_BUILD_JOBS=${BUILD_JOBS} && \
	rm -rf build/release-gcc; \
fi


# ------------------------------------------------------------
# Windows cross build dependencies: wine and its prefix

FROM deps AS w64-deps

# WINEPREFIX keeps the wine configuration out of the home directory.
# WINEDEBUG drops wine's own noise, and not the output of a test.
# WINEARCH keeps the prefix 64-bit only, so wineboot skips the 32-bit setup.
ENV WINEPREFIX=/root/.wine-parser WINEDEBUG=-all WINEARCH=win64

RUN apt-get update && apt-get install -y --no-install-recommends wine

# Create the wine prefix once. A parallel test run against a missing prefix
# makes every wine process race to create the wineserver socket.
RUN wineboot --init && wineserver -w


# ------------------------------------------------------------
# windows-x86_64-mingw store resolver: resolve the packages the cross build
# reads, so the test job finds them in the layer cache. The publish is a side
# stage, so a publish argument never changes a resolve layer.

FROM w64-deps AS linux-mingw64-wine-resolve

ARG TESTS=yes

# Argument BUILD_JOBS=N raises the parallelism. One job is the safe default.
ARG BUILD_JOBS=1

# The remote store: configure reads a missing package from it before it
# builds one (cmake/parser-deps.cmake).
ARG TAU_STORE_REMOTE=
ENV TAU_STORE_REMOTE=${TAU_STORE_REMOTE}

COPY --from=source /parser /parser
WORKDIR /parser

# A TESTS=no build never reads the store.
RUN --mount=type=secret,id=gh_token \
	if [ "$TESTS" = "yes" ]; then \
		scripts/with-gh-token ./dev preset release-w64-tests --configure-only \
			-DTAU_PARSER_DEPS_FROM_STORE=ON -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
	fi

FROM linux-mingw64-wine-resolve AS linux-mingw64-wine-publish

# The trusted workflow turns this on to publish the mingw packages the w64
# consumers read.
ARG TAU_STORE_PUBLISH=OFF
RUN --mount=type=secret,id=gh_token \
	if [ "$TAU_STORE_PUBLISH" = "ON" ]; then \
		scripts/with-gh-token ./dev store-publish; \
	fi


# ------------------------------------------------------------
# Windows cross build, with the suite run under wine

FROM linux-mingw64-wine-resolve AS linux-mingw64-wine

ARG TESTS=yes

# Argument BUILD_JOBS=N raises the parallelism. One job is the safe default.
ARG BUILD_JOBS=1

# ccache keeps compiled objects in a cache mount, so a source change only
# recompiles what it touches. CI carries the mount across runs.
ENV CMAKE_C_COMPILER_LAUNCHER=ccache CMAKE_CXX_COMPILER_LAUNCHER=ccache \
	CCACHE_DIR=/root/.ccache CCACHE_MAXSIZE=1G

# wine, not wine64: the Ubuntu package runs these 64-bit PE binaries on its
# own, and needs no i386 multiarch.
RUN --mount=type=cache,target=/root/.ccache,sharing=locked \
	--mount=type=secret,id=gh_token \
	echo " (BUILD) -- Running tests under wine: $TESTS" && \
	if [ "$TESTS" = "yes" ]; then \
		scripts/with-gh-token ./dev preset release-w64-tests run -DTAU_BUILD_JOBS=${BUILD_JOBS} \
			-DTAU_PARSER_DEPS_FROM_STORE=ON; \
	fi

# The parity script needs only the native tgf beside the cross-built
# tgf.exe, so build that one target and run the comparison directly instead
# of the whole native suite. A missing binary or a mismatch stops the stage.
RUN --mount=type=cache,target=/root/.ccache,sharing=locked \
	--mount=type=secret,id=gh_token \
	if [ "$TESTS" = "yes" ]; then \
		echo " (BUILD) -- Running the wine parity test" && \
		scripts/with-gh-token ./dev preset release-tgf -DTAU_BUILD_JOBS=${BUILD_JOBS} \
			-DTAU_PARSER_DEPS_FROM_STORE=ON && \
		test -x build/release/tgf && \
		test -f build/release-w64/tgf.exe && \
		sh tests/parity/tgf_wine_parity.sh \
			build/release/tgf build/release-w64/tgf.exe \
			src/format/json/json.tgf; \
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

# Windows packages build in build/release-w64, so they never share a cache
# with the native build.
RUN if [ "$RELEASE" = "yes" ]; then \
	./dev preset release-w64-packages -DTAU_BUILD_JOBS=${BUILD_JOBS} && \
	./dev preset release-w64-packages-zip -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
fi

# If tgf executable does not exist already, build it
RUN if [ ! -f ./build/${BUILD_PRESET}/tgf ]; then \
	./dev preset ${BUILD_PRESET}-tgf -DTAU_BUILD_JOBS=${BUILD_JOBS}; \
fi

# Set the entrypoint to the tgf executable
WORKDIR /parser/build/${BUILD_PRESET}
ENTRYPOINT [ "./tgf" ]
CMD []

# ------------------------------------------------------------
# WebAssembly dependencies image: emsdk and its bundled Node.js

FROM deps AS wasm-deps

ARG BUILD_JOBS=1

# dep-emsdk.sh needs curl, ca-certificates, unzip and xz; the base image
# does not carry them.
RUN apt-get update && apt-get install -y --no-install-recommends \
	curl ca-certificates unzip xz-utils

# Only the files dep-emsdk.sh runs, so a source change keeps the emsdk layer.
COPY ./dev /parser/
COPY ./scripts/devrc ./scripts/dep-emsdk.sh ./scripts/dep-build /parser/scripts/
COPY ./cmake/tau-resolve.cmake /parser/cmake/
WORKDIR /parser

RUN echo "(BUILD) -- Building wasm dependencies: emsdk" && \
	./dev dep-emsdk.sh -DTAU_BUILD_JOBS=${BUILD_JOBS}

# emsdk ships the only node/npm/npx in the image, under a version directory
# whose name is not fixed. The Emscripten tests and the browser test deps
# need them on the path.
RUN EMSDK_NODE_BIN="$(ls -d /root/.tau/emsdk/node/*/bin | head -n1)" && \
	ln -s "$EMSDK_NODE_BIN/node" /usr/local/bin/node && \
	ln -s "$EMSDK_NODE_BIN/npm"  /usr/local/bin/npm && \
	ln -s "$EMSDK_NODE_BIN/npx"  /usr/local/bin/npx


# ------------------------------------------------------------
# WebAssembly resolve: install the xterm vendor and resolve the wasm
# dependencies. The publish and the build are separate stages, so a publish
# argument never changes a stage the browser job has to rebuild.

FROM wasm-deps AS wasm-resolve

ARG BUILD_JOBS=1

# The remote store: configure reads a missing package from it before it
# builds one (cmake/parser-deps.cmake).
ARG TAU_STORE_REMOTE=
ENV TAU_STORE_REMOTE=${TAU_STORE_REMOTE}

COPY --from=source /parser /parser

# The browser page ships in the package, and its build reads the xterm vendor.
RUN echo "(BUILD) -- Installing the xterm.js vendor" && \
	npm ci --prefix js/tau-wasm-terminal --no-audit --no-fund

RUN --mount=type=secret,id=gh_token \
	echo "(BUILD) -- Resolving the wasm dependencies" && \
	scripts/with-gh-token ./dev preset release-wasm-tests --configure-only \
		-DTAU_BUILD_JOBS=${BUILD_JOBS} -DTAU_PARSER_DEPS_FROM_STORE=ON \
		-DTAU_PARSER_BUILD_BROWSER_PAGE=ON

FROM wasm-resolve AS wasm-publish

# The trusted workflow turns this on. It runs before the parser-wasm producer
# below, so only the wasm FTXUI and unordered_dense packages are published;
# parser-wasm travels through the layer cache within one run.
ARG TAU_STORE_PUBLISH=OFF
RUN --mount=type=secret,id=gh_token \
	if [ "$TAU_STORE_PUBLISH" = "ON" ]; then \
		scripts/with-gh-token ./dev store-publish; \
	fi


# ------------------------------------------------------------
# WebAssembly build gate: build the Emscripten artifacts and the browser page
# (the page is a build output, not a browser test), and produce the
# parser-wasm store package the browser gate consumes. No tests here.

FROM wasm-resolve AS wasm-build

RUN --mount=type=secret,id=gh_token \
	echo "(BUILD) -- Building the wasm artifacts" && \
	scripts/with-gh-token ./dev preset release-wasm-tests --keep-cache \
		-DTAU_BUILD_JOBS=${BUILD_JOBS} -DTAU_PARSER_DEPS_FROM_STORE=ON \
		-DTAU_PARSER_BUILD_BROWSER_PAGE=ON

RUN --mount=type=secret,id=gh_token \
	scripts/with-gh-token ./dev dep-parser-wasm.sh \
		-DTAU_DEP_MODE=producer \
		-DTAU_PARSER_WASM_BUILD_DIR=build/release-wasm \
		-DTAU_PARSER_WASM_CMAKE_CACHE=build/release-wasm/CMakeCache.txt


# ------------------------------------------------------------
# WebAssembly Node.js gate: run the node tests on the wasm-build stage.

FROM wasm-build AS wasm-node

ARG BUILD_JOBS=1

RUN echo "(BUILD) -- Running the wasm node tests" && \
	ctest --preset release-wasm-tests -j ${BUILD_JOBS} --output-on-failure


# ------------------------------------------------------------
# WebAssembly browser dependencies: Chrome for Testing, puppeteer-core
# and the xterm.js packages.

FROM wasm-deps AS wasm-browser-deps

# Chrome's shared-library dependencies, needed only by the browser layer.
RUN apt-get update && apt-get install -y --no-install-recommends \
	libnss3 libnspr4 libatk1.0-0 libatk-bridge2.0-0 libcups2 libdrm2 \
	libdbus-1-3 libxkbcommon0 libxcomposite1 libxdamage1 libxfixes3 \
	libxrandr2 libgbm1 libasound2t64 libpango-1.0-0 libcairo2 libx11-6 \
	libx11-xcb1 libxcb1 libxext6 libxshmfence1 libglib2.0-0 fonts-liberation

# Only the files these installs read, so a source change keeps their layers.
COPY ./scripts/dep-chrome.sh /parser/scripts/
COPY ./tests/repl/package.json ./tests/repl/package-lock.json /parser/tests/repl/
COPY ./js/tau-wasm-terminal/package.json ./js/tau-wasm-terminal/package-lock.json \
	/parser/js/tau-wasm-terminal/

# Chrome for Testing, pinned separately from puppeteer-core's own version.
RUN echo "(BUILD) -- Installing Chrome for Testing" && \
	./dev dep-chrome.sh

RUN echo "(BUILD) -- Installing puppeteer-core" && \
	npm ci --prefix tests/repl --no-audit --no-fund

# xterm.js vendor files for the browser REPL page.
RUN echo "(BUILD) -- Installing js/tau-wasm-terminal dependencies" && \
	npm ci --prefix js/tau-wasm-terminal --no-audit --no-fund


# ------------------------------------------------------------
# WebAssembly browser gate: native tgf as the parity reference, the
# browser REPL, and the puppeteer/Chrome test layer.

FROM wasm-browser-deps AS wasm-browser

ARG BUILD_JOBS=1

# The remote store: configure reads a missing package from it before it
# builds one (cmake/parser-deps.cmake).
ARG TAU_STORE_REMOTE=
ENV TAU_STORE_REMOTE=${TAU_STORE_REMOTE}

COPY --from=source /parser /parser

# The parser-wasm store entry the wasm-build stage published, plus the wasm
# ftxui and unordered_dense entries its id references and the wasm configure's
# cache. The layer cache keeps this stage from rebuilding wasm: a cache miss
# reruns the producer, a hit copies it.
COPY --from=wasm-build /root/.tau/store /root/.tau/store
COPY --from=wasm-build /parser/build/release-wasm/CMakeCache.txt /parser-wasm-cmakecache.txt

# Native tgf is the parity reference for the browser parity tests.
RUN --mount=type=secret,id=gh_token \
	echo "(BUILD) -- Building native tgf" && \
	scripts/with-gh-token ./dev preset release-tgf -DTAU_BUILD_JOBS=${BUILD_JOBS} \
		-DTAU_PARSER_DEPS_FROM_STORE=ON

# Consume the parser-wasm package and register the browser tests against it.
# This configure generates the page and test lists but never builds wasm.
RUN --mount=type=secret,id=gh_token \
	wasm_prefix="$(scripts/with-gh-token ./dev dep-parser-wasm.sh \
		-DTAU_DEP_MODE=consumer \
		-DTAU_PARSER_WASM_CMAKE_CACHE=/parser-wasm-cmakecache.txt)" && \
	scripts/with-gh-token ./dev preset release-wasm-tests-browser --configure-only \
		-DTAU_BUILD_JOBS=${BUILD_JOBS} -DTAU_PARSER_DEPS_FROM_STORE=ON \
		-DTAU_PARSER_WASM_PREFIX="${wasm_prefix}" && \
	echo "(BUILD) -- Running the wasm browser tests" && \
	ctest --preset release-wasm-tests-browser -R tgf_browser \
		-j ${BUILD_JOBS} --output-on-failure
