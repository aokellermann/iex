if (IEX_ENABLE_STRESS_TESTS)
    add_definitions(-DIEX_ENABLE_STRESS_TESTS)
endif ()

enable_testing()
find_package(GTest 1.8 REQUIRED)
set(GTEST_LIBRARIES GTest::GTest GTest::Main)

add_executable(unit_test
        unit_test.cc
        curl_test.cc
        system_status_test.cc
        quote_test.cc
        company_test.cc
        api_test.cc
        symbols_test.cc
        )

target_link_libraries(unit_test ${PROJECT_NAME} ${GTEST_LIBRARIES})
gtest_discover_tests(unit_test)
