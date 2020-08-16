# Copyright 2020 Antony Kellermann

cmake_minimum_required(VERSION 3.16)

project(iex
        VERSION 0.1
        LANGUAGES CXX
        DESCRIPTION "C++17 library for querying IEX Cloud REST API."
        )

# Configure version
#configure_file(cmake/iex.h.in src/iex.h)
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

# Options
option(IEX_BUILD_LIBRARY "Build iex." ON)
option(IEX_BUILD_WARNINGS "Enable compiler warnings." OFF)
option(IEX_BUILD_TESTING "Enable unit testing." OFF)
option(IEX_BUILD_DOCUMENTATION "Generate Doxygen documentation." OFF)
option(IEX_ENABLE_STRESS_TESTS "Enable stress unit testing" OFF)

# At least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")

# Set a default build type if none was specified
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(default_build_type "Release")
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# Set default options for Debug build
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    if (NOT IEX_BUILD_WARNINGS)
        set(IEX_BUILD_WARNINGS ON)
    endif ()

    if (NOT CMAKE_EXPORT_COMPILE_COMMANDS)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    endif ()

    # Turn on export compile commands (for clang-tidy)
    if (NOT CMAKE_EXPORT_COMPILE_COMMANDS)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    endif ()
endif ()

message(STATUS "IEX_BUILD_WARNINGS: ${IEX_BUILD_WARNINGS}")
message(STATUS "IEX_BUILD_TESTING: ${IEX_BUILD_TESTING}")
message(STATUS "IEX_BUILD_DOCUMENTATION: ${IEX_BUILD_DOCUMENTATION}")
message(STATUS "IEX_ENABLE_STRESS_TESTS: ${IEX_ENABLE_STRESS_TESTS}")

include(GNUInstallDirs)

if (IEX_BUILD_LIBRARY)
    add_subdirectory(include)
    add_subdirectory(src)
endif ()

# Build GTest if unit testing enabled.
if (IEX_BUILD_TESTING)
    add_subdirectory(tests)
endif ()

if (IEX_BUILD_DOCUMENTATION)
    add_subdirectory(docs)
endif ()
