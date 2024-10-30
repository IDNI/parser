set(TESTS
	term_colors
	input
	parser
	tgf
	cli
)

foreach(X IN LISTS TESTS)
	set(N "test_${X}")
	add_executable(${N} "${N}.cpp")
	target_setup(${N})
	target_compile_options(${N} PUBLIC "-Wno-parentheses")
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
		CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			target_link_libraries(${N}
				${PARSER_OBJECT_LIB_NAME}
				${TEST_FRAMEWORK}
				-static-libgcc
				-static-libstdc++
			)
	else()
		target_link_libraries(${N} ${PARSER_OBJECT_LIB_NAME} ${TEST_FRAMEWORK})
	endif()
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
endforeach()

# add required sources to test_cli
target_sources("test_cli" PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../src/cli.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../tools/tgf/tgf_cli.cpp")
