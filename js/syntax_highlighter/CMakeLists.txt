# Lean syntax_highlighter WASM target for VS Code / Cursor extension.
# Single-threaded, modular, no FTXUI, no preload, no pthreads.

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# Paths are relative to the top-level source so they work from any
# subdirectory build.
set(TOP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../..")

set(SYNTAX_HIGHLIGHTER_SOURCES
	${TOP_SRC}/src/parser.cpp
	${TOP_SRC}/src/format/tgf/tgf_parser.generated.cpp
	${TOP_SRC}/src/format/treemr/treemr_parser.generated.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../syntax_highlighter_bindings.cpp
)

set(JSLIB_COMPILE_OPTIONS
	"$<$<CONFIG:DEBUG>:-O0;-DDEBUG;-gsource-map;-fno-exceptions>"
	"$<$<CONFIG:RELEASE>:-O3;-DNDEBUG;-g0;-fno-exceptions>"
)

set(SYNTAX_HIGHLIGHTER_LINK_FLAGS "--bind --no-entry")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s MODULARIZE=1")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s EXPORT_NAME='syntax_highlighter'")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s WASM=1")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s ALLOW_MEMORY_GROWTH=1")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s EXIT_RUNTIME=1")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s STRICT=1")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s ERROR_ON_UNDEFINED_SYMBOLS=1")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s DISABLE_EXCEPTION_THROWING=1")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s DISABLE_EXCEPTION_CATCHING=1")
string(APPEND SYNTAX_HIGHLIGHTER_LINK_FLAGS " -s ASSERTIONS=1")

include(target_setup)

add_executable(syntax_highlighter)
target_setup(syntax_highlighter PRIVATE
	"${TAU_PARSER_DEFINITIONS}"
	"${JSLIB_COMPILE_OPTIONS}"
	""
	"")

target_sources(syntax_highlighter PRIVATE ${SYNTAX_HIGHLIGHTER_SOURCES})
target_include_directories(syntax_highlighter PRIVATE
	${TOP_SRC}/src
	${TOP_SRC}/src/tgf
)
# The vendored unordered_dense.h is set up as an INTERFACE target by the
# top-level CMakeLists.
if(TARGET unordered_dense)
	target_link_libraries(syntax_highlighter PRIVATE unordered_dense)
else()
	target_include_directories(syntax_highlighter PRIVATE
		${TOP_SRC}/external)
endif()
target_link_libraries(syntax_highlighter PRIVATE embind)
set_target_properties(syntax_highlighter PROPERTIES
	SUFFIX ".js"
	LINK_FLAGS "${SYNTAX_HIGHLIGHTER_LINK_FLAGS}")
