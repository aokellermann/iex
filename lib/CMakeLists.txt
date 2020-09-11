# Find libraries
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
set(IEX_LINK_LIBRARIES CURL::libcurl nlohmann_json::nlohmann_json)

set(iex_sources
        api.cc
        curl_wrapper.cc
        )

# Add iex library
add_library(${IEX_LIBRARY} ${iex_sources})
target_link_libraries(${IEX_LIBRARY} ${IEX_LINK_LIBRARIES})

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
target_include_directories(${IEX_LIBRARY} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

# Install shared object
set(iex_targets_file "${PROJECT_NAME}Targets")
install(TARGETS ${IEX_LIBRARY}
        EXPORT ${IEX_LIBRARY}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(project_config_in "${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in")
set(project_config_out "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(config_targets_file "${PROJECT_NAME}ConfigTargets.cmake")
set(version_config_file "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

install(EXPORT ${IEX_LIBRARY}
        DESTINATION ${export_dest_dir}
        NAMESPACE ${PROJECT_NAME}::
        FILE ${config_targets_file}
        )

include(CMakePackageConfigHelpers)
configure_file("${project_config_in}" "${project_config_out}" @ONLY)

write_basic_package_version_file("${version_config_file}" COMPATIBILITY SameMajorVersion)
install(FILES
        "${project_config_out}"
        "${version_config_file}"
        DESTINATION "${export_dest_dir}")
