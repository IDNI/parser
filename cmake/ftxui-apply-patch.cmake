execute_process(
	COMMAND git apply --reverse --check "${PATCH}"
	RESULT_VARIABLE already_applied
	OUTPUT_QUIET
	ERROR_QUIET)

if(NOT already_applied EQUAL 0)
	execute_process(
		COMMAND git apply "${PATCH}"
		RESULT_VARIABLE apply_result)
	if(NOT apply_result EQUAL 0)
		message(FATAL_ERROR "Failed to apply patch: ${PATCH}")
	endif()
endif()
