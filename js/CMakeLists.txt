# This lists file requires CMAKE_TOOLCHAIN_FILE to be set
# to "<EMSCRIPTEN_DIR>/cmake/Modules/Platform/Emscripten.cmake"
# or use emscripten_toolchain module from ../cmake
# This setting must be done before using project()

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(tauparserjs)

option(WITH_THREADS "use Threads" FALSE)

set(JSLIB_SOURCES
	../src/parser.cpp
	../src/format/tgf/tgf_parser.generated.cpp
	../src/format/json/json_parser.generated.cpp
	../src/format/treemr/treemr_parser.generated.cpp
	../src/tgf/tgf_repl_parser.generated.cpp
	../src/tgf/tgf_cli.cpp
        embindings.cpp
)

set(JSLIB_COMPILE_OPTIONS
	"$<$<CONFIG:DEBUG>:-O0;-DDEBUG;-gsource-map;-fno-exceptions>"
	"$<$<CONFIG:RELEASE>:-O3;-DNDEBUG;-g0;-fno-exceptions>"
)
set(JSLIB_LINK_OPTIONS "--bind --no-entry -lnodefs.js")
string(APPEND JSLIB_LINK_OPTIONS " -s EXPORT_NAME='tauparser'")
string(APPEND JSLIB_LINK_OPTIONS " -s MODULARIZE=1")
# string(APPEND JSLIB_LINK_OPTIONS " -s EXPORT_ES6=1")
string(APPEND JSLIB_LINK_OPTIONS " -s STRICT=1")
string(APPEND JSLIB_LINK_OPTIONS " -s WASM=1")
string(APPEND JSLIB_LINK_OPTIONS " -s ALLOW_MEMORY_GROWTH=1")
string(APPEND JSLIB_LINK_OPTIONS " -s EXIT_RUNTIME=1")
string(APPEND JSLIB_LINK_OPTIONS " -s ERROR_ON_UNDEFINED_SYMBOLS=1")
string(APPEND JSLIB_LINK_OPTIONS " -s DISABLE_EXCEPTION_THROWING=1")
string(APPEND JSLIB_LINK_OPTIONS " -s DISABLE_EXCEPTION_CATCHING=1")
string(APPEND JSLIB_LINK_OPTIONS " -s 'EXPORTED_RUNTIME_METHODS=[\"lengthBytesUTF8\",\"stringToUTF8\",\"FS\",\"NODEFS\"]'")
string(APPEND JSLIB_LINK_OPTIONS " -s ASSERTIONS=1")

#string(APPEND JSLIB_LINK_OPTIONS " -s FETCH=1")
#string(APPEND JSLIB_LINK_OPTIONS " -s WARN_UNALIGNED=1")
# string(APPEND JSLIB_LINK_OPTIONS " -s USE_EMBIND=1")

include(target_setup)

# tauparser.js library
add_executable(tauparser)
target_setup(tauparser PRIVATE
        "${TAU_PARSER_DEFINITIONS}"
        "${JSLIB_COMPILE_OPTIONS}"
        ""
        "")

message(STATUS "JSLIB_LINK_OPTIONS: ${JSLIB_LINK_OPTIONS}")
target_sources(tauparser PRIVATE ${JSLIB_SOURCES})
target_include_directories(tauparser PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../src
	${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf)
target_link_libraries(tauparser PRIVATE embind)
# unordered_dense is set up by the top-level CMakeLists (unconditional)
if(TARGET unordered_dense)
	target_link_libraries(tauparser PRIVATE unordered_dense)
else()
	target_include_directories(tauparser PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../external)
endif()
set_target_properties(tauparser PROPERTIES LINK_FLAGS ${JSLIB_LINK_OPTIONS})
target_compile_definitions(tauparser PRIVATE
        "-DNODEFSMOUNT")

# tauparser.node.js does require('./tauparser.js'), so the harnesses go in the
# target's own output directory rather than this list file's binary dir
add_custom_command(TARGET tauparser POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_CURRENT_SOURCE_DIR}/tauparser.html
		${CMAKE_CURRENT_SOURCE_DIR}/tauparser.node.js
		$<TARGET_FILE_DIR:tauparser>)

# tgf_node.js - TGF REPL for Node.js (legacy repl.h, no FTXUI, no pthreads).
# Used for fast CI parity testing.  Command sequences are piped via stdin;
# stdout is captured and diffed against native tgf output.
set(TGF_NODE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf/tgf_cli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/format/tgf/tgf_parser.generated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/format/json/json_parser.generated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/format/treemr/treemr_parser.generated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf/tgf_repl_parser.generated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/embindings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser.cpp
)
set(TGF_NODE_LINK_OPTIONS
    -lnodefs.js
    -s MODULARIZE=1
    -s EXPORT_NAME='TGF'
    -s INVOKE_RUN=0
    -s WASM=1
    -s ALLOW_MEMORY_GROWTH=1
    -s EXIT_RUNTIME=1
    --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fixtures/tau.tgf@/tau.tgf
)

add_executable(tgf_node ${TGF_NODE_SOURCES})
target_setup(tgf_node PRIVATE "${TAU_PARSER_DEFINITIONS}" "" "" "")
target_include_directories(tgf_node PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf
)
# unordered_dense is set up by the top-level CMakeLists (unconditional)
if(TARGET unordered_dense)
    target_link_libraries(tgf_node PRIVATE unordered_dense)
else()
    target_include_directories(tgf_node PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../external)
endif()
target_compile_definitions(tgf_node PRIVATE
    TAU_PARSER_MEASURE_SCOPES
    TAU_PARSER_MEASURE_COUNTERS
    "-DNODEFSMOUNT")
# Suppress C++23 extension warnings from parser headers (static lambdas)
target_compile_options(tgf_node PRIVATE -Wno-c++23-extensions -fsigned-char)
target_link_libraries(tgf_node PRIVATE embind)
target_link_options(tgf_node PRIVATE
    "SHELL:--bind"
    "SHELL:--no-entry"
    "SHELL:-lnodefs.js"
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_NAME='TGF'"
    "SHELL:-s WASM=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s EXIT_RUNTIME=1"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['FS','NODEFS']"
    "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fixtures/tau.tgf@/tau.tgf"
)
