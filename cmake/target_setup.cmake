# passes definitions if they exist
function(target_compile_definitions_if target access project_definitions)
	foreach(X IN LISTS project_definitions)
		if(${X})
			target_compile_definitions(${target} ${access} "-D${X}")
		endif()
	endforeach()
endfunction()

# setups a target: sets COMPILE and LINK options, adds warnings, c++ std req...
function(target_setup target access compile_definitions compile_options link_options libs)
	target_compile_features(${target} ${access} cxx_std_23)
	if(NOT MSVC)
		target_compile_options(${target} ${access}
			-W -Wall -Wextra -Wpedantic
			-Wformat=2
			-Wcast-align
			-Wstrict-aliasing=2
			-Wfloat-equal
			-Wwrite-strings
			# Value-losing and sign conversions are errors in the -Werror
			# configurations. -Wsign-conversion is named explicitly because
			# GCC's -Wconversion does not report sign changes.
			-Wconversion -Wsign-conversion
			$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-Werror>
			# -Wfatal-errors
		)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			# GCC's -O2+ interprocedural array-bounds analysis produces a
			# false positive when it inlines ankerl::unordered_dense's
			# memset-based clear_buckets() (observed: GCC 13.3.0, -O3).
			# Keep the warning (still visible, just non-fatal) rather than
			# letting -Werror above turn third-party header code into a
			# hard build failure.
			target_compile_options(${target} ${access}
				-Wno-error=array-bounds
			)
		endif()
		if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
				CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			target_compile_options(${target} ${access}
				-Wstrict-overflow=2
			)
		else()
			target_compile_options(${target} ${access}
				-Wstrict-overflow=5
			)
		endif()
	else()
		target_compile_options(${target} ${access}
			/W4 /EHsc /utf-8 /bigobj)
		target_compile_definitions(${target} ${access}
			NOMINMAX
			WIN32_LEAN_AND_MEAN
			_CRT_DECLARE_NONSTDC_NAMES=0)
	endif()
	if(EMSCRIPTEN)
		target_compile_options(${target} ${access}
			# wasm32 is the only 32-bit target here. A 64-bit value narrowed
			# to size_t truncates silently there instead of trapping, so this
			# one stays fatal in every build type, not just the -Werror ones
			# above.
			-Werror=shorten-64-to-32
		)
	endif()
	target_compile_options(${target} ${access} "${compile_options}")
	target_compile_definitions_if(${target} ${access} "${compile_definitions}")
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
		CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			target_link_libraries(${target} ${access}
				${CMAKE_THREAD_LIBS_INIT}
				-static-libgcc
				-static-libstdc++
				${libs}
			)
			get_target_property(_tgt_type ${target} TYPE)
			if(_tgt_type STREQUAL "EXECUTABLE")
				target_link_options(${target} ${access} -static)
			endif()
	else()
		target_link_libraries(${target} ${access} ${CMAKE_THREAD_LIBS_INIT} ${libs})
	endif()
	target_link_options(${target} ${access} "${link_options}")
	set_target_properties(${target} PROPERTIES
		EXPORT_NAME "${target}"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
	)
endfunction()

# exclude target from all and default
function(exclude target)
	set_target_properties(${target} PROPERTIES
		EXCLUDE_FROM_ALL 1
		EXCLUDE_FROM_DEFAULT_BUILD 1)
endfunction()
