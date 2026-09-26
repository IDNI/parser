# tests/tau-manifest-self-check.cmake
#
# Hermetic self-check for cmake/tau-manifest.cmake. Run as:
#
#   cmake -P tau-manifest-self-check.cmake <module> [<temp-dir>]
#
# Exits zero and prints nothing on success. Any mismatch is a FATAL_ERROR.
# Fatal paths that would abort this process run through child `cmake -P` calls
# so their nonzero exit status can be asserted.
#
# Every path is under a random scratch namespace below the temp dir. The real
# ~/.tau is never touched.

if(CMAKE_ARGC LESS 4)
	message(FATAL_ERROR
		"usage: cmake -P tau-manifest-self-check.cmake <module> [<temp-dir>]")
endif()
set(_module "${CMAKE_ARGV3}")
if(NOT EXISTS "${_module}")
	message(FATAL_ERROR "tau-manifest module not found: ${_module}")
endif()
if(CMAKE_ARGC GREATER 4 AND NOT CMAKE_ARGV4 STREQUAL "")
	set(_tmp "${CMAKE_ARGV4}")
elseif(DEFINED ENV{TMPDIR} AND NOT "$ENV{TMPDIR}" STREQUAL "")
	set(_tmp "$ENV{TMPDIR}")
else()
	set(_tmp ".")
endif()
file(MAKE_DIRECTORY "${_tmp}")
get_filename_component(_tmp "${_tmp}" ABSOLUTE)
include("${_module}")

string(RANDOM LENGTH 16 _token)
set(_root "${_tmp}/tau-manifest-${_token}")

macro(tau_fail msg)
	file(REMOVE_RECURSE "${_root}")
	message(FATAL_ERROR "tau-manifest self-check: ${msg}")
endmacro()

