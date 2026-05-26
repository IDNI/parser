add_library(tgf_cli_core STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/tgf_cli.cpp")
target_compile_definitions(tgf_cli_core PUBLIC
	TAU_PARSER_MEASURE_SCOPES
	TAU_PARSER_MEASURE_COUNTERS)
tauparser_setup(tgf_cli_core PUBLIC tauparser)
target_include_directories(tgf_cli_core PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
if(NOT WIN32)
	target_compile_options(tgf_cli_core PRIVATE -fPIC)
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
