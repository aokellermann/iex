find_package(Doxygen REQUIRED)

set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_SOURCE_DIR}/README.md)
doxygen_add_docs(
        doc
        ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/.github/CONTRIBUTING.md
        COMMENT "Generate man pages"
)

install(DIRECTORY html
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/${PROJECT_NAME}
        COMPONENT development
        )
