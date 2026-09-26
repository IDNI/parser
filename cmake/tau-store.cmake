# cmake/tau-store.cmake
#
# The LOCAL dependency store primitive. It is pure CMake and is not wired into
# any dependency builder.
#
# Layout, under an explicit shared prefix:
#
#   <prefix>/store/<dep>/<input-id>/manifest.json
#   <prefix>/store/<dep>/<input-id>/prefix/
#   <prefix>/store/<dep>/<input-id>.lock
#
# The prefix is a function argument, not an environment read. Nothing here
# touches $HOME or ~/.tau.
#
# Lookup returns a clean miss for an absent entry. An entry that exists but is
# partial, malformed, or digest-invalid is a hard error, never a miss and never
# a fallback. A valid entry is a hit and returns its prefix directory.
#
# Publication takes a fully prepared staging entry that is a sibling of the
# final entry, under the same dependency directory, so the final rename is a
# same-filesystem operation. The staging manifest is verified before the
# rename, the input id is locked, and the entry is moved into place with
# file(RENAME). A partial final entry is never visible.
#
# If a valid final entry already exists under the lock, it must describe the
# same artifact as the staging manifest: the build-affecting fields and output
# digests must match, while a provenance-only difference is accepted. On a match
# the existing entry is returned and left untouched. On a mismatch the call is a
# hard error, so one input id is never re-tagged with different artifact bytes.
#
# Usage:
#   include(tau-store)
#     tau_store_entry(<out> <prefix> <dep> <input-id>)
#     tau_store_prefix(<out> <prefix> <dep> <input-id>)
#     tau_store_lookup(<prefix> <dep> <input-id> <out-found> <out-prefix>)
#     tau_store_publish(<out-prefix> <prefix> <dep> <input-id> <staging>)

if(CMAKE_VERSION VERSION_LESS 3.22.1)
	message(FATAL_ERROR "tau-store requires CMake 3.22.1 or newer")
endif()

include_guard(GLOBAL)

if(NOT COMMAND tau_manifest_verify)
	include("${CMAKE_CURRENT_LIST_DIR}/tau-manifest.cmake")
endif()

# A dependency id is one safe path component.
function(_tau_store_check_component name what)
	set(_n "${name}")
	if(_n STREQUAL "")
		message(FATAL_ERROR "tau-store: empty ${what}")
	endif()
	if(_n MATCHES "/" OR _n MATCHES "\\\\" OR _n MATCHES ";"
			OR _n MATCHES ":")
		message(FATAL_ERROR
			"tau-store: ${what} is not one path component: '${_n}'")
	endif()
	if(_n STREQUAL "." OR _n STREQUAL "..")
		message(FATAL_ERROR "tau-store: ${what} is not a usable name: '${_n}'")
	endif()
endfunction()

# An input id is exactly 64 lowercase hex characters.
function(_tau_store_check_id input_id)
	if(NOT input_id MATCHES "^[0-9a-f]+$")
		message(FATAL_ERROR
			"tau-store: input id is not lowercase hex: '${input_id}'")
	endif()
	string(LENGTH "${input_id}" _len)
	if(NOT _len EQUAL 64)
		message(FATAL_ERROR
			"tau-store: input id is not 64 hex characters: '${input_id}'")
	endif()
endfunction()

# True when a store directory name is a well-formed input id. CMake's regex has
# no {n} repetition, so the length is checked separately.
function(_tau_store_id_valid name out)
	set(_ok FALSE)
	if(name MATCHES "^[0-9a-f]+$")
		string(LENGTH "${name}" _len)
		if(_len EQUAL 64)
			set(_ok TRUE)
		endif()
	endif()
	set(${out} "${_ok}" PARENT_SCOPE)
endfunction()

# A symlinked staging root, entry root, manifest, or prefix root is rejected. A
# symlink inside a package prefix is package content and is not this module's
# concern.
function(_tau_store_reject_symlink path what)
	if(IS_SYMLINK "${path}")
		message(FATAL_ERROR
			"tau-store: ${what} is a symbolic link: '${path}'")
	endif()
endfunction()

function(tau_store_entry out prefix dep input_id)
	_tau_store_check_component("${dep}" "dep")
	_tau_store_check_id("${input_id}")
	get_filename_component(_prefix "${prefix}" ABSOLUTE)
	set(${out} "${_prefix}/store/${dep}/${input_id}" PARENT_SCOPE)
