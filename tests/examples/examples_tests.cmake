# Each example runs once on a small input, and its stdout must equal the
# expected output. json_parser reads a file that it gets as an argument.
set(EXAMPLE_TESTS
	basic_arithmetic
	basic_arithmetic_tgf
	csv_parser
	csv_parser1
	csv_parser2
	csv_parser3
	csv_parser4
	csv_parser5
	csv_parser6
	csv_parser7
	csv_parser8
	csv_parser9
	csv_parser10
	json_parser
)
set(_dir "${PROJECT_SOURCE_DIR}/tests/examples")
foreach(X IN LISTS EXAMPLE_TESTS)
	set(_input "${_dir}/${X}.in")
	set(_arg "")
	if(X STREQUAL "json_parser")
		set(_input "${_dir}/${X}.json")
		set(_arg "${_input}")
	endif()
	add_test(NAME example_${X}
		COMMAND ${CMAKE_COMMAND}
			-DEXE=$<TARGET_FILE:${X}>
			"-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
			-DINPUT=${_input}
			-DEXPECTED=${_dir}/${X}.out
			-DARG=${_arg}
			-P ${_dir}/run_example.cmake)
	set_tests_properties(example_${X} PROPERTIES TIMEOUT 120)
endforeach()
