set(TESTS
	input
	parser
	tgf
	cli
)

foreach(X IN LISTS TESTS)
	set(N "test_${X}")
	add_executable(${N} "${N}.cpp")
	target_setup(${N})
	if(WITH_DEVHELPERS)
		target_compile_definitions(${N} PRIVATE "-DWITH_DEVHELPERS")
	endif()
	target_compile_options(${N} PUBLIC "-Wno-parentheses")
	target_link_libraries(${N} ${PARSER_OBJECT_LIB_NAME} ${TEST_FRAMEWORK})
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
endforeach()

# add required sources to test_cli
target_sources("test_cli" PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../src/cli.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../tools/tgf/tgf_cli.cpp")
