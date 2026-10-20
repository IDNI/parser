# libFuzzer ships with clang only.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC)
	message(WARNING
		"TAU_PARSER_BUILD_FUZZ needs clang, so the fuzz targets are not built")
	return()
endif()

# One target per fuzz_*.cpp, so a new fuzz target needs no list entry.
file(GLOB FUZZ_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/fuzz_*.cpp")
foreach(SRC IN LISTS FUZZ_SOURCES)
	get_filename_component(N "${SRC}" NAME_WE)
	add_executable(${N} "${SRC}")
	tauparser_setup(${N} PRIVATE tauparser)
	target_include_directories(${N} PRIVATE
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
	target_compile_options(${N} PRIVATE -fsanitize=fuzzer,address)
	target_link_options(${N} PRIVATE -fsanitize=fuzzer,address)
endforeach()