endfunction()

function(tau_store_prefix out prefix dep input_id)
	tau_store_entry(_entry "${prefix}" "${dep}" "${input_id}")
	set(${out} "${_entry}/prefix" PARENT_SCOPE)
endfunction()

# Look up one entry. On a hit, set <out-found> to TRUE and <out-prefix> to the
# entry prefix. On a miss, set <out-found> to FALSE. A partial, malformed, or
# digest-invalid entry is a hard error.
function(tau_store_lookup prefix dep input_id out_found out_prefix)
	tau_store_entry(_entry "${prefix}" "${dep}" "${input_id}")
	# EXISTS follows a symlink, so a dangling entry link would read as a miss.
	_tau_store_reject_symlink("${_entry}" "entry")
	if(NOT EXISTS "${_entry}")
		set(${out_found} FALSE PARENT_SCOPE)
		set(${out_prefix} "" PARENT_SCOPE)
		return()
	endif()
	if(NOT IS_DIRECTORY "${_entry}")
		message(FATAL_ERROR
			"tau-store: entry is not a directory: '${_entry}'")
	endif()
	_tau_store_reject_symlink("${_entry}/manifest.json" "entry manifest")
	if(NOT EXISTS "${_entry}/manifest.json")
		message(FATAL_ERROR
			"tau-store: partial entry, manifest.json missing: '${_entry}'")
	endif()
	_tau_store_reject_symlink("${_entry}/prefix" "entry prefix")
	if(NOT IS_DIRECTORY "${_entry}/prefix")
		message(FATAL_ERROR
			"tau-store: partial entry, prefix/ missing: '${_entry}'")
	endif()
	tau_manifest_verify("${_entry}/manifest.json" "${_entry}/prefix"
		"${input_id}")
	set(${out_found} TRUE PARENT_SCOPE)
	set(${out_prefix} "${_entry}/prefix" PARENT_SCOPE)
endfunction()

# Rename a staging entry into place without replacing an existing destination.
# Sets <out-result> to 0 on success, NO_REPLACE when the destination exists, or
# an error message.
function(_tau_store_rename_no_replace staging entry out_result)
	file(RENAME "${staging}" "${entry}" RESULT _rc NO_REPLACE)
	set(${out_result} "${_rc}" PARENT_SCOPE)
endfunction()

# Probe whether a no-clobber directory move is available and behaves as
# required: an absent destination is moved, an existing destination is left
# untouched and the source is kept. Sets <out-ok> to TRUE only when both cases
# are observed. Never trusts the command name alone.
function(_tau_store_mv_no_clobber_supported out scratch)
	set(_ok FALSE)
	if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
		find_program(_mv mv NO_CACHE)
		if(_mv)
			string(RANDOM LENGTH 12 _tok)
			set(_a_src "${scratch}/.tau-mv-${_tok}-a-src")
			set(_a_dst "${scratch}/.tau-mv-${_tok}-a-dst")
			set(_b_src "${scratch}/.tau-mv-${_tok}-b-src")
			set(_b_dst "${scratch}/.tau-mv-${_tok}-b-dst")
			file(MAKE_DIRECTORY "${_a_src}")
			file(WRITE "${_a_src}/s" "s")
			file(MAKE_DIRECTORY "${_a_dst}")
			file(WRITE "${_a_dst}/keep" "k")
			file(MAKE_DIRECTORY "${_b_src}")
			file(WRITE "${_b_src}/s" "s")
			execute_process(COMMAND "${_mv}" --no-clobber --no-target-directory
				"${_a_src}" "${_a_dst}"
				RESULT_VARIABLE _arc OUTPUT_QUIET ERROR_QUIET)
			execute_process(COMMAND "${_mv}" --no-clobber --no-target-directory
				"${_b_src}" "${_b_dst}"
				RESULT_VARIABLE _brc OUTPUT_QUIET ERROR_QUIET)
			if(EXISTS "${_a_src}/s" AND EXISTS "${_a_dst}/keep"
					AND NOT EXISTS "${_b_src}" AND EXISTS "${_b_dst}/s")
				set(_ok TRUE)
			endif()
			file(REMOVE_RECURSE "${_a_src}" "${_a_dst}" "${_b_src}" "${_b_dst}")
		endif()
	endif()
	set(${out} "${_ok}" PARENT_SCOPE)
