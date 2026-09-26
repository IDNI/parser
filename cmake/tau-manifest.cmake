# cmake/tau-manifest.cmake
#
# Deterministic input identity, the package manifest schema, and manifest
# verification.
#
# This module is pure. It reads files only through file(SHA256),
# file(READ_SYMLINK), a stat of the permission bits, and the manifest verifier.
# It changes no state and is not wired into any builder.
#
# Field block. A field set is one string, not a CMake list. Newlines separate
# fields. Each field is "key=value", split at the first '='. A CMake list would
# split on ';', which recipe values such as a flags list can contain, so the
# block keeps the whole set in one argument and ';' is preserved. The cost is
# that a value cannot contain a newline, because a newline separates fields. A
# carriage return is rejected so a CRLF line ending cannot hide in a value.
#
# Input identity. An id is the SHA-256 of the canonical field bytes. Each field
# becomes one length-prefixed record, in key order:
#
#   <key-bytes>:<key>=<value-bytes>:<value>\n
#
# The explicit byte lengths make the records unambiguous even if a key or value
# held a newline or a ':'. The bytes are sorted by key, so field order does not
# change the id and a changed value does.
#
# Provenance fields. A key that starts with "provenance." records how the package
# was written, not what was built: the manifest writer's hash and the store
# writer's hash. Both run only after the prefix is installed, so they cannot
# change the artifact. They are recorded in the manifest and validated on read,
# but the id hashes only the build-affecting fields.
#
# Manifest schema 3. A manifest is a JSON object:
#   {
#     "schema": 3,
#     "input_id": "<64 lowercase hex>",
#     "fields": { "<key>": "<value>", ... },
#     "outputs": { "<relative path>": <record>, ... }
#   }
# Schema 2 is the same object without the file "mode" field; verification
# still accepts it so an entry published before the mode field verifies.
# Each record is one of:
#   { "type": "file",    "sha256": "<64 lowercase hex>", "mode": "<octal>" }
#   { "type": "symlink", "target": "<exact link text>" }
#
# The file mode is the stat permission bits, so an executable bit that flips
# changes the record and fails verification. A symlink has no mode of its own.
#
# outputs is the complete set of non-directory entries under the package
# prefix. Directories are not recorded; an empty directory is ignored. Paths
# are stored in sorted relative-path order. A package with no non-directory
# entries has an empty outputs object, and only an empty prefix is valid with
# it.
#
# Verification scans the prefix and compares the complete output set in one
# canonical pass. CMake sorts object keys and serializes deterministically, so
# the declared and the scanned object are canonicalized once and compared as
# text. A lookup per entry would re-parse the whole outputs object every time
# and is quadratic in the output count.
#
# Symlink policy. A symlink target must be relative and must resolve, by
# lexical path arithmetic, inside the package prefix. Chains are followed for
# validation, so an ordinary shared-library chain is supported. An absolute
# target, an escaping '..', a broken link, and a loop are hard errors. The
# record stores the exact target text and verification compares it exactly, so
# a file and a symlink at the same path are not interchangeable.
#
# Usage:
#   include(tau-manifest)
#     tau_input_id(<out-var> "<field block>")
#     tau_canonical_fields(<out-var> "<field block>")
#     tau_output_map(<out-var> <package prefix>)
#     tau_manifest_write(<path> <input_id> "<field block>" "<outputs json>")
#     tau_manifest_verify(<manifest path> <package prefix> <expected input id>)
#   cmake -P tau-manifest.cmake input-id <key=value>...
#     joins the arguments with newlines, prints the input id, and prints nothing
#     else
#
# An empty key, a missing '=', a duplicate key, and a carriage return are hard
# errors. An output path that is absolute, contains '..', or contains ';' is a
# hard error. A missing listed output, an extra unlisted output, a record
# mismatch, a schema mismatch, and an input id mismatch are hard errors. There
# is no fallback.

if(CMAKE_VERSION VERSION_LESS 3.22.1)
	message(FATAL_ERROR "tau-manifest requires CMake 3.22.1 or newer")
endif()

include_guard(GLOBAL)

