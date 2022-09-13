set(EXAMPLES
	basic_arithmetic
	g2cpp
)

foreach(X IN LISTS EXAMPLES)
	add_executable(${X})
	target_setup(${X})
	target_sources(${X} PRIVATE ${X}/main.cpp)
	message(STATUS "WITH_CHARCLASSES: ${WITH_CHARCLASSES}")
	target_link_libraries(${X} ${OBJECT_LIB_NAME})
endforeach()
