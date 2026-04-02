cmake_minimum_required(VERSION 3.16)

# Test executables
add_executable(test_grammar test_grammar.c)
target_link_libraries(test_grammar PRIVATE libglr)

add_executable(test_stack test_stack.c)
target_link_libraries(test_stack PRIVATE libglr)

add_executable(test_fork test_fork.c)
target_link_libraries(test_fork PRIVATE libglr)

add_executable(test_forest test_forest.c)
target_link_libraries(test_forest PRIVATE libglr)

add_executable(test_reduction test_reduction.c)
target_link_libraries(test_reduction PRIVATE libglr)

add_executable(test_parser test_parser.c)
target_link_libraries(test_parser PRIVATE libglr)

# Register tests
enable_testing()

add_test(NAME GrammarTests COMMAND test_grammar)
add_test(NAME StackTests COMMAND test_stack)
add_test(NAME ForkTests COMMAND test_fork)
add_test(NAME ForestTests COMMAND test_forest)
add_test(NAME ReductionTests COMMAND test_reduction)
add_test(NAME ParserTests COMMAND test_parser)

# Set test properties
set_tests_properties(GrammarTests StackTests ForkTests ForestTests ReductionTests ParserTests
    PROPERTIES
        TIMEOUT 30
        LABELS "unit"
)
