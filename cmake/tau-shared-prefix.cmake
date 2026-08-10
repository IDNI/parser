include_guard(GLOBAL)

set(TAU_SHARED_PREFIX_DOC "Install prefix for shared dependencies")

set(TAU_SHARED_PREFIX "" CACHE STRING "${TAU_SHARED_PREFIX_DOC}")

set(_tau_prefix_args "")
if(NOT TAU_SHARED_PREFIX STREQUAL "")
	set(_tau_prefix_args "-DTAU_SHARED_PREFIX=${TAU_SHARED_PREFIX}")
endif()

execute_process(
	COMMAND bash -c
		"source \"$1\" && dep_entry \"\${@:2}\" && dep_shared_prefix"
		bash "${CMAKE_CURRENT_LIST_DIR}/../scripts/devrc" ${_tau_prefix_args}
	OUTPUT_VARIABLE _tau_prefix
	OUTPUT_STRIP_TRAILING_WHITESPACE)

set(TAU_SHARED_PREFIX "${_tau_prefix}"
	CACHE STRING "${TAU_SHARED_PREFIX_DOC}" FORCE)

unset(_tau_prefix_args)
unset(_tau_prefix)
