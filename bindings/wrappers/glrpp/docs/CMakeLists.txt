find_package(Doxygen REQUIRED)

set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# Collect all markdown sources
file(GLOB DOC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.md")

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${DOC_SOURCES} ${DOXYGEN_IN}
    COMMENT "Generating API docs with Doxygen"
    VERBATIM
)
