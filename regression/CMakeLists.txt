set(EXAMPLES
	tree_extraction
	tree_extraction_tgf
	
	# TODO (HIGH) add test for missing nonterminal in a rule body
	# TODO (HIGH) add test for missing nonterminal in a rule head
)

foreach(X IN LISTS EXAMPLES)
	add_executable(${X})
	target_setup(${X})
	target_sources(${X} PRIVATE ${X}/main.cpp)
	target_link_libraries(${X} ${OBJECT_LIB_NAME})
endforeach()
