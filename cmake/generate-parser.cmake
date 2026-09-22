cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)

# One target per grammar: a static library, EXCLUDE_FROM_ALL, holding just
# that grammar's generated parser. Nothing here selects grammars -- the
# custom command that runs the tgf tool only fires once some other target
# actually links <stem>_parser, so an unlinked grammar never builds.
#
# `--header-only false` puts the grammar table in a .cpp compiled once,
# instead of in the header where every including TU pays for it. Linking
# <stem>_parser hands a consumer its include directory for free, so its
# bare #include "<stem>_parser.generated.h" keeps resolving unchanged.
#
# Not usable for a grammar the tgf tool itself needs to compile (tgf.tgf,
# and anything tgf_cli.cpp/.h pulls in): the custom command below depends
# on the tgf target, so linking its output back into tgf's own build is a
# cycle -- the tool cannot generate the parser it needs in order to exist.
# Those parsers' generated files stay committed in the source tree instead.
function(generate_parser tgf_filename)
	if(NOT TARGET ${TAU_TGF_DEPEND})
		message(FATAL_ERROR
			"generate_parser(${tgf_filename}): no '${TAU_TGF_DEPEND}' target -- "
			"enable TAU_PARSER_BUILD_TGF before anything links a generated parser")
	endif()

	get_filename_component(_stem "${tgf_filename}" NAME_WE)
	set(_out_dir "${CMAKE_BINARY_DIR}/generated/parser/${_stem}")
	set(_header "${_out_dir}/${_stem}_parser.generated.h")
	set(_source "${_out_dir}/${_stem}_parser.generated.cpp")

	add_custom_command(
		OUTPUT "${_header}" "${_source}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
		COMMAND ${TAU_TGF_EXECUTABLE} "${tgf_filename}" gen
			--header-only false --output-dir "${_out_dir}"
		DEPENDS ${TAU_TGF_DEPEND} "${tgf_filename}"
		COMMENT "Generating parser from ${_stem}.tgf"
		VERBATIM)

	add_library(${_stem}_parser STATIC EXCLUDE_FROM_ALL "${_source}")
	# grammar tables are data; optimizing them wastes build time
	set_source_files_properties("${_source}"
		PROPERTIES COMPILE_OPTIONS "-O0;-fno-lto")
	target_include_directories(${_stem}_parser PUBLIC "${_out_dir}")
endfunction(generate_parser)
