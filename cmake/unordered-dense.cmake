include_guard(GLOBAL)
include(GNUInstallDirs)

# ankerl/unordered_dense dependency resolution
#
# Header-only. Resolution takes a store prefix from
# TAU_PARSER_UNORDERED_DENSE_PREFIX, then a vendored header, then the pinned
# FetchContent source.
#
# Either path defines the plain `unordered_dense` interface target that
# src/CMakeLists.txt links into tauparser_static and installs with the SDK.
# The target's install interface is the SDK's own include dir, never a package
# or build-tree path, so an installed export stays relocatable.

set(TAU_PARSER_UNORDERED_DENSE_HEADER_ROOT "")
set(TAU_PARSER_UNORDERED_DENSE_PREFIX "" CACHE PATH
	"Store prefix holding the unordered_dense package; empty to search the toolchain root and fetch")

if(TAU_PARSER_UNORDERED_DENSE_PREFIX)
	find_package(unordered_dense CONFIG REQUIRED
		PATHS "${TAU_PARSER_UNORDERED_DENSE_PREFIX}"
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
else()
	find_package(unordered_dense CONFIG QUIET)
endif()
if(unordered_dense_FOUND)
	add_library(unordered_dense INTERFACE)
	get_target_property(_unordered_dense_package_include_dirs
		unordered_dense::unordered_dense INTERFACE_INCLUDE_DIRECTORIES)
	if(NOT _unordered_dense_package_include_dirs)
		message(FATAL_ERROR
			"the unordered_dense package exports no include directories")
	endif()
	# The package include dirs are the build interface. The install interface is
	# the SDK's own include dir, which src/CMakeLists.txt populates.
	set(_unordered_dense_include_dirs "")
	foreach(_unordered_dense_include_dir IN LISTS _unordered_dense_package_include_dirs)
		list(APPEND _unordered_dense_include_dirs
			"$<BUILD_INTERFACE:${_unordered_dense_include_dir}>")
		# The SDK installs the ankerl headers from this directory.
		if(EXISTS "${_unordered_dense_include_dir}/ankerl/unordered_dense.h"
				AND TAU_PARSER_UNORDERED_DENSE_HEADER_ROOT STREQUAL "")
			set(TAU_PARSER_UNORDERED_DENSE_HEADER_ROOT
				"${_unordered_dense_include_dir}")
		endif()
	endforeach()
	if(TAU_PARSER_UNORDERED_DENSE_HEADER_ROOT STREQUAL "")
		message(FATAL_ERROR
			"the unordered_dense package does not ship ankerl/unordered_dense.h")
	endif()
	list(APPEND _unordered_dense_include_dirs
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
	target_include_directories(unordered_dense INTERFACE
		${_unordered_dense_include_dirs})
	message(STATUS "unordered_dense: using installed package at ${unordered_dense_DIR}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/ankerl/unordered_dense.h")
	message(STATUS "Using vendored ankerl/unordered_dense from external/")
	add_library(unordered_dense INTERFACE)
	target_include_directories(unordered_dense INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
else()
	include(FetchContent)
	FetchContent_Declare(
		unordered_dense
		GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
		GIT_TAG        v4.4.0
		GIT_SHALLOW    TRUE
	)
	FetchContent_MakeAvailable(unordered_dense)
endif()

# Treat unordered_dense's headers as system includes: some compiler
# versions (observed: GCC 13.3.0 at -O3) emit a false-positive
# -Warray-bounds inside its memset-based clear_buckets() once inlined,
# which downstream -Werror consumers (e.g. tauparser_static) would
# otherwise turn into a hard build failure over third-party code.
get_target_property(unordered_dense_include_dirs unordered_dense
	INTERFACE_INCLUDE_DIRECTORIES)
if(unordered_dense_include_dirs)
	set_target_properties(unordered_dense PROPERTIES
		INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${unordered_dense_include_dirs}")
endif()
