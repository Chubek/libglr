Error reporting is where parser correctness meets user empathy. glrpp already exposes structured parse diagnostics; a good application turns them into actionable messages and, where possible, graceful recovery.

## The built-in diagnostic shape

`glrpp::util::parse_diagnostic` contains:

- `message`
- `expected`
- `found`
- `position` with offset, line, and column
- `consumed`

It also offers `format()` for a concise human-readable rendering.

## Parse errors versus grammar errors

glrpp distinguishes two major failure classes:

- `grammar_error` for malformed grammar construction or parser assembly
- `parse_diagnostic` / `parse_error` for input-driven failures

Keep those separate in your own interfaces. Users cannot fix a grammar bug in the same way they fix a malformed source file.

## Good error messages

A good parser error should answer:

- where did it happen?
- what was expected?
- what was actually found?
- what can the user do next?

Example:

```text
syntax error | expected: Expr | found: plus | consumed: 4 @1:5
```

That is the compact form. A polished tool can add source excerpts and hints.

## Recovery strategies

Even when the runtime reports a failure, your application can still recover operationally by:

- continuing to parse later files
- falling back to a token stream for debugging
- preserving partial semantic state from previous successful parses
- offering fix-it hints based on common mistakes

## Lexer and Unicode errors

Do not forget the lexical layer. A scanner failure or reader-bridge mismatch should produce diagnostics that make the lexical nature of the issue obvious. Users should not have to guess whether the problem is a grammar rule or token classification.

## Testing diagnostics

Treat diagnostics as part of the API. Add tests that assert:

- the right line and column
- stable expected token names
- sensible found snippets
- useful formatting for representative failures

Great error reporting is often what distinguishes a usable parser from a merely correct one.