# Split one key=value at the first '='. Values may contain '='.
function(_tau_manifest_parse pair out_key out_value)
	string(FIND "${pair}" "=" _eq)
	if(_eq EQUAL -1)
		message(FATAL_ERROR "tau-manifest: '${pair}' is not key=value")
	endif()
	string(SUBSTRING "${pair}" 0 ${_eq} _key)
	math(EXPR _vstart "${_eq} + 1")
	string(SUBSTRING "${pair}" ${_vstart} -1 _value)
	if(_key STREQUAL "")
		message(FATAL_ERROR "tau-manifest: empty key in '${pair}'")
	endif()
	string(FIND "${_key}" "\r" _ck)
	string(FIND "${_value}" "\r" _cv)
	if(NOT _ck EQUAL -1 OR NOT _cv EQUAL -1)
		message(FATAL_ERROR
			"tau-manifest: carriage return is not allowed in a field")
	endif()
	set(${out_key} "${_key}" PARENT_SCOPE)
	set(${out_value} "${_value}" PARENT_SCOPE)
endfunction()

# Reject an output path that is absolute, walks up with '..', or contains ';'
# (which a CMake list would split).
function(_tau_manifest_check_relative path)
	set(_p "${path}")
	if(_p STREQUAL "")
		message(FATAL_ERROR "tau-manifest: empty output path")
	endif()
	if(_p MATCHES "^/" OR _p MATCHES "^\\\\" OR _p MATCHES "^[A-Za-z]:")
		message(FATAL_ERROR "tau-manifest: absolute output path: '${_p}'")
	endif()
	if(_p MATCHES ";")
		message(FATAL_ERROR "tau-manifest: ';' is not allowed in an output path: '${_p}'")
	endif()
	# Normalize backslashes for the safety arithmetic only. The stored path
	# keeps its exact text.
	string(REPLACE "\\" "/" _p "${_p}")
	string(REPLACE "/" ";" _parts "${_p}")
	foreach(_c IN LISTS _parts)
		if(_c STREQUAL "..")
			message(FATAL_ERROR
				"tau-manifest: '..' is not allowed in output path: '${_p}'")
		endif()
	endforeach()
endfunction()

# Lexically normalize a relative path: drop '.', apply '..', and report an
# escape when '..' would leave the prefix.
function(_tau_manifest_normalize path out_resolved out_escaped)
	# Backslashes are normalized for the safety arithmetic only; a recorded
	# symlink target keeps its exact text.
	string(REPLACE "\\" "/" _normalized "${path}")
	string(REPLACE "/" ";" _parts "${_normalized}")
	set(_stack "")
	set(_escaped FALSE)
	foreach(_c IN LISTS _parts)
		if(_c STREQUAL "" OR _c STREQUAL ".")
			continue()
		elseif(_c STREQUAL "..")
			if(_stack STREQUAL "")
				set(_escaped TRUE)
				break()
			endif()
			list(POP_BACK _stack)
		else()
			list(APPEND _stack "${_c}")
		endif()
	endforeach()
	if(_escaped)
		set(${out_resolved} "" PARENT_SCOPE)
		set(${out_escaped} TRUE PARENT_SCOPE)
		return()
	endif()
	string(JOIN "/" _resolved ${_stack})
	set(${out_resolved} "${_resolved}" PARENT_SCOPE)
	set(${out_escaped} FALSE PARENT_SCOPE)
endfunction()

# Validate one symlink: a relative target that resolves inside the prefix and
# whose chain terminates at an existing entry.
function(_tau_manifest_check_symlink prefix link_rel target)
	if(target STREQUAL "")
		message(FATAL_ERROR
			"tau-manifest: empty symlink target for '${link_rel}'")
	endif()
	if(target MATCHES "^/" OR target MATCHES "^\\\\"
			OR target MATCHES "^[A-Za-z]:")
		message(FATAL_ERROR "tau-manifest: absolute symlink target for "
			"'${link_rel}': '${target}'")
	endif()
	if(target MATCHES ";")
		message(FATAL_ERROR
			"tau-manifest: ';' is not allowed in a symlink target: '${target}'")
	endif()
	set(_visited "")
	list(APPEND _visited "${link_rel}")
	get_filename_component(_dir "${link_rel}" DIRECTORY)
	set(_cur "${target}")
	while(TRUE)
		if(_dir STREQUAL "")
			set(_joined "${_cur}")
		else()
			set(_joined "${_dir}/${_cur}")
		endif()
		_tau_manifest_normalize("${_joined}" _resolved _escaped)
		if(_escaped OR _resolved STREQUAL "")
			message(FATAL_ERROR "tau-manifest: symlink '${link_rel}' escapes "
				"the package prefix via '${target}'")
		endif()
		if(_resolved IN_LIST _visited)
			message(FATAL_ERROR "tau-manifest: symlink loop at '${link_rel}' "
				"-> '${target}'")
		endif()
		list(APPEND _visited "${_resolved}")
		set(_full "${prefix}/${_resolved}")
		if(NOT EXISTS "${_full}")
			message(FATAL_ERROR "tau-manifest: broken symlink '${link_rel}' "
				"-> '${target}' ('${_resolved}' does not exist)")
		endif()
		if(NOT IS_SYMLINK "${_full}")
			return()
		endif()
		file(READ_SYMLINK "${_full}" _next)
		if(_next STREQUAL "" OR _next MATCHES "^/" OR _next MATCHES "^\\\\"
				OR _next MATCHES "^[A-Za-z]:")
			message(FATAL_ERROR "tau-manifest: absolute or empty symlink "
				"target in the chain at '${_resolved}': '${_next}'")
		endif()
		if(_next MATCHES ";")
			message(FATAL_ERROR "tau-manifest: ';' is not allowed in a "
				"symlink target: '${_next}'")
		endif()
		get_filename_component(_dir "${_resolved}" DIRECTORY)
		set(_cur "${_next}")
	endwhile()
