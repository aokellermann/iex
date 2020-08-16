set(iex_header
        iex/iex.h
        )
set(iex_detail_path iex/detail)
set(iex_api_path iex/api)

# Install headers
install(FILES ${iex_header}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        COMPONENT development
        )
install(DIRECTORY ${iex_detail_path} ${iex_api_path}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        COMPONENT development
        )
