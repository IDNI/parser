set(TOOLS
	tgf
)

foreach(X IN LISTS TOOLS)
	add_executable(${X})
	target_setup(${X})
	target_sources(${X} PRIVATE ${X}/main.cpp)
	target_link_libraries(${X} ${PARSER_OBJECT_LIB_NAME})
	target_include_directories(${X} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	)
endforeach()

# add required sources to tgf tool
target_sources("tgf" PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../src/cli.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/tgf/tgf_cli.cpp")
