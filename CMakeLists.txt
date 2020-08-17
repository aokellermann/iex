# Copyright 2020 Antony Kellermann

cmake_minimum_required(VERSION 3.16)

project(iex
        VERSION 0.1
        LANGUAGES CXX
        DESCRIPTION "C++17 library for querying IEX Cloud REST API."
        )
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

# Options
option(IEX_BUILD_LIBRARY "Build iex library." ON)
option(IEX_BUILD_WARNINGS "Build with compiler warnings." OFF)
option(IEX_BUILD_TESTING "Build unit tests." OFF)
option(IEX_BUILD_DOCUMENTATION "Build Doxygen documentation." OFF)
option(IEX_BUILD_STRESS_TESTS "Build stress unit tests." OFF)
option(IEX_TIDY "Run static analyzer." OFF)
option(BUILD_SHARED_LIBS "Build shared library instead of static." ON)

if (IEX_BUILD_TESTING AND NOT IEX_BUILD_LIBRARY)
    set(IEX_BUILD_LIBRARY ON)
endif ()

# At least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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

# clang-tidy
if (IEX_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()

include(GNUInstallDirs)

if (IEX_BUILD_LIBRARY)
    set(IEX_LIBRARY ${PROJECT_NAME})
    add_subdirectory(include)
    add_subdirectory(lib)
endif ()

if (IEX_BUILD_TESTING)
    add_subdirectory(tests)
endif ()

if (IEX_BUILD_DOCUMENTATION)
    add_subdirectory(doc)
endif ()
