add_library(tgf_cli_core STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/tgf_cli.cpp")
tauparser_setup(tgf_cli_core PUBLIC tauparser)
target_include_directories(tgf_cli_core PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
if(NOT WIN32)
	target_compile_options(tgf_cli_core PRIVATE -fPIC)
endif()

# FTXUI-based REPL.  TAU_PARSER_HAS_FTXUI is defined when the FTXUI targets resolve;
# otherwise the REPL silently falls back to the legacy terminal REPL.
include(ftxui)
if(TAU_PARSER_HAS_FTXUI_DEP)
	target_link_libraries(tgf_cli_core PUBLIC
		ftxui::screen ftxui::dom ftxui::component)
	target_compile_definitions(tgf_cli_core PUBLIC TAU_PARSER_HAS_FTXUI)
	message(STATUS "tgf: FTXUI REPL enabled (legacy via -X)")
elseif(NOT TAU_PARSER_DONT_USE_FTXUI)
	message(WARNING "tgf: FTXUI not available — legacy terminal REPL will be used")
endif()

add_executable(tgf)
target_sources(tgf PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
tauparser_setup(tgf PRIVATE tgf_cli_core)

# add_dependencies(tgf ${TAU_PARSER_STATIC_LIB_NAME})

message(STATUS "installing tgf to ${CMAKE_INSTALL_BINDIR}")
install(TARGETS tgf
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	COMPONENT "tgf")
