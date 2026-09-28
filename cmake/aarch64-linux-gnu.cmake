# Cross build of Linux arm64 from an x86 Linux host. The Debian cross runtime
# (g++-aarch64-linux-gnu) supplies libstdc++ and the sysroot under
# /usr/aarch64-linux-gnu; clang is the compiler, as for every other producer.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# The target sysroot, shared by the find root, the emulator and QEMU_LD_PREFIX.
set(TAU_AARCH64_SYSROOT /usr/aarch64-linux-gnu)

# The target flag travels in the flags, not in CMAKE_<LANG>_COMPILER_TARGET
# alone: the flags are what the store hands to a producer without a CMake
# configure, such as Boost's b2.
set(CMAKE_C_FLAGS_INIT "--target=aarch64-linux-gnu")
set(CMAKE_CXX_FLAGS_INIT "--target=aarch64-linux-gnu")

set(CMAKE_FIND_ROOT_PATH "${TAU_AARCH64_SYSROOT}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Run the cross binaries under user-mode qemu where it is installed; the test
# harness re-executes a suite through this emulator.
find_program(TAU_AARCH64_QEMU qemu-aarch64)
if(TAU_AARCH64_QEMU)
	set(CMAKE_CROSSCOMPILING_EMULATOR
		qemu-aarch64 -L "${TAU_AARCH64_SYSROOT}")
endif()

# FindPython searches the build host for the interpreter and the artifacts
# (PROGRAM mode never re-roots), so point it at the arm64 multiarch layout: the
# headers are shared under /usr/include, the library and the interpreter are
# the target's. The interpreter's own SOABI then names the target's extension
# suffix, which is what the binding builds.
file(GLOB _tau_py_libpython "/usr/lib/aarch64-linux-gnu/libpython3.*.so")
if(_tau_py_libpython)
	set(Python_ROOT_DIR "/usr")
	list(GET _tau_py_libpython 0 Python_LIBRARY)
	string(REGEX MATCH "python3\\.[0-9]+" _tau_py_version "${Python_LIBRARY}")
	file(GLOB _tau_py_include "/usr/include/${_tau_py_version}")
	if(_tau_py_include)
		list(GET _tau_py_include 0 Python_INCLUDE_DIR)
	endif()
	foreach(_tau_py_candidate
			"/usr/aarch64-linux-gnu/usr/bin/${_tau_py_version}"
			"/usr/aarch64-linux-gnu/bin/${_tau_py_version}"
			"/usr/bin/${_tau_py_version}"
			"/usr/bin/python3")
		if(NOT Python_EXECUTABLE AND EXISTS "${_tau_py_candidate}")
			set(Python_EXECUTABLE "${_tau_py_candidate}")
		endif()
	endforeach()
	string(REPLACE "python3." "cpython-3" _tau_py_abi "${_tau_py_version}")
	if(NOT Python_SOABI)
		set(Python_SOABI "${_tau_py_abi}-aarch64-linux-gnu")
	endif()
endif()
