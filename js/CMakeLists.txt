# This lists file requires CMAKE_TOOLCHAIN_FILE to be set
# to "<EMSCRIPTEN_DIR>/cmake/Modules/Platform/Emscripten.cmake"
# or use emscripten_toolchain module from ../cmake
# This setting must be done before using project()

cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(tauparserjs)

option(WITH_THREADS "use Threads" FALSE)

set(JSLIB_SOURCES
	../src/parser.cpp
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
string(APPEND JSLIB_LINK_OPTIONS " -s 'EXPORTED_RUNTIME_METHODS=[\"lengthBytesUTF8\",\"stringToUTF8\"]'")
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
target_include_directories(tauparser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(tauparser PRIVATE embind)
set_target_properties(tauparser PROPERTIES LINK_FLAGS ${JSLIB_LINK_OPTIONS})
target_compile_definitions(tauparser PRIVATE
        "-DNODEFSMOUNT")

