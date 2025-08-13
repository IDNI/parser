cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

message(STATUS "Adding ${CMAKE_CURRENT_SOURCE_DIR}/cmake to CMAKE_MODULE_PATH")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Read version from file and strip prerelease information for cmake project VERSION property
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" TAU_PARSER_VERSION)
string(STRIP "${TAU_PARSER_VERSION}" TAU_PARSER_VERSION)
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" TAU_PARSER_TARGET_VERSION "${TAU_PARSER_VERSION}")
message(STATUS "Parser version: ${TAU_PARSER_TARGET_VERSION} / ${TAU_PARSER_VERSION}")

if(TAU_PARSER_BUILD_EMSCRIPTEN)
	include(emscripten-toolchain)
	set_emscripten_toolchain()
endif()

# Project name and version
set(namespace "idni")
set(PROJECT_SHORT_NAME tauparser)
project("tauparser"
	VERSION "${TAU_PARSER_TARGET_VERSION}"
	DESCRIPTION "Tau Parser Library"
)

option(TAU_PARSER_INSTALL "Install tauparser targets and files" OFF)

if(NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	message(STATUS
		"${PROJECT_NAME} as a subproject of [${CMAKE_PROJECT_NAME}]")
	set(TAU_PARSER_INSTALL OFF)
else()
	message(STATUS "${PROJECT_NAME} as a top project")
	set(TAU_PARSER_INSTALL ON)
endif()

# Options
option(TAU_PARSER_MEASURE              "Allow performance measures"         OFF)
option(TAU_PARSER_ADDRESS_SANITIZER    "Enable address sanitizer"           OFF)
option(TAU_PARSER_BUILD_ALL            "Build all"                          OFF)
option(TAU_PARSER_BUILD_DOC            "Build documentation"                OFF)
option(TAU_PARSER_BUILD_HEADER_ONLY    "Build header only library"          OFF)
option(TAU_PARSER_BUILD_STATIC_LIBRARY "Build static library"               OFF)
option(TAU_PARSER_BUILD_SHARED_LIBRARY "Build shared library"               OFF)
option(TAU_PARSER_BUILD_EMSCRIPTEN     "Build for Emscripten"               OFF)
option(TAU_PARSER_BUILD_TESTS          "Build library tests"                OFF)
option(TAU_PARSER_BUILD_EXAMPLES       "Build examples"                     OFF)
option(TAU_PARSER_BUILD_TGF            "Build tgf tool"                     OFF)
option(TAU_PARSER_WINDOWS_PACKAGE      "Build Windows package"              OFF)
option(TAU_PARSER_WINDOWS_ZIP_PACKAGE  "Build Windows zip package"          OFF)

set_property(CACHE TAU_PARSER_MEASURE               PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_ADDRESS_SANITIZER     PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_ALL             PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_DOC             PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_HEADER_ONLY     PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_STATIC_LIBRARY  PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_SHARED_LIBRARY  PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_EMSCRIPTEN      PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_TESTS           PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_EXAMPLES        PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_BUILD_TGF             PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_WINDOWS_PACKAGE       PROPERTY STRINGS "OFF" "ON")
set_property(CACHE TAU_PARSER_WINDOWS_ZIP_PACKAGE   PROPERTY STRINGS "OFF" "ON")

# build all
if(TAU_PARSER_BUILD_ALL)
	set(TAU_PARSER_BUILD_HEADER_ONLY OFF)
	set(TAU_PARSER_BUILD_STATIC_LIBRARY ON)
	set(TAU_PARSER_BUILD_SHARED_LIBRARY ON)
	set(TAU_PARSER_BUILD_TESTS ON)
	set(TAU_PARSER_BUILD_TGF ON)
	set(TAU_PARSER_BUILD_EXAMPLES ON)
	set(TAU_PARSER_BUILD_DOC ON)
endif()

if(TAU_PARSER_BUILD_HEADER_ONLY AND (TAU_PARSER_BUILD_STATIC_LIBRARY OR TAU_PARSER_BUILD_SHARED_LIBRARY))
	message(FATAL_ERROR "Cannot build header-only library together with static or shared libraries")
endif()

if(NOT TAU_PARSER_BUILD_HEADER_ONLY AND NOT TAU_PARSER_BUILD_STATIC_LIBRARY AND NOT TAU_PARSER_BUILD_SHARED_LIBRARY)
	set(TAU_PARSER_BUILD_HEADER_ONLY ON)
endif()

if(TAU_PARSER_WINDOWS_PACKAGE AND TAU_PARSER_WINDOWS_ZIP_PACKAGE)
	message(FATAL_ERROR
	  "Cannot build both Windows and Windows zip packages at the same time")
endif()

################################################################################

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE true CACHE BOOL "")
set(USED_CMAKE_GENERATOR
	"${CMAKE_GENERATOR}" CACHE STRING "Expose CMAKE_GENERATOR" FORCE
)
if(USED_CMAKE_GENERATOR MATCHES "Ninja")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
endif()

