cmake_minimum_required(VERSION 3.20)
project(glrpp VERSION 0.1.0 LANGUAGES C CXX)

# -------------------------------------------------------
# Build options
# -------------------------------------------------------
option(BUILD_DEP "Build Debian package" OFF)
option(BUILD_RPM "Build RPM package" OFF)
option(BUILD_TARBALL "Build tarball archive" OFF)
option(BUILD_ZLIB "Build .zip archive" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(TARGET_MANPAGES "Build manpages" OFF)
option(GLRPP_BUILD_BINDINGS "Build SWIG C++ bindings" ON)
option(GLRPP_INSTALL "Install glrpp headers and bindings" ON)

# -------------------------------------------------------
# Global configuration
# -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(GNUInstallDirs)

# Python helper scripts
set(GLRPP_ARCHIVER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/glrpp/scripts/archive-services.py")
set(GLRPP_DOCSTRING_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/glrpp/scripts/docstring-services.py")

# -------------------------------------------------------
# Header-only target
# -------------------------------------------------------
add_subdirectory(third_party/libltdl EXCLUDE_FROM_ALL)

add_library(glrpp INTERFACE)
target_include_directories(glrpp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/brigand/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ctre/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/frozen/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libltdl
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/magic_enum/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nameof/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pfr/include
)
target_link_libraries(glrpp INTERFACE ltdl)

# -------------------------------------------------------
# SWIG Bindings
# -------------------------------------------------------
if(GLRPP_BUILD_BINDINGS)
    add_subdirectory(bindings)
endif()

# -------------------------------------------------------
# Installation (headers + targets + CMake package files)
# -------------------------------------------------------
if(GLRPP_INSTALL)

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/glrpp
        DESTINATION include
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/
        DESTINATION include/glrpp/third_party/
    )

    set(GLRPP_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/glrpp")

    install(
        FILES
            cmake/glrppConfig.cmake
            cmake/glrppConfigVersion.cmake
            cmake/glrppTargets.cmake
        DESTINATION
            ${GLRPP_CMAKE_DIR}
    )
endif()

# =======================================================
# Packaging Targets (deb / rpm / tarball / zlib)
# =======================================================

if(BUILD_DEP)
    add_custom_target(deb
        COMMAND ${CMAKE_COMMAND} -E echo "Building Debian package..."
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} deb
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

if(BUILD_RPM)
    add_custom_target(rpm
        COMMAND ${CMAKE_COMMAND} -E echo "Building RPM package..."
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} rpm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

if(BUILD_TARBALL)
    add_custom_target(tarball
        COMMAND ${CMAKE_COMMAND} -E echo "Building tarball archive..."
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} tarball
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

if(BUILD_ZLIB)
    add_custom_target(zlib
        COMMAND ${CMAKE_COMMAND} -E echo "Building zip archive..."
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} zlib
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# =======================================================
# Documentation targets (html/pdf/latex/singlehtml)
# =======================================================

if(BUILD_DOCS)

    # HTML documentation
    add_custom_target(html
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_DOCSTRING_SCRIPT} html
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # Single page HTML
    add_custom_target(singlehtml
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_DOCSTRING_SCRIPT} singlehtml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # PDF documentation
    add_custom_target(pdf
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_DOCSTRING_SCRIPT} pdf
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # LaTeX documentation
    add_custom_target(latex
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_DOCSTRING_SCRIPT} latex
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # Install docs
    add_custom_target(install-docs
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_DOCSTRING_SCRIPT} install
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    # Install docs with install target
    install(CODE "
        message(STATUS \"Installing documentation...\")
        execute_process(
            COMMAND ${Python3_EXECUTABLE} ${GLRPP_DOCSTRING_SCRIPT} install
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    ")

    # Bundle docs into archives if GLRPP_BUNDLE_DOCS set
    if(DEFINED ENV{GLRPP_BUNDLE_DOCS})
        set(DOC_TYPE "$ENV{GLRPP_BUNDLE_DOCS}")
        add_custom_target(bundle-docs
            COMMAND ${CMAKE_COMMAND} -E echo \"Bundling documentation type: ${DOC_TYPE}\"
            COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} bundle ${DOC_TYPE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()

endif()

# ======================================================
# Documentation
# ======================================================

find_package(Doxygen REQUIRED)

set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)

# =======================================================
# Manpage targets
# =======================================================

if(TARGET_MANPAGES)

    add_custom_target(build-man
        COMMAND ${CMAKE_COMMAND} -E echo "Building manpages..."
        COMMAND nuroff ${CMAKE_SOURCE_DIR}/glrpp/man
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_custom_target(install-man
        COMMAND ${CMAKE_COMMAND} -E echo "Installing manpages..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/glrpp/man
                ${CMAKE_INSTALL_PREFIX}/share/man
    )

endif()
