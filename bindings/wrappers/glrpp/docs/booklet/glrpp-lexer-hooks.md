Lexer hooks are the point where glrpp’s optional scanner meets libglr’s native reading machinery. They are especially important when parsing raw text with a scanner attached.

## What a lexer hook does

A hook receives a window of unread input from the runtime and decides whether it can classify the next token. In glrpp, the hook bridge:

- decodes the current UTF-16 reader event into a UTF-8 slice
- asks the CTRE scanner to match at offset zero
- translates the winning UTF-8 match length back into native byte consumption
- tells libglr which terminal was accepted

## Why this is better than pure pre-tokenization

Pre-tokenizing a whole file is fine in many situations. Hook-based tokenization is more flexible when:

- tokenization depends on parser mode
- you want native reader control over consumption
- Unicode unit accounting must stay aligned with the runtime
- you want a single parser entry point for raw text

## The hook bridge payload

Internally, the parser stores a payload object containing:

- the shared scanner
- scratch storage for the accepted terminal name

Scratch storage matters because native hook responses expect stable C string pointers while the callback returns.

## Richer metadata

The scanner and token model expose `bytes_consumed`, `codepoint`, and `from_hook`. These fields help bridge lexical and parser views of the same input. In particular:

- `bytes_consumed` expresses what the runtime should advance
- `codepoint` can capture the first character class hint for downstream logic
- `from_hook` distinguishes native hook-driven tokens from manually assembled ones

## Contextual lexing strategies

A simple hook can still support contextual behavior by swapping scanners or layering them. Common patterns include:

- keyword promotion after identifier matching
- mode-specific scanners for string interpolation
- parser-controlled precedence between competing token classes

Even when the current `dsl::scanner` is stateless, the hook system gives you a place to attach richer policies later.

## Debugging hook behavior

When hook-driven parsing fails unexpectedly, inspect in this order:

1. does the UTF-8 window contain the text you think it does?
2. does `scanner.match_at(window, 0)` succeed?
3. is the chosen rule accidentally marked `skip`?
4. does the reported `bytes_consumed` match the native UTF-16 span?

Mismatched unit accounting is the most subtle bug class here.

## Example mental model

Imagine the source `hello 🌍`. The runtime reader sees UTF-16 code units. The bridge converts the unread window to UTF-8. The scanner matches `word` then `emoji`. The hook returns terminal names plus native consumption counts that reflect the underlying UTF-16 units, not merely the UTF-8 byte length. That is the core discipline of glrpp’s lexer hooks.
