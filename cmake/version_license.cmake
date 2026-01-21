# Function version_license(template_file, output_file)
# takes a template file and replaces the following placeholders:
#   @VERSION@          with content of VERSION file
#   @LICENSE_CONTENT@  with content of LICENSE.md file
#   @BUILD_DATE_ISO@   with current date in ISO format
# and writes the result to the output file
# TODO generic BOM removal
function(version_license template_file output_file)
	# @VERSION@
	set(version_file "${PROJECT_SOURCE_DIR}/VERSION")
	file(READ ${version_file} VERSION ENCODING UTF-8)
	string(STRIP "${VERSION}" VERSION)
	string(REPLACE "\"" "\\\"" VERSION "${VERSION}")

	# @LICENSE_CONTENT@
	set(license_file "${PROJECT_SOURCE_DIR}/LICENSE.md")
	if(NOT EXISTS ${license_file})
		set(license_file "${PROJECT_SOURCE_DIR}/LICENSE.txt")
	endif()
	file(READ ${license_file} LICENSE_CONTENT ENCODING UTF-8)
	# remove BOM if any. Expects word License to be the start of the text
	string(FIND "${LICENSE_CONTENT}" "License" LICENSE_CONTENT_START)
	string(SUBSTRING "${LICENSE_CONTENT}" ${LICENSE_CONTENT_START} -1 LICENSE_CONTENT)
	string(REPLACE "\"" "\\\"" LICENSE_CONTENT "${LICENSE_CONTENT}")

	# @BUILD_DATE_ISO@
	execute_process(COMMAND date -I
			OUTPUT_VARIABLE BUILD_DATE_ISO
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	string(REPLACE "\"" "\\\"" BUILD_DATE_ISO "${BUILD_DATE_ISO}")

	configure_file(${template_file} ${output_file} @ONLY)
endfunction()
