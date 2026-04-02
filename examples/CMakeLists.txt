cmake_minimum_required(VERSION 3.16)

# Calculator example
add_executable(calc calc.c)
target_link_libraries(calc PRIVATE libglr)

# Ambiguous grammar example
add_executable(ambiguous ambiguous.c)
target_link_libraries(ambiguous PRIVATE libglr)

# Install examples
install(TARGETS calc ambiguous
    RUNTIME DESTINATION bin
)
