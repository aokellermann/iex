cmake_minimum_required(VERSION 3.16)
project(iex
        VERSION 0.0.1
        LANGUAGES CXX
        DESCRIPTION "C++17 library for querying IEX Cloud REST API."
        )
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

# Options related to weed whacking, testing, and documentation.
option(IEX_ENABLE_ALLWARNINGS "Add GCC/Clang compatible compile options." OFF)
option(IEX_BUILD_TESTING "Enable unit testing." OFF)
option(IEX_ENABLE_STRESS_TESTS "Enable stress unit testing" OFF)
option(IEX_BUILD_DOCUMENTATION "Generate Doxygen documentation." ON)

# Set configuration: Either Debug, Release (default), MinSizeRel, or RelWithDebInfo.
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(default_build_type "Release")
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
            STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# Set C++17 unless specified otherwise.
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    message(STATUS "${PROJECT_NAME} using CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
endif ()

# Set compiler warning flags if option is on, and using GCC/Clang.
if (IEX_ENABLE_ALLWARNINGS)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(IEX_EXTRA_COMPILE_OPTIONS
                -Wall -Wextra -Werror -pedantic -Wcast-align -Wcast-qual
                -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2
                -Winit-self -Wmissing-include-dirs -Wold-style-cast
                -Woverloaded-virtual -Wredundant-decls -Wshadow
                -Wsign-promo -Wswitch-default -Wundef -Wno-unused
                CACHE STRING "Add extra compile options to the CXX targets")
    endif ()
endif ()

message(STATUS "IEX_EXTRA_COMPILE_OPTIONS: ${IEX_EXTRA_COMPILE_OPTIONS}")
message(STATUS "IEX_BUILD_TESTING: ${IEX_BUILD_TESTING}")

# Set library headers and sources.
set(iex_headers
        iex.h
        detail/common.h
        detail/utils.h
        detail/curl_wrapper.h
        detail/singleton.h
        api/system_status.h
        api/quote.h
        api/company.h
        api/symbols.h
        )
set(iex_sources
        iex.cc
        detail/curl_wrapper.cc
        )

# Specify source directory.
list(TRANSFORM iex_headers PREPEND "iex/")
list(TRANSFORM iex_sources PREPEND "iex/")

# Create shared library.
add_library(iex SHARED ${iex_sources})
include(GNUInstallDirs) # Define LIBDIR, INCLUDEDIR, DOCDIR
target_include_directories(iex PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )
target_compile_options(iex PRIVATE ${IEX_EXTRA_COMPILE_OPTIONS})

set(IEX_LIBRARIES iex)

# Find and link dependencies.
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Doxygen)
target_link_libraries(iex CURL::libcurl nlohmann_json::nlohmann_json)

# Install shared object.
install(TARGETS ${IEX_LIBRARIES}
        EXPORT iexTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT development
        )

# Install public header(s).
install(FILES ${iex_headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iex
        COMPONENT development
        )

# Export to the build tree.
set(iex_export_file "${PROJECT_BINARY_DIR}/iexTargets.cmake")
export(TARGETS ${IEX_LIBRARIES}
        NAMESPACE iex::
        APPEND FILE ${iex_export_file}
        )

# Build GTest if unit testing enabled.
if (IEX_BUILD_TESTING)
    if (IEX_ENABLE_STRESS_TESTS)
        add_definitions(-DIEX_ENABLE_STRESS_TESTS)
    endif ()

    enable_testing()
    set(INSTALL_GTEST OFF)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # Propagate INSTALL_GTEST=OFF to subproject
    set(GTEST_LIBRARIES gtest gtest_main)
    include(GoogleTest)
    #############################################################################
    # Fetch GTest
    include(FetchContent)

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.8.x
    )

    FetchContent_GetProperties(googletest)
    if (NOT googletest_POPULATED)
        FetchContent_Populate(googletest)
        add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
    endif ()

    add_subdirectory(iex/test)
endif ()

if (IEX_BUILD_DOCUMENTATION)
    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif ()

    # Don't try to generate docs for testing.
    set(DOXYGEN_RECURSIVE NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    doxygen_add_docs(
            doc
            ALL
            ${CMAKE_CURRENT_SOURCE_DIR}/iex
            ${CMAKE_CURRENT_SOURCE_DIR}/README.md
            ${CMAKE_CURRENT_SOURCE_DIR}/.github/CONTRIBUTING.md
            COMMENT "Generate documentation"
    )

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
            DESTINATION ${CMAKE_INSTALL_DOCDIR}
            COMPONENT development
            )
endif ()

# Install
set(install_cmake_dir "${CMAKE_INSTALL_LIBDIR}/cmake/iex")

install(EXPORT iexTargets
        NAMESPACE iex::
        DESTINATION ${install_cmake_dir}
        )

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/iexConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/iexConfigVersion.cmake
        DESTINATION ${install_cmake_dir}
        )

include(CMakePackageConfigHelpers)

write_basic_package_version_file(iexConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        )

# Build tree
set(iex_TARGETS_FILE ${iex_export_file})
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/iexConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/iexConfig.cmake
        INSTALL_DESTINATION ${install_cmake_dir}
        PATH_VARS iex_TARGETS_FILE
        NO_CHECK_REQUIRED_COMPONENTS_MACRO # iex does not provide components
        INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}
)

# Install tree
set(iex_TARGETS_FILE ${CMAKE_INSTALL_PREFIX}/${install_cmake_dir}/iexTargets.cmake)
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/iexConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/iexConfig.cmake
        INSTALL_DESTINATION ${install_cmake_dir}
        PATH_VARS iex_TARGETS_FILE
        NO_CHECK_REQUIRED_COMPONENTS_MACRO # iex does not provide components
)

# Add custom target to only install component: runtime (libraries)
add_custom_target(iex-install-runtime
        ${CMAKE_COMMAND}
        -DCMAKE_INSTALL_COMPONENT=runtime
        -P "${PROJECT_BINARY_DIR}/cmake_install.cmake"
        DEPENDS ${IEX_LIBRARIES}
        )
message(STATUS "IEX_LIBRARIES: ${IEX_LIBRARIES}")
add_dependencies(iex-install-runtime ${IEX_LIBRARIES})