endfunction()

# Walk a package prefix, collecting every non-directory entry as a relative
# path. A symlink is recorded and not traversed.
function(_tau_manifest_walk dir rel out_paths)
	set(_paths "")
	file(GLOB _entries RELATIVE "${dir}" "${dir}/*")
	foreach(_e IN LISTS _entries)
		set(_full "${dir}/${_e}")
		if(rel STREQUAL "")
			set(_r "${_e}")
		else()
			set(_r "${rel}/${_e}")
		endif()
		if(IS_SYMLINK "${_full}")
			list(APPEND _paths "${_r}")
		elseif(IS_DIRECTORY "${_full}")
			_tau_manifest_walk("${_full}" "${_r}" _sub)
			list(APPEND _paths ${_sub})
		else()
			list(APPEND _paths "${_r}")
		endif()
	endforeach()
	set(${out_paths} "${_paths}" PARENT_SCOPE)
endfunction()

# The permission bits as stat prints them ("644", "755", "4755"). CMake
# cannot read a mode itself, so the GNU and BSD spellings are both tried; an
# empty result is fatal, because a mode-less record would verify any chmod.
function(_tau_manifest_file_mode path out)
	set(_mode "")
	if(UNIX)
		execute_process(COMMAND stat -c %a "${path}"
			RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_QUIET)
		if(_rc EQUAL 0)
			string(STRIP "${_out}" _mode)
		else()
			execute_process(COMMAND stat -f %Lp "${path}"
				RESULT_VARIABLE _rc2 OUTPUT_VARIABLE _out2 ERROR_QUIET)
			if(_rc2 EQUAL 0)
				string(STRIP "${_out2}" _mode)
			endif()
		endif()
	else()
		# Windows has no POSIX permission bits; one constant keeps the schema
		# total without pretending a mode change can be detected there.
		set(_mode "0")
	endif()
	if(_mode STREQUAL "")
		message(FATAL_ERROR
			"tau-manifest: cannot read the file mode of '${path}'")
	endif()
	set(${out} "${_mode}" PARENT_SCOPE)
endfunction()

