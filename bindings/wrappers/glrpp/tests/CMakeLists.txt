set(GLRPP_TEST_SOURCES
    test_ast.cpp
    test_dsl.cpp
    test_grammar.cpp
    test_meta.cpp
    test_parser.cpp)

foreach(test_source IN LISTS GLRPP_TEST_SOURCES)
  get_filename_component(test_name "${test_source}" NAME_WE)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} PRIVATE glrpp)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
