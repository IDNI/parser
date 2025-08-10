option(TAU_PARSER_MEASURE "Allow performance measures" OFF)

# public headers and sources
set(TAU_PARSER_HEADERS
	version_license.h
	defs.h
	devhelpers.h
	forest.h
	forest.tmpl.h
	grammar.tmpl.h
	parser_result.tmpl.h
	parser_term_color_macros.h
	parser.h
	parser.tmpl.h
	recoders.h
	tgf_parser.generated.h
	tgf.h
	utility/tree.h
	utility/tree.tmpl.h
	utility/characters.h
	utility/characters.impl.h
	utility/charclasses.h
	utility/charclasses.impl.h
	utility/cli.h
	utility/cli.impl.h
	utility/measure.h
	utility/memory_map.h
	utility/repl.h
	utility/term_colors.h
	utility/term.h
	utility/term.impl.h
	CACHE STRING "Tau Parser headers files"
)

set(TAU_PARSER_SOURCES
	parser.cpp
)

if(TAU_PARSER_BUILD_HEADER_ONLY)
	add_library(tauparser INTERFACE)
	target_include_directories(tauparser INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
	target_compile_definitions(tauparser INTERFACE TAU_PARSER_BUILD_HEADER_ONLY)
	tauparser_setup(tauparser INTERFACE "")
else()
	if(TAU_PARSER_BUILD_STATIC_LIBRARY)
		add_library(tauparser_static STATIC ${TAU_PARSER_SOURCES})
		set_target_properties(tauparser_static PROPERTIES
			OUTPUT_NAME "tauparser"
		)
		target_include_directories(tauparser_static PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
		)
		tauparser_setup(tauparser_static PUBLIC "")
	endif()
	
	if(TAU_PARSER_BUILD_SHARED_LIBRARY)
		add_library(tauparser_shared SHARED ${TAU_PARSER_SOURCES})
		set_target_properties(tauparser_shared PROPERTIES
			OUTPUT_NAME "tauparser"
		)
		target_include_directories(tauparser_shared PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
		)
		tauparser_setup(tauparser_shared PUBLIC "")
	endif()
	
	# Create an alias target for backward compatibility
	if(TAU_PARSER_BUILD_STATIC_LIBRARY)
		add_library(tauparser ALIAS tauparser_static)
	elseif(TAU_PARSER_BUILD_SHARED_LIBRARY)
		add_library(tauparser ALIAS tauparser_shared)
	endif()
endif()

# install targets
#################
if(TAU_PARSER_INSTALL)
	include(GNUInstallDirs)
	if(DEFINED CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install"
			CACHE PATH "library installation dir" FORCE
		)
	endif()

	message(STATUS "installing library targets to ${CMAKE_INSTALL_BINDIR}, ${CMAKE_INSTALL_LIBDIR}, ${CMAKE_INSTALL_INCLUDEDIR}")
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
		COMPONENT "Library"
		FILES_MATCHING 
			PATTERN "*.h"
			PATTERN "tgf" EXCLUDE
			PATTERN "CMakeFiles" EXCLUDE
			PATTERN "CMakeLists.txt" EXCLUDE
			PATTERN "*.cmake*" EXCLUDE
	)
	# Install targets based on what was built
	if(TAU_PARSER_BUILD_STATIC_LIBRARY)
		install(TARGETS tauparser_static
			EXPORT "${PROJECT_NAME}Targets"
			RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
			INCLUDES      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
			COMPONENT "Library"
		)
	endif()
	
	if(TAU_PARSER_BUILD_SHARED_LIBRARY)
		install(TARGETS tauparser_shared
			EXPORT "${PROJECT_NAME}Targets"
			RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
			INCLUDES      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
			COMPONENT "Library"
		)
	endif()

	# install cmake configuration if not header-only
	if(NOT TAU_PARSER_BUILD_HEADER_ONLY)
		message(STATUS "installing cmake configuration to cmake/")
		install(EXPORT "${PROJECT_NAME}Targets"
			FILE "${PROJECT_NAME}Targets.cmake"
			NAMESPACE ${namespace}::
			DESTINATION cmake
		)
	endif()
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	VERSION "${version}"
	COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	INSTALL_DESTINATION cmake
)

if(TAU_PARSER_INSTALL)
	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
		DESTINATION cmake
	)
	
	if(NOT TAU_PARSER_BUILD_HEADER_ONLY)
		message(STATUS "Exporting ${PROJECT_NAME}Targets to ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
		export(EXPORT "${PROJECT_NAME}Targets"
			FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
			NAMESPACE ${namespace}::
		)
	endif()
endif()

