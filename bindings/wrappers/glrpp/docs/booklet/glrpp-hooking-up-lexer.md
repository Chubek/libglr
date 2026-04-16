glrpp is scannerless by default, but it can also cooperate with a lexer through the `dsl::scanner` abstraction. That scanner is intentionally small: it stores rules, chooses the best match at a position, and emits `token_stream` values or per-position matches.

## Why an optional lexer exists

Scannerless parsing is expressive, but some languages still benefit from explicit token boundaries:

- numeric literals are easier to classify lexically
- comments and whitespace are easier to skip lexically
- contextual token priority is easier to encode as rule ordering
- diagnostics can talk about token classes rather than raw characters

The optional scanner lets you get those benefits without giving up the reader hook architecture.

## Defining scan rules

Rules are normally created through CTRE helpers:

```cpp
auto scanner = std::make_shared<glrpp::scanner>(std::vector<glrpp::dsl::scan_rule>{
    glrpp::skip_rule<"[ \t\n]+">("ws", 1),
    glrpp::token_rule<"[A-Za-z_][A-Za-z0-9_]*">("identifier", 10),
    glrpp::token_rule<"[0-9]+">("number", 20),
});
```

Each rule has:

- a `name`
- a numeric `priority`
- a `skip` flag
- a matcher function pointer

## Longest match, then priority

The scanner chooses a winner using two criteria:

1. longest matched lexeme
2. higher priority if lengths are equal

That policy is a classic lexer rule and usually matches user expectations.

## Standalone scanning

You can use the scanner separately from parsing:

```cpp
auto tokens = scanner->scan("sum 42");
```

This is useful for lexer tests, editor tooling, and debugging tokenization before grammar work begins.

## Parsing through the scanner

When you pass the scanner into `glr::parser`, glrpp installs a native lexer hook bridge. At parse time, the runtime reader presents a UTF-16 event window, and the bridge asks the scanner what token starts there.

That means the scanner is not merely a pre-tokenization utility. It can actively participate in the native parsing workflow.

## Designing token names

Choose token names to match grammar vocabulary. For instance:

- `number`
- `identifier`
- `string`
- `lparen`, `rparen`
- `kw_if`, `kw_else`

Using raw punctuation as terminal names can be pleasant for tiny grammars, but descriptive names age better in bigger grammars and diagnostics.

## Example: arithmetic lexer

```cpp
auto scanner = std::make_shared<scanner>(std::vector<dsl::scan_rule>{
    skip_rule<"[ \t]+">("ws", 1),
    token_rule<"[0-9]+">("number", 10),
    token_rule<"\+">("plus", 20),
    token_rule<"-">("minus", 20),
    token_rule<"\*">("star", 30),
    token_rule<"/">("slash", 30),
    token_rule<"\(">("lparen", 40),
    token_rule<"\)">("rparen", 40),
});
```

With this in place, your grammar can stay clean and token-oriented.

## Limits and extension points

The built-in scanner is intentionally simple. If you need stateful lexing, indentation stacks, or token post-processing, you can still wrap or precompute around it. The scanner API is small enough to be a stable bridge rather than a whole lexing framework.
