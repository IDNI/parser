set(TESTS
	cli
	input
	negation_fix
	parser
	term_colors
	tgf
	tree
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
	if (TAU_PARSER_IS_GNU_OR_CLANG)
		target_compile_options(${N} PUBLIC "-Wno-parentheses")
	endif()
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
		CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			target_link_libraries(${N} PRIVATE
				${PARSER_OBJECT_LIB_NAME}
				-static-libgcc
				-static-libstdc++
			)
	else()
		target_link_libraries(${N} PRIVATE ${PARSER_OBJECT_LIB_NAME})
	endif()
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
	add_test(NAME ${N} COMMAND ${N})
endforeach()

set(DOCTEST_HEADER "${PROJECT_SOURCE_DIR}/src/doctest.h" CACHE PATH "Doctest header")
if (NOT EXISTS "${DOCTEST_HEADER}")
	set(DOCTEST_VERSION "2.4.11")
	set(DOCTEST_URL
		"https://raw.githubusercontent.com/doctest/doctest/v${DOCTEST_VERSION}/doctest/doctest.h")
	message(STATUS "Downloading doctest ${DOCTEST_VERSION} to '${DOCTEST_HEADER}'")
	file(DOWNLOAD
		"${DOCTEST_URL}"
		"${DOCTEST_HEADER}"
		STATUS DOCTEST_DOWNLOAD_STATUS
		TLS_VERIFY ON)
	list(GET DOCTEST_DOWNLOAD_STATUS 0 DOCTEST_DOWNLOAD_CODE)
	list(GET DOCTEST_DOWNLOAD_STATUS 1 DOCTEST_DOWNLOAD_MESSAGE)
	if (NOT DOCTEST_DOWNLOAD_CODE EQUAL 0)
		message(FATAL_ERROR
			"Failed to download doctest from '${DOCTEST_URL}': "
			"${DOCTEST_DOWNLOAD_MESSAGE}")
	endif()
endif ()

add_library(doctest INTERFACE)
target_compile_definitions(doctest INTERFACE TAU_USE_DOCTEST)
set(DOCTEST_CONFIG_ASSERTION_PARAMETERS_BY_VALUE "true")

if (NOT EMSCRIPTEN AND TAU_PARSER_IS_GNU_OR_CLANG)
	add_link_options("-flto")
endif()

enable_testing()

set(DOCTESTS
	diagnostics
	dynamic_grow
	escapes
	input
	json
	measure
	repl_history
	tree
	tree_traversal1
	tree_traversal2
	tree_rewriter
	enhancements
	utf8
	boolean_grammar
	negation_regression
	parser
)

foreach(X IN LISTS DOCTESTS)
	set(N "doctest_${X}")
	add_executable(${N} "${N}.cpp")
	tauparser_setup(${N} PRIVATE "")
	if (TAU_PARSER_IS_GNU_OR_CLANG)
		target_compile_options(${N} PUBLIC "-Wno-parentheses")
	endif()
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
		CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			target_link_libraries(${N} PRIVATE
				tauparser
				${TEST_FRAMEWORK}
				-static-libgcc
				-static-libstdc++
			)
	else()
		target_link_libraries(${N} PRIVATE tauparser doctest)
	endif()
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
	add_test(NAME ${N} COMMAND ${N})
	# add_dependencies(${N} ${TAU_PARSER_STATIC_LIB_NAME})
endforeach()

# Benchmark executable (not a test — run manually)
add_executable(bench_enhancements bench_enhancements.cpp)
tauparser_setup(bench_enhancements PRIVATE "")
if (TAU_PARSER_IS_GNU_OR_CLANG)
	target_compile_options(bench_enhancements
		PUBLIC "-O3" "-Wno-parentheses")
elseif (MSVC)
	target_compile_options(bench_enhancements PUBLIC "/O2")
endif()
target_link_libraries(bench_enhancements PRIVATE tauparser)
target_include_directories(bench_enhancements PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
)
