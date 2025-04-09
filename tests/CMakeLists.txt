set(TESTS
	term_colors
	input
	parser
	tgf
	cli
	tree
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
				-static-libgcc
				-static-libstdc++
			)
	else()
		target_link_libraries(${N} ${PARSER_OBJECT_LIB_NAME})
	endif()
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
endforeach()

# add required sources to test_cli
target_sources("test_cli" PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../src/utility/cli.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../tools/tgf/tgf_cli.cpp")

set(DOCTEST_HEADER "${PROJECT_SOURCE_DIR}/src/doctest.h" CACHE PATH "Doctest header")
if (NOT EXISTS "${DOCTEST_HEADER}")
	message(STATUS "Downloading doctest to '${PROJECT_SOURCE_DIR}'")
	find_package(Wget REQUIRED)
	# TODO (MEDIUM) we should use a fixed tag or commit instead of master
	execute_process(COMMAND "${WGET_EXECUTABLE}" https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h -P ${PROJECT_SOURCE_DIR}/src)
endif ()

add_library(doctest INTERFACE)
target_compile_definitions(doctest INTERFACE TAU_USE_DOCTEST)
set(DOCTEST_CONFIG_ASSERTION_PARAMETERS_BY_VALUE "true")

add_link_options("-flto")

enable_testing()

set(DOCTESTS
	tree
	tree_traversals
)

foreach(X IN LISTS DOCTESTS)
	set(N "doctest_${X}")
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
		target_link_libraries(${N} ${PARSER_OBJECT_LIB_NAME} doctest)
	endif()
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
endforeach()
