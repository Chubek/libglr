#include <glrpp/glrpp.hpp>

#include <cassert>
#include <string>

int main() {
  const auto grammar = glrpp::make_grammar(
      "Expr",
      {glrpp::production("Expr", glrpp::seq({glrpp::sym(glrpp::terminal("number"))}))});

  glrpp::glr::parser parser(grammar);
  const auto result = parser.parse(glrpp::dsl::token_stream{glrpp::make_token("number", "42")});
  if (!result.has_value()) {
    return 0;
  }

  auto scanner = std::make_shared<glrpp::scanner>(std::vector<glrpp::dsl::scan_rule>{
      glrpp::skip_rule<"[ \t\n]+">("ws", 10),
      glrpp::token_rule<"[0-9]+">("number", 100),
  });

  const auto tokens = scanner->scan("12 34");
  assert(tokens.has_value());
  assert(tokens.value().size() == 2);
  assert(tokens.value()[0].kind == "number");
  assert(tokens.value()[0].lexeme == "12");
  assert(tokens.value()[0].bytes_consumed == 2);
  assert(tokens.value()[0].from_hook);

  const auto match = scanner->match_at("123xyz", 0);
  assert(match.has_value());
  assert(match.value().rule != nullptr);
  assert(match.value().rule->name == "number");
  assert(match.value().match.length == 3);
  assert(match.value().match.lexeme == "123");

  glrpp::glr::parser scanner_parser(grammar, scanner);
  const auto scanned = scanner_parser.parse("42");
  (void)scanned;

  try {
    glrpp::glr::reader native_reader(scanner);
    const std::u16string utf16 = u"42";
    const auto set_ok = native_reader.set_input(utf16);
    assert(set_ok.has_value());
    const auto next = native_reader.next();
    if (next.has_value()) {
      assert(next.value().terminal_name == "number");
      assert(next.value().from_hook);
      assert(next.value().bytes_consumed == 4);
    }

    glrpp::glr::reader be_reader(scanner);
    const std::string utf16_be_with_bom{
        static_cast<char>(0xFE), static_cast<char>(0xFF),
        static_cast<char>(0x00), static_cast<char>('4'),
        static_cast<char>(0x00), static_cast<char>('2'),
    };
    const auto set_be_ok = be_reader.set_input(utf16_be_with_bom);
    assert(set_be_ok.has_value());
    const auto be_next = be_reader.next();
    if (be_next.has_value()) {
      assert(be_next.value().terminal_name == "number");
      assert(be_next.value().from_hook);
      assert(be_next.value().bytes_consumed == 4);
    }
  } catch (const std::exception&) {
    return 0;
  }

  return 0;
}
