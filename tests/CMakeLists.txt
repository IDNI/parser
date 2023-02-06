set(TESTS
	input
	parser
	tgf
)

foreach(X IN LISTS TESTS)
	set(N "test_${X}")
	add_executable(${N} "${N}.cpp")
	target_setup(${N})
	target_compile_options(${N} PUBLIC "-Wno-parentheses")
	target_link_libraries(${N} ${OBJECT_LIB_NAME} ${TEST_FRAMEWORK})
endforeach()
