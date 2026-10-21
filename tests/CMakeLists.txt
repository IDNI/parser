set(TESTS
	input
	negation_fix
	parser
	term_colors
	tgf
	tree
)

set(TEST_LIBS tauparser)
if(EMSCRIPTEN)
	# Emscripten produces .js files; run via node
	find_program(NODE_EXECUTABLE node REQUIRED)
	set(TEST_RUNNER "${NODE_EXECUTABLE}")
	set(TEST_SUFFIX ".js")
	# tauparser is an EXECUTABLE under emscripten, not a library.
	# Tests are header-only; just need include paths + link flags.
	set(TEST_LIBS)
	macro(tauparser_emcc_setup target)
		target_compile_definitions(${target} PRIVATE TAU_PARSER_BUILD_HEADER_ONLY)
		target_compile_features(${target} PRIVATE cxx_std_23)
		target_include_directories(${target} PRIVATE
			${PROJECT_SOURCE_DIR}/src
			${PROJECT_SOURCE_DIR}/src/tgf)
		if(TARGET unordered_dense)
			target_link_libraries(${target} PRIVATE unordered_dense)
		endif()
		target_link_options(${target} PRIVATE
			"SHELL:-s ALLOW_MEMORY_GROWTH=1")
		# node starts a worker only when the main thread yields, so a test
		# that joins a new thread needs a worker from the pool.
		if(TAU_PARSER_WASM_PTHREADS)
			target_compile_options(${target} PRIVATE -pthread)
			target_link_options(${target} PRIVATE
				-pthread -Wno-pthreads-mem-growth
				"SHELL:-s PTHREAD_POOL_SIZE=10")
		endif()
	endmacro()
else()
	set(TEST_RUNNER "")
	set(TEST_SUFFIX "")
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
    CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(TEST_LIBS ${TEST_LIBS} -static-libgcc -static-libstdc++)
endif()

foreach(X IN LISTS TESTS)
	set(N "test_${X}")
	add_executable(${N} "${N}.cpp")
	if(EMSCRIPTEN)
		tauparser_emcc_setup(${N})
	else()
		tauparser_setup(${N} PRIVATE ${TEST_LIBS})
	endif()
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
	add_test(NAME ${N} COMMAND ${TEST_RUNNER} ${N}${TEST_SUFFIX})
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
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# clang accepts an unknown -Wno-* silently, so probe the positive form
	include(CheckCXXCompilerFlag)
	set(CMAKE_REQUIRED_FLAGS "-Werror")
	check_cxx_compiler_flag("-Wc2y-extensions" TAU_HAVE_WC2Y_EXTENSIONS)
	unset(CMAKE_REQUIRED_FLAGS)
	if(TAU_HAVE_WC2Y_EXTENSIONS)
		target_compile_options(doctest INTERFACE -Wno-c2y-extensions)
	endif()
	target_compile_options(doctest INTERFACE "-Wno-#warnings")
endif()
if(TAU_PARSER_IS_GNU_OR_CLANG)
	target_compile_options(doctest INTERFACE -Wno-float-equal)
endif()
add_compile_definitions(DOCTEST_CONFIG_ASSERTION_PARAMETERS_BY_VALUE)

if (NOT EMSCRIPTEN AND TAU_PARSER_IS_GNU_OR_CLANG)
	add_link_options("-flto")
endif()

enable_testing()

set(DOCTESTS
	diagnostics
	program_codes
	dynamic_grow
	escapes
	hashing
	input
	json
	measure
	repl_history
	repl_evaluator_base
	repl_json
	tree
	tree_traversal1
	tree_traversal2
	tree_rewriter
	enhancements
	utf8
	unicode_properties
	highlight
	relaxed_directives
	boolean_grammar
	negation_regression
	parser
	treemr
	tgf
	tgf_cli
	char_classes
	tgf_json_api
	grammar_ownership
	options
	program_args
)

# The serve and connect tests need Boost.Asio. They run on every platform;
# a cross build runs them through CMAKE_CROSSCOMPILING_EMULATOR.
if(TAU_PARSER_BUILD_SERVE AND NOT EMSCRIPTEN AND TARGET tgf)
	list(APPEND DOCTESTS tgf_serve tgf_connect)
endif()

# The JSON document tree on the tree library.
list(APPEND DOCTESTS json_tree)

list(APPEND DOCTESTS filesystem)

