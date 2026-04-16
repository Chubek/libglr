#include "grammar.hpp"

#include <glrpp/glrpp.hpp>

#include <iostream>

int main() {
  glrpp::glr::parser parser(json_example::grammar());
  auto result = parser.parse(glrpp::dsl::token_stream{glrpp::make_token("string", "\"hello\"")});
  if (!result) {
    std::cerr << result.error().format() << '\n';
    return 1;
  }
  glrpp::util::dump(result.value());
  return 0;
}
