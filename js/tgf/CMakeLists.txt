# TGF FTXUI REPL for WebAssembly, run by Node.js or by the browser page.
# Requires Emscripten and FTXUI (pthreads).
# Built when TAU_PARSER_BUILD_TGF=ON, the page when TAU_PARSER_BUILD_BROWSER_TESTS=ON.

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(tgf_wasm)

# Source files live in the main src/ tree; paths are relative to the
# top-level source directory so they work from any subdirectory build.
set(TOP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../..")

set(TGF_STANDALONE_SOURCES
    ${TOP_SRC}/src/tgf/tgf_cli.cpp
    ${TOP_SRC}/src/tgf/tgf_json_api.cpp
    ${TOP_SRC}/src/format/tgf/tgf_parser.generated.cpp
    ${TOP_SRC}/src/format/json/json_parser.generated.cpp
    ${TOP_SRC}/src/format/treemr/treemr_parser.generated.cpp
    ${TOP_SRC}/src/tgf/tgf_repl_parser.generated.cpp
    ${TOP_SRC}/src/tgf/main.cpp
    ${TOP_SRC}/src/parser.cpp
)

add_executable(tgf_standalone ${TGF_STANDALONE_SOURCES})
target_compile_features(tgf_standalone PRIVATE cxx_std_23)
target_include_directories(tgf_standalone PRIVATE
    ${TOP_SRC}/src
    ${TOP_SRC}/src/tgf
)

# The vendored unordered_dense.h is set up as an INTERFACE target by the
# top-level CMakeLists (unconditional, not gated on EMSCRIPTEN).  If for
# some reason it is missing, fall back to the FetchContent target.
if(TARGET unordered_dense)
    target_link_libraries(tgf_standalone PRIVATE unordered_dense)
elseif(EXISTS "${TOP_SRC}/external/ankerl/unordered_dense.h")
    target_include_directories(tgf_standalone PRIVATE
        ${TOP_SRC}/external)
else()
    find_package(unordered_dense REQUIRED)
    target_link_libraries(tgf_standalone PRIVATE unordered_dense)
endif()

# FTXUI - resolves via cmake/ftxui.cmake (prefers system, falls back to FetchContent)
include(ftxui)
if(NOT TAU_PARSER_HAS_FTXUI_DEP)
    message(FATAL_ERROR "tgf_wasm: FTXUI not available - required for the standalone build")
endif()
target_link_libraries(tgf_standalone PRIVATE
    ftxui::screen ftxui::dom ftxui::component)
target_compile_definitions(tgf_standalone PRIVATE TAU_PARSER_HAS_FTXUI)
# header-only mode makes tgf_test.h resolve productions() inline instead
# of needing the out-of-line definition linked in
if(TAU_PARSER_BUILD_HEADER_ONLY)
    target_compile_definitions(tgf_standalone PRIVATE TAU_PARSER_BUILD_HEADER_ONLY)
endif()

# Disable exceptions for TGF sources only; FTXUI may need them internally.
target_compile_options(tgf_standalone PRIVATE -fno-exceptions -pthread)

# Emscripten output: plain JS (no HTML wrapper - a host script loads it)
# Preload the default tau.tgf grammar so the REPL starts immediately.
# FTXUI under Emscripten requires pthreads + asyncify + proxy-to-pthread.
# The generated parser tables need a larger stack during static init.
set_target_properties(tgf_standalone PROPERTIES
    SUFFIX ".js"
    LINK_FLAGS "-pthread -Wno-pthreads-mem-growth -s ASYNCIFY -s PROXY_TO_PTHREAD -s STACK_SIZE=16777216 -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=64MB -s MAXIMUM_MEMORY=1GB -s EXIT_RUNTIME=1 -lnodefs.js --preload-file ${TOP_SRC}/tests/fixtures/tau.tgf@/tau.tgf")

if(TAU_PARSER_BUILD_BROWSER_TESTS)
    # Browser page files, copied next to the program output
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/index.html
        ${CMAKE_CURRENT_BINARY_DIR}/index.html
        COPYONLY
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/index.mjs
        ${CMAKE_CURRENT_BINARY_DIR}/index.mjs
        COPYONLY
    )
    # Generic terminal page glue (shell.css, terminal.js, sw.js) and the
    # vendored xterm.js files, shared with any other Emscripten terminal page.
    include(${TOP_SRC}/js/tau-wasm-terminal/tau-wasm-terminal.cmake)
    tau_wasm_terminal_page(${CMAKE_CURRENT_BINARY_DIR})
endif()
