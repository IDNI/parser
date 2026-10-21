# Registers the .tgf.test grammar suites as ctest tests, run through the tgf CLI.
add_test(NAME tgf.test.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/tgf.test/tgf.test.tgf" test
	"${PROJECT_SOURCE_DIR}/tests/tgf.test.tgf.test")
add_test(NAME treemr.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/treemr/treemr.tgf" test
	"${PROJECT_SOURCE_DIR}/src/format/treemr/treemr.tgf.test")
add_test(NAME ambig_bc.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/tests/fixtures/ambig_bc.tgf" test
	"${PROJECT_SOURCE_DIR}/tests/ambig_bc.tgf.test")
# The same suite with --json: a failed item or summary fails the test.
add_test(NAME tgf_test_json COMMAND tgf
	"${PROJECT_SOURCE_DIR}/tests/fixtures/ambig_bc.tgf" test --json
	"${PROJECT_SOURCE_DIR}/tests/ambig_bc.tgf.test")
set_tests_properties(tgf_test_json PROPERTIES
	PASS_REGULAR_EXPRESSION
		"{\"test_summary\":{\"files\":1,\"passed\":[1-9][0-9]*,\"failed\":0,\"ok\":true}}"
	FAIL_REGULAR_EXPRESSION "\"passed\":false")

set(CSV_TEST_DIR "${PROJECT_SOURCE_DIR}/tests/format/csv")
add_test(NAME csv_rfc4180.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	"${CSV_TEST_DIR}/csv.tgf.test")
add_test(NAME csv_tab.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	--variant tab "${CSV_TEST_DIR}/csv-tab.tgf.test")
add_test(NAME csv_lf.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	--variant lf "${CSV_TEST_DIR}/csv-lf.tgf.test")
add_test(NAME csv_tab_lf.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	--variant tab_lf "${CSV_TEST_DIR}/csv-tab-lf.tgf.test")
add_test(NAME csv_header.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	--variant header "${CSV_TEST_DIR}/csv-header.tgf.test")
add_test(NAME json.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/json/json.tgf" test
	"${PROJECT_SOURCE_DIR}/tests/format/json/json.tgf.test")
add_test(NAME json_nonfinite.tgf.test COMMAND tgf
	"${PROJECT_SOURCE_DIR}/src/format/json/json.tgf" test
	--variant nonfinite "${PROJECT_SOURCE_DIR}/tests/format/json/json-nonfinite.tgf.test")
