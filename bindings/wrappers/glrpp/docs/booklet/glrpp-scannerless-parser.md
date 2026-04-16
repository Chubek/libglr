Scannerless parsing treats the character stream as the primary source of syntax instead of requiring a separate tokenization phase. glrpp is designed to be scannerless by default, with optional CTRE rules available when they help.

## What scannerless means in practice

A scannerless parser can recognize punctuation, keywords, layout, and other lexical structure directly within grammar-aware parsing. This is especially attractive for:

- languages with embedded syntaxes
- indentation-sensitive constructs
- contexts where token boundaries depend on parse state
- rapid prototyping from a reference grammar

## Why glrpp still offers a scanner

Scannerless and lexical assistance are not enemies. glrpp’s optional scanner exists because some tasks are still easier lexically, but the design keeps raw-text parsing central.

## Raw-text parse flow

Without a scanner:

```cpp
glr::parser parser(grammar);
auto result = parser.parse(source_text);
```

With a scanner attached, the parser still accepts raw text, but token recognition is delegated through the reader-hook bridge.

## Advantages

- grammar stays close to the real surface syntax
- fewer mismatches between lexer and parser phases
- easier experimentation with contextual tokenization
- better support for mixed-language or template-heavy syntaxes

## Pitfalls

- broad grammars can become highly ambiguous quickly
- whitespace handling must be designed carefully
- error reporting may need more work because there is no separate token layer to lean on

## Hybrid strategy

In practice, many glrpp projects use a hybrid approach:

- scannerless at the architectural level
- optional CTRE hooks for obvious lexical classes such as identifiers, numbers, and trivia

That hybrid often delivers the best of both worlds.

## Example use case

A templating language with embedded expressions can let the outer grammar remain scannerless while delegating obvious inner tokens to CTRE rules. The parser still feels like one parser, not two disconnected subsystems.

Scannerless parsing is not about refusing to tokenize. It is about keeping tokenization subordinate to syntax rather than the other way around.
