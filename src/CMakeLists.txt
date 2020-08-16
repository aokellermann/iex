set(iex_sources
        api.cc
        curl_wrapper.cc
        )

add_library(${PROJECT_NAME} SHARED ${iex_sources})

# Find and link dependencies
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(${PROJECT_NAME} CURL::libcurl nlohmann_json::nlohmann_json)

# Enable compiler warnings
if (IEX_BUILD_WARNINGS)
    target_compile_options(
            ${PROJECT_NAME}
            PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -Wall
            -Wextra
            -pedantic
            -Werror>
            $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            /WX>)
endif ()

# Set include dirs
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

# Install shared object
set(iex_targets_file "${PROJECT_NAME}Targets")
install(TARGETS ${PROJECT_NAME}
        EXPORT ${iex_targets_file}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT development
        )

# Export to the build tree
set(iex_cmake_dir "${CMAKE_SOURCE_DIR}/cmake")
set(iex_export_file "${iex_targets_file}.cmake")
set(cmake_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

export(TARGETS ${PROJECT_NAME}
        NAMESPACE ${PROJECT_NAME}::
        APPEND FILE ${iex_export_file}
        )
install(EXPORT ${iex_targets_file}
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${cmake_install_dir}
        )

# Package config
include(CMakePackageConfigHelpers)

# Package version
set(iex_config_version_file ${PROJECT_NAME}ConfigVersion.cmake)
write_basic_package_version_file(${iex_config_version_file}
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${iex_config_version_file}
        DESTINATION ${cmake_install_dir}
        )

# Build tree
set(iex_config_file ${iex_cmake_dir}/${PROJECT_NAME}Config.cmake)
configure_package_config_file(
        ${iex_config_file}.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${cmake_install_dir}
        PATH_VARS iex_export_file
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
        INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}
)

# Install tree
set(iex_targets_file ${CMAKE_INSTALL_PREFIX}/${cmake_install_dir}/${iex_export_file})
configure_package_config_file(
        ${iex_config_file}.in
        ${iex_export_file}
        INSTALL_DESTINATION ${cmake_install_dir}
        PATH_VARS iex_targets_file
        NO_CHECK_REQUIRED_COMPONENTS_MACRO # iex does not provide components
)
