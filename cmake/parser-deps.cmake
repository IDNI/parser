# cmake/parser-deps.cmake
# Resolve FTXUI, unordered_dense, and the Boost headers from the dependency
# store at configure time. OFF by default: a build that does not opt in (tau's
# parser SDK, a local build) keeps the system-package / FetchContent paths in
# cmake/ftxui.cmake and cmake/unordered-dense.cmake. CI stages pass
# -DTAU_PARSER_DEPS_FROM_STORE=ON.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/tau-store.cmake")

option(TAU_PARSER_DEPS_FROM_STORE
	"Resolve FTXUI, unordered_dense, and the Boost headers from the dependency store" OFF)

# The producer scripts are bash; CMake must reach a bash that is not WSL's.
if(NOT DEFINED TAU_BASH OR TAU_BASH STREQUAL "")
	find_program(TAU_BASH NAMES bash NO_CACHE)
	if(NOT TAU_BASH)
		set(TAU_BASH bash)
	endif()
endif()

# The store target of this configure, by the same rules the dep scripts use
# for their -DTAU_DEP_TARGET default. Empty means "no store target"; the
# producer then falls back to its own host target.
function(_parser_deps_target out)
	set(_t "")
	if(TAU_PARSER_BUILD_EMSCRIPTEN)
		set(_t "wasm32-emscripten")
	elseif(APPLE)
		if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
			set(_t "darwin-arm64")
		else()
			set(_t "darwin-x86_64")
		endif()
	elseif(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
		set(_t "windows-x86_64-mingw")
	elseif(WIN32 AND MSVC)
		set(_t "windows-x86_64-msvc")
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
			set(_t "linux-arm64")
		else()
			set(_t "linux-x86_64")
		endif()
	endif()
	set(${out} "${_t}" PARENT_SCOPE)
endfunction()

# The command that runs one producer with the flag environment cleared and
# this configure's compiler, flags, target and job count.
function(_parser_deps_producer_command out script)
	_parser_deps_target(_target)
	string(TOUPPER "${CMAKE_BUILD_TYPE}" _build_type)
	set(_cflags_var "CMAKE_C_FLAGS_${_build_type}")
	set(_cxxflags_var "CMAKE_CXX_FLAGS_${_build_type}")
	set(_toolchain
		"-DTAU_DEP_CC=${CMAKE_C_COMPILER}"
		"-DTAU_DEP_CXX=${CMAKE_CXX_COMPILER}"
		"-DTAU_DEP_CFLAGS=${CMAKE_C_FLAGS} ${${_cflags_var}}"
		"-DTAU_DEP_CXXFLAGS=${CMAKE_CXX_FLAGS} ${${_cxxflags_var}}"
		"-DTAU_DEP_TARGET=${_target}")
	if(CMAKE_TOOLCHAIN_FILE)
		list(APPEND _toolchain "-DTAU_DEP_TOOLCHAIN=${CMAKE_TOOLCHAIN_FILE}")
	endif()
	# Git Bash rewrites an argument that starts with / as a path, which mangles
	# the /D flags in TAU_DEP_CFLAGS when a producer starts the nested cmake.
	# Exclude the flag arguments only: a real path, such as the toolchain file,
	# must still convert.
	set(${out}
		"${CMAKE_COMMAND}" -E env
			--unset=CPPFLAGS --unset=CFLAGS --unset=CXXFLAGS --unset=LDFLAGS
			"MSYS2_ARG_CONV_EXCL=-DTAU_DEP_CFLAGS=\;-DTAU_DEP_CXXFLAGS=\;-DCMAKE_C_FLAGS=\;-DCMAKE_CXX_FLAGS="
			"TAU_SHARED_PREFIX=${TAU_SHARED_PREFIX_RESOLVED}"
			"TAU_BUILD_JOBS=${TAU_BUILD_JOBS_RESOLVED}"
			"${TAU_BASH}" "${script}" ${_toolchain}
				"-DTAU_BUILD_JOBS=${TAU_BUILD_JOBS_RESOLVED}"
		PARENT_SCOPE)
endfunction()

# Run one producer and record its printed prefix in <cache_var>.
function(_parser_deps_ensure dep script cache_var)
	if(${cache_var})
		return()
	endif()
	_parser_deps_producer_command(_cmd "${script}")
	# ECHO_ERROR_VARIABLE needs 3.18, and the parser still allows 3.10. It
	# streams the progress and error lines live; the capture holds those, and
	# the build output stays in the entry log.
	set(_echo_error "")
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
		set(_echo_error ECHO_ERROR_VARIABLE)
	endif()
	execute_process(
		COMMAND ${_cmd}
		RESULT_VARIABLE _rc
		OUTPUT_VARIABLE _out
		ERROR_VARIABLE _err
		${_echo_error})
	if(NOT _rc EQUAL 0)
		message(FATAL_ERROR "cannot resolve the ${dep} package.\n${_err}\n${_out}")
	endif()
	if(NOT _out MATCHES "package prefix: (.*)")
		message(FATAL_ERROR "no prefix in the ${dep} producer output:\n${_out}")
	endif()
	set(_prefix "${CMAKE_MATCH_1}")
	string(STRIP "${_prefix}" _prefix)
	if(NOT IS_DIRECTORY "${_prefix}")
		message(FATAL_ERROR "the ${dep} prefix is not a directory: '${_prefix}'")
	endif()
	get_filename_component(_entry "${_prefix}" DIRECTORY)
	tau_store_use("${_entry}")
	set(${cache_var} "${_prefix}" CACHE PATH "" FORCE)
endfunction()

if(TAU_PARSER_DEPS_FROM_STORE)
	_parser_deps_ensure(unordered_dense
		"${PROJECT_SOURCE_DIR}/scripts/dep-unordered-dense.sh"
		TAU_PARSER_UNORDERED_DENSE_PREFIX)
	if(NOT TAU_PARSER_DONT_USE_FTXUI)
		_parser_deps_ensure(ftxui
			"${PROJECT_SOURCE_DIR}/scripts/dep-ftxui.sh"
			TAU_PARSER_FTXUI_PREFIX)
	endif()
	# The headers are identical on every target, so the producer needs no
	# compiler. Only serve and connect read them.
	if(TAU_PARSER_BUILD_SERVE)
		_parser_deps_ensure(boost_headers
			"${PROJECT_SOURCE_DIR}/scripts/dep-boost-headers.sh"
			TAU_PARSER_BOOST_HEADERS_PREFIX)
	endif()
endif()
