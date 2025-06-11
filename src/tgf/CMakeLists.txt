add_executable(tgf)
target_sources(tgf PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/tgf_cli.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
tauparser_setup(tgf PRIVATE tauparser)
target_include_directories(tgf PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
if(NOT WIN32)
	target_compile_options(tgf PRIVATE -fPIC)
endif()

# add_dependencies(tgf ${TAU_PARSER_STATIC_LIB_NAME})

message(STATUS "installing tgf to ${CMAKE_INSTALL_BINDIR}")
install(TARGETS tgf
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	COMPONENT "tgf")