endfunction()

# Require an existing final entry to be complete, non-symlinked, and to agree
# with the staging manifest bytes and digests. Sets <out-prefix> on agreement.
# A partial entry, a symlink, a manifest mismatch, or a digest mismatch is a
# hard error.
function(_tau_store_existing_prefix entry input_id staging out_prefix)
	_tau_store_reject_symlink("${entry}" "entry")
	if(NOT EXISTS "${entry}/manifest.json"
			OR NOT IS_DIRECTORY "${entry}/prefix")
		message(FATAL_ERROR "tau-store: existing entry is partial: '${entry}'")
	endif()
	_tau_store_reject_symlink("${entry}/manifest.json" "entry manifest")
	_tau_store_reject_symlink("${entry}/prefix" "entry prefix")
	# Compare what the two manifests say the artifact is (build-affecting fields
	# and output digests), not the raw manifest bytes: a provenance-only writer
	# change must not turn into a spurious conflict, while an id that maps to
	# different artifact bytes stays a hard error.
	tau_manifest_same_artifact("${entry}/manifest.json"
		"${staging}/manifest.json" _same_artifact)
	if(NOT _same_artifact)
		message(FATAL_ERROR "tau-store: input id '${input_id}' already exists "
			"with different artifact bytes")
	endif()
	tau_manifest_verify("${entry}/manifest.json" "${entry}/prefix"
		"${input_id}")
	set(${out_prefix} "${entry}/prefix" PARENT_SCOPE)
endfunction()

# Publish a fully prepared staging entry. The staging directory must be a
# sibling of the final entry, under the same dependency directory, and its name
# must start with "<input-id>.staging".
function(tau_store_publish out_prefix prefix dep input_id staging)
	tau_store_entry(_entry "${prefix}" "${dep}" "${input_id}")
	get_filename_component(_entry "${_entry}" ABSOLUTE)
	get_filename_component(_dep_dir "${_entry}" DIRECTORY)
	if(NOT EXISTS "${staging}")
		message(FATAL_ERROR "tau-store: staging does not exist: '${staging}'")
	endif()
	get_filename_component(_staging "${staging}" ABSOLUTE)
	get_filename_component(_staging_parent "${_staging}" DIRECTORY)
	get_filename_component(_staging_name "${_staging}" NAME)
	_tau_store_reject_symlink("${_staging}" "staging")
	get_filename_component(_staging_parent_real "${_staging_parent}" REALPATH)
	get_filename_component(_dep_dir_real "${_dep_dir}" REALPATH)
	if(NOT _staging_parent_real STREQUAL "${_dep_dir_real}")
		message(FATAL_ERROR "tau-store: staging is not a sibling of the final "
			"entry: '${_staging}' is not under '${_dep_dir}'")
	endif()
	if(NOT _staging_name MATCHES "^${input_id}\\.staging")
		message(FATAL_ERROR "tau-store: staging name must start with "
			"'${input_id}.staging': '${_staging_name}'")
	endif()
	if(NOT IS_DIRECTORY "${_staging}")
		message(FATAL_ERROR
			"tau-store: staging is not a directory: '${_staging}'")
	endif()
	_tau_store_reject_symlink("${_staging}/manifest.json" "staging manifest")
	_tau_store_reject_symlink("${_staging}/prefix" "staging prefix")
	if(NOT EXISTS "${_staging}/manifest.json"
			OR NOT IS_DIRECTORY "${_staging}/prefix")
		message(FATAL_ERROR "tau-store: staging is incomplete: '${_staging}'")
	endif()
	tau_manifest_verify("${_staging}/manifest.json" "${_staging}/prefix"
		"${input_id}")
	set(_lock "${_dep_dir}/${input_id}.lock")
	file(LOCK "${_lock}" RESULT_VARIABLE _lock_rc TIMEOUT 60)
	if(NOT _lock_rc EQUAL 0)
		message(FATAL_ERROR "tau-store: could not lock '${_lock}': "
			"${_lock_rc}")
	endif()
	if(EXISTS "${_entry}")
		_tau_store_existing_prefix("${_entry}" "${input_id}" "${_staging}"
			_existing_prefix)
		file(LOCK "${_lock}" RELEASE)
		set(${out_prefix} "${_existing_prefix}" PARENT_SCOPE)
		return()
	endif()
	_tau_store_rename_no_replace("${_staging}" "${_entry}" _rename_rc)
	if(NOT _rename_rc EQUAL 0 AND NOT EXISTS "${_entry}")
		# CMake directory NO_REPLACE is unavailable on some kernels and
		# filesystems (renameat2 returns EPERM). Use a proven no-clobber move
		# instead, and never a plain rename.
		_tau_store_mv_no_clobber_supported(_mv_ok "${_dep_dir}")
		if(_mv_ok)
			find_program(_mv mv NO_CACHE)
			execute_process(COMMAND "${_mv}" --no-clobber --no-target-directory
				"${_staging}" "${_entry}"
				RESULT_VARIABLE _mv_rc OUTPUT_QUIET ERROR_QUIET)
			# GNU mv -n can exit 0 without moving, so judge the outcome
			# from the filesystem.
			if(NOT EXISTS "${_staging}" AND EXISTS "${_entry}")
				set(_rename_rc 0)
			elseif(EXISTS "${_staging}" AND EXISTS "${_entry}")
				set(_rename_rc "NO_REPLACE")
			else()
				set(_rename_rc "mv no-clobber left an unexpected state")
			endif()
		else()
			set(_rename_rc "no no-replace directory move is available")
		endif()
	endif()
	if(NOT _rename_rc EQUAL 0)
		if(EXISTS "${_entry}")
			_tau_store_existing_prefix("${_entry}" "${input_id}"
				"${_staging}" _existing_prefix)
			file(LOCK "${_lock}" RELEASE)
			set(${out_prefix} "${_existing_prefix}" PARENT_SCOPE)
			return()
		endif()
		file(LOCK "${_lock}" RELEASE)
		message(FATAL_ERROR "tau-store: rename failed: ${_rename_rc}")
	endif()
	if(NOT EXISTS "${_entry}/manifest.json"
			OR NOT IS_DIRECTORY "${_entry}/prefix")
		file(LOCK "${_lock}" RELEASE)
		message(FATAL_ERROR "tau-store: publish did not produce a complete "
			"entry at '${_entry}'")
	endif()
	file(LOCK "${_lock}" RELEASE)
	set(${out_prefix} "${_entry}/prefix" PARENT_SCOPE)
