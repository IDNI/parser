set(TOOLS
	parser_gen
)

foreach(X IN LISTS TOOLS)
	add_executable(${X})
	target_setup(${X})
	target_sources(${X} PRIVATE ${X}/main.cpp)
	target_link_libraries(${X} ${OBJECT_LIB_NAME})
endforeach()
