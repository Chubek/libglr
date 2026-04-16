#include <glrpp/glrpp.hpp>

#include <iostream>
#include <memory>

int main() {
  using namespace glrpp;

  const auto grammar = make_grammar(
      "Expr",
      {production("Expr", sym(nonterminal("Term")) | seq({sym(nonterminal("Term")), sym(literal("+")), sym(nonterminal("Expr"))})),
       production("Term", sym(terminal("number")) | sym(terminal("identifier")))});

  auto scanner = std::make_shared<dsl::scanner>(std::vector<dsl::scan_rule>{
      skip_rule<"[ \\t\\n]+">("ws", 1),
      token_rule<"[0-9]+">("number", 20),
      token_rule<"[A-Za-z_][A-Za-z0-9_]*">("identifier", 10),
      token_rule<"\\+">("+", 30),
  });

  std::cout << "Grammar summary:\n";
  util::dump(grammar);
  std::cout << "\n\nScanned tokens:\n";

  const auto scanned = scanner->scan("answer + 42");
  if (!scanned) {
    std::cerr << scanned.error().format() << '\n';
    return 1;
  }

  for (const auto& tok : scanned.value()) {
    util::dump(tok);
    std::cout << " @" << tok.line << ':' << tok.column << '\n';
  }

  try {
    glr::parser parser(grammar, scanner);
    const auto result = parser.parse("answer + 42");
    if (!result) {
      std::cout << "\nParser note: " << result.error().format() << '\n';
    } else {
      std::cout << "\nParse forest:\n";
      util::dump(result.value());
    }
  } catch (const std::exception& ex) {
    std::cout << "\nParser unavailable: " << ex.what() << '\n';
  }

  return 0;
}
