#include <glrpp/glrpp.hpp>

#include <cassert>

int main() {
  const auto grammar = glrpp::make_grammar(
      "Expr",
      {glrpp::production("Expr", glrpp::seq({glrpp::sym(glrpp::terminal("number"))}))});

  assert(grammar.start() == "Expr");
  assert(grammar.find_rule("Expr") != nullptr);
  assert(grammar.terminals().size() == 1);
  return 0;
}