# Scan a package prefix into a deterministic, compact outputs object.
#
# The object is one JSON text literal assembled entry by entry and validated
# once. A per-entry string(JSON SET) re-serializes the whole object on every
# entry, which is quadratic in the output count.
function(_tau_manifest_scan_prefix prefix with_mode out_json)
	if(NOT IS_DIRECTORY "${prefix}")
		message(FATAL_ERROR
			"tau-manifest: package prefix is not a directory: '${prefix}'")
	endif()
	set(_paths "")
	_tau_manifest_walk("${prefix}" "" _paths)
	list(SORT _paths)
	set(_outputs "{")
	set(_first TRUE)
	foreach(_rel IN LISTS _paths)
		_tau_manifest_check_relative("${_rel}")
		set(_full "${prefix}/${_rel}")
		if(IS_SYMLINK "${_full}")
			file(READ_SYMLINK "${_full}" _target)
			_tau_manifest_check_symlink("${prefix}" "${_rel}" "${_target}")
			_tau_manifest_json_string("${_target}" _target_literal)
			set(_record
				"{\"type\":\"symlink\",\"target\":${_target_literal}}")
		elseif(IS_DIRECTORY "${_full}")
			message(FATAL_ERROR
				"tau-manifest: a directory appeared in the scan: '${_rel}'")
		elseif(EXISTS "${_full}")
			file(SHA256 "${_full}" _sha)
			_tau_manifest_json_string("${_sha}" _sha_literal)
			if(with_mode)
				_tau_manifest_file_mode("${_full}" _mode)
				_tau_manifest_json_string("${_mode}" _mode_literal)
				set(_record
					"{\"type\":\"file\",\"sha256\":${_sha_literal},\"mode\":${_mode_literal}}")
			else()
				set(_record
					"{\"type\":\"file\",\"sha256\":${_sha_literal}}")
			endif()
		else()
			message(FATAL_ERROR
				"tau-manifest: cannot classify '${_rel}'")
		endif()
		_tau_manifest_json_string("${_rel}" _key_literal)
		if(_first)
			set(_first FALSE)
		else()
			string(APPEND _outputs ",")
		endif()
		string(APPEND _outputs "${_key_literal}:${_record}")
	endforeach()
	string(APPEND _outputs "}")
	set(${out_json} "${_outputs}" PARENT_SCOPE)
endfunction()

# Validate a JSON object literal and return CMake's canonical serialization of
# it. CMake sorts object keys and serializes deterministically, so two objects
# that hold the same entries produce the same bytes here regardless of how they
# were written.
function(_tau_manifest_canonical_object json out)
	string(JSON _type ERROR_VARIABLE _err TYPE "${json}")
	if(NOT _err STREQUAL "NOTFOUND")
		message(FATAL_ERROR "tau-manifest: not valid JSON: ${_err}")
	endif()
	if(NOT _type STREQUAL "OBJECT")
		message(FATAL_ERROR "tau-manifest: not a JSON object: '${_type}'")
	endif()
	string(JSON _holder SET "{}" "outputs" "${json}")
	string(JSON _canon GET "${_holder}" "outputs")
	set(${out} "${_canon}" PARENT_SCOPE)
endfunction()

# Reconstruct the canonical field block from a manifest's fields object. A
# non-string value, a key that is not a valid field, and a newline are hard
# errors.
function(_tau_manifest_fields_block json out_block)
	string(JSON _ftype ERROR_VARIABLE _ferr TYPE "${json}" "fields")
	if(NOT _ferr STREQUAL "NOTFOUND")
		message(FATAL_ERROR "tau-manifest: manifest fields is missing: "
			"${_ferr}")
	endif()
	if(NOT _ftype STREQUAL "OBJECT")
		message(FATAL_ERROR "tau-manifest: manifest fields is not an object: "
			"'${_ftype}'")
	endif()
	string(JSON _count LENGTH "${json}" "fields")
	set(_block "")
	if(_count GREATER 0)
		math(EXPR _last "${_count} - 1")
		foreach(_i RANGE 0 ${_last})
			string(JSON _key MEMBER "${json}" "fields" ${_i})
			string(JSON _vtype ERROR_VARIABLE _verr TYPE "${json}"
				"fields" "${_key}")
			if(NOT _verr STREQUAL "NOTFOUND")
				message(FATAL_ERROR
					"tau-manifest: field '${_key}' is missing")
			endif()
			if(NOT _vtype STREQUAL "STRING")
				message(FATAL_ERROR "tau-manifest: field '${_key}' is not a "
					"string: '${_vtype}'")
			endif()
			string(JSON _value GET "${json}" "fields" "${_key}")
			string(FIND "${_key}" "\n" _kn)
			string(FIND "${_value}" "\n" _vn)
			if(NOT _kn EQUAL -1 OR NOT _vn EQUAL -1)
				message(FATAL_ERROR "tau-manifest: field '${_key}' "
					"contains a newline")
			endif()
			string(FIND "${_key}" "=" _keq)
			if(NOT _keq EQUAL -1)
				message(FATAL_ERROR "tau-manifest: field key contains '=': "
					"'${_key}'")
			endif()
			_tau_manifest_parse("${_key}=${_value}" _k _v)
			if(NOT _k STREQUAL "${_key}")
				message(FATAL_ERROR "tau-manifest: ambiguous field key: "
					"'${_key}'")
			endif()
			string(APPEND _block "${_key}=${_value}\n")
		endforeach()
	endif()
	set(${out_block} "${_block}" PARENT_SCOPE)
