add_executable(test_parser test_parser.cpp)
target_setup(test_parser)
target_link_libraries(test_parser ${OBJECT_LIB_NAME} ${TEST_FRAMEWORK})
