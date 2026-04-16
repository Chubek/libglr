# glrppTargets.cmake

include_guard()

# -------------------------------------------------------
# Compute install prefix
# -------------------------------------------------------

get_filename_component(_GLRPP_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

# -------------------------------------------------------
# Header-only target
# -------------------------------------------------------

add_library(glrpp INTERFACE IMPORTED)

set_target_properties(glrpp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
        "${_GLRPP_PREFIX}/include"
)

# -------------------------------------------------------
# SWIG binding library
# -------------------------------------------------------

add_library(glrpp_glr_bindings SHARED IMPORTED)

set_target_properties(glrpp_glr_bindings PROPERTIES
    IMPORTED_LOCATION
        "${_GLRPP_PREFIX}/lib/libglrpp_glr_bindings.so"

    INTERFACE_INCLUDE_DIRECTORIES
        "${_GLRPP_PREFIX}/include"
)

# -------------------------------------------------------
# Link dependency relationship
# -------------------------------------------------------

target_link_libraries(glrpp_glr_bindings
    INTERFACE
        glrpp
)
