# Registers the .tgf.test grammar suites as ctest tests, run through the tgf CLI.
add_test(NAME tgf_test_tgf_test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/src/format/tgf.test/tgf_test.tgf" test
	"${PROJECT_SOURCE_DIR}/tests/tgf_test.tgf.test")
add_test(NAME treemr_tgf_test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/src/format/treemr/treemr.tgf" test
	"${PROJECT_SOURCE_DIR}/src/format/treemr/treemr.tgf.test")
add_test(NAME ambig_bc_tgf_test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/tests/fixtures/ambig_bc.tgf" test
	"${PROJECT_SOURCE_DIR}/tests/ambig_bc.tgf.test")

set(CSV_TEST_DIR "${PROJECT_SOURCE_DIR}/tests/format/csv")
add_test(NAME csv_rfc4180_tgf_test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	"${CSV_TEST_DIR}/csv.tgf.test")
add_test(NAME csv_tab_tgf_test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	--productions tab,crlf "${CSV_TEST_DIR}/csv-tab.tgf.test")
add_test(NAME csv_lf_tgf_test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	--productions comma,lf "${CSV_TEST_DIR}/csv-lf.tgf.test")
add_test(NAME csv_tab_lf_tgf_test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/src/format/csv/csv.tgf" test
	--productions tab,lf "${CSV_TEST_DIR}/csv-tab-lf.tgf.test")
add_test(NAME json_tgf_test COMMAND $<TARGET_FILE:tgf>
	"${PROJECT_SOURCE_DIR}/src/format/json/json.tgf" test
	"${PROJECT_SOURCE_DIR}/tests/format/json/json.tgf.test")