endfunction()

# Reconstruct the build-affecting field block (provenance excluded), in the
# manifest's stored key order.
function(_tau_manifest_build_fields_block json out_block)
	_tau_manifest_fields_block("${json}" _all)
	set(_block "")
	_tau_manifest_split("${_all}" _tau_line_ _tau_n)
	if(_tau_n GREATER 0)
		math(EXPR _last "${_tau_n} - 1")
		foreach(_i RANGE 0 ${_last})
			_tau_manifest_parse("${_tau_line_${_i}}" _key _value)
			_tau_manifest_is_provenance("${_key}" _is_prov)
			if(NOT _is_prov)
				string(APPEND _block "${_key}=${_value}\n")
			endif()
		endforeach()
	endif()
	set(${out_block} "${_block}" PARENT_SCOPE)
endfunction()

# Split a field block on newlines into caller-scope variables <prefix>0..<n-1>.
# Each line lives in its own variable, never in a CMake list, so a ';' survives.
function(_tau_manifest_split block prefix out_count)
	set(_n 0)
	set(_rest "${block}")
	while(NOT _rest STREQUAL "")
		string(FIND "${_rest}" "\n" _nl)
		if(_nl EQUAL -1)
			set(_line "${_rest}")
			set(_rest "")
		else()
			string(SUBSTRING "${_rest}" 0 ${_nl} _line)
			math(EXPR _after "${_nl} + 1")
			string(SUBSTRING "${_rest}" ${_after} -1 _rest)
		endif()
		if(NOT _line STREQUAL "")
			set(${prefix}${_n} "${_line}" PARENT_SCOPE)
			math(EXPR _n "${_n} + 1")
		endif()
	endwhile()
	set(${out_count} "${_n}" PARENT_SCOPE)
endfunction()

# Order a list of integer indices by the variable <prefix><index>. The lengths
# are integer lists, so a ';' in a key or path cannot split them.
function(_tau_manifest_sort_indices pool prefix out)
	set(_remaining ${pool})
	set(_order "")
	list(LENGTH _remaining _len)
	while(_len GREATER 0)
		list(GET _remaining 0 _min)
		foreach(_b IN LISTS _remaining)
			string(COMPARE LESS "${${prefix}${_b}}" "${${prefix}${_min}}"
				_less)
			if(_less)
				set(_min "${_b}")
			endif()
		endforeach()
		list(APPEND _order "${_min}")
		list(REMOVE_ITEM _remaining "${_min}")
		list(LENGTH _remaining _len)
	endwhile()
	set(${out} "${_order}" PARENT_SCOPE)
endfunction()

# TRUE when a field records how the package was written rather than what was
# built. Such a field cannot change the prefix bytes, so it stays out of the id.
function(_tau_manifest_is_provenance key out)
	if("${key}" MATCHES "^provenance\\.")
		set(${out} TRUE PARENT_SCOPE)
	else()
		set(${out} FALSE PARENT_SCOPE)
	endif()
endfunction()

# Canonical field bytes: length-prefixed records, sorted by key. Provenance
# fields are validated but not recorded.
function(_tau_manifest_canonical out block)
	_tau_manifest_split("${block}" _tau_line_ _tau_n)
	set(_pool "")
	if(_tau_n GREATER 0)
		math(EXPR _last "${_tau_n} - 1")
		foreach(_i RANGE 0 ${_last})
			_tau_manifest_parse("${_tau_line_${_i}}" _key _value)
			if(_i GREATER 0)
				math(EXPR _pmax "${_i} - 1")
				foreach(_p RANGE 0 ${_pmax})
					if(_tau_key_${_p} STREQUAL "${_key}")
						message(FATAL_ERROR
							"tau-manifest: duplicate key '${_key}'")
					endif()
				endforeach()
			endif()
			set(_tau_key_${_i} "${_key}")
			set(_tau_val_${_i} "${_value}")
			_tau_manifest_is_provenance("${_key}" _is_prov)
			if(NOT _is_prov)
				list(APPEND _pool "${_i}")
			endif()
		endforeach()
	endif()
	_tau_manifest_sort_indices("${_pool}" _tau_key_ _order)
	set(_canonical "")
	foreach(_i IN LISTS _order)
		string(LENGTH "${_tau_key_${_i}}" _kl)
		string(LENGTH "${_tau_val_${_i}}" _vl)
		string(APPEND _canonical
			"${_kl}:${_tau_key_${_i}}=${_vl}:${_tau_val_${_i}}\n")
	endforeach()
	set(${out} "${_canonical}" PARENT_SCOPE)
