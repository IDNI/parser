cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

set(namespace "idni")
set(PROJECT_SHORT_NAME parser)
project("${namespace}_${PROJECT_SHORT_NAME}"
	VERSION 0.0.1
	DESCRIPTION "IDNI's parser library"
)

################################################################################
option(WITH_TO_METHODS "add to_dot, to_tml and to_tml_rules methods" FALSE)
################################################################################
option(BUILD_TESTS "Build library tests" FALSE)
option(BUILD_STATIC_LIBRARY "build static library" FALSE)
option(BUILD_SHARED_LIBRARY "build shared library" FALSE)
set(BUILD_EXECUTABLE FALSE)
set(BUILD_SHARED_EXECUTABLE FALSE)
################################################################################
message(STATUS "WITH_TO_METHODS: ${WITH_TO_METHODS}")
message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "BUILD_STATIC_LIBRARY: ${BUILD_STATIC_LIBRARY}")
message(STATUS "BUILD_SHARED_LIBRARY: ${BUILD_SHARED_LIBRARY}")
################################################################################

# public headers and sources
set(PROJECT_HEADERS src/parser.h src/forest.h src/characters.h)
set(PROJECT_SOURCES src/parser.cpp src/parser_printing.cpp src/characters.cpp)	

# load initialize cmake and load target_setup and exclude functions
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(idni-common)

if(WITH_TO_METHODS)
	target_compile_definitions(${OBJECT_LIB_NAME} PRIVATE "-DWITH_TO_METHODS")
endif()

if(BUILD_TESTS)
	add_subdirectory(tests)
endif()	

include(installation)
