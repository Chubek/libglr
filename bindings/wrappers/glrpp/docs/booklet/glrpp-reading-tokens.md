When you already have lexical structure, glrpp can parse a `token_stream` directly. This chapter explains the token model, how tokens are consumed, and how to preserve useful source information.

## The token shape

A `glrpp::dsl::token` contains:

- `kind`: terminal name used by the grammar
- `lexeme`: original text slice
- `offset`: byte offset in the source
- `line` and `column`: human-readable coordinates
- `bytes_consumed`: especially useful in hook-driven or Unicode-aware tokenization
- `codepoint`: first code point or other leading code-point hint
- `from_hook`: whether the token came from a lexer hook path

That is richer than a minimal lexer token because scannerless and Unicode-aware bridges need extra information.

## Constructing tokens

The convenience helper is `make_token`:

```cpp
auto tok = glrpp::make_token("identifier", "answer", 0, 1, 1, 6, 'a', false);
```

For hand-built tests, it is often enough to provide only kind and lexeme:

```cpp
dsl::token_stream tokens{
  make_token("number", "1"),
  make_token("+", "+"),
  make_token("number", "2"),
};
```

## Matching grammar terminals

The parser matches tokens by `kind`. The `lexeme` is not used for grammar dispatch directly, but it is valuable for diagnostics and semantic layers.

This leads to a practical rule: make terminal names stable and descriptive. Prefer `identifier`, `number`, `string`, `kw_if`, or `plus` over ad hoc raw text unless your grammar is intentionally literal-driven.

## Position tracking

The parser’s diagnostic creation logic uses token offsets and coordinates. If you supply precise positions, failures can refer back to the right token. If you omit them, parsing still works, but diagnostics become less helpful.

For multiline sources, keep columns one-based and reset the column after each newline in your lexer.

## Token streams as contracts

A token stream is not just a bag of tokens; it is an agreement between your lexer and grammar. For example:

```cpp
const auto g = make_grammar(
    "Expr",
    {production("Expr", seq({sym(terminal("number")), sym(terminal("plus")), sym(terminal("number"))}))});
```

The grammar above expects token kinds `number` and `plus`. A stream using `+` instead of `plus` will fail unless the grammar says `literal("+")` or a terminal of that exact name.

## Token pre-processing patterns

Before parsing, many applications normalize the token stream:

- collapse trivia or drop comments
- rewrite contextual keywords based on mode
- merge lexical fragments such as string pieces
- attach semantic payloads in side tables

glrpp does not prevent any of these. Just preserve kind and coordinates consistently.

## Testing token-driven grammars

Token-stream parsing is excellent for unit tests because it isolates grammar behavior from lexing behavior. When a grammar test fails, you know the lexer is not the culprit.

Example:

```cpp
glr::parser parser(grammar);
auto result = parser.parse(dsl::token_stream{
  make_token("identifier", "sum", 0, 1, 1),
  make_token("plus", "+", 4, 1, 5),
  make_token("number", "42", 6, 1, 7),
});
```

In practice, direct token streams are the cleanest way to write precise parser tests and semantic action tests.
