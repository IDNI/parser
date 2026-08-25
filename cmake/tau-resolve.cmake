# cmake/tau-resolve.cmake
#
# Single source of truth for two build settings that CMake and scripts/devrc
# both need to agree on:
#
#   TAU_BUILD_JOBS     -D  >  environment  >  auto = (cores + 1) / 2, min 1
#   TAU_SHARED_PREFIX  -D  >  environment  >  $HOME/.tau
#
# include()d from CMakeLists.txt, this module declares the two settings as
# cache variables holding *user intent* and computes the *resolved* value
# into a separate, uncached variable (TAU_BUILD_JOBS_RESOLVED /
# TAU_SHARED_PREFIX_RESOLVED) on every configure. Consumers that want the
# actual number/path to build with must read the _RESOLVED variable, not
# the cache entry - and the module must never write the resolved value
# back into the cache entry (not even with FORCE): doing so would turn
# "auto" into whatever it first resolved to, and the environment variable
# would silently stop being read on the next reconfigure of the same build
# directory. TAU_BUILD_JOBS's cache default is "" (nobody passed -D),
# distinct from an explicit -DTAU_BUILD_JOBS=0: "" falls through to the
# environment, "0" means "auto, ignoring the environment" -- see the
# comment where TAU_BUILD_JOBS_RESOLVED is computed, below.
#
# It also runs standalone as:
#
#   cmake -P cmake/tau-resolve.cmake <TAU_BUILD_JOBS|TAU_SHARED_PREFIX> [<requested>]
#
# which applies the same rule outside of a project() and prints the result -
# and only the result - to stdout, so scripts/devrc can shell out to this
# module instead of reimplementing the resolution rule in bash. <requested>
# plays the role the cache entry plays under CMake: pass the explicit value
# if the caller has one (e.g. a -D from the command line), or omit it (or
# pass "") for none. As with the cache entry above, an omitted/"" <requested>
# is "nothing explicit requested" (fall through to the environment) while an
# explicit "0" (a present 4th argv, distinguishable from a missing one -- see
# scripts/devrc's _tau_resolve) means "auto, ignore the environment".

include_guard(GLOBAL)

# Auto-detection path for _tau_resolve_build_jobs, below: half the logical
# core count, rounded up, minimum 1.
function(_tau_resolve_auto_build_jobs out_var)
	include(ProcessorCount)
	ProcessorCount(cores)
	if(cores EQUAL 0)
		message(WARNING
			"Could not detect number of logical CPU cores, defaulting to 1")
		set(cores 1)
	endif()
	math(EXPR cores_jobs "(${cores} + 1) / 2")
	if(cores_jobs LESS 1)
		set(cores_jobs 1)
	endif()
	set("${out_var}" "${cores_jobs}" PARENT_SCOPE)
endfunction()

# -D (or, standalone, the <requested> argument) beats the environment beats
# auto-detection (see _tau_resolve_auto_build_jobs above). requested="0" is
# a real value, not "unspecified": it means "auto-detect" explicitly and,
# per normal -D > env > default precedence, wins over the environment --
# it does NOT fall through to consult TAU_BUILD_JOBS from the environment.
# Only a genuinely absent/empty requested falls through to the environment,
# where (as before) a positive env value is used verbatim and a
# non-positive or unset one means auto.
function(_tau_resolve_build_jobs requested out_var)
	set(jobs "${requested}")
	if(NOT jobs MATCHES "^[1-9][0-9]*$")
		if(NOT requested STREQUAL "0" AND DEFINED ENV{TAU_BUILD_JOBS} AND "$ENV{TAU_BUILD_JOBS}" MATCHES "^[1-9][0-9]*$")
			set(jobs "$ENV{TAU_BUILD_JOBS}")
		else()
			_tau_resolve_auto_build_jobs(jobs)
		endif()
	endif()
	set("${out_var}" "${jobs}" PARENT_SCOPE)