endfunction()

# Record that an entry was used, so eviction keeps entries by last use. The
# marker sits beside the manifest, never inside the prefix a manifest verifies.
function(tau_store_use entry)
	if(IS_DIRECTORY "${entry}")
		string(TIMESTAMP _now "%s" UTC)
		file(WRITE "${entry}/.last-used" "${_now}\n")
	endif()
endfunction()

# Keep the newest <keep> entries per dependency by last use. ARGN lists entry
# directories in use, which are never removed. A locked, claimed, staging, or
# incomplete entry is skipped rather than risked.
function(tau_store_evict prefix keep)
	if(NOT keep MATCHES "^[0-9]+$" OR keep LESS 1)
		return()
	endif()
	set(_in_use ${ARGN})
	get_filename_component(_prefix "${prefix}" ABSOLUTE)
	set(_store "${_prefix}/store")
	if(NOT IS_DIRECTORY "${_store}")
		return()
	endif()
	file(GLOB _deps RELATIVE "${_store}" "${_store}/*")
	foreach(_dep IN LISTS _deps)
		set(_dep_dir "${_store}/${_dep}")
		if(NOT IS_DIRECTORY "${_dep_dir}")
			continue()
		endif()
		# Only this dependency's in-use entries count toward its keep budget; a
		# sibling dependency's entries must not inflate it.
		set(_dep_in_use "")
		foreach(_u IN LISTS _in_use)
			get_filename_component(_u_dir "${_u}" DIRECTORY)
			if(_u_dir STREQUAL "${_dep_dir}")
				list(APPEND _dep_in_use "${_u}")
			endif()
		endforeach()
		file(GLOB _entries RELATIVE "${_dep_dir}" "${_dep_dir}/*")
		set(_ranked "")
		foreach(_id IN LISTS _entries)
			_tau_store_id_valid("${_id}" _is_id)
			if(NOT _is_id)
				continue()
			endif()
			set(_entry "${_dep_dir}/${_id}")
			if(NOT IS_DIRECTORY "${_entry}"
					OR NOT EXISTS "${_entry}/manifest.json"
					OR NOT IS_DIRECTORY "${_entry}/prefix")
				continue()
			endif()
			set(_used 0)
			if(EXISTS "${_entry}/.last-used")
				file(READ "${_entry}/.last-used" _used)
				string(STRIP "${_used}" _used)
				if(NOT _used MATCHES "^[0-9]+$")
					set(_used 0)
				endif()
			endif()
			list(APPEND _ranked "${_used}|${_id}")
		endforeach()
		if(_ranked STREQUAL "")
			continue()
		endif()
		list(SORT _ranked COMPARE NATURAL ORDER DESCENDING)
		set(_keep_list ${_dep_in_use})
		foreach(_item IN LISTS _ranked)
			string(REPLACE "|" ";" _pair "${_item}")
			list(GET _pair 1 _id)
			set(_entry "${_dep_dir}/${_id}")
			list(FIND _dep_in_use "${_entry}" _in_use_idx)
			if(NOT _in_use_idx EQUAL -1)
				continue()
			endif()
			list(LENGTH _keep_list _kept)
			if(_kept LESS keep)
				list(APPEND _keep_list "${_entry}")
				continue()
			endif()
			if(EXISTS "${_dep_dir}/${_id}.claim")
				continue()
			endif()
			file(GLOB _staging "${_dep_dir}/${_id}.staging-*")
			if(_staging)
				continue()
			endif()
			file(LOCK "${_dep_dir}/${_id}.lock" RESULT_VARIABLE _lock_rc TIMEOUT 0)
			if(NOT _lock_rc EQUAL 0)
				continue()
			endif()
			file(REMOVE_RECURSE "${_entry}")
			file(LOCK "${_dep_dir}/${_id}.lock" RELEASE)
			message(STATUS "tau-store: evicted ${_dep}/${_id}")
		endforeach()
	endforeach()
