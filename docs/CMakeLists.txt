cmake_minimum_required(VERSION 3.16)

# Doxygen configuration
find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
    set(DOXYGEN_PROJECT_NAME "${PROJECT_NAME}")
    set(DOXYGEN_PROJECT_NUMBER "${PROJECT_VERSION}")
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_HTML_OUTPUT html)
    set(DOXYGEN_GENERATE_MAN YES)
    set(DOXYGEN_MAN_OUTPUT man)
    set(DOXYGEN_MAN_EXTENSION .3tb)
    set(DOXYGEN_MAN_LINK_TO_FIRST NO)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE NO)
    set(DOXYGEN_EXTRACT_STATIC NO)
    set(DOXYGEN_SHOW_INCLUDE_FILES YES)
    set(DOXYGEN_FULL_PATH_NAMES YES)
    set(DOXYGEN_CREATE_SUBDIRS NO)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_INLINE_INLINED_DECLARATIONS YES)
    set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)
    set(DOXYGEN_STRIP_FROM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
    set(DOXYGEN_EXCLUDE_PATTERNS
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.h"
    )
    set(DOXYGEN_PREDEFINED "")
    set(DOXYGEN_WARN_FORMAT "$file:$line: $text")
    set(DOXYGEN_WARN_LOG_FILE "${CMAKE_CURRENT_BINARY_DIR}/doxygen_warnings.log")
    
    doxygen_add_docs(docs
        ALL
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../disambstd
        ${CMAKE_CURRENT_SOURCE_DIR}/../README.md
        ${CMAKE_CURRENT_SOURCE_DIR}/rewrite-language.dox
        ${CMAKE_CURRENT_SOURCE_DIR}/procedural-rewrite-api.dox
        COMMENT "Generating API documentation with Doxygen"
    )
else()
    message(STATUS "Doxygen not found; documentation targets are disabled")
endif()

install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/docs/html/
    DESTINATION share/doc/${PROJECT_NAME}
    OPTIONAL
)

install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/docs/man/
    DESTINATION share/man
    OPTIONAL
)
