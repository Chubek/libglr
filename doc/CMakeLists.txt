cmake_minimum_required(VERSION 3.16)

# Doxygen configuration
find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN YES)
    set(DOXYGEN_MAN_EXTENSION .3tb)
    set(DOXYGEN_MAN_LINK_TO_FIRST NO)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE NO)
    set(DOXYGEN_EXTRACT_STATIC NO)
    set(DOXYGEN_SHOW_INCLUDE_FILES YES)
    set(DOXYGEN_FULL_PATH_NAMES YES)
    set(DOXYGEN_CREATE_SUBDIRS YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_SORT_MEMBERS_CTORS_ FIRST YES)
    set(DOXYGEN_INLINE_INLINED_DECLARATIONS YES)
    set(DOXYGEN_PREDEFINED "")
    set(DOXYGEN_WARN_FORMAT "$file:$line: $text")
    set(DOXYGEN_WARN_LOG_FILE "${CMAKE_CURRENT_BINARY_DIR}/doxygen_warnings.log")
    
    doxygen_add_docs(docs
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        COMMENT "Generating API documentation with Doxygen"
    )
endif()

# Install manpage
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../man/libglr.3tb
    DESTINATION share/man/man3
)
