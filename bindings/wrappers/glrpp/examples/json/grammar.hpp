#pragma once

#include <glrpp/glrpp.hpp>

namespace json_example {

inline glrpp::dsl::grammar grammar() {
  using namespace glrpp;
  return make_grammar(
      "Value",
      {production("Value", alt({sym(terminal("string")), sym(terminal("number")), sym(terminal("bool"))}))});
}

}  // namespace json_example
