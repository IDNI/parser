# Run one example with INPUT on stdin and compare its stdout with EXPECTED.
# add_test cannot feed stdin, so a script runs the example.
#   cmake -DEXE=<example> -DINPUT=<file> -DEXPECTED=<file> [-DARG=<arg>]
#         [-DEMULATOR=<runner>] -P run_example.cmake
set(_args "")
if(DEFINED ARG AND NOT ARG STREQUAL "")
	set(_args "${ARG}")
endif()
execute_process(COMMAND ${EMULATOR} "${EXE}" ${_args}
	INPUT_FILE "${INPUT}"
	OUTPUT_VARIABLE _out
	ERROR_VARIABLE _err
	RESULT_VARIABLE _rc
	TIMEOUT 60)
if(NOT _rc EQUAL 0)
	message(FATAL_ERROR "${EXE} exited with ${_rc}\n${_out}\n${_err}")
endif()
# Windows text-mode stdout writes CRLF.
string(REPLACE "\r\n" "\n" _out "${_out}")
file(READ "${EXPECTED}" _expected)
if(NOT _out STREQUAL _expected)
	message(FATAL_ERROR "the output differs from ${EXPECTED}\n"
		"--- expected\n${_expected}\n--- actual\n${_out}\n--- stderr\n${_err}")
endif()
