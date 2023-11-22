set(EXAMPLES
	basic_arithmetic
	basic_arithmetic_tgf
	tml_parser
	bitparser
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
)

foreach(X IN LISTS EXAMPLES)
	add_executable(${X})
	target_setup(${X})
	target_sources(${X} PRIVATE ${X}/main.cpp)
	target_link_libraries(${X} ${PARSER_OBJECT_LIB_NAME})
	target_include_directories(${X} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
endforeach()
