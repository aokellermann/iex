set(_link_libraries iex ${GTEST_LIBRARIES})

add_executable(init_test unit_test.cc)
target_link_libraries(init_test ${_link_libraries})

gtest_discover_tests(init_test)