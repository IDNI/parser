# tests/tau-store-self-check.cmake
#
# Hermetic self-check for cmake/tau-store.cmake. Run as:
#
#   cmake -P tau-store-self-check.cmake <store-module> [<temp-dir>]
#
# Exits zero and prints nothing on success. Any mismatch is a FATAL_ERROR.
# Fatal paths that would abort this process run through child `cmake -P` calls
# so their nonzero exit status can be asserted.
#
# Every path is under a random scratch namespace below the temp dir. The real
# ~/.tau is never touched.

if(CMAKE_ARGC LESS 4)
	message(FATAL_ERROR
		"usage: cmake -P tau-store-self-check.cmake <store-module> [<temp-dir>]")
endif()
set(_module "${CMAKE_ARGV3}")
if(NOT EXISTS "${_module}")
	message(FATAL_ERROR "tau-store module not found: ${_module}")
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

macro(tau_fail msg)
	file(REMOVE_RECURSE "${_root}")
	message(FATAL_ERROR "tau-store self-check: ${msg}")
endmacro()

function(tau_expect_fatal root what)
	execute_process(COMMAND ${ARGN}
		RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
	if(_rc EQUAL 0)
		file(REMOVE_RECURSE "${root}")
		message(FATAL_ERROR "tau-store self-check: ${what} was accepted")
	endif()
endfunction()

string(RANDOM LENGTH 16 _token)
set(_root "${_tmp}/tau-store-${_token}")
set(_prefix "${_root}/shared")
tau_input_id(_id "dep=demo\nsource=abc")
set(_dep_dir "${_prefix}/store/demo")

# a fresh store is a miss
tau_store_lookup("${_prefix}" "demo" "${_id}" _found _pfx)
if(_found)
	tau_fail("lookup on a fresh store was a hit")
endif()

# layout helpers
tau_store_entry(_entry "${_prefix}" "demo" "${_id}")
tau_store_prefix(_layout "${_prefix}" "demo" "${_id}")
if(NOT _layout STREQUAL "${_entry}/prefix")
	tau_fail("tau_store_prefix does not match the entry layout")
endif()
if(NOT _entry STREQUAL "${_dep_dir}/${_id}")
	tau_fail("tau_store_entry does not match the store layout")
endif()

# build a staging entry and publish it
set(_staging "${_dep_dir}/${_id}.staging-${_token}")
file(MAKE_DIRECTORY "${_staging}/prefix/lib")
file(WRITE "${_staging}/prefix/liba.so" "contents A")
file(WRITE "${_staging}/prefix/lib/b.so" "contents B")
tau_output_map(_map "${_staging}/prefix")
tau_manifest_write("${_staging}/manifest.json" "${_id}" "dep=demo\nsource=abc"
	"${_map}")
tau_store_publish(_published "${_prefix}" "demo" "${_id}" "${_staging}")
if(NOT _published STREQUAL "${_entry}/prefix")
	tau_fail("publish did not return the entry prefix")
endif()
if(EXISTS "${_staging}")
	tau_fail("publish did not move the staging entry")
endif()
if(NOT EXISTS "${_entry}/manifest.json" OR NOT IS_DIRECTORY "${_entry}/prefix")
	tau_fail("publish did not produce a complete final entry")
endif()

# a hit returns the prefix and exposes the package
tau_store_lookup("${_prefix}" "demo" "${_id}" _found2 _pfx2)
if(NOT _found2)
	tau_fail("lookup after publish was a miss")
endif()
if(NOT _pfx2 STREQUAL "${_entry}/prefix")
	tau_fail("hit prefix mismatch")
endif()
file(READ "${_pfx2}/liba.so" _content)
if(NOT _content STREQUAL "contents A")
	tau_fail("hit prefix does not expose the package")
endif()

# an identical second publish is accepted and does not replace the entry
set(_staging2 "${_dep_dir}/${_id}.staging-${_token}-same")
file(MAKE_DIRECTORY "${_staging2}/prefix/lib")
file(WRITE "${_staging2}/prefix/liba.so" "contents A")
file(WRITE "${_staging2}/prefix/lib/b.so" "contents B")
tau_output_map(_map2 "${_staging2}/prefix")
tau_manifest_write("${_staging2}/manifest.json" "${_id}"
	"source=abc\ndep=demo" "${_map2}")
tau_store_publish(_published2 "${_prefix}" "demo" "${_id}" "${_staging2}")
if(NOT _published2 STREQUAL "${_entry}/prefix")
	tau_fail("an identical publish returned a different prefix")
endif()

# a provenance-only difference is the same artifact and is accepted
set(_staging2b "${_dep_dir}/${_id}.staging-${_token}-prov")
file(MAKE_DIRECTORY "${_staging2b}/prefix/lib")
file(WRITE "${_staging2b}/prefix/liba.so" "contents A")
file(WRITE "${_staging2b}/prefix/lib/b.so" "contents B")
tau_output_map(_map2b "${_staging2b}/prefix")
tau_manifest_write("${_staging2b}/manifest.json" "${_id}"
	"source=abc\ndep=demo\nprovenance.manifest_writer_hash=changed" "${_map2b}")
tau_store_publish(_published2b "${_prefix}" "demo" "${_id}" "${_staging2b}")
if(NOT _published2b STREQUAL "${_entry}/prefix")
	tau_fail("a provenance-only publish returned a different prefix")
endif()

# relocation: a copy of the store under a new prefix is a hit
set(_moved "${_root}/moved")
file(COPY "${_prefix}/" DESTINATION "${_moved}")
tau_store_lookup("${_moved}" "demo" "${_id}" _found_moved _pfx_moved)
if(NOT _found_moved)
	tau_fail("lookup after relocation was a miss")
endif()
if(NOT _pfx_moved STREQUAL "${_moved}/store/demo/${_id}/prefix")
	tau_fail("relocated prefix mismatch")
endif()

# child dispatcher for the fatal paths
set(_child "${_root}/tau-store-child.cmake")
file(WRITE "${_child}"
	"include(\"${_module}\")\n"
	"if(CMAKE_ARGV3 STREQUAL \"lookup\")\n"
	"\ttau_store_lookup(\"\${CMAKE_ARGV4}\" \"\${CMAKE_ARGV5}\" "
		"\"\${CMAKE_ARGV6}\" _f _p)\n"
	"elseif(CMAKE_ARGV3 STREQUAL \"publish\")\n"
	"\ttau_store_publish(_p \"\${CMAKE_ARGV4}\" \"\${CMAKE_ARGV5}\" "
		"\"\${CMAKE_ARGV6}\" \"\${CMAKE_ARGV7}\")\n"
	"else()\n"
	"\tmessage(FATAL_ERROR \"unknown store child command\")\n"
	"endif()\n")

# a corrupt output in a final entry is a hard error, not a miss
file(WRITE "${_entry}/prefix/liba.so" "corrupted")
tau_expect_fatal("${_root}" "a corrupt output"
	"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo" "${_id}")
file(WRITE "${_entry}/prefix/liba.so" "contents A")

# a partial entry is a hard error, not a miss
tau_input_id(_partial_id "dep=demo\nsource=partial")
set(_partial "${_dep_dir}/${_partial_id}")
file(MAKE_DIRECTORY "${_partial}/prefix")
tau_expect_fatal("${_root}" "a partial entry"
	"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo"
	"${_partial_id}")

# a malformed manifest is a hard error
file(WRITE "${_partial}/manifest.json" "not json")
tau_expect_fatal("${_root}" "a malformed manifest"
	"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo"
	"${_partial_id}")

# a second publish under the same id with different bytes is a hard error
set(_staging3 "${_dep_dir}/${_id}.staging-${_token}-conflict")
file(MAKE_DIRECTORY "${_staging3}/prefix")
file(WRITE "${_staging3}/prefix/liba.so" "different A")
tau_output_map(_map3 "${_staging3}/prefix")
tau_manifest_write("${_staging3}/manifest.json" "${_id}" "dep=demo\nsource=abc"
	"${_map3}")
tau_expect_fatal("${_root}" "a conflicting publish"
	"${CMAKE_COMMAND}" -P "${_child}" publish "${_prefix}" "demo" "${_id}"
	"${_staging3}")
# the final entry is unchanged and still complete
tau_store_lookup("${_prefix}" "demo" "${_id}" _found3 _pfx3)
if(NOT _found3)
	tau_fail("the final entry disappeared after a conflicting publish")
endif()
file(READ "${_entry}/prefix/liba.so" _still)
if(NOT _still STREQUAL "contents A")
	tau_fail("a conflicting publish changed the final entry")
endif()

# a tampered staging is rejected before publication
set(_staging4 "${_dep_dir}/${_id}.staging-${_token}-tampered")
file(MAKE_DIRECTORY "${_staging4}/prefix")
file(WRITE "${_staging4}/prefix/liba.so" "value")
tau_output_map(_map4 "${_staging4}/prefix")
tau_manifest_write("${_staging4}/manifest.json" "${_id}" "dep=demo\nsource=abc"
	"${_map4}")
file(WRITE "${_staging4}/prefix/liba.so" "tampered")
tau_expect_fatal("${_root}" "a tampered staging"
	"${CMAKE_COMMAND}" -P "${_child}" publish "${_prefix}" "demo" "${_id}"
	"${_staging4}")
if(NOT EXISTS "${_entry}/manifest.json" OR NOT IS_DIRECTORY "${_entry}/prefix")
	tau_fail("a failed publish left no complete final entry")
endif()
file(READ "${_entry}/prefix/liba.so" _after_failed)
if(NOT _after_failed STREQUAL "contents A")
	tau_fail("a failed publish altered the final entry")
endif()

# unsafe inputs are rejected
tau_expect_fatal("${_root}" "a relative dep with a separator"
	"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "../evil" "${_id}")
tau_expect_fatal("${_root}" "a short input id"
	"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo" "abc")
tau_expect_fatal("${_root}" "a non-hex input id"
	"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo"
	"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz")
file(MAKE_DIRECTORY "${_root}/outside/prefix")
tau_expect_fatal("${_root}" "a staging outside the dependency directory"
	"${CMAKE_COMMAND}" -P "${_child}" publish "${_prefix}" "demo" "${_id}"
	"${_root}/outside")
set(_wrong_name "${_dep_dir}/wrong-name")
file(MAKE_DIRECTORY "${_wrong_name}/prefix")
tau_expect_fatal("${_root}" "a staging with the wrong name"
	"${CMAKE_COMMAND}" -P "${_child}" publish "${_prefix}" "demo" "${_id}"
	"${_wrong_name}")
tau_expect_fatal("${_root}" "an incomplete staging"
	"${CMAKE_COMMAND}" -P "${_child}" publish "${_prefix}" "demo" "${_id}"
	"${_dep_dir}/${_id}.staging-${_token}-missing")

# a rename never replaces an existing destination
set(_dr_src "${_dep_dir}/${_id}.staging-${_token}-dirnr")
set(_dr_dst "${_dep_dir}/dirnr-probe")
file(MAKE_DIRECTORY "${_dr_src}/prefix")
file(WRITE "${_dr_src}/prefix/x" "source")
file(MAKE_DIRECTORY "${_dr_dst}")
file(WRITE "${_dr_dst}/keep" "destination")
_tau_store_rename_no_replace("${_dr_src}" "${_dr_dst}" _dr_rc)
if(_dr_rc EQUAL 0)
	tau_fail("a directory rename replaced an existing destination")
endif()
file(READ "${_dr_dst}/keep" _dr_keep)
if(NOT _dr_keep STREQUAL "destination")
	tau_fail("a directory rename changed an existing destination")
endif()
if(NOT EXISTS "${_dr_src}/prefix/x")
	tau_fail("a directory rename moved the source when the destination existed")
endif()
# a file destination reports NO_REPLACE and is not replaced
file(WRITE "${_dep_dir}/nr-src" "source")
file(WRITE "${_dep_dir}/nr-dst" "destination")
_tau_store_rename_no_replace("${_dep_dir}/nr-src" "${_dep_dir}/nr-dst" _nr_rc)
if(NOT _nr_rc STREQUAL "NO_REPLACE")
	tau_fail("a file rename over an existing target returned '${_nr_rc}'")
endif()
file(READ "${_dep_dir}/nr-dst" _nr_keep)
if(NOT _nr_keep STREQUAL "destination")
	tau_fail("a file rename replaced an existing target")
endif()

# the GNU mv no-clobber probe must prove both outcomes on Linux, and an
# independent run of the same command must agree
_tau_store_mv_no_clobber_supported(_mv_ok "${_root}")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	if(NOT _mv_ok)
		tau_fail("the mv no-clobber probe did not prove no-replace semantics")
	endif()
	string(RANDOM LENGTH 12 _mv_tok)
	find_program(_mv_prog mv)
	set(_mv_a_src "${_root}/mv-${_mv_tok}-a-src")
	set(_mv_a_dst "${_root}/mv-${_mv_tok}-a-dst")
	file(MAKE_DIRECTORY "${_mv_a_src}")
	file(WRITE "${_mv_a_src}/s" "s")
	file(MAKE_DIRECTORY "${_mv_a_dst}")
	file(WRITE "${_mv_a_dst}/keep" "k")
	execute_process(COMMAND "${_mv_prog}" --no-clobber --no-target-directory
		"${_mv_a_src}" "${_mv_a_dst}" RESULT_VARIABLE _mv_arc
		OUTPUT_QUIET ERROR_QUIET)
	if(NOT EXISTS "${_mv_a_src}/s" OR NOT EXISTS "${_mv_a_dst}/keep")
		tau_fail("mv --no-clobber --no-target-directory replaced an existing destination")
	endif()
	set(_mv_b_src "${_root}/mv-${_mv_tok}-b-src")
	set(_mv_b_dst "${_root}/mv-${_mv_tok}-b-dst")
	file(MAKE_DIRECTORY "${_mv_b_src}")
	file(WRITE "${_mv_b_src}/s" "s")
	execute_process(COMMAND "${_mv_prog}" --no-clobber --no-target-directory
		"${_mv_b_src}" "${_mv_b_dst}" RESULT_VARIABLE _mv_brc
		OUTPUT_QUIET ERROR_QUIET)
	if(EXISTS "${_mv_b_src}" OR NOT EXISTS "${_mv_b_dst}/s")
		tau_fail("mv --no-clobber --no-target-directory did not move to an absent destination")
	endif()
	file(REMOVE_RECURSE "${_mv_a_src}" "${_mv_a_dst}" "${_mv_b_src}" "${_mv_b_dst}")
endif()

# a failed publish for a fresh id leaves no final entry
tau_input_id(_miss_id "dep=demo\nsource=miss")
set(_miss_stage "${_dep_dir}/${_miss_id}.staging-${_token}-miss")
file(MAKE_DIRECTORY "${_miss_stage}/prefix")
tau_expect_fatal("${_root}" "an incomplete staging for a fresh id"
	"${CMAKE_COMMAND}" -P "${_child}" publish "${_prefix}" "demo" "${_miss_id}"
	"${_miss_stage}")
tau_store_lookup("${_prefix}" "demo" "${_miss_id}" _miss_found _miss_pfx)
if(_miss_found)
	tau_fail("a failed publish left a final entry for a fresh id")
endif()
tau_store_entry(_miss_entry "${_prefix}" "demo" "${_miss_id}")
if(EXISTS "${_miss_entry}")
	tau_fail("a failed publish created the final entry path")
endif()

if(UNIX)
	# a package with a shared-library symlink chain publishes and verifies
	tau_input_id(_chain_id "dep=demo\nsource=chain")
	set(_chain_stage "${_dep_dir}/${_chain_id}.staging-${_token}-chain")
	file(MAKE_DIRECTORY "${_chain_stage}/prefix/lib")
	file(WRITE "${_chain_stage}/prefix/lib/liby.so.2.0.0" "ylib")
	file(CREATE_LINK "liby.so.2.0.0" "${_chain_stage}/prefix/lib/liby.so.2" SYMBOLIC)
	file(CREATE_LINK "liby.so.2" "${_chain_stage}/prefix/lib/liby.so" SYMBOLIC)
	tau_output_map(_chain_map "${_chain_stage}/prefix")
	tau_manifest_write("${_chain_stage}/manifest.json" "${_chain_id}" "dep=demo\nsource=chain" "${_chain_map}")
	tau_store_publish(_chain_pub "${_prefix}" "demo" "${_chain_id}" "${_chain_stage}")
	tau_store_lookup("${_prefix}" "demo" "${_chain_id}" _chain_found _chain_pfx)
	if(NOT _chain_found)
		tau_fail("a package with a symlink chain did not publish")
	endif()
	if(NOT IS_SYMLINK "${_chain_pfx}/lib/liby.so")
		tau_fail("the published symlink chain was not preserved")
	endif()
	# a staging root that is a symlink to outside the dependency directory
	set(_outside "${_root}/outside-target")
	file(MAKE_DIRECTORY "${_outside}/prefix")
	file(WRITE "${_outside}/prefix/x" "x")
	set(_sym_stage "${_dep_dir}/${_id}.staging-${_token}-symlink")
	file(CREATE_LINK "${_outside}" "${_sym_stage}" SYMBOLIC)
	tau_expect_fatal("${_root}" "a symlinked staging root"
		"${CMAKE_COMMAND}" -P "${_child}" publish "${_prefix}" "demo"
		"${_id}" "${_sym_stage}")
	if(NOT EXISTS "${_outside}/prefix/x")
		tau_fail("a symlinked staging was moved")
	endif()

	# a final entry root that is a symlink is rejected by lookup
	tau_input_id(_sym_id "dep=demo\nkind=sym-entry")
	set(_sym_entry "${_dep_dir}/${_sym_id}")
	file(CREATE_LINK "${_outside}" "${_sym_entry}" SYMBOLIC)
	tau_expect_fatal("${_root}" "a symlinked final entry root"
		"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo"
		"${_sym_id}")

	# a dangling final entry symlink is a hard error, not a clean miss
	tau_input_id(_dangling_id "dep=demo\nkind=dangling-entry")
	set(_dangling_entry "${_dep_dir}/${_dangling_id}")
	file(CREATE_LINK "${_root}/dangling-target" "${_dangling_entry}" SYMBOLIC)
	tau_expect_fatal("${_root}" "a dangling final entry symlink"
		"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo"
		"${_dangling_id}")

	# a symlinked manifest is rejected
	tau_input_id(_msym_id "dep=demo\nkind=manifest-sym")
	set(_msym_entry "${_dep_dir}/${_msym_id}")
	file(MAKE_DIRECTORY "${_msym_entry}/prefix")
	file(WRITE "${_outside}/m.json" "{}")
	file(CREATE_LINK "${_outside}/m.json" "${_msym_entry}/manifest.json"
		SYMBOLIC)
	tau_expect_fatal("${_root}" "a symlinked manifest"
		"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo"
		"${_msym_id}")

	# a symlinked prefix root is rejected
	tau_input_id(_psym_id "dep=demo\nkind=prefix-sym")
	set(_psym_entry "${_dep_dir}/${_psym_id}")
	file(MAKE_DIRECTORY "${_psym_entry}")
	file(WRITE "${_psym_entry}/manifest.json" "{}")
	file(CREATE_LINK "${_outside}" "${_psym_entry}/prefix" SYMBOLIC)
	tau_expect_fatal("${_root}" "a symlinked prefix root"
		"${CMAKE_COMMAND}" -P "${_child}" lookup "${_prefix}" "demo"
		"${_psym_id}")

	# the configured shared prefix itself may be a symlink
	set(_link_prefix "${_root}/shared-link")
	file(CREATE_LINK "${_prefix}" "${_link_prefix}" SYMBOLIC)
	tau_store_lookup("${_link_prefix}" "demo" "${_id}" _link_found _link_pfx)
	if(NOT _link_found)
		tau_fail("lookup through a symlinked shared prefix was a miss")
	endif()
endif()

# eviction keeps the newest 3 by last use and never removes an in-use entry
set(_evict_prefix "${_root}/evict-shared")
set(_evict_dep "${_evict_prefix}/store/evict")
file(MAKE_DIRECTORY "${_evict_dep}")
set(_evict_in_use "")
foreach(_rank 1 2 3 4 5)
	tau_input_id(_eid "dep=evict\nrank=${_rank}")
	set(_eentry "${_evict_dep}/${_eid}")
	file(MAKE_DIRECTORY "${_eentry}/prefix")
	file(WRITE "${_eentry}/prefix/f" "rank ${_rank}")
	tau_output_map(_emap "${_eentry}/prefix")
	tau_manifest_write("${_eentry}/manifest.json" "${_eid}"
		"dep=evict\nrank=${_rank}" "${_emap}")
	file(WRITE "${_eentry}/.last-used" "${_rank}\n")
	if(_rank EQUAL 1)
		set(_evict_in_use "${_eentry}")
	endif()
endforeach()
# a second dependency's in-use entries must not count toward the first's budget
set(_other_dep "${_evict_prefix}/store/other")
file(MAKE_DIRECTORY "${_other_dep}")
set(_other_in_use "")
foreach(_on 1 2)
	tau_input_id(_oid "dep=other\nn=${_on}")
	set(_oentry "${_other_dep}/${_oid}")
	file(MAKE_DIRECTORY "${_oentry}/prefix")
	file(WRITE "${_oentry}/prefix/f" "other ${_on}")
	tau_output_map(_omap "${_oentry}/prefix")
	tau_manifest_write("${_oentry}/manifest.json" "${_oid}"
		"dep=other\nn=${_on}" "${_omap}")
	file(WRITE "${_oentry}/.last-used" "${_on}\n")
	list(APPEND _other_in_use "${_oentry}")
endforeach()
tau_store_evict("${_evict_prefix}" 3 "${_evict_in_use}" ${_other_in_use})
file(GLOB _evict_left RELATIVE "${_evict_dep}" "${_evict_dep}/*")
set(_evict_count 0)
foreach(_e IN LISTS _evict_left)
	_tau_store_id_valid("${_e}" _evict_is_id)
	if(IS_DIRECTORY "${_evict_dep}/${_e}" AND _evict_is_id)
		math(EXPR _evict_count "${_evict_count} + 1")
	endif()
endforeach()
if(NOT _evict_count EQUAL 3)
	tau_fail("eviction kept ${_evict_count} entries, expected 3")
endif()
if(NOT IS_DIRECTORY "${_evict_in_use}")
	tau_fail("eviction removed the in-use entry")
endif()
tau_input_id(_evict_top "dep=evict\nrank=5")
tau_input_id(_evict_next "dep=evict\nrank=4")
tau_input_id(_evict_gone "dep=evict\nrank=2")
if(NOT IS_DIRECTORY "${_evict_dep}/${_evict_top}"
		OR NOT IS_DIRECTORY "${_evict_dep}/${_evict_next}")
	tau_fail("eviction dropped an entry newer than the in-use one")
endif()
if(IS_DIRECTORY "${_evict_dep}/${_evict_gone}")
	tau_fail("eviction kept an entry beyond the newest 3")
endif()

# remove only the random scratch content this self-check created
file(REMOVE_RECURSE "${_root}")
