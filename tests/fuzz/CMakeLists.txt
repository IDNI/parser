# Emscripten and AppleClang report Clang but have no libFuzzer runtime.
if(EMSCRIPTEN OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	message(WARNING
		"TAU_PARSER_BUILD_FUZZ needs clang with libFuzzer, so the fuzz targets are not built")
	return()
endif()
# libFuzzer brings its own main, so the probe has the fuzz entry only.
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=fuzzer")
check_cxx_source_compiles([[
#include <cstddef>
#include <cstdint>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t*, size_t) { return 0; }
]] TAU_PARSER_HAS_LIBFUZZER)
cmake_pop_check_state()
if(NOT TAU_PARSER_HAS_LIBFUZZER)
	message(WARNING
		"The compiler links no libFuzzer, so the fuzz targets are not built")
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