# parallel processing
cmake_host_system_information(RESULT CORE_COUNT QUERY NUMBER_OF_LOGICAL_CORES)
if(CORE_COUNT GREATER 1)
	if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
		set(CMAKE_BUILD_PARALLEL_LEVEL ${CORE_COUNT})
	endif()
	message(STATUS "Parallel processing enabled (${CORE_COUNT} cores)")
endif()

if(TAU_PARSER_ADDRESS_SANITIZER)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
	set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=x86_64-w64-mingw32")
endif()

# allowed build types
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo)
	set(CMAKE_CONFIGURATION_TYPES
		"${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "" FORCE
	)
endif()

# default build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# link options
set(TAU_PARSER_DEFINITIONS "TAU_PARSER_MEASURE")
set(TAU_PARSER_LINK_OPTIONS "-flto=auto")

# compile options for different build types
set(TAU_PARSER_DEBUG_OPTIONS          "-O0;-DDEBUG;-ggdb3")
set(TAU_PARSER_RELEASE_OPTIONS        "-O3;-DNDEBUG")
set(TAU_PARSER_RELWITHDEBINFO_OPTIONS "-O3;-DNDEBUG;-ggdb3")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(TAU_PARSER_COMPILE_OPTIONS "${TAU_PARSER_DEBUG_OPTIONS}")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(TAU_PARSER_COMPILE_OPTIONS "${TAU_PARSER_RELWITHDEBINFO_OPTIONS}")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
	set(TAU_PARSER_COMPILE_OPTIONS "${TAU_PARSER_RELEASE_OPTIONS}")
endif()

# install destinations
set(TAU_PARSER_BIN_DESTINATION "bin")
set(TAU_PARSER_DOC_DESTINATION "share/doc/${PROJECT_NAME}")

