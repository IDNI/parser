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
			#-Werror
			#-Wfatal-errors
		)
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
		target_compile_options(${target} ${access} /W4)
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

