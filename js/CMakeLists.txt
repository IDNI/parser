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

# tgf_cli.cpp reaches tgf_test_parser via tgf_test.h; a header-only build
# resolves productions() inline instead, so this stays out of that build
if(NOT TAU_PARSER_BUILD_HEADER_ONLY)
	list(APPEND JSLIB_SOURCES ../src/format/tgf.test/tgf_test_parser.generated.cpp)
endif()

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

message(STATUS "JSLIB_LINK_OPTIONS: ${JSLIB_LINK_OPTIONS}")

# EXPORT_ES6 and CommonJS exclude each other in one link, so each module
# format is its own target. Emscripten names the .wasm after the JS output,
# so the ES module build writes tauparser.esm.wasm.
function(tauparser_js_add_target target output_name suffix extra_link_options wrapper)
	add_executable(${target})
	target_setup(${target} PRIVATE
		"${TAU_PARSER_DEFINITIONS}"
		"${JSLIB_COMPILE_OPTIONS}"
		""
		"")
	set_target_properties(${target} PROPERTIES
		OUTPUT_NAME "${output_name}"
		SUFFIX "${suffix}"
		LINK_FLAGS "${JSLIB_LINK_OPTIONS} ${extra_link_options}")
	target_sources(${target} PRIVATE ${JSLIB_SOURCES})
	target_include_directories(${target} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../src
		${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf)
	target_link_libraries(${target} PRIVATE embind)
	# unordered_dense is set up by the top-level CMakeLists (unconditional)
	if(TARGET unordered_dense)
		target_link_libraries(${target} PRIVATE unordered_dense)
	else()
		target_include_directories(${target} PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/../external)
	endif()
	target_compile_definitions(${target} PRIVATE
		"-DNODEFSMOUNT")
	# header-only mode makes tgf_test.h resolve productions() inline instead
	# of needing the out-of-line definition linked in
	if(TAU_PARSER_BUILD_HEADER_ONLY)
		target_compile_definitions(${target} PRIVATE TAU_PARSER_BUILD_HEADER_ONLY)
	endif()
	# The smoke script loads the module by a relative path, so it goes in the
	# output directory of the module. It has a copy step of its own, so an
	# edit of the script reaches the build folder without a relink.
	add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/${wrapper}"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/${wrapper}" "${CMAKE_BINARY_DIR}/${wrapper}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${wrapper}")
	add_custom_target(${target}_smoke ALL DEPENDS "${CMAKE_BINARY_DIR}/${wrapper}")
endfunction()

tauparser_js_add_target(tauparser     tauparser     .js  ""                tauparser.node.js)
tauparser_js_add_target(tauparser_esm tauparser.esm .mjs "-s EXPORT_ES6=1" tauparser.node.mjs)

add_custom_command(TARGET tauparser POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_CURRENT_SOURCE_DIR}/tauparser.html
		$<TARGET_FILE_DIR:tauparser>)

# tgf.js - the TGF program for Node.js (legacy repl.h, no FTXUI, no pthreads).
# `node tgf.js` and the npm bin run main() through tgf.post.js. require() gets
# the factory, which the parity tests use.
set(TGF_NODE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf/tgf_cli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf/tgf_json_api.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/format/tgf/tgf_parser.generated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/format/json/json_parser.generated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/format/treemr/treemr_parser.generated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf/tgf_repl_parser.generated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/embindings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser.cpp
)
if(NOT TAU_PARSER_BUILD_HEADER_ONLY)
    list(APPEND TGF_NODE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../src/format/tgf.test/tgf_test_parser.generated.cpp)
endif()

add_executable(tgf_node ${TGF_NODE_SOURCES})
set_target_properties(tgf_node PROPERTIES OUTPUT_NAME tgf)
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
# header-only mode makes tgf_test.h resolve productions() inline instead
# of needing the out-of-line definition linked in
if(TAU_PARSER_BUILD_HEADER_ONLY)
    target_compile_definitions(tgf_node PRIVATE TAU_PARSER_BUILD_HEADER_ONLY)
endif()
# Suppress C++23 extension warnings from parser headers (static lambdas)
target_compile_options(tgf_node PRIVATE -Wno-c++23-extensions -fsigned-char)
target_link_libraries(tgf_node PRIVATE embind)
# INVOKE_RUN=0: a program run mounts the host file system before main().
# tgf.pre.js puts the #! line first, which the npm bin needs.
target_link_options(tgf_node PRIVATE
    "SHELL:--bind"
    "SHELL:-lnodefs.js"
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_NAME='TGF'"
    "SHELL:-s INVOKE_RUN=0"
    "SHELL:-s WASM=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s EXIT_RUNTIME=1"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['FS','NODEFS','callMain']"
    "SHELL:--extern-pre-js ${CMAKE_CURRENT_SOURCE_DIR}/tgf.pre.js"
    "SHELL:--extern-post-js ${CMAKE_CURRENT_SOURCE_DIR}/tgf.post.js"
)
set_property(TARGET tgf_node APPEND PROPERTY LINK_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tgf.pre.js
    ${CMAKE_CURRENT_SOURCE_DIR}/tgf.post.js)

# The build directory is the npm package: package.json lands beside
# tauparser.js, tauparser.esm.mjs and tgf.js, with the license that its
# "files" list names.
if(EMSCRIPTEN)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/package.json.in
		${CMAKE_BINARY_DIR}/package.json
		@ONLY)
	add_custom_target(tauparser_js_package_assets ALL
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE.md
			${CMAKE_BINARY_DIR}/LICENSE.md
		COMMENT "Copying the npm package license")
endif()
