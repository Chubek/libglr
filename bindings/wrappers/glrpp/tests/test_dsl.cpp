#include <glrpp/glrpp.hpp>

#include <cassert>
#include <string>

int main() {
  using namespace glrpp::dsl;

  const auto number = sym(terminal("number"));
  const auto ident = terminal("identifier");
  const auto plus = literal("+");
  const auto plus_expr = sym(plus);

  const auto choice = number | ident | plus;
  assert(choice.kind == expr_kind::choice);
  assert(choice.children.size() == 3);
  assert(choice.children[0].atom.name == "number");
  assert(choice.children[1].atom.name == "identifier");
  assert(choice.children[2].atom.name == "+");

  const auto grouped = seq({sym(nonterminal("Expr")), choice, plus_expr});
  assert(grouped.kind == expr_kind::sequence);
  assert(grouped.children.size() == 3);
  assert(grouped.children[1].kind == expr_kind::choice);

  const auto grammar = make_grammar(
       "Expr",
      {production("Expr", sym(nonterminal("Term")) | sym(nonterminal("ExprTail"))),
       production("ExprTail", opt(sym(nonterminal("Additive")))),
       production("Additive", seq({sym(nonterminal("Term")), plus_expr, number})),
       production("Term", number | ident)});

  assert(grammar.start() == "Expr");
  assert(grammar.find_rule("Term") != nullptr);
  assert(grammar.nonterminals().size() == 4);
  assert(grammar.terminals().size() >= 4);

  auto scanner = glrpp::scanner(std::vector<scan_rule>{
      skip_rule<"[ \\t]+">("ws", 1),
      token_rule<"[A-Za-z_][A-Za-z0-9_]*">("identifier", 10),
      token_rule<"[0-9]+">("number", 20),
      token_rule<"\\+">("plus", 30),
  });

  const auto tokens = scanner.scan("sum + 42");
  assert(tokens.has_value());
  assert(tokens.value().size() == 3);
  assert(tokens.value()[0].kind == "identifier");
  assert(tokens.value()[0].line == 1);
  assert(tokens.value()[0].column == 1);
  assert(tokens.value()[1].kind == "plus");
  assert(tokens.value()[1].offset == 4);
  assert(tokens.value()[2].kind == "number");
  assert(tokens.value()[2].lexeme == "42");
  assert(tokens.value()[2].from_hook);

  const auto scanned = scanner.match_at("123abc", 0);
  assert(scanned.has_value());
  assert(scanned.value().rule != nullptr);
  assert(scanned.value().rule->name == "number");
  assert(scanned.value().match.bytes_consumed == 3);

  return 0;
}
