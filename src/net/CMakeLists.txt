# The line server and client. Only this library includes Boost.Asio and
# Boost.Beast, so the parser core stays free of Boost.
add_library(tauparser_net STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/line_support.cpp")
tauparser_setup(tauparser_net PUBLIC tauparser)
if(NOT WIN32)
	target_compile_options(tauparser_net PRIVATE -fPIC)
endif()
include(GNUInstallDirs)
target_include_directories(tauparser_net PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>
)
find_package(Threads REQUIRED)
target_link_libraries(tauparser_net PUBLIC Threads::Threads)
if(WIN32)
	target_link_libraries(tauparser_net PUBLIC ws2_32 mswsock bcrypt advapi32)
endif()

if(TAU_PARSER_INSTALL)
	install(TARGETS tauparser_net
		EXPORT "${PROJECT_NAME}Targets"
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		COMPONENT "Library")
endif()