endfunction()

# Canonical field bytes for a block.
function(tau_canonical_fields out block)
	_tau_manifest_canonical(_canonical "${block}")
	set(${out} "${_canonical}" PARENT_SCOPE)
endfunction()

# SHA-256 of the canonical field bytes, lowercase hex. Only build-affecting
# fields are hashed.
function(tau_input_id out block)
	_tau_manifest_canonical(_canonical "${block}")
	string(SHA256 _id "${_canonical}")
	set(${out} "${_id}" PARENT_SCOPE)
endfunction()

# TRUE when two manifests describe the same artifact: the same build-affecting
# fields and the same recorded outputs. Provenance fields may differ.
function(tau_manifest_same_artifact manifest_a manifest_b out)
	if(NOT EXISTS "${manifest_a}" OR NOT EXISTS "${manifest_b}")
		message(FATAL_ERROR "tau-manifest: manifest not found: '${manifest_a}' or '${manifest_b}'")
	endif()
	file(READ "${manifest_a}" _ja)
	file(READ "${manifest_b}" _jb)
	_tau_manifest_build_fields_block("${_ja}" _ba)
	_tau_manifest_build_fields_block("${_jb}" _bb)
	if(NOT _ba STREQUAL "${_bb}")
		set(${out} FALSE PARENT_SCOPE)
		return()
	endif()
	string(JSON _oa GET "${_ja}" "outputs")
	string(JSON _ob GET "${_jb}" "outputs")
	if(NOT _oa STREQUAL "${_ob}")
		set(${out} FALSE PARENT_SCOPE)
		return()
	endif()
	set(${out} TRUE PARENT_SCOPE)
endfunction()

# Escape one value into a JSON string literal. CMake's JSON parser validates
# the literal and round-trips it, so a value the parser cannot represent is a
# hard error rather than a corrupt manifest.
function(_tau_manifest_json_string value out)
	set(_s "${value}")
	string(REPLACE "\\" "\\\\" _s "${_s}")
	string(REPLACE "\"" "\\\"" _s "${_s}")
	string(REPLACE "\n" "\\n" _s "${_s}")
	string(REPLACE "\r" "\\r" _s "${_s}")
	string(REPLACE "\t" "\\t" _s "${_s}")
	set(_literal "\"${_s}\"")
	string(JSON _probe ERROR_VARIABLE _err SET "{}" "v" "${_literal}")
	if(NOT _err STREQUAL "NOTFOUND")
		message(FATAL_ERROR
			"tau-manifest: cannot encode JSON value '${value}': ${_err}")
	endif()
	string(JSON _round GET "${_probe}" "v")
	if(NOT _round STREQUAL "${value}")
		message(FATAL_ERROR
			"tau-manifest: JSON value '${value}' did not round-trip")
	endif()
	set(${out} "${_literal}" PARENT_SCOPE)
endfunction()

# Build the complete outputs object for a package prefix, with file modes.
function(tau_output_map out package_prefix)
	_tau_manifest_scan_prefix("${package_prefix}" TRUE _outputs)
	set(${out} "${_outputs}" PARENT_SCOPE)
endfunction()

