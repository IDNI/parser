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
option(BUILD_TESTS "build library tests" TRUE)
option(BUILD_STATIC_LIBRARY "build static library" TRUE)
option(BUILD_SHARED_LIBRARY "build shared library" TRUE)
option(BUILD_EXAMPLES "build examples" TRUE)
option(BUILD_TOOLS "build library tools" TRUE)
set(BUILD_EXECUTABLE FALSE)
set(BUILD_SHARED_EXECUTABLE FALSE)
if(BUILD_TESTS)
	set(WITH_DEVHELPERS TRUE)
endif()
################################################################################
message(STATUS "MEASURE: ${MEASURE}")
message(STATUS "MEASURE_EACH_POS: ${MEASURE_EACH_POS}")
message(STATUS "WITH_DEVHELPERS: ${WITH_DEVHELPERS}")
message(STATUS "BUILD_STATIC_LIBRARY: ${BUILD_STATIC_LIBRARY}")
message(STATUS "BUILD_SHARED_LIBRARY: ${BUILD_SHARED_LIBRARY}")
message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "BUILD_TOOLS: ${BUILD_TOOLS}")
################################################################################

# public headers and sources
set(PROJECT_HEADERS
	src/characters.h
	src/charclasses.h
	src/defs.h
	src/devhelpers.h
	src/forest.h
	src/grammar.h
	src/parser.h
	src/parser.tpl.h
	src/tgf.h
)
set(PROJECT_SOURCES
	src/characters.cpp
	src/charclasses.cpp
	src/grammar.cpp
	src/parser.cpp
)

set(PROJECT_DEFINITIONS WITH_DEVHELPERS)

include(idni-common)

if(MEASURE)
	target_compile_definitions(${OBJECT_LIB_NAME} PRIVATE "-DMEASURE")
endif()
if(MEASURE_EACH_POS)
	target_compile_definitions(${OBJECT_LIB_NAME} PRIVATE
		"-DMEASURE_EACH_POS")
endif()

if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(BUILD_TESTS)
	add_subdirectory(tests)
endif()	

if(BUILD_TOOLS)
	add_subdirectory(tools)
endif()

include(installation)
