#include "grammar.hpp"

#include <glrpp/glrpp.hpp>

#include <iostream>

int main() {
  glrpp::glr::parser parser(calc_example::grammar());
  auto result = parser.parse(glrpp::dsl::token_stream{
      glrpp::make_token("number", "1", 0),
      glrpp::make_token("+", "+", 1),
      glrpp::make_token("number", "2", 2),
  });

  if (!result) {
    std::cerr << result.error().format() << '\n';
    return 1;
  }

  glrpp::util::dump(result.value());
  return 0;
}
