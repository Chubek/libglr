if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.20)
  if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 0.4)
  endif()
  project(LibGLR-docs VERSION ${PROJECT_VERSION} LANGUAGES C)
  set(LibGLR_DOCS_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
else()
  set(LibGLR_DOCS_SOURCE_ROOT "${CMAKE_SOURCE_DIR}")
endif()

include(GNUInstallDirs)

find_package(Doxygen QUIET)

set(LibGLR_DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
set(LibGLR_DOXYGEN_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
set(LibGLR_DOXYGEN_HTML_DIR "${LibGLR_DOXYGEN_OUTPUT_DIR}/html")
set(LibGLR_DOXYGEN_LATEX_DIR "${LibGLR_DOXYGEN_OUTPUT_DIR}/latex")
set(LibGLR_DOXYGEN_MAN_DIR "${LibGLR_DOXYGEN_OUTPUT_DIR}/man")
set(LibGLR_DOXYGEN_WARN_LOG "${CMAKE_CURRENT_BINARY_DIR}/doxygen-warnings.log")
set(LibGLR_DOC_STAMP "${CMAKE_CURRENT_BINARY_DIR}/doxygen.stamp")
set(LibGLR_PROJECT_VERSION "${PROJECT_VERSION}")
set(LibGLR_TOP_SRCDIR "${LibGLR_DOCS_SOURCE_ROOT}")
set(LibGLR_OUTPUT_DIRECTORY "${LibGLR_DOXYGEN_OUTPUT_DIR}")
set(LibGLR_LAYOUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/layout.xml")
set(LibGLR_STRIP_FROM_INC_PATH "${LibGLR_DOCS_SOURCE_ROOT}/include ${LibGLR_DOCS_SOURCE_ROOT}/src")
set(LibGLR_WARN_LOGFILE "${LibGLR_DOXYGEN_WARN_LOG}")

if(NOT DEFINED INSTALL_DOCS)
  set(INSTALL_DOCS OFF)
endif()

if(NOT DEFINED INSTALL_MANFILES)
  set(INSTALL_MANFILES OFF)
endif()

set(LibGLR_DOC_TARGETS)

if(DOXYGEN_FOUND)
  set(LibGLR_HAVE_DOT NO)
  if(DOXYGEN_DOT_FOUND)
    set(LibGLR_HAVE_DOT YES)
  endif()

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${LibGLR_DOXYGEN_CONFIG}
    @ONLY
  )

  add_custom_command(
    OUTPUT ${LibGLR_DOC_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LibGLR_DOXYGEN_OUTPUT_DIR}
    COMMAND ${DOXYGEN_EXECUTABLE} ${LibGLR_DOXYGEN_CONFIG}
    COMMAND ${CMAKE_COMMAND} -E touch ${LibGLR_DOC_STAMP}
    DEPENDS
      ${LibGLR_DOXYGEN_CONFIG}
      ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
      ${CMAKE_CURRENT_SOURCE_DIR}/layout.xml
      ${LibGLR_DOCS_SOURCE_ROOT}/README.md
    WORKING_DIRECTORY ${LibGLR_DOCS_SOURCE_ROOT}
    VERBATIM
  )

  add_custom_target(wirble-docs
    DEPENDS ${LibGLR_DOC_STAMP}
  )
  list(APPEND LibGLR_DOC_TARGETS wirble-docs)

  find_program(LibGLR_MAKE_PROGRAM NAMES ${CMAKE_MAKE_PROGRAM} gmake make)
  if(LibGLR_MAKE_PROGRAM)
    add_custom_command(
      OUTPUT ${LibGLR_DOXYGEN_LATEX_DIR}/refman.pdf
      COMMAND ${LibGLR_MAKE_PROGRAM} -C ${LibGLR_DOXYGEN_LATEX_DIR} pdf
      DEPENDS ${LibGLR_DOC_STAMP}
      WORKING_DIRECTORY ${LibGLR_DOXYGEN_LATEX_DIR}
      VERBATIM
    )
    add_custom_target(wirble-docs-pdf DEPENDS ${LibGLR_DOXYGEN_LATEX_DIR}/refman.pdf)
    add_dependencies(wirble-docs-pdf wirble-docs)
    list(APPEND LibGLR_DOC_TARGETS wirble-docs-pdf)
  endif()

  if(INSTALL_DOCS)
    install(CODE
      "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target wirble-docs RESULT_VARIABLE wirble_docs_result)\nif(NOT wirble_docs_result EQUAL 0)\n  message(FATAL_ERROR \"Failed to build documentation target wirble-docs\")\nendif()"
    )
    install(DIRECTORY ${LibGLR_DOXYGEN_HTML_DIR}/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/html OPTIONAL)
    install(DIRECTORY ${LibGLR_DOXYGEN_LATEX_DIR}/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/latex OPTIONAL)

    if(TARGET wirble-docs-pdf)
      install(CODE
        "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target wirble-docs-pdf RESULT_VARIABLE wirble_docs_pdf_result)\nif(NOT wirble_docs_pdf_result EQUAL 0)\n  message(FATAL_ERROR \"Failed to build documentation target wirble-docs-pdf\")\nendif()"
      )
      install(FILES ${LibGLR_DOXYGEN_LATEX_DIR}/refman.pdf DESTINATION ${CMAKE_INSTALL_DOCDIR}/pdf OPTIONAL)
    endif()
  endif()

  if(INSTALL_MANFILES)
    install(CODE
      "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target wirble-docs RESULT_VARIABLE wirble_man_result)\nif(NOT wirble_man_result EQUAL 0)\n  message(FATAL_ERROR \"Failed to build documentation target wirble-docs for manpages\")\nendif()"
    )
    install(DIRECTORY ${LibGLR_DOXYGEN_MAN_DIR}/man3glr/ DESTINATION ${CMAKE_INSTALL_MANDIR}/man3glr OPTIONAL)
  endif()
else()
  message(STATUS "Doxygen not found: documentation targets are disabled")
endif()

