set(EXAMPLES
	basic_arithmetic
	basic_arithmetic_tgf
	csv_parser
	csv_parser1
	csv_parser2
	csv_parser3
	csv_parser4
	csv_parser5
	csv_parser6
	csv_parser7
	csv_parser8
	csv_parser9
	csv_parser10
	json_parser
)

foreach(X IN LISTS EXAMPLES)
	add_executable(${X})
	tauparser_setup(${X} PRIVATE tauparser)
	target_sources(${X} PRIVATE ${X}/main.cpp)
	target_include_directories(${X} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
	# add_dependencies(${X} ${TAU_PARSER_STATIC_LIB_NAME})
	if(TAU_PARSER_INSTALL)
		message(STATUS "installing ${X} to ${TAU_PARSER_DOC_DESTINATION}/examples")
		install(DIRECTORY ${X}
			DESTINATION "${TAU_PARSER_DOC_DESTINATION}/examples"
			COMPONENT "Examples")
		install(TARGETS ${X}
			RUNTIME DESTINATION "${TAU_PARSER_DOC_DESTINATION}/examples/${X}"
			COMPONENT "Examples")
	endif()
endforeach()
