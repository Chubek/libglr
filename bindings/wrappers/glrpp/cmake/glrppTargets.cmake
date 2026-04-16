include_guard()

get_filename_component(_GLRPP_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

add_library(glrpp INTERFACE IMPORTED)

set_target_properties(glrpp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
        "${_GLRPP_PREFIX}/include"
)
set_target_properties(glrpp PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS "GLRPP_HAS_SWIG_BINDINGS=0"
)
