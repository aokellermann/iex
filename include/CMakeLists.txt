set(iex_header iex/iex.h)
set(iex_detail_paths iex/api iex/detail)

# Install headers
set(iex_include_install_destination ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(FILES ${iex_header}
        DESTINATION ${iex_include_install_destination}
        COMPONENT development
        )
install(DIRECTORY ${iex_detail_paths}
        DESTINATION ${iex_include_install_destination}
        COMPONENT development
        )
