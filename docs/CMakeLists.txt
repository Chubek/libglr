cmake_minimum_required(VERSION 3.16)

find_package(Doxygen QUIET)

set(LIBGLR_DOC_MARKDOWN
    ${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md
    ${CMAKE_CURRENT_SOURCE_DIR}/theory.md
    ${CMAKE_CURRENT_SOURCE_DIR}/architecture.md
    ${CMAKE_CURRENT_SOURCE_DIR}/disambiguation.md
    ${CMAKE_CURRENT_SOURCE_DIR}/rewriting.md
    ${CMAKE_CURRENT_SOURCE_DIR}/incremental_parsing.md
    ${CMAKE_CURRENT_SOURCE_DIR}/syntax_highlighting.md
)

if(DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
            ${CMAKE_CURRENT_SOURCE_DIR}/stylesheet.css
            ${CMAKE_CURRENT_SOURCE_DIR}/header.html
            ${CMAKE_CURRENT_SOURCE_DIR}/footer.html
            ${CMAKE_CURRENT_SOURCE_DIR}/header.tex
            ${CMAKE_CURRENT_SOURCE_DIR}/footer.tex
            ${LIBGLR_DOC_MARKDOWN}
        COMMENT "Generating libglr API and guide documentation"
        VERBATIM
    )

    add_custom_target(docs DEPENDS doc)
else()
    add_custom_target(doc
        COMMAND ${CMAKE_COMMAND} -E echo "Doxygen not found; documentation cannot be generated"
        VERBATIM
    )
    add_custom_target(docs DEPENDS doc)
endif()

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/build/html/
    DESTINATION share/doc/${PROJECT_NAME}/html
    OPTIONAL
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/build/man/
    DESTINATION share/man
    OPTIONAL
)
