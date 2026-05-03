cmake_minimum_required(VERSION 3.20)
project(glrpp VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(BUILD_DEP "Build Debian package" OFF)
option(BUILD_RPM "Build RPM package" OFF)
option(BUILD_TARBALL "Build tarball archive" OFF)
option(BUILD_ZLIB "Build .zip archive" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(TARGET_MANPAGES "Build manpages" OFF)
option(GLRPP_INSTALL "Install glrpp headers and metadata" ON)
option(GLRPP_INSTALL_BUNDLED_LIBLTDL "Install the bundled libltdl helper library" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python3 COMPONENTS Interpreter)

set(GLRPP_ARCHIVER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/archive-services.py")
set(GLRPP_DOCSTRING_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/docstring-services.py")
set(GLRPP_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/brigand/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/compile-time-regular-expressions/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/frozen/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libltdl
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libltdl/libltdl
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/magic_enum/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nameof/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pfr/include)

add_subdirectory(third_party/libltdl EXCLUDE_FROM_ALL)

add_library(glrpp INTERFACE)
add_library(glrpp::glrpp ALIAS glrpp)
target_include_directories(glrpp INTERFACE
    $<BUILD_INTERFACE:${GLRPP_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(glrpp INTERFACE
    $<BUILD_INTERFACE:ltdl>
    $<INSTALL_INTERFACE:ltdl>)

if(CMAKE_DL_LIBS)
    target_link_libraries(glrpp INTERFACE $<INSTALL_INTERFACE:${CMAKE_DL_LIBS}>)
endif()

if(HAVE_LMDB OR HAVE_LIBMDBX)
    target_compile_definitions(glrpp INTERFACE HAVE_LMDB=1)
endif()

if(GLRPP_INSTALL)
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/glrpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/brigand/include/brigand
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/compile-time-regular-expressions/include/ctre
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/frozen/include/frozen
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libltdl/libltdl
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libltdl/ltdl.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/compile-time-regular-expressions/include/ctre.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/compile-time-regular-expressions/include/ctre-unicode.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/compile-time-regular-expressions/include/unicode-db.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/magic_enum/include/magic_enum
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nameof/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pfr/include/boost
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(TARGETS glrpp EXPORT glrppTargets)

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/glrppConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/glrppConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/glrppConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/glrpp"
    )

    install(
        EXPORT glrppTargets
        FILE glrppTargets.cmake
        NAMESPACE glrpp::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glrpp
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/glrppConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/glrppConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glrpp
    )

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/glrpp.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/glrpp.pc"
        @ONLY
    )
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/glrpp.pc"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endif()

# Documentation subdirectory
if(BUILD_DOCS)
    add_subdirectory(docs)
endif()

if(BUILD_DEP AND Python3_Interpreter_FOUND)
    add_custom_target(deb
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} deb
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(BUILD_RPM AND Python3_Interpreter_FOUND)
    add_custom_target(rpm
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} rpm
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(BUILD_TARBALL AND Python3_Interpreter_FOUND)
    add_custom_target(tarball
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} tarball
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(BUILD_ZLIB AND Python3_Interpreter_FOUND)
    add_custom_target(zlib
        COMMAND ${Python3_EXECUTABLE} ${GLRPP_ARCHIVER_SCRIPT} zlib
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
