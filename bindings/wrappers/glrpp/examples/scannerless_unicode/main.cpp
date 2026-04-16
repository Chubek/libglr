#include <glrpp/glrpp.hpp>

#include <iostream>
#include <memory>
#include <string_view>

int main() {
  using namespace glrpp;

  const auto grammar = make_grammar(
      "Greeting",
      {production("Greeting", seq({sym(terminal("word")), sym(terminal("emoji"))})),
       production("GreetingTail", opt(sym(terminal("punct"))))});

  auto scanner = std::make_shared<dsl::scanner>(std::vector<dsl::scan_rule>{
      skip_rule<"[ \\t]+">("ws", 1),
      token_rule<"[A-Za-z]+">("word", 10),
      token_rule<"[\x21\x2e]">("punct", 5),
      token_rule<"[\xF0-\xF4][\x80-\xBF]{3}">("emoji", 50),
  });

  constexpr std::string_view input = "hello \xF0\x9F\x8C\x8D";

  const auto tokens = scanner->scan(input);
  if (!tokens) {
    std::cerr << tokens.error().format() << '\n';
    return 1;
  }

  std::cout << "UTF-8 scanner bridge demo:\n";
  for (const auto& tok : tokens.value()) {
    std::cout << " - " << tok.kind << " => " << tok.lexeme
              << " (bytes=" << tok.bytes_consumed
              << ", codepoint=" << tok.codepoint
              << ", hook=" << tok.from_hook << ")\n";
  }

  try {
    glr::reader reader(scanner);
    const auto set_ok = reader.set_input(input);
    if (!set_ok) {
      std::cerr << set_ok.error().format() << '\n';
      return 1;
    }

    std::cout << "\nReader events:\n";
    while (true) {
      const auto next = reader.next();
      if (!next) {
        std::cout << " - stop: " << next.error().format() << '\n';
        break;
      }
      std::cout << " - " << next.value().terminal_name
                << " consumed " << next.value().bytes_consumed
                << " bytes from the UTF-16 bridge\n";
    }

    glr::parser parser(grammar, scanner);
    const auto result = parser.parse(input);
    if (!result) {
      std::cout << "\nScannerless parse note: " << result.error().format() << '\n';
    } else {
      std::cout << "\nParse forest:\n";
      util::dump(result.value());
    }
  } catch (const std::exception& ex) {
    std::cout << "\nRuntime unavailable: " << ex.what() << '\n';
  }

  return 0;
}