endfunction()

# -D (or, standalone, the <requested> argument) beats the environment beats
# $HOME/.tau.
function(_tau_resolve_shared_prefix requested out_var)
	set(prefix "${requested}")
	if(prefix STREQUAL "")
		if(DEFINED ENV{TAU_SHARED_PREFIX} AND NOT "$ENV{TAU_SHARED_PREFIX}" STREQUAL "")
			set(prefix "$ENV{TAU_SHARED_PREFIX}")
		else()
			set(prefix "$ENV{HOME}/.tau")
		endif()
	endif()
	set("${out_var}" "${prefix}" PARENT_SCOPE)
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
	# Standalone lookup for scripts/devrc. message() has no "plain stdout,
	# no prefix" mode (STATUS gets a leading "-- ", everything else -
	# including plain message() - goes to stderr), so write straight to
	# /dev/stdout instead of shelling out to a second `cmake -E echo` just
	# to print one value. /dev/stdout is POSIX-only; this branch is only
	# ever reached from bash (scripts/devrc), so that's fine here.
	if(CMAKE_ARGC LESS 4)
		message(FATAL_ERROR
			"usage: cmake -P tau-resolve.cmake <TAU_BUILD_JOBS|TAU_SHARED_PREFIX> [<requested>]")
	endif()
	set(_tau_resolve_var "${CMAKE_ARGV3}")
	set(_tau_resolve_requested "${CMAKE_ARGV4}")

	if(_tau_resolve_var STREQUAL "TAU_BUILD_JOBS")
		_tau_resolve_build_jobs("${_tau_resolve_requested}" _tau_resolve_result)
	elseif(_tau_resolve_var STREQUAL "TAU_SHARED_PREFIX")
		_tau_resolve_shared_prefix("${_tau_resolve_requested}" _tau_resolve_result)
	else()
		message(FATAL_ERROR "tau-resolve: unknown variable '${_tau_resolve_var}'")
	endif()

	file(WRITE /dev/stdout "${_tau_resolve_result}\n")
else()
	# include()d from CMakeLists.txt (directly, or via cmake/use-emscripten.cmake
	# which needs TAU_SHARED_PREFIX_RESOLVED before project()): declare the
	# cache (user-intent) variables and resolve them into separate, uncached
	# variables that get recomputed on every configure.
	set(TAU_BUILD_JOBS_DOC
		"Number of jobs to use for the build (empty to auto-detect from the TAU_BUILD_JOBS env var, or half the cores; 0 to force auto-detection, ignoring the TAU_BUILD_JOBS env var)")
	set(TAU_BUILD_JOBS "" CACHE STRING "${TAU_BUILD_JOBS_DOC}")
	set_property(CACHE TAU_BUILD_JOBS PROPERTY STRINGS
		"" "0" "1" "2" "3" "4" "5" "6" "7" "8" "9" "10"
		"11" "12" "13" "14" "15" "16" "17" "18" "19" "20")
	# Passing the cache entry straight through here is what keeps a
	# changed TAU_BUILD_JOBS env value honoured across reconfigures of the
	# same build directory, as long as the cache entry itself is still
	# the empty default.
	_tau_resolve_build_jobs("${TAU_BUILD_JOBS}" TAU_BUILD_JOBS_RESOLVED)

	set(TAU_SHARED_PREFIX_DOC
		"Install prefix for shared dependencies (empty to auto-detect from the TAU_SHARED_PREFIX env var, or ~/.tau)")
	set(TAU_SHARED_PREFIX "" CACHE STRING "${TAU_SHARED_PREFIX_DOC}")
	_tau_resolve_shared_prefix("${TAU_SHARED_PREFIX}" TAU_SHARED_PREFIX_RESOLVED)

	unset(TAU_BUILD_JOBS_DOC)
	unset(TAU_SHARED_PREFIX_DOC)
endif()