# Write the manifest. The input_id must be exactly 64 lowercase hex
# characters. The outputs argument is the complete tree map.
function(tau_manifest_write path input_id block)
	set(_outputs "{}")
	if(ARGC GREATER 3)
		set(_outputs "${ARGV3}")
	endif()
	if(NOT input_id MATCHES "^[0-9a-f]+$")
		message(FATAL_ERROR
			"tau-manifest: input_id is not lowercase hex: '${input_id}'")
	endif()
	string(LENGTH "${input_id}" _id_len)
	if(NOT _id_len EQUAL 64)
		message(FATAL_ERROR
			"tau-manifest: input_id is not 64 hex characters: '${input_id}'")
	endif()
	_tau_manifest_canonical(_ignored "${block}")
	tau_input_id(_computed "${block}")
	if(NOT _computed STREQUAL "${input_id}")
		message(FATAL_ERROR "tau-manifest: input_id '${input_id}' does not "
			"match the field block ('${_computed}')")
	endif()
	_tau_manifest_split("${block}" _tau_line_ _tau_n)
	set(_fields "{}")
	if(_tau_n GREATER 0)
		math(EXPR _last "${_tau_n} - 1")
		foreach(_i RANGE 0 ${_last})
			_tau_manifest_parse("${_tau_line_${_i}}" _key _value)
			_tau_manifest_json_string("${_value}" _literal)
			string(JSON _fields SET "${_fields}" "${_key}" "${_literal}")
		endforeach()
	endif()
	string(JSON _out_type ERROR_VARIABLE _out_err TYPE "${_outputs}")
	if(NOT _out_err STREQUAL "NOTFOUND")
		message(FATAL_ERROR
			"tau-manifest: outputs is not valid JSON: ${_out_err}")
	endif()
	if(NOT _out_type STREQUAL "OBJECT")
		message(FATAL_ERROR "tau-manifest: outputs is not a JSON object")
	endif()
	set(_manifest "{}")
	string(JSON _manifest SET "${_manifest}" "schema" "3")
	_tau_manifest_json_string("${input_id}" _id_literal)
	string(JSON _manifest SET "${_manifest}" "input_id" "${_id_literal}")
	string(JSON _manifest SET "${_manifest}" "fields" "${_fields}")
	string(JSON _manifest SET "${_manifest}" "outputs" "${_outputs}")
	file(WRITE "${path}" "${_manifest}\n")
endfunction()

# Compare the manifest outputs with the scanned outputs. A missing listed
# entry, an extra unlisted entry, and a record mismatch are hard errors.
function(_tau_manifest_compare_outputs manifest actual)
	string(JSON _mcount LENGTH "${manifest}" "outputs")
	if(_mcount GREATER 0)
		math(EXPR _mlast "${_mcount} - 1")
		foreach(_i RANGE 0 ${_mlast})
			string(JSON _rel MEMBER "${manifest}" "outputs" ${_i})
			_tau_manifest_check_relative("${_rel}")
			string(JSON _actual_record ERROR_VARIABLE _absent
				GET "${actual}" "outputs" "${_rel}")
			if(NOT _absent STREQUAL "NOTFOUND")
				message(FATAL_ERROR
					"tau-manifest: listed output is missing: '${_rel}'")
			endif()
			string(JSON _manifest_record
				GET "${manifest}" "outputs" "${_rel}")
			if(NOT _manifest_record STREQUAL _actual_record)
				message(FATAL_ERROR
					"tau-manifest: output record mismatch for '${_rel}': "
					"manifest ${_manifest_record} actual ${_actual_record}")
			endif()
		endforeach()
	endif()
	string(JSON _acount LENGTH "${actual}" "outputs")
	if(_acount GREATER 0)
		math(EXPR _alast "${_acount} - 1")
		foreach(_i RANGE 0 ${_alast})
			string(JSON _rel MEMBER "${actual}" "outputs" ${_i})
			string(JSON _record ERROR_VARIABLE _extra
				GET "${manifest}" "outputs" "${_rel}")
			if(NOT _extra STREQUAL "NOTFOUND")
				message(FATAL_ERROR
					"tau-manifest: unlisted output: '${_rel}'")
			endif()
		endforeach()
	endif()
endfunction()

