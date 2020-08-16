if (IEX_ENABLE_STRESS_TESTS)
    add_definitions(-DIEX_ENABLE_STRESS_TESTS)
endif ()

include(GoogleTest)
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.10.x
)
FetchContent_GetProperties(googletest)
if (NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif ()

add_executable(unit_test
        unit_test.cc
        curl_test.cc
        system_status_test.cc
        quote_test.cc
        company_test.cc
        api_test.cc
        symbols_test.cc
        )
target_link_libraries(unit_test ${PROJECT_NAME} gtest gtest_main)
gtest_discover_tests(unit_test)