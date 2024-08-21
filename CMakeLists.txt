cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(namespace "idni")
set(PROJECT_SHORT_NAME parser)
project("${namespace}_${PROJECT_SHORT_NAME}"
	VERSION 0.0.1
	DESCRIPTION "IDNI's parser library"
)

option(PARSER_MEASURE              "Allow performance measures"           false)
option(PARSER_ADDRESS_SANITIZER    "Enable address sanitizer"               OFF)
option(PARSER_BUILD_STATIC_LIBRARY "Build static library"                   OFF)
option(PARSER_BUILD_SHARED_LIBRARY "Build shared library"                   OFF)
option(PARSER_BUILD_TESTS          "Build library tests"                    OFF)
option(PARSER_BUILD_EXAMPLES       "Build examples"                         OFF)
option(PARSER_BUILD_TOOLS          "Build library tools"                    OFF)
set_property(CACHE PARSER_ADDRESS_SANITIZER         PROPERTY STRINGS "OFF" "ON")
set_property(CACHE PARSER_BUILD_STATIC_LIBRARY      PROPERTY STRINGS "OFF" "ON")
set_property(CACHE PARSER_BUILD_SHARED_LIBRARY      PROPERTY STRINGS "OFF" "ON")
set_property(CACHE PARSER_BUILD_TESTS               PROPERTY STRINGS "OFF" "ON")
set_property(CACHE PARSER_BUILD_EXAMPLES            PROPERTY STRINGS "OFF" "ON")
set_property(CACHE PARSER_BUILD_TOOLS               PROPERTY STRINGS "OFF" "ON")

if(NOT PARSER_BUILD_STATIC_LIBRARY AND NOT PARSER_BUILD_SHARED_LIBRARY AND
	NOT PARSER_BUILD_TESTS AND NOT PARSER_BUILD_TOOLS AND NOT PARSER_BUILD_EXAMPLES)
		set(PARSER_BUILD_STATIC_LIBRARY true)
		set(PARSER_BUILD_TESTS true)
		set(PARSER_BUILD_TOOLS true)
		set(PARSER_BUILD_EXAMPLES true)
endif()

# fine tuning definitions according to build type
if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(PARSER_MEASURE true)
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT PARSER_MEASURE)
	set(PARSER_MEASURE true)
endif()

# defining available definitions
set(PARSER_DEFINITIONS
	PARSER_MEASURE
)

message(STATUS "PARSER_MEASURE: ${PARSER_MEASURE}")
message(STATUS "PARSER_BUILD_STATIC_LIBRARY: ${PARSER_BUILD_STATIC_LIBRARY}")
message(STATUS "PARSER_BUILD_SHARED_LIBRARY: ${PARSER_BUILD_SHARED_LIBRARY}")
message(STATUS "PARSER_BUILD_TESTS: ${PARSER_BUILD_TESTS}")
message(STATUS "PARSER_BUILD_EXAMPLES: ${PARSER_BUILD_EXAMPLES}")
message(STATUS "PARSER_BUILD_TOOLS: ${PARSER_BUILD_TOOLS}")
message(STATUS "PARSER_ADDRESS_SANITIZER: ${PARSER_ADDRESS_SANITIZER}")
################################################################################

if(PARSER_ADDRESS_SANITIZER)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
	set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif()

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
