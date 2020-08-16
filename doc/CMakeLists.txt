find_package(Doxygen)

if (DOXYGEN_FOUND)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_SOURCE_DIR}/README.md)
    doxygen_add_docs(
            doc
            ALL
            ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}
            ${CMAKE_SOURCE_DIR}/.github/CONTRIBUTING.md
            COMMENT "Generate documentation"
    )

    install(DIRECTORY html
            DESTINATION ${CMAKE_INSTALL_DOCDIR}
            COMPONENT development
            )
else ()
    message(FATAL_ERROR "Doxygen is needed to build the documentation.")
endif ()
