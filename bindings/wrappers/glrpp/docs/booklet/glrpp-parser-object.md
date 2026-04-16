`glrpp::glr::parser` is the main orchestration object in the wrapper. It owns the native grammar and parser handles, optionally owns a lexer-hook registry, and offers two user-facing parse entry points.

## Construction model

The primary constructor is:

```cpp
explicit parser(dsl::grammar grammar,
                std::shared_ptr<dsl::scanner> scanner = nullptr)
```

This design says a lot about the intended usage:

- the grammar is copied or moved into the parser
- the scanner is optional
- scanner ownership is shared because the same scanner may be reused elsewhere

During construction the parser:

1. loads runtime API function pointers from libglr through `context`
2. creates a native grammar handle
3. interns all symbols from your grammar
4. flattens rule expressions into the production bodies expected by libglr
5. creates the native parser
6. installs lexer hooks if a scanner was provided

## Non-copyable by design

The wrapper disables copy construction and copy assignment. That is a good default because native parser handles represent runtime resources with a clear ownership boundary.

If you need multiple parser instances, construct them independently from a shared grammar value or a factory function.

## Destruction and cleanup

The destructor releases, in order:

- native parser
- lexer hooks
- native grammar

This matters if you hold onto a parse forest after parser destruction. The forest wrapper therefore stores shared ownership glue so native resources remain valid as long as the forest needs them.

## Parse overloads

There are two main ways to call the parser:

### Token-stream mode

```cpp
auto result = parser.parse(dsl::token_stream{make_token("number", "42")});
```

This mode is ideal when lexical analysis already happened elsewhere. The parser serializes token kinds into the format expected by libglr.

### Text mode

```cpp
auto result = parser.parse("42 + answer");
```

If no scanner is installed, the text is handed directly to the runtime parser. If a scanner is installed, the wrapper converts UTF-8 text to BOM-tagged UTF-16LE bytes so the reader-hook bridge can feed richer token metadata back into libglr.

## Diagnostics

On failure, `parse` returns `parse_diagnostic` rather than throwing. The diagnostic includes:

- a message string
- what was expected
- what was found
- source position
- consumed byte count

That makes the parser object pleasant to use in batch tools and IDE-like workflows where failures are data, not exceptional control flow.

## Native grammar flattening

The current wrapper flattens expression trees into symbol sequences when registering productions. That means the parser object is the place where the high-level DSL meets the lower-level libglr API. If you later add advanced lowering passes such as precedence encoding or rewrite expansion, the parser assembly phase is where they belong.

## Scanner integration lifecycle

When a scanner is provided, the parser creates native lexer hooks and registers a bridge callback. The callback:

- receives a UTF-16 event window from libglr
- converts that window into a UTF-8 view plus byte-offset mapping
- runs CTRE matching at the window start
- returns the accepted terminal name and native byte consumption

That bridge is what lets glrpp remain scannerless by default while still offering optional lexical structure.

## Usage recommendations

- treat the parser as the compiled form of a grammar
- reuse parser instances when parsing many inputs with the same grammar
- use one parser per thread unless you have explicitly audited thread-safety requirements
- keep the scanner immutable after construction

The parser object is the point where user ergonomics, dynamic loading, and native runtime behavior all meet.
