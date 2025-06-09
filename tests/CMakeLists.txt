set(TESTS
	term_colors
	input
	parser
	tgf
	cli
	bintree
)

set(TEST_LIBS tauparser ${TEST_FRAMEWORK})
if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
    CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(TEST_LIBS ${TEST_LIBS} -static-libgcc -static-libstdc++)
endif()

foreach(X IN LISTS TESTS)
	set(N "test_${X}")
	add_executable(${N} "${N}.cpp")
	tauparser_setup(${N} PRIVATE ${TEST_LIBS})
	target_compile_options(${N} PUBLIC "-Wno-parentheses")
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
	# add_dependencies(${N} ${TAU_PARSER_STATIC_LIB_NAME})
endforeach()

# add required sources to test_cli
target_sources("test_cli" PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../src/tgf/tgf_cli.cpp")
