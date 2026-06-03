include_guard(GLOBAL)

# FTXUI dependency resolution
#
# FTXUI is enabled by default.  Set TAU_PARSER_DONT_USE_FTXUI=ON to disable.
# Prefers a system / package-provided FTXUI (vcpkg, conan, distro),
# falling back to fetching from source.
#
# Sets TAU_PARSER_HAS_FTXUI_DEP=ON when the FTXUI targets are available.
# The tgf build (src/tgf/CMakeLists.txt) turns this into the TAU_PARSER_HAS_FTXUI
# compile definition once the source includes repl_ftxui.tmpl.h (see tgf_cli.cpp)

set(TAU_PARSER_HAS_FTXUI_DEP OFF)

if(TAU_PARSER_DONT_USE_FTXUI)
	message(STATUS "FTXUI: disabled (TAU_PARSER_DONT_USE_FTXUI=ON)")
else()
	find_package(ftxui QUIET)
	if(ftxui_FOUND)
		set(TAU_PARSER_HAS_FTXUI_DEP ON)
		message(STATUS "FTXUI: using system package")
	else()
		include(FetchContent)
		FetchContent_Declare(ftxui
			GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
			GIT_TAG        v6.1.9   # verified tag (commit 5cfed50)
			GIT_SHALLOW    TRUE)
		set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(FTXUI_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
		set(FTXUI_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
		FetchContent_MakeAvailable(ftxui)
		set(TAU_PARSER_HAS_FTXUI_DEP ON)
		message(STATUS "FTXUI: fetched v6.1.9 from source")
	endif()
endif()

# Expose availability to other directory scopes (e.g. tests/).
set(TAU_PARSER_HAS_FTXUI_DEP ${TAU_PARSER_HAS_FTXUI_DEP} CACHE INTERNAL "FTXUI available")
