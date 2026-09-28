include_guard(GLOBAL)

# A test with no TIMEOUT property has no time limit, so a hang holds the whole
# ctest run until the job is cancelled. One cache variable keeps a plain ctest
# run and every preset on the same limit.
set(TAU_TEST_TIMEOUT 600 CACHE STRING
	"Default time limit in seconds for a ctest test")

# set_tests_properties reaches only the tests of the calling directory, so call
# this after the last add_test() of every directory that adds tests. A test
# that sets TIMEOUT itself keeps it.
function(tau_set_default_test_timeout)
	get_property(_tests DIRECTORY PROPERTY TESTS)
	if(NOT _tests)
		return()
	endif()
	foreach(_test IN LISTS _tests)
		get_test_property("${_test}" TIMEOUT _timeout)
		if(_timeout STREQUAL "NOTFOUND")
			set_tests_properties("${_test}" PROPERTIES
				TIMEOUT "${TAU_TEST_TIMEOUT}")
		endif()
	endforeach()
endfunction()
