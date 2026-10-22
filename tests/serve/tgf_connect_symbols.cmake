# Fail when the tgf-connect program holds a symbol of the grammar loader or
# of the local evaluator. Run with -DNM=<nm> -DBINARY=<tgf-connect>.
execute_process(COMMAND "${NM}" -C "${BINARY}"
	OUTPUT_VARIABLE symbols RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "nm failed on ${BINARY}")
endif()
# A whole name, so a symbol of tgf_repl_evaluator_base passes.
foreach(name tgf_repl_evaluator grammar_builder)
	string(REGEX MATCH "${name}([^_A-Za-z0-9]|$)" hit "${symbols}")
	if(hit)
		message(FATAL_ERROR "tgf-connect holds a symbol of ${name}")
	endif()
endforeach()
