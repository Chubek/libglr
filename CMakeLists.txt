cmake_minimum_required(VERSION 3.16)
project(libglr VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_DOCUMENTATION "Build documentation" ON)

# Library sources - only .c files in src
set(GLR_SOURCES
    src/glr/grammar.c
    src/glr/stack.c
    src/glr/fork.c
    src/glr/forest.c
    src/glr/reduction.c
    src/glr/graph.c
    src/glr/parser.c
)

# Create library
add_library(${PROJECT_NAME} STATIC ${GLR_SOURCES})

# Set include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

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

install(FILES
    include/glr/glr.h
    include/glr/grammar.h
    include/glr/stack.h
    include/glr/fork.h
    include/glr/forest.h
    include/glr/reduction.h
    include/glr/graph.h
    include/glr/parser.h
    DESTINATION include/glr
)

# Install documentation
if(BUILD_DOCUMENTATION)
    add_subdirectory(doc)
endif()

# Install scripts
install(PROGRAMS
    scripts/test-library.sh
    scripts/compile.sh
    scripts/install.sh
    scripts/build-docs.sh
    DESTINATION bin
    PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)
install(FILES
    scripts/extract-constructs.py
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

# Install manpage
if(BUILD_DOCUMENTATION)
    install(FILES man/libglr.3tb
        DESTINATION share/man/man3
    )
endif()

# Export targets
install(EXPORT ${PROJECT_NAME}-exports
    FILE ${PROJECT_NAME}-config.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
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
