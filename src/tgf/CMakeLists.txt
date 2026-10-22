# The client of tgf serve: the REPL command parser, the text renderer and,
# with the Boost headers, the connection. It holds no grammar loader.
add_library(tgf_connect_core STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/tgf_repl_evaluator_base.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/repl_client_evaluator.cpp")
tauparser_setup(tgf_connect_core PUBLIC tauparser)
target_compile_definitions_if(tgf_connect_core PRIVATE
	"${TAU_PARSER_DEFINITIONS}")
target_include_directories(tgf_connect_core PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
# a header-only build resolves productions() inline instead
if(NOT TAU_PARSER_BUILD_HEADER_ONLY)
	target_sources(tgf_connect_core PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/tgf_repl_parser.generated.cpp")
	set_source_files_properties(
		"${CMAKE_CURRENT_SOURCE_DIR}/tgf_repl_parser.generated.cpp"
		PROPERTIES COMPILE_OPTIONS "${TAU_PARSER_DATA_SOURCE_OPTIONS}")
endif()
if(NOT WIN32)
	target_compile_options(tgf_connect_core PRIVATE -fPIC)
endif()

# tgf connect and tgf-connect need the line transport of tauparser_net.
# Only tgf_serve.cpp and the net library include Boost.Asio, so the parser
# core stays free of Boost.
if(TAU_PARSER_BUILD_SERVE OR TAU_PARSER_BUILD_CONNECT)
	# serve or connect without TAU_PARSER_BUILD_NET still needs the library
	if(NOT TARGET tauparser_net)
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../net"
			"${CMAKE_CURRENT_BINARY_DIR}/net")
	endif()
	target_sources(tgf_connect_core PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/tgf_connect.cpp")
	target_link_libraries(tgf_connect_core PUBLIC tauparser_net)
	target_include_directories(tgf_connect_core SYSTEM PUBLIC
		"${TAU_BOOST_INCLUDE_DIR_RESOLVED}")
	find_package(Threads REQUIRED)
	target_link_libraries(tgf_connect_core PUBLIC Threads::Threads)
	if(WIN32)
		target_link_libraries(tgf_connect_core PUBLIC ws2_32 mswsock)
	endif()
endif()

# FTXUI-based REPL.  TAU_PARSER_HAS_FTXUI is defined when the FTXUI targets resolve;
# otherwise the REPL silently falls back to the legacy terminal REPL.
include(ftxui)
if(TAU_PARSER_HAS_FTXUI_DEP)
	find_package(Threads REQUIRED)
	target_link_libraries(tgf_connect_core PUBLIC
		ftxui::screen ftxui::dom ftxui::component Threads::Threads)
	target_compile_definitions(tgf_connect_core PUBLIC TAU_PARSER_HAS_FTXUI)
	message(STATUS "tgf: FTXUI REPL enabled (legacy via -X)")
elseif(NOT TAU_PARSER_DONT_USE_FTXUI)
	message(WARNING "tgf: FTXUI not available - legacy terminal REPL will be used")
endif()

if(TAU_PARSER_BUILD_CONNECT)
	add_executable(tgf-connect)
	target_sources(tgf-connect PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/tgf_connect_main.cpp")
	tauparser_setup(tgf-connect PRIVATE tgf_connect_core)
	message(STATUS "installing tgf-connect to ${CMAKE_INSTALL_BINDIR}")
	install(TARGETS tgf-connect
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		COMPONENT "tgf")
endif()

if(NOT TAU_PARSER_BUILD_TGF)
	return()
endif()

add_library(tgf_cli_core STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/tgf_cli.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/tgf_json_api.cpp")
tauparser_setup(tgf_cli_core PUBLIC tgf_connect_core)
target_compile_definitions_if(tgf_cli_core PRIVATE
	"${TAU_PARSER_DEFINITIONS}")

# tgf_cli.cpp reaches tgf_test_parser and treemr_parser (via tgf_test.h)
# directly; a header-only build resolves productions() inline instead, so
# these stay out of that build entirely
if(NOT TAU_PARSER_BUILD_HEADER_ONLY)
	set(TGF_CLI_CORE_GENERATED_SOURCES
		"${CMAKE_CURRENT_SOURCE_DIR}/../format/treemr/treemr_parser.generated.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/../format/tgf.test/tgf_test_parser.generated.cpp"
	)
	target_sources(tgf_cli_core PRIVATE ${TGF_CLI_CORE_GENERATED_SOURCES})
	set_source_files_properties(${TGF_CLI_CORE_GENERATED_SOURCES}
		PROPERTIES COMPILE_OPTIONS "${TAU_PARSER_DATA_SOURCE_OPTIONS}")
endif()

if(NOT WIN32)
	target_compile_options(tgf_cli_core PRIVATE -fPIC)
endif()

if(TAU_PARSER_BUILD_SERVE)
	target_sources(tgf_cli_core PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/tgf_serve.cpp")
	target_compile_definitions(tgf_cli_core PRIVATE TAU_PARSER_BUILD_SERVE)
	if(WIN32)
		# Asio's accept path calls AcceptEx and GetAcceptExSockaddrs, which live in mswsock.
		# The session id reads the OS entropy source through BCryptGenRandom.
		target_link_libraries(tgf_cli_core PUBLIC ws2_32 mswsock bcrypt)
	endif()
endif()

add_executable(tgf)
target_sources(tgf PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
tauparser_setup(tgf PRIVATE tgf_cli_core)

# Round-trip self-check first, archive only the binary that passed it.
# Both steps run the binary on the host, which a cross build cannot do.
# bash runs the scripts: cmd.exe opens a .sh in a GUI program and does not wait.
# The archive writes into the source tree, so only a build under its own
# build/ folder archives. A store build or a parent project leaves it alone.
get_filename_component(_tgf_build_root "${PROJECT_SOURCE_DIR}/build" REALPATH)
get_filename_component(_tgf_binary_dir "${PROJECT_BINARY_DIR}" REALPATH)
string(FIND "${_tgf_binary_dir}/" "${_tgf_build_root}/" _tgf_in_build_root)
if(NOT CMAKE_CROSSCOMPILING)
	if(TAU_PARSER_BASH AND _tgf_in_build_root EQUAL 0)
		add_custom_command(TARGET tgf POST_BUILD
			COMMAND "${TAU_PARSER_BASH}"
				"${PROJECT_SOURCE_DIR}/scripts/tgf-roundtrip-self-check.sh" "$<TARGET_FILE:tgf>"
			COMMAND "${TAU_PARSER_BASH}"
				"${PROJECT_SOURCE_DIR}/scripts/archive-tgf.sh" "$<TARGET_FILE:tgf>"
			COMMENT "Round-trip self-check, then archiving the passing binary"
		)
	elseif(TAU_PARSER_BASH)
		add_custom_command(TARGET tgf POST_BUILD
			COMMAND "${TAU_PARSER_BASH}"
				"${PROJECT_SOURCE_DIR}/scripts/tgf-roundtrip-self-check.sh" "$<TARGET_FILE:tgf>"
			COMMENT "Round-trip self-check"
		)
	else()
		message(WARNING "tgf: no bash found, so the build skips the round-trip self-check")
	endif()
endif()

# add_dependencies(tgf ${TAU_PARSER_STATIC_LIB_NAME})

message(STATUS "installing tgf to ${CMAKE_INSTALL_BINDIR}")
install(TARGETS tgf
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	COMPONENT "tgf")
