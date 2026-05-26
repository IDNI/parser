function(icecc_find_program out_var program)
	cmake_parse_arguments(ARG "" "" "PATHS" ${ARGN})
	if(NOT ARG_PATHS)
		set(ARG_PATHS
			/usr/lib/icecream/bin
			/usr/lib/icecream/libexec/icecc
			/usr/lib/icecream/libexec/icecc/bin)
	endif()
	find_program(${out_var} ${program}
		PATHS ${ARG_PATHS}
		NO_DEFAULT_PATH
	)
	if(NOT ${out_var})
		find_program(${out_var} ${program})
	endif()
	set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

function(icecc_set_launchers env_vars wrapper)
	set(_ICECC_LAUNCHER "${CMAKE_COMMAND}" -E env)
	list(APPEND _ICECC_LAUNCHER ${env_vars} ${wrapper})
	set(CMAKE_C_COMPILER_LAUNCHER "${_ICECC_LAUNCHER}"
		CACHE STRING "C compiler launcher (icecc)" FORCE)
	set(CMAKE_CXX_COMPILER_LAUNCHER "${_ICECC_LAUNCHER}"
		CACHE STRING "C++ compiler launcher (icecc)" FORCE)
endfunction()

# -- find icecc ---------------------------------------------------------------

if(NOT ICECC_BINARY)
	icecc_find_program(ICECC_BINARY icecc)
endif()
if(NOT ICECC_BINARY)
	message(WARNING "icecc not found")
	return()
endif()

# -- cache options ------------------------------------------------------------

set(TAU_ICECC_DEBUG "debug" CACHE STRING
	"icecc debug output level streamed to terminal")
set_property(CACHE TAU_ICECC_DEBUG PROPERTY STRINGS
	"error" "warning" "info" "debug")
set(TAU_ICECC_REMOTE_CPP "0" CACHE STRING
	"icecc remote preprocessing mode")
set_property(CACHE TAU_ICECC_REMOTE_CPP PROPERTY STRINGS "0" "1")
option(TAU_ICECC_REQUIRE_REMOTE
	"Fail the build when icecc selects localhost" ON)

# REQUIRE_REMOTE relies on the "building myself" log line, which only
# appears at debug level — force it on so the option is effective
# regardless of the user-chosen TAU_ICECC_DEBUG.
set(_ICECC_EFFECTIVE_DEBUG "${TAU_ICECC_DEBUG}")
if(TAU_ICECC_REQUIRE_REMOTE)
	set(_ICECC_EFFECTIVE_DEBUG "debug")
endif()

# -- compiler discovery -------------------------------------------------------

set(_ICECC_ENV_DIR "${CMAKE_BINARY_DIR}/icecc")
file(MAKE_DIRECTORY "${_ICECC_ENV_DIR}")

find_program(_ICECC_GCC_BINARY     gcc)
find_program(_ICECC_GXX_BINARY     g++)
find_program(_ICECC_CLANG_BINARY   clang)
find_program(_ICECC_CLANGXX_BINARY clang++)
icecc_find_program(_ICECC_COMPILER_WRAPPER compilerwrapper
	PATHS /usr/lib/icecream/libexec/icecc)
icecc_find_program(ICECC_CREATE_ENV icecc-create-env)

set(_ICECC_IS_CLANG OFF)
if(CMAKE_CXX_COMPILER MATCHES "(^|/)clang[+][+]")
	set(_ICECC_IS_CLANG ON)
endif()

# primary / cross-compiler entries and create-env args
set(_ICECC_ENTRIES)
set(_ICECC_CREATE_ARGS)
set(_ICECC_PRIMARY_CC)
set(_ICECC_PRIMARY_CXX)

if(_ICECC_IS_CLANG)
	if(NOT _ICECC_CLANG_BINARY OR NOT _ICECC_CLANGXX_BINARY)
		message(FATAL_ERROR
			"icecc clang build requires clang and clang++")
	endif()
	if(NOT _ICECC_COMPILER_WRAPPER)
		message(FATAL_ERROR
			"icecc clang build requires the icecream compilerwrapper;"
			" install the icecream package or set"
			" _ICECC_COMPILER_WRAPPER manually")
	endif()
	list(APPEND _ICECC_ENTRIES usr/bin/clang usr/bin/clang++)
	list(APPEND _ICECC_CREATE_ARGS
		--clang "${_ICECC_CLANG_BINARY}"
			"${_ICECC_COMPILER_WRAPPER}"
		--addfile "${_ICECC_CLANGXX_BINARY}")
	if(_ICECC_GCC_BINARY AND _ICECC_GXX_BINARY)
		list(APPEND _ICECC_ENTRIES
			usr/bin/gcc.bin usr/bin/g++.bin
			usr/bin/cc1 usr/bin/cc1plus)
		# Build a fresh list so --gcc args come first; list(PREPEND)
		# requires CMake 3.15 which is above the project minimum.
		set(_ICECC_NEW_ARGS
			--gcc "${_ICECC_GCC_BINARY}" "${_ICECC_GXX_BINARY}")
		list(APPEND _ICECC_NEW_ARGS ${_ICECC_CREATE_ARGS})
		set(_ICECC_CREATE_ARGS ${_ICECC_NEW_ARGS})
	endif()
	set(_ICECC_PRIMARY_CC  "${_ICECC_CLANG_BINARY}")
	set(_ICECC_PRIMARY_CXX "${_ICECC_CLANGXX_BINARY}")
