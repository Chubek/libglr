cmake_minimum_required(VERSION 3.16)
project(libglr VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_DOCUMENTATION "Build documentation" ON)
option(BUILD_DISAMBSTD "Build the standard disambiguation library" ON)
option(BUILD_REWRITELIB "Install the standard rewrite library" ON)
option(BUILD_SWIG_BINDINGS "Build SWIG bindings when SWIG is available" OFF)
option(ENABLE_TEST_SANITIZERS "Enable address/undefined sanitizers for tests" OFF)
option(BUILD_SHAREDLIB "Build libglr as a shared library" OFF)
option(INSTALL_LOCAL "Install libglr into ~/.local instead of /usr/local" OFF)

if(INSTALL_LOCAL)
    if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install prefix" FORCE)
    else()
        message(FATAL_ERROR "INSTALL_LOCAL=ON requires the HOME environment variable to be set.")
    endif()
endif()

# Bundle the local S-expression parser used by the rewrite subsystem.
file(GLOB SFSEXP_SOURCES CONFIGURE_DEPENDS
    third_party/sfsexp/src/*.c
)
add_library(sfsexp STATIC ${SFSEXP_SOURCES})
target_include_directories(sfsexp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/sfsexp/src>
        $<INSTALL_INTERFACE:include/libglr/third_party/sfsexp>)
target_compile_options(sfsexp PRIVATE -w)
set_target_properties(sfsexp PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
install(TARGETS sfsexp
    EXPORT ${PROJECT_NAME}-exports
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY third_party/sfsexp/src/
    DESTINATION include/libglr/third_party/sfsexp
    FILES_MATCHING PATTERN "*.h")

# Library sources
set(GLR_SOURCES
    src/glr/grammar.c
    src/glr/stack.c
    src/glr/fork.c
    src/glr/forest.c
    src/glr/reduction.c
    src/glr/graph.c
    src/glr/parser.c
    src/glr/disambiguate.c
    src/glr/lexer-hooks.c
    src/glr/reader.c
    src/glr/rewrite.c
)

# Create library
set(GLR_LIBRARY_KIND STATIC)
if(BUILD_SHAREDLIB)
    set(GLR_LIBRARY_KIND SHARED)
endif()
add_library(${PROJECT_NAME} ${GLR_LIBRARY_KIND} ${GLR_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Set include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
target_link_libraries(${PROJECT_NAME} PUBLIC sfsexp)

# Set compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

# Create alias for use in subdirectories
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Install targets
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-exports
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(INSTALL_LOCAL)
    install(CODE "file(MAKE_DIRECTORY \"$ENV{HOME}/.local/lib\")")
endif()

install(FILES
    include/glr/glr.h
    include/glr/grammar.h
    include/glr/stack.h
    include/glr/fork.h
    include/glr/forest.h
    include/glr/reduction.h
    include/glr/graph.h
    include/glr/parser.h
    include/glr/disambiguate.h
    include/glr/lexer-hooks.h
    include/glr/reader.h
    include/glr/rewrite.h
    DESTINATION include/glr
)

# Install documentation
if(BUILD_DOCUMENTATION)
    add_subdirectory(docs)
endif()

# Export targets
install(EXPORT ${PROJECT_NAME}-exports
    FILE ${PROJECT_NAME}-config.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)

if(BUILD_DISAMBSTD)
    add_subdirectory(disambstd)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_REWRITELIB)
    add_subdirectory(rewritelib)
endif()

if(BUILD_SWIG_BINDINGS)
    add_subdirectory(bindings)
endif()

# Generate package config file
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)
