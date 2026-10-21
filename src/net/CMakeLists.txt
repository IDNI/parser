# The line server and client. Only this library includes Boost.Asio and
# Boost.Beast, so the parser core stays free of Boost.
add_library(tauparser_net STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/line_transport.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/line_server.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/line_client.cpp"
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
# Only the private line_server_asio.h includes Boost, and a program outside
# this repository uses line_server_post, so only this build reads Boost.
target_include_directories(tauparser_net SYSTEM PUBLIC
	$<BUILD_INTERFACE:${TAU_BOOST_INCLUDE_DIR_RESOLVED}>)
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