################################################################################
message(STATUS "TAU_PARSER_INSTALL:              ${TAU_PARSER_INSTALL}")
message(STATUS "TAU_PARSER_BUILD_ALL:            ${TAU_PARSER_BUILD_ALL}")
message(STATUS "TAU_PARSER_BUILD_DOC:            ${TAU_PARSER_BUILD_DOC}")
message(STATUS "TAU_PARSER_BUILD_HEADER_ONLY:    ${TAU_PARSER_BUILD_HEADER_ONLY}")
message(STATUS "TAU_PARSER_BUILD_STATIC_LIBRARY: ${TAU_PARSER_BUILD_STATIC_LIBRARY}")
message(STATUS "TAU_PARSER_BUILD_SHARED_LIBRARY: ${TAU_PARSER_BUILD_SHARED_LIBRARY}")
message(STATUS "TAU_PARSER_BUILD_EMSCRIPTEN:     ${TAU_PARSER_BUILD_EMSCRIPTEN}")
message(STATUS "TAU_PARSER_BUILD_TESTS:          ${TAU_PARSER_BUILD_TESTS}")
message(STATUS "TAU_PARSER_BUILD_EXAMPLES:       ${TAU_PARSER_BUILD_EXAMPLES}")
message(STATUS "TAU_PARSER_BUILD_TGF:            ${TAU_PARSER_BUILD_TGF}")
message(STATUS "TAU_PARSER_ADDRESS_SANITIZER:    ${TAU_PARSER_ADDRESS_SANITIZER}")
message(STATUS "TAU_PARSER_WINDOWS_PACKAGE:      ${TAU_PARSER_WINDOWS_PACKAGE}")
message(STATUS "TAU_PARSER_WINDOWS_ZIP_PACKAGE:  ${TAU_PARSER_WINDOWS_ZIP_PACKAGE}")
message(STATUS "TAU_PARSER_BIN_DESTINATION:      ${TAU_PARSER_BIN_DESTINATION}")
message(STATUS "TAU_PARSER_DOC_DESTINATION:      ${TAU_PARSER_DOC_DESTINATION}")
message(STATUS "TAU_PARSER_DEFINITIONS:          ${TAU_PARSER_DEFINITIONS}")
message(STATUS "TAU_PARSER_COMPILE_OPTIONS:      ${TAU_PARSER_COMPILE_OPTIONS}")
message(STATUS "TAU_PARSER_LINK_OPTIONS:         ${TAU_PARSER_LINK_OPTIONS}")
message(STATUS "TAU_PARSER_MEASURE:              ${TAU_PARSER_MEASURE}")
################################################################################
message(STATUS "CMAKE_BUILD_TYPE:                ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_COMPILER_ID:           ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER:              ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_GENERATOR:                 ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_BUILD_PARALLEL_LEVEL:      ${CMAKE_BUILD_PARALLEL_LEVEL}")
message(STATUS "CMAKE_MODULE_PATH:               ${CMAKE_MODULE_PATH}")
################################################################################

include(target_git_definitions)

# setups a tauparser target
function(tauparser_setup target access libs)
	include(target_setup)
	target_setup(${target} ${access}
		"${TAU_PARSER_DEFINITIONS}"
		"${TAU_PARSER_COMPILE_OPTIONS}"
		"${TAU_PARSER_LINK_OPTIONS}"
		"${libs}")
	target_git_definitions(${target} INTERFACE)
endfunction()

################################################################################

include(version_license)
version_license(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/version_license.h.template"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/version_license.h")

if (TAU_PARSER_BUILD_EMSCRIPTEN)
	if (EMSCRIPTEN)
		add_subdirectory("js")
	endif()
else()
	add_subdirectory("src")
endif()

if(TAU_PARSER_BUILD_TGF)
	add_subdirectory(src/tgf)
endif()

if(TAU_PARSER_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(TAU_PARSER_BUILD_TESTS)
	add_subdirectory(tests)
endif()

# Documentation
if(TAU_PARSER_INSTALL)
	message(STATUS "Installing license and static docs to ${TAU_PARSER_DOC_DESTINATION}")
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt" "${CMAKE_CURRENT_SOURCE_DIR}/README.md" "${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md"
		DESTINATION "${TAU_PARSER_DOC_DESTINATION}"
		COMPONENT "Documentation")
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs"
		DESTINATION "${TAU_PARSER_DOC_DESTINATION}"
		COMPONENT "Documentation")
endif()

# Documentation
if(TAU_PARSER_BUILD_DOC)
	find_package(Doxygen)
	if(DOXYGEN_FOUND)
		message("Doxygen build started")
		# note the option ALL which allows to build the docs together with the application
		add_custom_target(documentation ALL
			COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating API documentation with Doxygen"
			VERBATIM
		)
	else()
		message(ERROR " Doxygen need to be installed to generate the doxygen documentation")
	endif()
endif()

# if _site directory with generated API documentation exists, install it
if(TAU_PARSER_INSTALL AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_site)
	message(STATUS "Installing generated docs to ${TAU_PARSER_DOC_DESTINATION}/api")
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/_site/
		DESTINATION ${TAU_PARSER_DOC_DESTINATION}/api
		COMPONENT "Documentation")
endif()

# cpack configuration for building release packages
include(tauparser-packages)