endfunction()

# Script mode. Runs only when this file is the top-level script, so an
# include() from tau-manifest (or a self-check) does not dispatch.
#
#   staging <prefix> <dep> <input id>
#     prints a unique sibling staging entry path, with prefix/ created
#   lookup <prefix> <dep> <input id>
#     prints "hit" then the prefix, or "miss"
#   publish <prefix> <dep> <input id> <staging>
#     prints the final prefix
#
# Stdout carries only the requested value. Diagnostics go to stderr.
if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	if(CMAKE_ARGC LESS 4)
		message(FATAL_ERROR "usage: cmake -P tau-store.cmake "
			"<staging|lookup|publish> ...")
	endif()
	if(CMAKE_ARGV3 STREQUAL "staging")
		if(CMAKE_ARGC LESS 7)
			message(FATAL_ERROR
				"usage: cmake -P tau-store.cmake staging <prefix> <dep> <id>")
		endif()
		tau_store_entry(_entry "${CMAKE_ARGV4}" "${CMAKE_ARGV5}"
			"${CMAKE_ARGV6}")
		get_filename_component(_dep_dir "${_entry}" DIRECTORY)
		file(MAKE_DIRECTORY "${_dep_dir}")
		string(RANDOM LENGTH 16 _token)
		set(_staging "${_dep_dir}/${CMAKE_ARGV6}.staging-${_token}")
		file(MAKE_DIRECTORY "${_staging}/prefix")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_staging}")
	elseif(CMAKE_ARGV3 STREQUAL "lookup")
		if(CMAKE_ARGC LESS 7)
			message(FATAL_ERROR
				"usage: cmake -P tau-store.cmake lookup <prefix> <dep> <id>")
		endif()
		tau_store_lookup("${CMAKE_ARGV4}" "${CMAKE_ARGV5}" "${CMAKE_ARGV6}"
			_found _prefix)
		if(_found)
			execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "hit")
			execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_prefix}")
		else()
			execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "miss")
		endif()
	elseif(CMAKE_ARGV3 STREQUAL "publish")
		if(CMAKE_ARGC LESS 8)
			message(FATAL_ERROR "usage: cmake -P tau-store.cmake publish "
				"<prefix> <dep> <id> <staging>")
		endif()
		tau_store_publish(_prefix "${CMAKE_ARGV4}" "${CMAKE_ARGV5}"
			"${CMAKE_ARGV6}" "${CMAKE_ARGV7}")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_prefix}")
	else()
		message(FATAL_ERROR "tau-store: unknown command '${CMAKE_ARGV3}'")
	endif()
	return()
endif()
