set(_link_libraries iex ${GTEST_LIBRARIES})

add_executable(unit_test
        unit_test.cc
        curl_test.cc
        file_test.cc
        keychain_test.cc
        system_status_test.cc
        quote_test.cc
        )
target_link_libraries(unit_test ${_link_libraries})

gtest_discover_tests(unit_test)