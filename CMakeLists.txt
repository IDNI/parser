cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(namespace "idni")
set(PROJECT_SHORT_NAME parser)
project("${namespace}_${PROJECT_SHORT_NAME}"
	VERSION 0.0.1
	DESCRIPTION "IDNI's parser library"
)

################################################################################
option(MEASURE "measure and print various parsing times" FALSE)
option(MEASURE_EACH_POS
	"measure and print time spent on each position of the input string"
	FALSE)
option(WITH_DEVHELPERS "add to_dot, to_tml and to_tml_rules functions" FALSE)
################################################################################
option(PARSER_BUILD_STATIC_LIBRARY "build static library" FALSE)
option(PARSER_BUILD_SHARED_LIBRARY "build shared library" FALSE)
option(PARSER_BUILD_TESTS "build library tests" FALSE)
option(PARSER_BUILD_EXAMPLES "build examples" FALSE)
option(PARSER_BUILD_TOOLS "build library tools" FALSE)
set(PARSER_BUILD_EXECUTABLE FALSE)
set(PARSER_BUILD_SHARED_EXECUTABLE FALSE)
if(NOT PARSER_BUILD_STATIC_LIBRARY AND NOT PARSER_BUILD_SHARED_LIBRARY AND
	NOT PARSER_BUILD_TESTS AND NOT PARSER_BUILD_TOOLS AND NOT PARSER_BUILD_EXAMPLES)
		set(PARSER_BUILD_STATIC_LIBRARY true)
		set(PARSER_BUILD_TESTS true)
		set(PARSER_BUILD_TOOLS true)
		set(PARSER_BUILD_EXAMPLES true)
endif()

if(PARSER_BUILD_TESTS)
	set(WITH_DEVHELPERS true)
endif()
################################################################################
message(STATUS "MEASURE: ${MEASURE}")
message(STATUS "MEASURE_EACH_POS: ${MEASURE_EACH_POS}")
message(STATUS "WITH_DEVHELPERS: ${WITH_DEVHELPERS}")
message(STATUS "PARSER_BUILD_STATIC_LIBRARY: ${PARSER_BUILD_STATIC_LIBRARY}")
message(STATUS "PARSER_BUILD_SHARED_LIBRARY: ${PARSER_BUILD_SHARED_LIBRARY}")
message(STATUS "PARSER_BUILD_TESTS: ${PARSER_BUILD_TESTS}")
message(STATUS "PARSER_BUILD_EXAMPLES: ${PARSER_BUILD_EXAMPLES}")
message(STATUS "PARSER_BUILD_TOOLS: ${PARSER_BUILD_TOOLS}")
################################################################################

# load common cmake settings and load functions
# 		target_setup(), target_compile_definitions_if() and exclude()
include(parser-common)

add_subdirectory(src)

if(PARSER_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(PARSER_BUILD_TESTS)
	add_subdirectory(tests)
endif()

if(PARSER_BUILD_TOOLS)
	add_subdirectory(tools)
endif()
