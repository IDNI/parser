include_guard(GLOBAL)

set(TAU_BUILD_JOBS_DOC "Number of jobs to use for the build (0 to auto-detect)")

set(TAU_BUILD_JOBS "0" CACHE STRING "${TAU_BUILD_JOBS_DOC}")

set(_tau_jobs_args "")
if(TAU_BUILD_JOBS MATCHES "^[1-9][0-9]*$")
	set(_tau_jobs_args "-DTAU_BUILD_JOBS=${TAU_BUILD_JOBS}")
endif()

execute_process(
	COMMAND bash -c
		"source \"$1\" && dep_entry \"\${@:2}\" && dep_jobs"
		bash "${CMAKE_CURRENT_LIST_DIR}/../scripts/devrc" ${_tau_jobs_args}
	OUTPUT_VARIABLE _tau_jobs
	OUTPUT_STRIP_TRAILING_WHITESPACE)

set(TAU_BUILD_JOBS "${_tau_jobs}"
	CACHE STRING "${TAU_BUILD_JOBS_DOC}" FORCE)

set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} "${TAU_BUILD_JOBS}")

set_property(CACHE TAU_BUILD_JOBS PROPERTY STRINGS "0" "1" "2" "3" "4" "5" "6" "7" "8" "9" "10" "11" "12" "13" "14" "15" "16" "17" "18" "19" "20")

message(STATUS "TAU_BUILD_JOBS: ${TAU_BUILD_JOBS}")

unset(_tau_jobs_args)
unset(_tau_jobs)
