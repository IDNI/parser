# cmake/parser-bash.cmake
# TAU_PARSER_BASH: the bash that runs the parser scripts, or empty when none
# is found. On a Windows host the Git for Windows bash wins, because
# System32/bash.exe is the WSL launcher, which cannot see Windows paths.

include_guard(GLOBAL)

if(NOT TAU_PARSER_BASH)
	if(CMAKE_HOST_WIN32)
		find_program(TAU_PARSER_BASH NAMES bash
			PATHS "$ENV{ProgramFiles}/Git/bin" "$ENV{ProgramFiles}/Git/usr/bin"
			NO_DEFAULT_PATH
			NO_CACHE)
	endif()
	if(NOT TAU_PARSER_BASH)
		find_program(TAU_PARSER_BASH NAMES bash NO_CACHE)
	endif()
	if(TAU_PARSER_BASH MATCHES "[Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\][Bb][Aa][Ss][Hh]\\.[Ee][Xx][Ee]$")
		set(TAU_PARSER_BASH "")
	endif()
endif()
