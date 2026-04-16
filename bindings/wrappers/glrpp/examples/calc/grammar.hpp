#pragma once

#include <glrpp/glrpp.hpp>

namespace calc_example {

inline glrpp::dsl::grammar grammar() {
  using namespace glrpp;
  return make_grammar(
      "Expr",
      {production("Expr",
                  seq({sym(terminal("number")), star(seq({sym(literal("+")), sym(terminal("number"))}))}))});
}

}  // namespace calc_example
