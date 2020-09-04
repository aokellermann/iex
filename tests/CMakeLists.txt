find_package(GTest 1.8 REQUIRED)

if (IEX_BUILD_STRESS_TESTS)
    add_compile_definitions(IEX_BUILD_STRESS_TESTS)
endif ()

enable_testing()
set(GTEST_LIBRARIES GTest::GTest GTest::Main)

add_executable(unit_test
        system_status_test.cc
        symbols_test.cc
        quote_test.cc
        curl_test.cc
        company_test.cc
        api_test.cc
        unit_test.cc
        )

target_link_libraries(unit_test ${IEX_LIBRARY} ${GTEST_LIBRARIES})
gtest_discover_tests(unit_test)