function(tau_expect_fatal root what)
	execute_process(COMMAND ${ARGN}
		RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
	if(_rc EQUAL 0)
		file(REMOVE_RECURSE "${root}")
		message(FATAL_ERROR "tau-manifest self-check: ${what} was accepted")
	endif()
endfunction()

# --- determinism -------------------------------------------------------------
tau_input_id(_a "a=1\nb=2")
tau_input_id(_b "a=1\nb=2")
if(NOT _a STREQUAL _b)
	tau_fail("identical blocks gave different ids")
endif()
if(NOT _a MATCHES "^[0-9a-f]+$")
	tau_fail("id is not lowercase hex: ${_a}")
endif()
string(LENGTH "${_a}" _a_len)
if(NOT _a_len EQUAL 64)
	tau_fail("id is not 64 hex characters: ${_a}")
endif()

# --- order independence ------------------------------------------------------
tau_input_id(_c "b=2\na=1")
if(NOT _a STREQUAL _c)
	tau_fail("field order changed the id")
endif()

# --- canonical records are length-prefixed -----------------------------------
# A naive "key=value\n" framing would conflate a value holding a newline with a
# following field. The byte lengths make that impossible. A value cannot hold a
# newline here, because a newline separates fields.
tau_canonical_fields(_canon "a=1\nbb=22")
if(NOT _canon STREQUAL "1:a=1:1\n2:bb=2:22\n")
	tau_fail("canonical records are not length-prefixed: [${_canon}]")
endif()
tau_input_id(_pair "a=x\nb=y")
tau_input_id(_pair_changed "a=x\nb=z")
if(_pair STREQUAL _pair_changed)
	tau_fail("a changed value did not change the id")
endif()

# --- provenance fields stay out of the id ------------------------------------
tau_input_id(_base "dep=demo\nsource=abc")
tau_input_id(_prov_changed
	"dep=demo\nsource=abc\nprovenance.manifest_writer_hash=deadbeef")
if(NOT _base STREQUAL _prov_changed)
	tau_fail("a provenance field changed the id")
endif()
tau_input_id(_build_changed "dep=demo\nsource=abcd")
if(_base STREQUAL _build_changed)
	tau_fail("a build-affecting field did not change the id")
endif()

# --- lexical path helpers handle Windows separators --------------------------
_tau_manifest_normalize("..\\outside" _res_esc _esc_esc)
if(NOT _esc_esc)
	tau_fail("a backslash escape was not detected")
endif()
_tau_manifest_normalize("sub\\libx.so" _res_sub _esc_sub)
if(_esc_sub OR NOT _res_sub STREQUAL "sub/libx.so")
	tau_fail("an ordinary backslash path did not normalize to "
		"'sub/libx.so' (got '${_res_sub}')")
endif()
_tau_manifest_normalize("a\\..\\b" _res_dot _esc_dot)
if(_esc_dot OR NOT _res_dot STREQUAL "b")
	tau_fail("'a\\..\\b' did not normalize to 'b' (got '${_res_dot}')")
endif()

# --- a value may contain ';' (function API) ----------------------------------
set(_semi "note=a;b")
tau_input_id(_semi_id "${_semi}")
tau_input_id(_semi_other "note=ab")
if(_semi_id STREQUAL _semi_other)
	tau_fail("'note=a;b' and 'note=ab' hashed the same")
endif()

# --- a value may contain ';' (script mode) -----------------------------------
execute_process(
	COMMAND "${CMAKE_COMMAND}" -P "${_module}" input-id "${_semi}"
	RESULT_VARIABLE _semi_rc OUTPUT_VARIABLE _semi_out
	ERROR_VARIABLE _semi_err)
if(NOT _semi_rc EQUAL 0)
	tau_fail("script-mode semicolon field failed: ${_semi_err}")
endif()
string(STRIP "${_semi_out}" _semi_script_id)
if(NOT _semi_script_id STREQUAL "${_semi_id}")
	tau_fail("function API and script mode differ on a ';' value")
endif()

# --- a value may contain '=' and spaces --------------------------------------
tau_input_id(_mixed "note=a=b c")
tau_input_id(_mixed_other "note=a=b  c")
if(_mixed STREQUAL _mixed_other)
	tau_fail("a space change did not change the id")
endif()

# --- fatal field paths -------------------------------------------------------
tau_expect_fatal("${_root}" "duplicate key"
	"${CMAKE_COMMAND}" -P "${_module}" input-id "a=1" "a=2")
tau_expect_fatal("${_root}" "a field without '='"
	"${CMAKE_COMMAND}" -P "${_module}" input-id "no_equals")
tau_expect_fatal("${_root}" "an empty key"
	"${CMAKE_COMMAND}" -P "${_module}" input-id "=value")
string(ASCII 13 _cr)
tau_expect_fatal("${_root}" "a carriage return"
	"${CMAKE_COMMAND}" -P "${_module}" input-id "a=x${_cr}")

if(UNIX)
	# --- a package with files and a shared-library symlink chain -----------------
	set(_pkg "${_root}/pkg")
	file(MAKE_DIRECTORY "${_pkg}/include")
	file(MAKE_DIRECTORY "${_pkg}/lib")
	file(MAKE_DIRECTORY "${_pkg}/emptydir")
	file(WRITE "${_pkg}/.hidden" "hidden")
	file(WRITE "${_pkg}/include/x.h" "header")
	file(WRITE "${_pkg}/lib/libx.so.1.2.3" "library")
	file(CREATE_LINK "libx.so.1.2.3" "${_pkg}/lib/libx.so.1" SYMBOLIC)
	file(CREATE_LINK "libx.so.1" "${_pkg}/lib/libx.so" SYMBOLIC)

	tau_output_map(_map "${_pkg}")
	string(JSON _map_count LENGTH "${_map}")
	if(NOT _map_count EQUAL 5)
		tau_fail("output map has ${_map_count} entries, expected 5")
	endif()
	string(JSON _hidden_type GET "${_map}" ".hidden" "type")
	if(NOT _hidden_type STREQUAL "file")
		tau_fail("a hidden file is not recorded as a file")
	endif()
	string(JSON _chain_type GET "${_map}" "lib/libx.so" "type")
	if(NOT _chain_type STREQUAL "symlink")
		tau_fail("libx.so is not recorded as a symlink")
	endif()
	string(JSON _chain_target GET "${_map}" "lib/libx.so" "target")
	if(NOT _chain_target STREQUAL "libx.so.1")
		tau_fail("libx.so target is '${_chain_target}'")
	endif()
	string(JSON _link_type GET "${_map}" "lib/libx.so.1" "type")
	if(NOT _link_type STREQUAL "symlink")
		tau_fail("libx.so.1 is not recorded as a symlink")
	endif()
	string(JSON _file_type GET "${_map}" "lib/libx.so.1.2.3" "type")
	if(NOT _file_type STREQUAL "file")
		tau_fail("libx.so.1.2.3 is not recorded as a file")
	endif()

	# an empty directory is ignored: the map is byte-identical without it
	file(MAKE_DIRECTORY "${_pkg}/another-empty")
	tau_output_map(_map_with_empty "${_pkg}")
	if(NOT _map STREQUAL _map_with_empty)
		tau_fail("an empty directory changed the output map")
	endif()
	file(REMOVE_RECURSE "${_pkg}/another-empty")

	# --- manifest write and verify -----------------------------------------------
	tau_input_id(_pkg_id "dep=demo\nsource=abc")
	set(_mpath "${_root}/pkg.json")
	tau_manifest_write("${_mpath}" "${_pkg_id}" "dep=demo\nsource=abc" "${_map}")
	tau_manifest_verify("${_mpath}" "${_pkg}" "${_pkg_id}")

	# a file record carries its permission bits, so the map must name a mode.

	# a provenance-only difference is the same artifact; a different output is
	# a different artifact
	set(_prov_manifest "${_root}/prov.json")
	file(READ "${_mpath}" _prov_json)
	set(_prov_fields "{}")
	string(JSON _prov_fields SET "${_prov_fields}" "dep" "\"demo\"")
	string(JSON _prov_fields SET "${_prov_fields}" "source" "\"abc\"")
	string(JSON _prov_fields SET "${_prov_fields}"
		"provenance.manifest_writer_hash" "\"changed\"")
	string(JSON _prov_json SET "${_prov_json}" "fields" "${_prov_fields}")
	file(WRITE "${_prov_manifest}" "${_prov_json}")
	tau_manifest_same_artifact("${_mpath}" "${_prov_manifest}" _same_prov)
	if(NOT _same_prov)
		tau_fail("a provenance-only difference was not the same artifact")
	endif()
	set(_diff_manifest "${_root}/diff.json")
	file(READ "${_mpath}" _diff_json)
	string(JSON _diff_json SET "${_diff_json}" "outputs" "{}")
	file(WRITE "${_diff_manifest}" "${_diff_json}")
	tau_manifest_same_artifact("${_mpath}" "${_diff_manifest}" _same_diff)
	if(_same_diff)
		tau_fail("a different output was treated as the same artifact")
	endif()

	# relocation: relative symlinks follow the copied prefix
	set(_pkg_r "${_root}/relocated")
	file(COPY "${_pkg}/" DESTINATION "${_pkg_r}")
	tau_manifest_verify("${_mpath}" "${_pkg_r}" "${_pkg_id}")

	# empty outputs are valid only for an empty prefix
	set(_empty_prefix "${_root}/empty-prefix")
	file(MAKE_DIRECTORY "${_empty_prefix}")
	set(_m_empty "${_root}/empty.json")
	tau_input_id(_empty_id "dep=demo")
	tau_manifest_write("${_m_empty}" "${_empty_id}" "dep=demo")
	tau_manifest_verify("${_m_empty}" "${_empty_prefix}" "${_empty_id}")

	# child dispatchers
	set(_verify_child "${_root}/verify.cmake")
	file(WRITE "${_verify_child}"
		"include(\"${_module}\")\n"
		"tau_manifest_verify(\"\${CMAKE_ARGV3}\" \"\${CMAKE_ARGV4}\" \"\${CMAKE_ARGV5}\")\n")
	set(_map_child "${_root}/map.cmake")
	file(WRITE "${_map_child}"
		"include(\"${_module}\")\n"
		"tau_output_map(_m \"\${CMAKE_ARGV3}\")\n")

	# a file record carries its permission bits: a changed executable bit is a
	# different artifact and must fail verification.
	string(JSON _mode GET "${_map}" "lib/libx.so.1.2.3" "mode")
	if(_mode STREQUAL "" OR _mode STREQUAL "NOTFOUND")
		tau_fail("libx.so.1.2.3 has no mode in the output map")
	endif()
	file(CHMOD "${_pkg}/include/x.h"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
	tau_expect_fatal("${_root}" "a changed file mode"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_mpath}" "${_pkg}"
		"${_pkg_id}")
	file(CHMOD "${_pkg}/include/x.h"
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

	tau_expect_fatal("${_root}" "empty outputs against a non-empty prefix"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_m_empty}" "${_pkg}"
		"${_empty_id}")

	# extra file
	file(WRITE "${_pkg}/extra.txt" "extra")
	tau_expect_fatal("${_root}" "an extra unlisted file"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_mpath}" "${_pkg}" "${_pkg_id}")
	file(REMOVE "${_pkg}/extra.txt")

	# missing file
	file(REMOVE "${_pkg}/include/x.h")
	tau_expect_fatal("${_root}" "a missing listed file"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_mpath}" "${_pkg}" "${_pkg_id}")
	file(WRITE "${_pkg}/include/x.h" "header")

	# file replaced by a symlink
	file(REMOVE "${_pkg}/include/x.h")
	file(CREATE_LINK "../.hidden" "${_pkg}/include/x.h" SYMBOLIC)
	tau_expect_fatal("${_root}" "a file replaced by a symlink"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_mpath}" "${_pkg}" "${_pkg_id}")
	file(REMOVE "${_pkg}/include/x.h")
	file(WRITE "${_pkg}/include/x.h" "header")

	# symlink replaced by a file
	file(REMOVE "${_pkg}/lib/libx.so.1")
	file(WRITE "${_pkg}/lib/libx.so.1" "library")
	tau_expect_fatal("${_root}" "a symlink replaced by a file"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_mpath}" "${_pkg}" "${_pkg_id}")
	file(REMOVE "${_pkg}/lib/libx.so.1")
	file(CREATE_LINK "libx.so.1.2.3" "${_pkg}/lib/libx.so.1" SYMBOLIC)

	# changed symlink target
	file(WRITE "${_pkg}/lib/libx.so.1.2.4" "other")
	file(REMOVE "${_pkg}/lib/libx.so.1")
	file(CREATE_LINK "libx.so.1.2.4" "${_pkg}/lib/libx.so.1" SYMBOLIC)
	tau_expect_fatal("${_root}" "a changed symlink target"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_mpath}" "${_pkg}" "${_pkg_id}")
	file(REMOVE "${_pkg}/lib/libx.so.1")
	file(CREATE_LINK "libx.so.1.2.3" "${_pkg}/lib/libx.so.1" SYMBOLIC)

	# wrong expected id
	tau_expect_fatal("${_root}" "a wrong expected input id"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_mpath}" "${_pkg}"
		"0000000000000000000000000000000000000000000000000000000000000000")

	# --- identity recomputation ----------------------------------------------
	# a write whose id does not match its fields is rejected
	set(_mw_child "${_root}/mismatch-write.cmake")
	file(WRITE "${_mw_child}"
		"include(\"${_module}\")\n"
		"tau_manifest_write(\"${_root}/mismatch.json\" "
		"\"0000000000000000000000000000000000000000000000000000000000000000\" "
		"\"dep=demo\" \"{}\")\n")
	tau_expect_fatal("${_root}" "a write with a mismatched id"
		"${CMAKE_COMMAND}" -P "${_mw_child}")

	set(_tapath "${_root}/tampered-fields.json")
	# a field changed after write
	file(READ "${_mpath}" _j_changed)
	string(JSON _j_changed SET "${_j_changed}" "fields" "source" "\"changed\"")
	file(WRITE "${_tapath}" "${_j_changed}")
	tau_expect_fatal("${_root}" "a field changed after write"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_tapath}" "${_pkg}"
		"${_pkg_id}")
	# a missing field
	file(READ "${_mpath}" _j_missing)
	string(JSON _j_missing REMOVE "${_j_missing}" "fields" "source")
	file(WRITE "${_tapath}" "${_j_missing}")
	tau_expect_fatal("${_root}" "a missing field"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_tapath}" "${_pkg}"
		"${_pkg_id}")
	# an added field
	file(READ "${_mpath}" _j_added)
	string(JSON _j_added SET "${_j_added}" "fields" "extra" "\"x\"")
	file(WRITE "${_tapath}" "${_j_added}")
	tau_expect_fatal("${_root}" "an added field"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_tapath}" "${_pkg}"
		"${_pkg_id}")
	# a non-string field
	file(READ "${_mpath}" _j_number)
	string(JSON _j_number SET "${_j_number}" "fields" "source" "5")
	file(WRITE "${_tapath}" "${_j_number}")
	tau_expect_fatal("${_root}" "a non-string field"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_tapath}" "${_pkg}"
		"${_pkg_id}")

	# a hand-crafted alternate field structure must not hash the same.
	# {"a": "b=x"} and {"a=b": "x"} reconstruct to the same text; the
	# second is rejected because a field key may not contain '='.
	set(_amb_id "")
	tau_input_id(_amb_id "a=b=x")
	set(_amb_pkg "${_root}/amb-pkg")
	file(MAKE_DIRECTORY "${_amb_pkg}")
	_tau_manifest_json_string("${_amb_id}" _amb_id_lit)
	set(_ok_json "{}")
	string(JSON _ok_json SET "${_ok_json}" "schema" "2")
	string(JSON _ok_json SET "${_ok_json}" "input_id" "${_amb_id_lit}")
	set(_ok_fields "{}")
	string(JSON _ok_fields SET "${_ok_fields}" "a" "\"b=x\"")
	string(JSON _ok_json SET "${_ok_json}" "fields" "${_ok_fields}")
	string(JSON _ok_json SET "${_ok_json}" "outputs" "{}")
	set(_ok_path "${_root}/amb-ok.json")
	file(WRITE "${_ok_path}" "${_ok_json}\n")
	tau_manifest_verify("${_ok_path}" "${_amb_pkg}" "${_amb_id}")
	set(_alt_json "{}")
	string(JSON _alt_json SET "${_alt_json}" "schema" "2")
	string(JSON _alt_json SET "${_alt_json}" "input_id" "${_amb_id_lit}")
	set(_alt_fields "{}")
	string(JSON _alt_fields SET "${_alt_fields}" "a=b" "\"x\"")
	string(JSON _alt_json SET "${_alt_json}" "fields" "${_alt_fields}")
	string(JSON _alt_json SET "${_alt_json}" "outputs" "{}")
	set(_alt_path "${_root}/amb-alt.json")
	file(WRITE "${_alt_path}" "${_alt_json}\n")
	tau_expect_fatal("${_root}" "an ambiguous '=' field key"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_alt_path}" "${_amb_pkg}"
		"${_amb_id}")

	# --- symlink policy in the scan ----------------------------------------------
	set(_sym_pkg "${_root}/sym-pkg")
	file(MAKE_DIRECTORY "${_sym_pkg}")
	file(WRITE "${_sym_pkg}/real" "real")
	file(CREATE_LINK "/etc/hostname" "${_sym_pkg}/absolute" SYMBOLIC)
	tau_expect_fatal("${_root}" "an absolute symlink target"
		"${CMAKE_COMMAND}" -P "${_map_child}" "${_sym_pkg}")
	file(REMOVE "${_sym_pkg}/absolute")

	file(CREATE_LINK "../../../etc/hostname" "${_sym_pkg}/escape" SYMBOLIC)
	tau_expect_fatal("${_root}" "an escaping symlink target"
		"${CMAKE_COMMAND}" -P "${_map_child}" "${_sym_pkg}")
	file(REMOVE "${_sym_pkg}/escape")

	file(CREATE_LINK "absent" "${_sym_pkg}/broken" SYMBOLIC)
	tau_expect_fatal("${_root}" "a broken symlink"
		"${CMAKE_COMMAND}" -P "${_map_child}" "${_sym_pkg}")
	file(REMOVE "${_sym_pkg}/broken")

	file(CREATE_LINK "loop-b" "${_sym_pkg}/loop-a" SYMBOLIC)
	file(CREATE_LINK "loop-a" "${_sym_pkg}/loop-b" SYMBOLIC)
	tau_expect_fatal("${_root}" "a symlink loop"
		"${CMAKE_COMMAND}" -P "${_map_child}" "${_sym_pkg}")
	file(REMOVE "${_sym_pkg}/loop-a" "${_sym_pkg}/loop-b")

	# an ordinary in-prefix chain is accepted
	file(CREATE_LINK "real" "${_sym_pkg}/link" SYMBOLIC)
	tau_output_map(_chain_map "${_sym_pkg}")
	string(JSON _chain_ok GET "${_chain_map}" "link" "target")
	if(NOT _chain_ok STREQUAL "real")
		tau_fail("an in-prefix chain was not recorded")
	endif()

	# --- scale: several thousand outputs ------------------------------------
	# The complete-output-set comparison must stay linear. One lookup per entry
	# made an 8487-output package take about ten minutes. A synthetic package of
	# this many tiny files must write and verify well inside the cap below; a
	# return to per-entry lookups fails it.
	set(_scale_n 4000)
	set(_scale_pkg "${_root}/scale-pkg")
	file(MAKE_DIRECTORY "${_scale_pkg}")
	foreach(_i RANGE 1 ${_scale_n})
		file(WRITE "${_scale_pkg}/f${_i}.txt" "x")
	endforeach()
	string(TIMESTAMP _scale_t0 "%s")
	tau_output_map(_scale_map "${_scale_pkg}")
	string(JSON _scale_count LENGTH "${_scale_map}")
	if(NOT _scale_count EQUAL ${_scale_n})
		tau_fail("scale map has ${_scale_count} entries, expected ${_scale_n}")
	endif()
	set(_scale_manifest "${_root}/scale-manifest.json")
	tau_input_id(_scale_id "dep=scale\noutputs=${_scale_n}")
	tau_manifest_write("${_scale_manifest}" "${_scale_id}"
		"dep=scale\noutputs=${_scale_n}" "${_scale_map}")
	tau_manifest_verify("${_scale_manifest}" "${_scale_pkg}" "${_scale_id}")
	string(TIMESTAMP _scale_t1 "%s")
	math(EXPR _scale_elapsed "${_scale_t1} - ${_scale_t0}")
	if(_scale_elapsed GREATER 60)
		tau_fail("scale verify took ${_scale_elapsed}s for ${_scale_n} outputs "
			"(cap 60s)")
	endif()
	# one changed byte at scale is still rejected
	file(WRITE "${_scale_pkg}/f2000.txt" "y")
	tau_expect_fatal("${_root}" "a changed byte at scale"
		"${CMAKE_COMMAND}" -P "${_verify_child}" "${_scale_manifest}"
		"${_scale_pkg}" "${_scale_id}")
endif()

# --- script mode prints only one id ------------------------------------------
execute_process(
	COMMAND "${CMAKE_COMMAND}" -P "${_module}" input-id "d=1" "x=a;b"
	RESULT_VARIABLE _script_rc OUTPUT_VARIABLE _script_out
	ERROR_VARIABLE _script_err)
if(NOT _script_rc EQUAL 0)
	tau_fail("script-mode input-id failed: ${_script_err}")
endif()
string(STRIP "${_script_out}" _script_id)
if(NOT _script_id MATCHES "^[0-9a-f]+$")
	tau_fail("script-mode stdout is not one id: [${_script_out}]")
endif()
tau_input_id(_script_fn "d=1\nx=a;b")
if(NOT _script_fn STREQUAL _script_id)
	tau_fail("script-mode id and function id differ")
endif()

file(REMOVE_RECURSE "${_root}")
