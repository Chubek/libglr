find_package(Doxygen REQUIRED)
find_program(GLRPP_DOCS_MAKE_EXECUTABLE NAMES gmake make REQUIRED)

set(GLRPP_PROJECT_NAME "${PROJECT_NAME}")
set(GLRPP_PROJECT_VERSION "${PROJECT_VERSION}")
set(GLRPP_PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
set(GLRPP_DOCS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(GLRPP_DOCS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

file(STRINGS
    "${CMAKE_CURRENT_SOURCE_DIR}/booklet/Manifest.yaml"
    GLRPP_BOOKLET_LINES
    REGEX "^[ \t]*- file:[ \t]*"
)

set(GLRPP_BOOKLET_FILES)
set(GLRPP_BOOKLET_INPUT "")
foreach(GLRPP_BOOKLET_LINE IN LISTS GLRPP_BOOKLET_LINES)
    string(REGEX REPLACE "^[ \t]*- file:[ \t]*" "" GLRPP_BOOKLET_ENTRY "${GLRPP_BOOKLET_LINE}")
    set(GLRPP_BOOKLET_PATH "${CMAKE_CURRENT_SOURCE_DIR}/booklet/${GLRPP_BOOKLET_ENTRY}")
    if(EXISTS "${GLRPP_BOOKLET_PATH}")
        list(APPEND GLRPP_BOOKLET_FILES "${GLRPP_BOOKLET_PATH}")
        string(APPEND GLRPP_BOOKLET_INPUT "INPUT += \"${GLRPP_BOOKLET_PATH}\"\n")
    else()
        message(STATUS "Skipping missing booklet chapter: ${GLRPP_BOOKLET_ENTRY}")
    endif()
endforeach()

file(GLOB GLRPP_ALL_BOOKLET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/booklet/*.md")
foreach(GLRPP_BOOKLET_PATH IN LISTS GLRPP_ALL_BOOKLET_FILES)
    list(FIND GLRPP_BOOKLET_FILES "${GLRPP_BOOKLET_PATH}" GLRPP_BOOKLET_INDEX)
    if(GLRPP_BOOKLET_INDEX EQUAL -1)
        list(APPEND GLRPP_BOOKLET_FILES "${GLRPP_BOOKLET_PATH}")
        string(APPEND GLRPP_BOOKLET_INPUT "INPUT += \"${GLRPP_BOOKLET_PATH}\"\n")
    endif()
endforeach()

file(GLOB_RECURSE GLRPP_API_HEADERS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/glrpp/*.hpp"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    @ONLY
)

set(GLRPP_DOCS_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-darkmode.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/ltx-header.tex"
    "${CMAKE_CURRENT_SOURCE_DIR}/ltx-footer.tex"
    "${CMAKE_CURRENT_SOURCE_DIR}/booklet/Manifest.yaml"
    ${GLRPP_BOOKLET_FILES}
    ${GLRPP_API_HEADERS}
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/doxygen.stamp"
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_CURRENT_BINARY_DIR}/doxygen.stamp"
    DEPENDS ${GLRPP_DOCS_DEPENDS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Generating GLR++ API documentation"
    VERBATIM
)

add_custom_target(html DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/doxygen.stamp")
add_custom_target(latex DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/doxygen.stamp")
add_custom_target(docs DEPENDS html)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/glrpp-reference.pdf"
    COMMAND "${GLRPP_DOCS_MAKE_EXECUTABLE}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/latex/refman.pdf"
            "${CMAKE_CURRENT_BINARY_DIR}/glrpp-reference.pdf"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/doxygen.stamp"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/latex"
    COMMENT "Compiling GLR++ PDF manual"
    VERBATIM
)

add_custom_target(pdf DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/glrpp-reference.pdf")
