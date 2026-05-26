function(add_tgf_specialization_for TGF_FILE)
	cmake_parse_arguments(SPEC "" "EXE_NAME;GEN_NAME" "" ${ARGN})

	get_filename_component(BASENAME "${TGF_FILE}" NAME_WE)
	if(NOT SPEC_EXE_NAME)
		set(SPEC_EXE_NAME "${BASENAME}_grammar")
	endif()
	if(NOT SPEC_GEN_NAME)
		set(SPEC_GEN_NAME "${BASENAME}_parser")
	endif()

	set(WORK_DIR  "${CMAKE_BINARY_DIR}/specialize/${SPEC_EXE_NAME}")
	set(GEN_H     "${WORK_DIR}/${SPEC_GEN_NAME}.generated.h")
	set(MAIN_CPP  "${WORK_DIR}/main.cpp")

	file(MAKE_DIRECTORY "${WORK_DIR}")

	add_custom_command(
		OUTPUT  "${GEN_H}"
		COMMAND $<TARGET_FILE:tgf> "${TGF_FILE}" gen
			--name "${SPEC_GEN_NAME}" --output-dir "${WORK_DIR}"
			--utf8
		DEPENDS "${TGF_FILE}" tgf
		COMMENT "specialize: generating ${SPEC_GEN_NAME}.generated.h"
		VERBATIM
	)
	set_source_files_properties("${GEN_H}" PROPERTIES GENERATED TRUE)

	file(READ "${TGF_FILE}" SPEC_GRAMMAR_SOURCE)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${TGF_FILE}")
	set(SPEC_DELIM "TGF_GRAMMAR")
	set(_attempt 0)
	set(SPEC_END_MARKER ")${SPEC_DELIM}\"")
	string(FIND "${SPEC_GRAMMAR_SOURCE}" "${SPEC_END_MARKER}" _found)
	while(NOT _found EQUAL -1)
		math(EXPR _attempt "${_attempt} + 1")
		set(SPEC_DELIM "TGF_GRAMMAR_${_attempt}")
		if(_attempt GREATER 16)
			message(FATAL_ERROR "specialize: cannot find raw-string delimiter")
		endif()
		set(SPEC_END_MARKER ")${SPEC_DELIM}\"")
		string(FIND "${SPEC_GRAMMAR_SOURCE}" "${SPEC_END_MARKER}" _found)
	endwhile()

	set(SPEC_PARSER_NAME      "${SPEC_GEN_NAME}")
	set(SPEC_GENERATED_HEADER "${SPEC_GEN_NAME}.generated.h")
	set(SPEC_DISPLAY_NAME     "${BASENAME}.tgf")
	configure_file(
		"${PROJECT_SOURCE_DIR}/cmake/specialize/specialized_main.cpp.in"
		"${MAIN_CPP}"
		@ONLY
	)

	add_executable(${SPEC_EXE_NAME} "${MAIN_CPP}" "${GEN_H}")
	target_include_directories(${SPEC_EXE_NAME} PRIVATE
		"${WORK_DIR}"
		"${PROJECT_SOURCE_DIR}/src"
	)
	tauparser_setup(${SPEC_EXE_NAME} PRIVATE tgf_cli_core)
endfunction()