else()
	if(NOT _ICECC_GCC_BINARY OR NOT _ICECC_GXX_BINARY)
		message(FATAL_ERROR
			"icecc gcc build requires gcc and g++")
	endif()
	list(APPEND _ICECC_ENTRIES
		usr/bin/gcc usr/bin/g++
		usr/bin/cc1 usr/bin/cc1plus)
	list(APPEND _ICECC_CREATE_ARGS
		--gcc "${_ICECC_GCC_BINARY}" "${_ICECC_GXX_BINARY}")
	if(_ICECC_CLANG_BINARY AND _ICECC_CLANGXX_BINARY
			AND _ICECC_COMPILER_WRAPPER)
		list(APPEND _ICECC_ENTRIES usr/bin/clang usr/bin/clang++)
		list(APPEND _ICECC_CREATE_ARGS
			--clang "${_ICECC_CLANG_BINARY}"
				"${_ICECC_COMPILER_WRAPPER}"
			--addfile "${_ICECC_CLANGXX_BINARY}")
	elseif(_ICECC_CLANG_BINARY AND _ICECC_CLANGXX_BINARY)
		message(STATUS
			"icecc: skipping clang cross-compiler"
			" (compilerwrapper not found)")
	endif()
endif()

# -- create or reuse compiler environment -------------------------------------

string(JOIN " " _ICECC_ENTRIES_ARG ${_ICECC_ENTRIES})
set(_ICECC_VERSION)
if(ICECC_CREATE_ENV)
	execute_process(
		COMMAND "${CMAKE_CURRENT_LIST_DIR}/use-icecream/icecc-env-resolve"
			--env-dir "${_ICECC_ENV_DIR}"
			--entries "${_ICECC_ENTRIES_ARG}"
			--create-env "${ICECC_CREATE_ENV}"
			-- ${_ICECC_CREATE_ARGS}
		OUTPUT_VARIABLE _ICECC_VERSION
		ERROR_VARIABLE _ICECC_RESOLVE_LOG
		RESULT_VARIABLE _ICECC_RESOLVE_RESULT
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(_ICECC_RESOLVE_RESULT EQUAL 0 AND _ICECC_VERSION)
		message(STATUS "Using icecream env: ${_ICECC_VERSION}")
	else()
		set(_ICECC_VERSION "")
		message(WARNING
			"icecc failed to resolve compiler environment:\n"
			"${_ICECC_RESOLVE_LOG}")
	endif()
endif()

# -- launcher (always via wrapper) --------------------------------------------

set(_ICECC_ENV_VARS
	"ICECC_DEBUG=${_ICECC_EFFECTIVE_DEBUG}"
	"ICECC_LOG_DIR=${CMAKE_BINARY_DIR}/icecc/logs"
	"ICECC_REMOTE_CPP=${TAU_ICECC_REMOTE_CPP}")
if(_ICECC_VERSION)
	list(APPEND _ICECC_ENV_VARS "ICECC_VERSION=${_ICECC_VERSION}")
endif()
if(_ICECC_IS_CLANG)
	list(APPEND _ICECC_ENV_VARS
		"ICECC_CC=${_ICECC_PRIMARY_CC}"
		"ICECC_CXX=${_ICECC_PRIMARY_CXX}"
		"ICECC_COLOR_DIAGNOSTICS=0")
endif()
if(TAU_ICECC_REQUIRE_REMOTE)
	list(APPEND _ICECC_ENV_VARS "TAU_ICECC_REQUIRE_REMOTE=1")
endif()

set(_ICECC_WRAPPER
	/usr/bin/env bash
	"${CMAKE_CURRENT_LIST_DIR}/use-icecream/icecc-terminal-log"
	"${ICECC_BINARY}")

icecc_set_launchers("${_ICECC_ENV_VARS}" "${_ICECC_WRAPPER}")

message(STATUS "Using icecream: ${ICECC_BINARY}")
