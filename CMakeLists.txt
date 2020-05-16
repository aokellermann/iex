cmake_minimum_required(VERSION 3.16)
project(iex
        VERSION 0.0.1
        LANGUAGES CXX)
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

option(IEX_BUILD_TESTING "Enable C++ unit testing." OFF)
include(GNUInstallDirs) # Define CMAKE_INSTALL_xxx: LIBDIR, INCLUDEDIR
set(iex_export_file "${PROJECT_BINARY_DIR}/iexTargets.cmake")

set(default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
            STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    message(STATUS "${PROJECT_NAME} using CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
endif ()

# Set extra compile options
set(_iex_extra_compile_options_docstring "Add extra compile options to the CXX targets")
if (MSVC)
    set(IEX_EXTRA_COMPILE_OPTIONS /EHsc CACHE STRING ${_iex_extra_compile_options_docstring})
else ()
    option(IEX_ENABLE_ALLWARNINGS "Add compile options to the C++ targets. The warnings are set in
    IEX_EXTRA_COMPILE_OPTIONS" OFF)
    if (IEX_ENABLE_ALLWARNINGS)
        set(IEX_EXTRA_COMPILE_OPTIONS
                -Wall -Wextra -Werror -pedantic -Wcast-align -Wcast-qual
                -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2
                -Winit-self -Wmissing-include-dirs -Wold-style-cast
                -Woverloaded-virtual -Wredundant-decls -Wshadow
                -Wsign-promo -Wswitch-default -Wundef -Wno-unused
                CACHE STRING ${_iex_extra_compile_options_docstring})
    endif ()
endif ()
message(STATUS "IEX_BUILD_TESTING: ${IEX_BUILD_TESTING}")
message(STATUS "IEX_EXTRA_COMPILE_OPTIONS: ${IEX_EXTRA_COMPILE_OPTIONS}")

set(iex_headers
        iex.h
        )
set(iex_sources
        iex.cc
        )
list(TRANSFORM iex_headers PREPEND "iex/")
list(TRANSFORM iex_sources PREPEND "iex/")

add_library(iex SHARED ${iex_sources})
target_include_directories(iex PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )
target_compile_options(iex PRIVATE ${IEX_EXTRA_COMPILE_OPTIONS})

set(IEX_LIBRARIES iex)

find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

target_include_directories(iex PUBLIC ${CURL_INCLUDE_DIR})
target_link_libraries(iex ${CURL_LIBRARIES} nlohmann_json::nlohmann_json)

install(TARGETS ${IEX_LIBRARIES}
        EXPORT iexTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT development
        )

install(FILES ${iex_headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iex
        COMPONENT development
        )

# export to the build tree
export(TARGETS ${IEX_LIBRARIES}
        NAMESPACE iex::
        APPEND FILE ${iex_export_file}
        )

if (IEX_BUILD_TESTING)
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

    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW) # google test raises warning about it
    FetchContent_GetProperties(googletest)
    if (NOT googletest_POPULATED)
        FetchContent_Populate(googletest)
        add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
    endif ()
    #############################################################################

    add_subdirectory(iex/test)
endif ()

# INSTALL
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