foreach(X IN LISTS DOCTESTS)
	# The highlight and char class tests read host grammar files
	# through PROJECT_SOURCE_DIR and are not WASM-runnable.
	if(EMSCRIPTEN AND (X STREQUAL "highlight"
			OR X STREQUAL "char_classes"))
		continue()
	endif()
	# The JSON API test drives the evaluator, so it needs tgf_cli_core.
	if(X STREQUAL "tgf_json_api")
		if(EMSCRIPTEN OR NOT TARGET tgf_cli_core)
			continue()
		endif()
	endif()
	if(X STREQUAL "tgf_serve" OR X STREQUAL "tgf_connect")
		if(NOT TARGET tgf_cli_core)
			continue()
		endif()
	endif()
	set(N "doctest_${X}")
	add_executable(${N} "${N}.cpp")
	if(EMSCRIPTEN)
		tauparser_emcc_setup(${N})
	elseif(X STREQUAL "tgf_json_api")
		tauparser_setup(${N} PRIVATE tgf_cli_core)
	elseif(X STREQUAL "tgf_serve")
		tauparser_setup(${N} PRIVATE tgf_cli_core)
	elseif(X STREQUAL "tgf_connect")
		tauparser_setup(${N} PRIVATE tgf_cli_core)
	else()
		tauparser_setup(${N} PRIVATE "")
	endif()
	if (TAU_PARSER_IS_GNU_OR_CLANG)
		target_compile_options(${N} PUBLIC "-Wno-parentheses")
	endif()
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
		CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		if(NOT EMSCRIPTEN)
			target_link_libraries(${N} PRIVATE
				tauparser
				doctest
				-static-libgcc
				-static-libstdc++
			)
		endif()
	else()
		if(NOT EMSCRIPTEN)
			target_link_libraries(${N} PRIVATE tauparser doctest)
		endif()
	endif()
	target_include_directories(${N} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
	if((N STREQUAL "doctest_highlight"
			OR N STREQUAL "doctest_char_classes"
			OR N STREQUAL "doctest_tgf_cli") AND NOT EMSCRIPTEN)
		target_compile_definitions(${N} PRIVATE
			PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
	endif()
	if(${N} STREQUAL "doctest_tgf_json_api"
			OR ${N} STREQUAL "doctest_tgf_serve"
			OR ${N} STREQUAL "doctest_tgf_connect")
		target_compile_definitions(${N} PRIVATE
			PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
			PROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}")
	endif()
	# the highlighter compiles treemr patterns, so it needs the DSL parser
	if(${N} STREQUAL "doctest_highlight" AND NOT EMSCRIPTEN AND NOT TAU_PARSER_BUILD_HEADER_ONLY)
		target_sources(${N} PRIVATE
			"${PROJECT_SOURCE_DIR}/src/format/treemr/treemr_parser.generated.cpp")
		set_source_files_properties(
			"${PROJECT_SOURCE_DIR}/src/format/treemr/treemr_parser.generated.cpp"
			PROPERTIES COMPILE_OPTIONS "${TAU_PARSER_DATA_SOURCE_OPTIONS}")
	endif()
	# these tests include a generated header directly, without linking
	# tgf_cli_core; a header-only build resolves productions() inline instead
	if(${N} STREQUAL "doctest_treemr" AND NOT EMSCRIPTEN AND NOT TAU_PARSER_BUILD_HEADER_ONLY)
		target_sources(${N} PRIVATE
			"${PROJECT_SOURCE_DIR}/src/format/treemr/treemr_parser.generated.cpp")
		set_source_files_properties(
			"${PROJECT_SOURCE_DIR}/src/format/treemr/treemr_parser.generated.cpp"
			PROPERTIES COMPILE_OPTIONS "${TAU_PARSER_DATA_SOURCE_OPTIONS}")
	endif()
	# the generic REPL base runs on the tgf REPL parser
	if(${N} STREQUAL "doctest_repl_evaluator_base" AND NOT EMSCRIPTEN
			AND NOT TAU_PARSER_BUILD_HEADER_ONLY)
		target_sources(${N} PRIVATE
			"${PROJECT_SOURCE_DIR}/src/tgf/tgf_repl_parser.generated.cpp")
		set_source_files_properties(
			"${PROJECT_SOURCE_DIR}/src/tgf/tgf_repl_parser.generated.cpp"
			PROPERTIES COMPILE_OPTIONS "${TAU_PARSER_DATA_SOURCE_OPTIONS}")
	endif()
	if(${N} STREQUAL "doctest_tgf_cli" AND NOT EMSCRIPTEN)
		if(TARGET tgf_cli_core)
			# the core library owns the generated parsers
			target_link_libraries(${N} PRIVATE tgf_cli_core)
			# tgf_run() is defined by the core library only
			target_compile_definitions(${N} PRIVATE TAU_TEST_HAS_TGF_CLI)
		elseif(NOT TAU_PARSER_BUILD_HEADER_ONLY)
			# a header-only build resolves productions() inline instead
			target_sources(${N} PRIVATE
				"${PROJECT_SOURCE_DIR}/src/tgf/tgf_repl_parser.generated.cpp")
			set_source_files_properties(
				"${PROJECT_SOURCE_DIR}/src/tgf/tgf_repl_parser.generated.cpp"
				PROPERTIES COMPILE_OPTIONS "${TAU_PARSER_DATA_SOURCE_OPTIONS}")
		endif()
	endif()
	# The options tests run a few threads. A wasm build without
	# TAU_PARSER_WASM_PTHREADS skips its thread cases instead.
	if((${N} STREQUAL "doctest_options"
			OR ${N} STREQUAL "doctest_program_args")
			AND (NOT EMSCRIPTEN OR TAU_PARSER_WASM_PTHREADS))
		find_package(Threads REQUIRED)
		target_link_libraries(${N} PRIVATE Threads::Threads)
	endif()
	add_test(NAME ${N} COMMAND ${TEST_RUNNER} ${N}${TEST_SUFFIX})
	if(${N} STREQUAL "doctest_tgf_serve"
			OR ${N} STREQUAL "doctest_tgf_connect")
		# The Windows fixture starts the real tgf; ctest passes its path
		# through the environment so no backslash needs escaping.
		set_tests_properties(${N} PROPERTIES
			ENVIRONMENT "TGF_BINARY_PATH=$<TARGET_FILE:tgf>")
	endif()
	if(${N} STREQUAL "doctest_measure")
		set_tests_properties(${N} PROPERTIES TIMEOUT 120)
	endif()
	# add_dependencies(${N} ${TAU_PARSER_STATIC_LIB_NAME})
endforeach()

# The FTXUI REPL fixture and its pipe-mode tests (only when the tgf tool is
# built with FTXUI). The fixture builds on every platform, because Windows
# PowerShell puts the byte order mark before piped text.
if(TARGET tgf AND TAU_PARSER_HAS_FTXUI_DEP)
	find_package(Threads REQUIRED)
	add_executable(repl_ftxui_async_fixture repl_ftxui_async_fixture.cpp)
	tauparser_setup(repl_ftxui_async_fixture PRIVATE tauparser)
	target_include_directories(repl_ftxui_async_fixture PRIVATE
		"${PROJECT_SOURCE_DIR}/src")
	target_link_libraries(repl_ftxui_async_fixture PRIVATE
		ftxui::screen ftxui::dom ftxui::component Threads::Threads)
	# A Windows host pipes through PowerShell, which writes the mark itself.
	# Elsewhere the script writes it, and a cross build runs the fixture
	# through its emulator.
	if(CMAKE_HOST_WIN32)
		add_test(NAME ftxui_repl_pipe_bom
			COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
				"${CMAKE_CURRENT_SOURCE_DIR}/parity/ftxui_repl_pipe_bom.ps1"
				$<TARGET_FILE:repl_ftxui_async_fixture>)
	else()
		add_test(NAME ftxui_repl_pipe_bom
			COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/parity/ftxui_repl_pipe_bom.sh"
				${CMAKE_CROSSCOMPILING_EMULATOR}
				$<TARGET_FILE:repl_ftxui_async_fixture>)
	endif()
	if(NOT CMAKE_HOST_WIN32)
		add_test(NAME ftxui_repl_pipe_continuation
			COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/parity/ftxui_repl_pipe_continuation.sh"
				${CMAKE_CROSSCOMPILING_EMULATOR}
				$<TARGET_FILE:repl_ftxui_async_fixture>)
		add_test(NAME plain_repl_pipe_order
			COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/parity/plain_repl_pipe_order.sh"
				${CMAKE_CROSSCOMPILING_EMULATOR}
				$<TARGET_FILE:repl_ftxui_async_fixture>)
	endif()
	# Runs `tgf repl` vs `tgf repl -x` over identical scripted input and
	# asserts identical .tgf_history. See ftxui_repl_parity.sh.
	if(NOT WIN32)
		add_test(NAME ftxui_repl_parity
			COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/parity/ftxui_repl_parity.sh"
				$<TARGET_FILE:tgf>
				"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ambig.tgf")
	endif()
endif()

if(TARGET tgf AND NOT CMAKE_HOST_WIN32)
	add_test(NAME tgf_repl_prompt
		COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/parity/tgf_repl_prompt.sh"
			${CMAKE_CROSSCOMPILING_EMULATOR}
			$<TARGET_FILE:tgf>)
endif()

# WASM parity test - compare native tgf output against tgf.js (Node.js)
# for a grammar file and REPL commands. Requires the tgf tool, Node.js and
# a configured wasm tree, whose tgf.js the dev run builds before ctest.
if(TARGET tgf AND NOT WIN32)
	find_program(NODE_EXECUTABLE node)
	if(NODE_EXECUTABLE
		AND EXISTS "${PROJECT_SOURCE_DIR}/build/release-wasm/CMakeCache.txt")
		add_test(NAME tgf_wasm_parity
			COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/parity/tgf_wasm_parity.sh"
				$<TARGET_FILE:tgf>
				"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tau.tgf"
				"grammar" "p x = 0 || y = 1" "p x = 2" "quit")
		add_test(NAME tgf_wasm_reload
			COMMAND ${NODE_EXECUTABLE}
				"${CMAKE_CURRENT_SOURCE_DIR}/parity/tgf_wasm_reload.js"
				"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.tgf"
				"${PROJECT_SOURCE_DIR}/src/format/json/json.tgf")
		# The real tgf.js program (not require()), run from host paths
		# under /tmp; the grammar mount and the cwd both cover /tmp.
		add_test(NAME tgf_wasm_cli_paths
			COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/parity/tgf_wasm_cli_paths.sh"
				"${NODE_EXECUTABLE}"
				"${PROJECT_SOURCE_DIR}/build/release-wasm/tgf.js"
				"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.tgf")
	endif()
endif()

# The presets pass the interpreter that dep_python proves to run. FindPython3
# runs each candidate too, so the Windows store stub never passes.
if(NOT Python3_EXECUTABLE)
	find_package(Python3 COMPONENTS Interpreter QUIET)
endif()

# The JSON API must end every message with one LF on the real stdout. An
# in-process capture cannot see the line-ending translation that a
# text-mode stdout does, so this runs the program and reads its bytes.
if(TARGET tgf)
	if(Python3_EXECUTABLE)
		add_test(NAME json_stdout_bytes
			COMMAND "${Python3_EXECUTABLE}"
				"${CMAKE_CURRENT_SOURCE_DIR}/parity/json_stdout_bytes.py"
				"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.tgf"
				${CMAKE_CROSSCOMPILING_EMULATOR}
				$<TARGET_FILE:tgf>)
	endif()
endif()

# A real server process on a free port, driven over a plain socket. The
# harness runs the native python3 and passes the emulator before tgf.exe.
if(TARGET tgf AND TAU_PARSER_BUILD_SERVE AND NOT EMSCRIPTEN)
	if(Python3_EXECUTABLE)
		add_test(NAME tgf_serve_smoke
			COMMAND "${Python3_EXECUTABLE}"
				"${CMAKE_CURRENT_SOURCE_DIR}/serve/tgf_serve_smoke.py"
				"${PROJECT_SOURCE_DIR}/tests/fixtures/tiny.tgf"
				${CMAKE_CROSSCOMPILING_EMULATOR}
				$<TARGET_FILE:tgf>)
		set_tests_properties(tgf_serve_smoke PROPERTIES TIMEOUT 120)
	endif()
endif()

# Benchmark executable (not a test - run manually)
if(NOT EMSCRIPTEN)
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
endif()

# The committed generated parsers must match their grammars, so a grammar
# change without a regenerated parser fails here.
if(TARGET tgf AND TAU_PARSER_BASH AND NOT CMAKE_CROSSCOMPILING)
	add_test(NAME tgf_roundtrip_self_check
		COMMAND "${TAU_PARSER_BASH}" "${PROJECT_SOURCE_DIR}/scripts/tgf-roundtrip-self-check.sh"
			$<TARGET_FILE:tgf>)
endif()

# Parity between the native tgf and the MinGW build run under wine.
# Registered only when the cross-build and wine are both present.
if(TARGET tgf AND NOT WIN32)
	set(MINGW_DIR "${PROJECT_BINARY_DIR}/../release-w64")
	find_program(WINE_EXECUTABLE wine)
	if(WINE_EXECUTABLE AND EXISTS "${MINGW_DIR}/tgf.exe")
		add_test(NAME tgf_wine_parity
			COMMAND sh
				"${CMAKE_CURRENT_SOURCE_DIR}/parity/tgf_wine_parity.sh"
				$<TARGET_FILE:tgf>
				"${MINGW_DIR}/tgf.exe"
				"${PROJECT_SOURCE_DIR}/src/format/json/json.tgf")
		if(Python3_EXECUTABLE)
			add_test(NAME json_stdout_bytes_wine
				COMMAND "${Python3_EXECUTABLE}"
					"${CMAKE_CURRENT_SOURCE_DIR}/parity/json_stdout_bytes.py"
					"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.tgf"
					"${WINE_EXECUTABLE}" "${MINGW_DIR}/tgf.exe")
		endif()
	endif()
endif()

# The REPL and schema tests need the modules of tests/requirements.txt. When
# the configure interpreter lacks them, dep-test-python makes a venv with them.
# Without the venv these tests are not built, or with
# TAU_PARSER_TEST_PYTHON_REQUIRED the configure fails.
if(TARGET tgf AND NOT WIN32)
	set(TAU_PARSER_TEST_PYTHON "${Python3_EXECUTABLE}")
	set(_py_deps_missing 1)
	if(TAU_PARSER_TEST_PYTHON)
		execute_process(
			COMMAND "${TAU_PARSER_TEST_PYTHON}" -c
				"import pexpect, pyte, jsonschema, referencing"
			RESULT_VARIABLE _py_deps_missing
			OUTPUT_QUIET ERROR_QUIET)
	endif()
	if(_py_deps_missing)
		set(TAU_PARSER_TEST_PYTHON "")
		set(_out "")
		if(TAU_PARSER_BASH)
			execute_process(
				COMMAND "${TAU_PARSER_BASH}"
					"${PROJECT_SOURCE_DIR}/scripts/dep-test-python.sh"
				RESULT_VARIABLE _rc
				OUTPUT_VARIABLE _out)
			if(_rc EQUAL 0 AND _out MATCHES "test python: ([^\n]*)")
				set(TAU_PARSER_TEST_PYTHON "${CMAKE_MATCH_1}")
			endif()
		endif()
		if(NOT TAU_PARSER_TEST_PYTHON)
			# a host without uv or without the network still builds the
			# other tests; CI sets the option, so there it is an error
			if(TAU_PARSER_TEST_PYTHON_REQUIRED)
				message(FATAL_ERROR "cannot make the test Python venv; see the output above.\n${_out}")
			endif()
			message(WARNING "cannot make the test Python venv, so the REPL and schema tests are not built; see the output above.\n${_out}")
		endif()
	endif()
	message(STATUS "Test Python: ${TAU_PARSER_TEST_PYTHON}")
endif()

# Native REPL tests - python3 + pexpect/pyte.
if(TARGET tgf AND NOT WIN32 AND TAU_PARSER_TEST_PYTHON)
	# Legacy (-X) REPL keybindings - no FTXUI required.
	add_test(NAME repl_keybindings_test
		COMMAND "${TAU_PARSER_TEST_PYTHON}"
			"${CMAKE_CURRENT_SOURCE_DIR}/repl/test_repl_keybindings.py"
			$<TARGET_FILE:tgf>
			"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tau.tgf")
	if(TAU_PARSER_HAS_FTXUI_DEP)
		add_test(NAME repl_ftxui_test
			COMMAND "${TAU_PARSER_TEST_PYTHON}"
				"${CMAKE_CURRENT_SOURCE_DIR}/repl/test_repl_ftxui.py"
				$<TARGET_FILE:tgf>
				"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tau.tgf")
		add_test(NAME repl_ftxui_async_test
			COMMAND "${TAU_PARSER_TEST_PYTHON}"
				"${CMAKE_CURRENT_SOURCE_DIR}/repl/test_repl_ftxui_async.py"
				$<TARGET_FILE:repl_ftxui_async_fixture>)
		add_test(NAME repl_ftxui_drain_test
			COMMAND "${TAU_PARSER_TEST_PYTHON}"
				"${CMAKE_CURRENT_SOURCE_DIR}/repl/test_repl_ftxui_drain.py"
				$<TARGET_FILE:tgf>
				"${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tau.tgf")
	endif()
	# tau.tgf is large; serialize REPL tests and allow generous load time.
	set_tests_properties(repl_keybindings_test
		PROPERTIES TIMEOUT 300 RESOURCE_LOCK repl_tau)
	if(TAU_PARSER_HAS_FTXUI_DEP)
		set_tests_properties(repl_ftxui_test repl_ftxui_async_test
				repl_ftxui_drain_test
			PROPERTIES TIMEOUT 300 RESOURCE_LOCK repl_tau)
	endif()
endif()

# Validate real tgf JSON output against the AST schema and the API schema.
if(TARGET tgf AND NOT WIN32 AND TAU_PARSER_TEST_PYTHON)
	add_test(NAME ast_schema_check
		COMMAND "${TAU_PARSER_TEST_PYTHON}"
			"${CMAKE_CURRENT_SOURCE_DIR}/schema_check.py"
			"--ast"
			"$<TARGET_FILE:tgf>"
			"${PROJECT_SOURCE_DIR}/tests/fixtures/ambig.tgf"
			"${PROJECT_SOURCE_DIR}/src/format/ast.json/ast.schema.json")
	add_test(NAME tgf_api_schema_check
		COMMAND "${TAU_PARSER_TEST_PYTHON}"
			"${CMAKE_CURRENT_SOURCE_DIR}/schema_check.py"
			"--api"
			"$<TARGET_FILE:tgf>"
			"${PROJECT_SOURCE_DIR}/tests/fixtures/ambig.tgf"
			"${PROJECT_SOURCE_DIR}/src/format/tgf_api.json/tgf_api.schema.json"
			"${PROJECT_SOURCE_DIR}/src/format/json/report.schema.json"
			"${PROJECT_SOURCE_DIR}/src/format/ast.json/ast.schema.json")
endif()

add_test(NAME tau_manifest_input_id
	COMMAND ${CMAKE_COMMAND} -P
		"${CMAKE_CURRENT_SOURCE_DIR}/tau-manifest-self-check.cmake"
		"${PROJECT_SOURCE_DIR}/cmake/tau-manifest.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}")

# LOCAL store primitive: another pure, hermetic self-check with the same
# build-local temp dir.
add_test(NAME tau_store_local
	COMMAND ${CMAKE_COMMAND} -P
		"${CMAKE_CURRENT_SOURCE_DIR}/tau-store-self-check.cmake"
		"${PROJECT_SOURCE_DIR}/cmake/tau-store.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}")

# dep_ensure shell bridge: hermetic, fake producer, build-local scratch prefix.
# Skipped only when bash is unavailable.
find_program(TAU_TEST_BASH bash NO_CACHE)
if(TAU_TEST_BASH)
	add_test(NAME tau_dep_ensure
		COMMAND "${TAU_TEST_BASH}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tau-dep-ensure-self-check.sh")
	# Transport round-trip and its failure paths: export, import, verify, a
	# forced mode loss, a truncated archive, a content corruption, and a missing
	# expected entry.
	add_test(NAME tau_store_transport
		COMMAND "${TAU_TEST_BASH}"
			"${CMAKE_CURRENT_SOURCE_DIR}/store-transport-self-check.sh")
	# Remote store bridge: the <dep>-<id> tag naming and the "remote miss
	# builds" rule, with a fake oras on PATH.
	add_test(NAME tau_store_remote
		COMMAND "${TAU_TEST_BASH}"
			"${CMAKE_CURRENT_SOURCE_DIR}/store-remote-self-check.sh")
	# The release workflow must pin a new tag to the built commit.
	add_test(NAME release_workflow_publish_target
		COMMAND "${TAU_TEST_BASH}"
			"${CMAKE_CURRENT_SOURCE_DIR}/release-workflow-self-check.sh")
endif()

if(TAU_PARSER_BUILD_NET AND NOT EMSCRIPTEN)
	add_subdirectory(net)
endif()
if(TAU_PARSER_BUILD_FUZZ)
	add_subdirectory(fuzz)
endif()

include(ctest-test-defaults)
tau_set_test_defaults()