# Verify a manifest against the package prefix and an expected input id.
# Checks the schema version, the input id, path safety, the complete output
# set, each record type, each digest, and each symlink target. Any mismatch is
# a hard error.
function(tau_manifest_verify manifest_path package_prefix expected_input_id)
	if(NOT EXISTS "${manifest_path}")
		message(FATAL_ERROR
			"tau-manifest: manifest not found: '${manifest_path}'")
	endif()
	file(READ "${manifest_path}" _json)
	string(JSON _schema ERROR_VARIABLE _schema_err GET "${_json}" "schema")
	if(NOT _schema_err STREQUAL "NOTFOUND")
		message(FATAL_ERROR "tau-manifest: manifest is not valid JSON: "
			"${_schema_err}")
	endif()
	if(NOT _schema STREQUAL "2" AND NOT _schema STREQUAL "3")
		message(FATAL_ERROR "tau-manifest: schema is not 2 or 3: '${_schema}'")
	endif()
	string(JSON _id ERROR_VARIABLE _id_err GET "${_json}" "input_id")
	if(NOT _id_err STREQUAL "NOTFOUND")
		message(FATAL_ERROR "tau-manifest: manifest input_id is missing: "
			"${_id_err}")
	endif()
	if(NOT _id STREQUAL "${expected_input_id}")
		message(FATAL_ERROR "tau-manifest: input_id mismatch: manifest "
			"'${_id}' expected '${expected_input_id}'")
	endif()
	_tau_manifest_fields_block("${_json}" _fields_block)
	tau_input_id(_recomputed "${_fields_block}")
	if(NOT _recomputed STREQUAL "${_id}")
		message(FATAL_ERROR "tau-manifest: manifest input_id does not match "
			"its fields")
	endif()
	if(NOT _recomputed STREQUAL "${expected_input_id}")
		message(FATAL_ERROR "tau-manifest: manifest fields do not match the "
			"expected input id")
	endif()
	string(JSON _out_type ERROR_VARIABLE _out_err TYPE "${_json}" "outputs")
	if(NOT _out_err STREQUAL "NOTFOUND")
		message(FATAL_ERROR "tau-manifest: manifest outputs is missing: "
			"${_out_err}")
	endif()
	if(NOT _out_type STREQUAL "OBJECT")
		message(FATAL_ERROR "tau-manifest: outputs is not an object: "
			"'${_out_type}'")
	endif()
	if(_schema STREQUAL "3")
		_tau_manifest_scan_prefix("${package_prefix}" TRUE _actual_outputs)
	else()
		# schema 2 predates the file-mode field; scan the legacy shape so an
		# entry published before the bump still verifies.
		_tau_manifest_scan_prefix("${package_prefix}" FALSE _actual_outputs)
	endif()
	_tau_manifest_canonical_object("${_actual_outputs}" _actual)
	string(JSON _declared GET "${_json}" "outputs")
	if(NOT _actual STREQUAL "${_declared}")
		# The canonical forms differ. Re-run the entry-by-entry comparison so the
		# error names the missing, extra, or mismatched entry. This path runs only
		# on failure, so its per-entry lookups are not on the hot path.
		set(_wrapped "{}")
		string(JSON _wrapped SET "${_wrapped}" "outputs" "${_actual}")
		_tau_manifest_compare_outputs("${_json}" "${_wrapped}")
		message(FATAL_ERROR
			"tau-manifest: output set does not match the manifest")
	endif()
endfunction()

# Script mode. Runs only when this file is the top-level script, so an
# include() from another script (the self-check) does not dispatch.
#
#   input-id <key=value>...
#     prints the id; each argument is one field
#   input-id-file <block file>
#     prints the id for a newline-separated field block in the file
#   manifest <manifest path> <input id> <block file> <package prefix>
#     scans the prefix and writes the manifest; prints nothing
#
# Stdout carries only the requested value. Diagnostics go to stderr.
if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	if(CMAKE_ARGC LESS 4)
		message(FATAL_ERROR "usage: cmake -P tau-manifest.cmake "
			"<input-id|input-id-file|manifest> ...")
	endif()
	if(CMAKE_ARGV3 STREQUAL "input-id")
		set(_block "")
		math(EXPR _last "${CMAKE_ARGC} - 1")
		foreach(_i RANGE 4 ${_last})
			string(APPEND _block "${CMAKE_ARGV${_i}}\n")
		endforeach()
		tau_input_id(_id "${_block}")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_id}")
	elseif(CMAKE_ARGV3 STREQUAL "input-id-file")
		if(CMAKE_ARGC LESS 5)
			message(FATAL_ERROR
				"usage: cmake -P tau-manifest.cmake input-id-file <file>")
		endif()
		file(READ "${CMAKE_ARGV4}" _block)
		tau_input_id(_id "${_block}")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_id}")
	elseif(CMAKE_ARGV3 STREQUAL "manifest")
		if(CMAKE_ARGC LESS 8)
			message(FATAL_ERROR "usage: cmake -P tau-manifest.cmake "
				"manifest <path> <input-id> <block-file> <prefix>")
		endif()
		file(READ "${CMAKE_ARGV6}" _block)
		tau_output_map(_outputs "${CMAKE_ARGV7}")
		tau_manifest_write("${CMAKE_ARGV4}" "${CMAKE_ARGV5}" "${_block}"
			"${_outputs}")
	else()
		message(FATAL_ERROR "tau-manifest: unknown command '${CMAKE_ARGV3}'")
	endif()
	return()
endif()
