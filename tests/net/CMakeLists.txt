set(NET_DOCTESTS
	line_address
	line_transport
	line_server
	line_support
)

foreach(X IN LISTS NET_DOCTESTS)
	set(N "doctest_${X}")
	add_executable(${N} "${N}.cpp")
	tauparser_setup(${N} PRIVATE tauparser_net)
	if(TAU_PARSER_IS_GNU_OR_CLANG)
		target_compile_options(${N} PUBLIC "-Wno-parentheses")
	endif()
	target_link_libraries(${N} PRIVATE tauparser_net doctest)
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
		CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_link_libraries(${N} PRIVATE -static-libgcc -static-libstdc++)
	endif()
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
	)
	target_include_directories(${N} PRIVATE "${PROJECT_SOURCE_DIR}/tests")
	add_test(NAME ${N} COMMAND ${N})
endforeach()

include(ctest-test-defaults)
tau_set_test_defaults()
