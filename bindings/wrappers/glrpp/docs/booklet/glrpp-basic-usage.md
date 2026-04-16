The fastest way to understand glrpp is to build a tiny grammar, create a parser, feed it either tokens or raw text, and inspect the result. This chapter walks through both modes.

## A minimal grammar

The grammar DSL models runtime grammar expressions. The main building blocks are:

- `terminal(name)`
- `nonterminal(name)`
- `literal(text)`
- `sym(symbol)` to turn a symbol into an expression
- `seq({...})` for concatenation
- `alt({...})` or `|` for alternation
- `opt`, `star`, and `plus` for repetition
- `production(lhs, rhs)` to make a rule
- `make_grammar(start, {...})` to finalize the grammar

Example:

```cpp
#include <glrpp/glrpp.hpp>

using namespace glrpp;

const auto grammar = make_grammar(
    "Expr",
    {production("Expr", seq({sym(terminal("number")), star(seq({sym(literal("+")), sym(terminal("number"))}))}))});
```

## Parsing a token stream

If you already have tokens, parsing is direct:

```cpp
glr::parser parser(grammar);
auto result = parser.parse(dsl::token_stream{
    make_token("number", "1", 0, 1, 1),
    make_token("+", "+", 1, 1, 2),
    make_token("number", "2", 2, 1, 3),
});

if (!result) {
  std::cerr << result.error().format() << '\\n';
}
```

The wrapper serializes token kinds for the runtime parser and retains diagnostic coordinates from the original stream.

## Parsing raw text with a scanner

If you prefer to parse text, attach an optional CTRE scanner:

```cpp
auto scanner = std::make_shared<scanner>(std::vector<dsl::scan_rule>{
    skip_rule<"[ \t\n]+">("ws", 1),
    token_rule<"[0-9]+">("number", 10),
    token_rule<"\+">("+", 20),
});

glr::parser parser(grammar, scanner);
auto result = parser.parse("1 + 2");
```

With a scanner attached, glrpp routes parsing through the UTF-16 reader bridge so native lexer hooks and CTRE matching can cooperate.

## Understanding the result

`parser.parse(...)` returns `glrpp::util::expected<glrpp::glr::forest, glrpp::util::parse_diagnostic>`. In practice that means:

- call `has_value()` or use the boolean conversion
- use `value()` on success
- use `error()` on failure

Example:

```cpp
if (result) {
  glrpp::util::dump(result.value());
} else {
  std::cerr << result.error().format() << '\\n';
}
```

## A scanner-only workflow

Sometimes you want to test tokenization before parsing. The scanner is useful independently:

```cpp
auto tokens = scanner->scan("sum + 42");
for (const auto& tok : tokens.value()) {
  std::cout << tok.kind << ' ' << tok.lexeme << '\\n';
}
```

Each token includes offset, line, column, `bytes_consumed`, first `codepoint`, and a `from_hook` marker.

## Common beginner mistakes

- forgetting `sym(...)` inside `seq({...})`
- using literal text in the grammar but token names in the token stream, or vice versa
- constructing a grammar with an empty start symbol
- assuming a missing libglr runtime is a grammar problem
- expecting a single tree when GLR returns a forest

## A full tiny example

```cpp
using namespace glrpp;

const auto g = make_grammar(
    "Term",
    {production("Term", sym(terminal("identifier")) | sym(terminal("number")))});

auto s = std::make_shared<scanner>(std::vector<dsl::scan_rule>{
    skip_rule<"[ \t]+">("ws", 1),
    token_rule<"[A-Za-z_][A-Za-z0-9_]*">("identifier", 10),
    token_rule<"[0-9]+">("number", 20),
});

glr::parser p(g, s);
auto r = p.parse("answer");
```

That is the whole beginner loop: define symbols, define rules, choose token-stream or scanner-driven parsing, inspect the forest or diagnostic.
