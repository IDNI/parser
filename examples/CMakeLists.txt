set(EXAMPLES
	basic_arithmetic
	basic_arithmetic_tgf
	tau_parser
)

foreach(X IN LISTS EXAMPLES)
	add_executable(${X})
	target_setup(${X})
	target_sources(${X} PRIVATE ${X}/main.cpp)
	target_link_libraries(${X} ${OBJECT_LIB_NAME})
endforeach()
