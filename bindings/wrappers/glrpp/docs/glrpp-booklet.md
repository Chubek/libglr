# glrpp-what-is-glr-parsing

GLR parsing is the family of generalized LR techniques designed for grammars that are hard, awkward, or impossible to express with a single deterministic LR state machine. In ordinary LR parsing, every parser state assumes that a single action is correct. In GLR parsing, multiple actions may be valid at once, so the parser follows all viable paths and later merges equivalent states. That is the core idea behind libglr and, by extension, glrpp.

## Why generalized parsing exists

Traditional parser generators work best when the grammar has already been simplified into a deterministic form. Real languages are less polite. They contain:

- precedence-sensitive operators
- optional separators and list forms
- contextual keywords
- ambiguous constructs that need semantic filtering later
- scannerless regions where tokenization depends on parser context

GLR parsing accepts that ambiguity is normal. Instead of rejecting conflicts up front, it represents them explicitly and postpones the final choice until enough structure is available.

## How GLR differs from LL and classic LR

LL parsers read from left to right and produce a leftmost derivation. They are pleasant to hand-write and easy to reason about, but they struggle with left recursion and many natural expression grammars.

Classic LR parsers also read left to right, but they build a rightmost derivation in reverse. They are more powerful than LL parsers, especially for expression-heavy languages, but still rely on deterministic parse tables.

GLR starts from the LR model and generalizes the places where LR would otherwise fail. When a shift/reduce or reduce/reduce conflict appears, the parser forks. Internally it uses a graph-structured stack instead of separate linear stacks, so common prefixes are shared efficiently.

## Parse forests instead of single trees

A deterministic parser usually returns one parse tree or one failure. A GLR parser usually returns a forest. A forest records all surviving interpretations compactly. glrpp exposes this through `glrpp::glr::forest` and `glrpp::glr::node`, which let you inspect roots, children, and native symbol names.

That design matters because ambiguity is often useful:

```cpp
using namespace glrpp;

const auto grammar = make_grammar(
    "Expr",
    {production("Expr", sym(nonterminal("Expr")) | seq({sym(nonterminal("Expr")), sym(literal("+")), sym(nonterminal("Expr"))})),
     production("Expr", sym(terminal("number")))});
```

A deterministic parser would insist that you encode precedence and associativity immediately. A GLR parser can accept the ambiguous grammar first, then let you disambiguate later.

## When GLR is the right tool

GLR shines when one or more of the following are true:

- you need a direct transcription of a language reference grammar
- you are prototyping and do not want to normalize away every conflict first
- you want scannerless parsing or lexer hooks
- your language has embedded sublanguages or contextual operators
- ambiguity itself is valuable, as in structural search, editor tooling, or syntax highlighting

## Costs and tradeoffs

Generalization is not free. In the happy path, deterministic grammars behave close to LR performance. In the worst case, highly ambiguous inputs can grow substantially in memory use and runtime. The practical lesson is simple: embrace GLR where it helps, but still design your grammar intentionally.

## What glrpp adds on top of libglr

libglr is the parsing engine. glrpp is the C++ wrapper layer. It adds:

- a header-only user-facing API
- a grammar DSL with `sym`, `seq`, `alt`, `opt`, `star`, `plus`, and `|`
- optional CTRE-powered tokenization
- UTF-16 reader bridging for native lexer hooks
- small metaprogramming utilities for reflection and type-level composition

If you remember one idea from this chapter, make it this: GLR parsing is a disciplined way to keep the language natural while moving disambiguation to the place where you actually have enough information to decide.



# glrpp-setting-up

A productive glrpp setup has two layers: the wrapper headers themselves and the runtime library that the wrapper opens dynamically. Because glrpp is header-only, compilation is straightforward. Because it delegates actual parsing to libglr through `libltdl`, runtime availability still matters.

## Toolchain checklist

For day-to-day work, prepare the following:

- a C++20 compiler such as `g++` or `clang++`
- the glrpp headers under `bindings/wrappers/glrpp/include`
- the bundled third-party headers under `bindings/wrappers/glrpp/third_party`
- a discoverable `libglr.so` in the system shared-library search path
- optionally, Pandoc for booklet generation and Doxygen for API docs

The wrapper already assumes several bundled libraries are available. The notable ones are CTRE for compile-time regex, Brigand-style type-list utilities, and `libltdl` for portable dynamic loading.

## Recommended project layout

A small project using glrpp usually looks like this:

```text
my-parser/
  CMakeLists.txt
  include/
  src/
  grammars/
  tests/
```

Within that project, keep the grammar definition close to its semantic layer. A pleasant pattern is:

- `grammar.hpp` for the DSL grammar
- `scanner.hpp` for CTRE token rules
- `parser.cpp` for assembly and entry points
- `ast.hpp` for domain nodes

## Include paths

At compile time you need glrpp headers plus any third-party headers that are not re-exported by your build system. A direct command line often looks like this:

```bash
g++ -std=c++20   -Ibindings/wrappers/glrpp/include   -Ibindings/wrappers/glrpp/third_party/compile-time-regular-expressions/include   -Ibindings/wrappers/glrpp/third_party/libltdl   -Iinclude   src/main.cpp -o my_parser
```

## Runtime search path expectations

glrpp loads libglr from normal shared-library search locations. On Linux that commonly means one of:

- `/usr/lib`
- `/usr/local/lib`
- a directory listed in `ld.so.conf`
- a path exposed through `LD_LIBRARY_PATH`

The wrapper tries portable loader lookups such as `lt_dlopenext("libglr")` and `lt_dlopenext("glr")`. You do not hardcode the library path in ordinary use.

## Sanity-checking the environment

Before writing grammar code, verify two things:

1. a trivial program including `glrpp/glrpp.hpp` compiles
2. a parser object can be constructed without a runtime loader error

A minimal smoke test is:

```cpp
#include <glrpp/glrpp.hpp>

int main() {
  using namespace glrpp;
  const auto g = make_grammar(
      "Start",
      {production("Start", sym(terminal("word")))});
  glr::parser p(g);
}
```

If compilation fails, your include paths are wrong. If construction throws, libglr is probably not installed in a searchable location.

## Development workflow tips

- start with token-stream parsing first; introduce scanner hooks later
- keep grammar examples tiny until parser creation succeeds reliably
- add tests that only compile the headers, because many wrapper mistakes are header hygiene mistakes
- when experimenting with scannerless parsing, keep Unicode examples in your test set from the start

A good setup is not glamorous, but it removes almost every source of confusion later in the project.



# glrpp-installation

glrpp is best understood as a header-only facade over a dynamically loaded parsing runtime. Installation therefore has two parts: make the headers visible to the compiler, and make `libglr.so` visible to the process loader.

## Header-only integration

The simplest installation model is vendoring. Copy or reference:

- `bindings/wrappers/glrpp/include/glrpp`
- the third-party include trees used by those headers

Then expose them with your build system.

### CMake example

```cmake
add_library(glrpp INTERFACE)
target_include_directories(glrpp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/wrappers/glrpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/wrappers/glrpp/third_party/compile-time-regular-expressions/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/wrappers/glrpp/third_party/libltdl
)
```

Because the wrapper is header-only, consumers usually link no wrapper object file. They still need the libglr runtime present when the program starts.

### Meson example

```meson
glrpp_inc = include_directories(
  'bindings/wrappers/glrpp/include',
  'bindings/wrappers/glrpp/third_party/compile-time-regular-expressions/include',
  'bindings/wrappers/glrpp/third_party/libltdl',
)
```

### Autotools example

Expose the include directories through `AM_CPPFLAGS`, then ensure libglr is installed into a directory your loader can discover.

## Runtime installation

The parser implementation lives in libglr. If it is installed globally, the wrapper can discover it through shared-library search paths. If it is not installed globally, common strategies are:

- install it under `/usr/local/lib` and run the platform-specific cache update
- ship it beside your application and configure the loader path appropriately
- package it as a runtime dependency of your project

## Linking notes

The user-facing wrapper does not require static linkage to libglr because loading is dynamic. The notable dependency in the wrapper layer is `libltdl`, which provides a cross-platform abstraction over `dlopen`, `LoadLibrary`, and related facilities.

That design has a few advantages:

- the wrapper remains header-only
- the application can fail gracefully if libglr is missing
- loader behavior is consistent across supported platforms

## Verifying installation from C++

A practical validation step is to instantiate a parser and try a trivial parse:

```cpp
using namespace glrpp;

const auto grammar = make_grammar(
    "Start",
    {production("Start", sym(terminal("number")))});

glr::parser parser(grammar);
auto result = parser.parse(dsl::token_stream{make_token("number", "42")});
```

If parser construction succeeds but `parse` reports a grammar issue, installation is fine and the problem is in your grammar. If parser construction fails immediately, inspect runtime library discovery first.

## Packaging guidance

When shipping glrpp to users, package these concerns separately:

- development package: headers and examples
- runtime package: libglr and any loader dependencies
- optional docs package: generated booklet and API reference

This separation keeps builds lightweight while ensuring that runtime deployment remains explicit.

## Versioning advice

Header-only libraries can drift from runtimes. Try to keep glrpp and libglr versions aligned within the same distribution or repository snapshot. If you upgrade one, rerun parser smoke tests and Unicode scanner tests immediately.



# glrpp-basic-usage

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



# glrpp-parser-object

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



# glrpp-reading-files

Parsing usually starts from files, not string literals. glrpp itself focuses on parsing and tokenization, so file reading is intentionally lightweight and explicit. The wrapper expects you to decide how much input to buffer and when.

## Whole-file reading

For many grammars, the simplest strategy is to read the full file into memory before parsing:

```cpp
std::ifstream in(path, std::ios::binary);
std::string source((std::istreambuf_iterator<char>(in)), {});
auto result = parser.parse(source);
```

Whole-file buffering is a good default when:

- inputs are small or moderate
- diagnostics need stable offsets into the full original source
- scannerless parsing benefits from contiguous storage

## Binary versus text mode

Prefer binary mode when reading parser input. It prevents platform newline translation from silently altering byte offsets. That matters because glrpp diagnostics track offsets and the reader bridge reasons about byte counts.

## Memory tradeoffs

Whole-file reads are easy, but not always ideal. Large inputs can inflate memory use in two places:

- the raw source buffer
- the parse forest for ambiguous or deeply nested constructs

If memory is tight, consider splitting work by file sections or grammar units. Even when libglr itself processes contiguous buffers, your higher-level application can still chunk work at logical boundaries.

## Feeding pre-buffered views

The parser accepts `std::string_view` in text mode, so you can parse slices of a larger buffer safely as long as the underlying storage outlives the parse call:

```cpp
std::string source = load_file();
std::string_view header = std::string_view(source).substr(0, 256);
auto result = parser.parse(header);
```

This is useful for embedded languages, configuration preambles, or targeted diagnostics.

## Incremental reading patterns

glrpp does not yet expose a dedicated incremental parser facade, but you can still model incremental workflows by:

- maintaining your own document buffer
- reparsing only the affected region or syntax unit
- caching semantic products above the parse layer

For editor integrations, a common pattern is "small region parse plus surrounding context" rather than true streaming parse state reuse.

## File encodings

The wrapper mostly assumes UTF-8 text at the public API boundary. When a scanner is attached, it converts UTF-8 input to UTF-16 bytes for the native reader hook path. If your files are already UTF-16, decode or normalize them at the file layer unless you are deliberately testing the reader bridge itself.

## Source mapping advice

When you read files, keep these artifacts together:

- the original byte buffer
- the file path
- a line index or newline table if you need random-access diagnostics
- any preprocessor or include expansion maps

This makes it easier to transform raw `parse_diagnostic` values into user-facing messages.

## Example utility

```cpp
struct source_file {
  std::string path;
  std::string text;
};

source_file read_source(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return {path, std::string((std::istreambuf_iterator<char>(in)), {})};
}
```

Read deliberately, keep offsets stable, and treat file loading as part of your diagnostic pipeline rather than a mere prelude.



# glrpp-reading-tokens

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



# glrpp-hooking-up-lexer

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



# glrpp-lexer-hooks

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



# glrpp-hooking-the-reader

The reader layer is where glrpp turns raw character data into the event stream expected by native lexer hooks. If the parser is the orchestrator, the reader is the adapter between textual encodings and token decisions.

## Reader responsibilities

`glrpp::glr::reader` is responsible for:

- storing the input buffer
- interpreting UTF-16 byte order correctly
- exposing token events through a hook-aware API
- preserving native byte-consumption semantics

It is not merely a file reader. It is a Unicode-aware lexical event bridge.

## Setting input

The reader accepts input in multiple forms depending on the wrapper surface:

```cpp
glrpp::glr::reader reader(scanner);
reader.set_input(u"42");
reader.set_input(std::string{"\xFE\xFF\x00\x34\x00\x32", 6});
```

The wrapper contains helpers to detect BOMs, choose little-endian or big-endian decoding, and interpret surrogate pairs.

## UTF-16 event windows

A reader event is not just a code unit. The bridge logic needs:

- a UTF-8 view of the next readable window
- a mapping from UTF-8 match boundaries back to UTF-16 byte counts
- the default runtime consumption size

This is why the reader helpers build an offset table when converting windows.

## Hook cooperation

With a scanner attached, the reader does not classify tokens itself. Instead, it prepares input for the lexer hook callback. The callback decides which terminal to accept. This separation is healthy:

- the reader understands encodings
- the scanner understands token classes
- the parser understands grammar structure

## Endianness and BOM handling

The wrapper includes logic that can distinguish UTF-16LE and UTF-16BE using a BOM when present. This matters for tests, cross-platform input fixtures, and interoperability with external tools.

## Example: native-reader probing

```cpp
auto scanner = std::make_shared<glrpp::scanner>(std::vector<glrpp::dsl::scan_rule>{
    glrpp::token_rule<"[0-9]+">("number", 10),
});

glrpp::glr::reader reader(scanner);
reader.set_input(u"42");
auto next = reader.next();
```

If `next` succeeds, it typically reports terminal name, hook origin, and consumed byte count in UTF-16 units.

## When to customize reading behavior

You will care about the reader directly when:

- testing Unicode and encoding correctness
- implementing preprocessing before lexical classification
- validating hook behavior without running a full parse
- integrating non-file sources such as editor buffers or network payloads

The reader layer is where raw bytes become parser-visible structure. Understanding it pays off whenever Unicode and contextual lexing enter the picture.



# glrpp-string-utils

String utilities in glrpp exist to support parsing work, not to replace a general-purpose text library. Their role is to help with slicing, view management, and encoding-sensitive operations that appear frequently in wrapper code.

## Why string helpers matter in a parser wrapper

Parsing code is full of boundaries:

- symbol names are strings
- tokens carry lexemes and coordinates
- scanners inspect windows of input
- readers translate between encodings
- diagnostics display expected and found fragments

Small helper functions reduce repeated indexing logic and make byte-versus-code-unit distinctions explicit.

## Typical use cases

Even if you never call a utility directly, you benefit from them in three places:

- scanner matching and diagnostic snippets
- parser serialization of token streams
- reader conversion between UTF-8 and UTF-16 representations

## Views over copies

Whenever possible, prefer `std::string_view` for transient parser work. glrpp follows that style in its public text-facing interfaces. This makes scanning and parse entry cheap and keeps the wrapper predictable.

```cpp
std::string source = load_source();
std::string_view head = std::string_view(source).substr(0, 16);
```

The rule is simple: views are excellent for read-only inspection, but the underlying storage must outlive the parse call.

## Diagnostic excerpts

One of the most common string helper tasks is extracting a short snippet around the failure point. glrpp’s parse diagnostics already carry `found` and `expected`, but application-level tools often augment those with source excerpts, carets, or highlighted ranges.

## Normalization guidance

The wrapper does not attempt aggressive Unicode normalization automatically. That is a deliberate choice. Grammar matching should not silently rewrite source text unless the application has chosen a normalization policy. If your language considers canonically equivalent forms identical, normalize at the ingestion layer and document that decision.

## Symbol names and literals

Grammar authors often mix descriptive terminals and literal punctuation. Keep symbol text stable and ASCII-friendly where possible. This makes grammar dumps, trace logs, and error explanations easier to read.

## Performance advice

- avoid repeated substring allocation in hot lexical paths
- keep token lexemes as lightweight slices until you truly need owned strings
- measure before inventing custom small-string optimizations

In a wrapper like glrpp, clarity about textual boundaries matters more than cleverness. Good string utilities are the quiet infrastructure that keeps the parser precise.



# glrpp-utf16-conversion

UTF-16 conversion is a central part of glrpp’s scanner bridge. Even if your public API uses UTF-8 strings, the native hook path needs UTF-16-aware accounting so that libglr and CTRE can agree on how much input was consumed.

## Why UTF-16 appears at all

The wrapper is built around libglr’s native reader and lexer-hook interfaces. Those interfaces reason about native character windows and byte consumption. To support Unicode correctly and portably, glrpp converts UTF-8 input into BOM-tagged UTF-16LE bytes before calling the runtime when a scanner is attached.

## Key tasks in conversion

The wrapper’s conversion helpers must:

- prepend or detect a BOM
- decode little-endian and big-endian UTF-16 correctly
- handle surrogate pairs
- map UTF-8 match boundaries back to UTF-16 byte counts

That last point is the one people miss most often. A CTRE match length in UTF-8 bytes is not enough. The runtime must know how many native bytes to advance in the original UTF-16 stream.

## Surrogate pairs

Characters outside the Basic Multilingual Plane are represented as surrogate pairs in UTF-16. A scanner that matches a single emoji may consume four UTF-8 bytes but also four UTF-16 bytes, split across two code units. The conversion layer must preserve that exact accounting.

## Endianness

The reader helpers can detect whether a BOM marks the buffer as big-endian or little-endian. This is especially helpful in tests, where you may want to prove that both representations produce identical tokenization results.

## Example mental mapping

Take the UTF-8 string `"hello 🌍"`.

- the word portion maps one byte per ASCII character in UTF-8 and two bytes per code unit in UTF-16
- the globe emoji maps four bytes in UTF-8 and four bytes in UTF-16 via a surrogate pair

When the scanner matches `emoji`, the bridge must return the UTF-16 byte count expected by the runtime, not just the UTF-8 match width.

## Performance considerations

Conversion costs time, but the payoff is correct scanner-hook integration. In practice:

- deterministic ASCII-heavy input stays simple
- non-ASCII input remains correct rather than silently misaligned
- the most expensive part is usually not conversion but ambiguity in the grammar itself

## Testing advice

Always include at least these test cases:

- pure ASCII
- BMP characters outside ASCII
- supplementary-plane characters using surrogate pairs
- UTF-16BE with BOM
- whitespace and newline tracking around non-ASCII text

UTF-16 conversion is not glamorous, but it is the difference between a Unicode-capable parser and a parser that only looks Unicode-capable on paper.



# glrpp-what-are-nodes

In glrpp, nodes are the units of structure you inspect after parsing. They live inside a parse forest and represent recognized symbols, derivations, or ambiguous alternatives depending on the runtime shape.

## Trees versus forests

A tree assumes there is one correct parse. A forest records all surviving parse structures compactly. Because glrpp is built on GLR parsing, the forest abstraction is primary and node inspection happens inside that larger structure.

## The wrapper types

The main runtime-facing types are:

- `glrpp::glr::forest`
- `glrpp::glr::node`

A forest yields its root nodes, and each node can reveal its name and children.

## Node identity

A node corresponds to a native libglr node handle plus wrapper metadata. Identity is therefore not merely textual. Two nodes with the same symbol name may still represent different spans or derivations.

## Parenting and shared substructure

Because the parse product is a forest, substructures may be shared. Conceptually, a node can participate in multiple higher-level derivations. This is one reason forests are more memory-efficient than naively materializing every tree separately.

## Inspecting nodes

The debug utilities make node inspection straightforward:

```cpp
for (const auto& root : forest.roots()) {
  glrpp::util::dump(root);
}
```

The dump routine recursively prints node names with indentation. That is enough for smoke tests and initial debugging.

## Node names

Node names usually come from grammar symbols. If your grammar names terminals and nonterminals clearly, your forests will read clearly too. This is another reason to invest in good symbol naming early.

## Nodes versus AST values

Do not confuse parse-forest nodes with semantic AST nodes. glrpp also provides a generic `dsl::ast_node` structure for semantic work, but that is a separate layer. Parse nodes are syntactic facts; AST nodes are your chosen interpretation of those facts.

## Practical advice

- use forest nodes to debug grammar behavior
- use semantic AST nodes to build language tools
- expect shared structure when ambiguity exists
- never assume a node name alone determines meaning

The parser proves what was syntactically possible. Nodes are how that proof becomes inspectable.



# glrpp-specifying-grammars

Grammar specification in glrpp is intentionally direct. The DSL is small, the runtime grammar object is simple, and the emphasis is on readable, explicit C++.

## The three core concepts

A grammar consists of:

- symbols
- expressions
- production rules

Symbols are terminals, nonterminals, literals, or epsilon. Expressions combine symbols into sequences, choices, and repetition. Rules assign an expression to a left-hand-side nonterminal.

## Symbols

Create symbols with:

```cpp
auto t = glrpp::terminal("number");
auto n = glrpp::nonterminal("Expr");
auto l = glrpp::literal("+");
auto e = glrpp::epsilon();
```

A terminal is a named token class. A literal is punctuation or other exact text you want to name directly. A nonterminal is a syntactic category.

## Expressions

Wrap symbols with `sym` to obtain atomic expressions, then combine them:

```cpp
auto atom = sym(nonterminal("Term"));
auto seq_expr = seq({sym(nonterminal("Term")), sym(literal("+")), sym(nonterminal("Expr"))});
auto alt_expr = alt({sym(terminal("number")), sym(terminal("identifier"))});
auto maybe = opt(sym(terminal("sign")));
auto many = star(sym(terminal("comma")));
auto one_or_more = plus(sym(terminal("digit")));
```

## Rules and grammars

Rules are built with `production` and grouped with `make_grammar`:

```cpp
const auto g = make_grammar(
    "Expr",
    {production("Expr", sym(nonterminal("Term")) | seq({sym(nonterminal("Term")), sym(literal("+")), sym(nonterminal("Expr"))})),
     production("Term", sym(terminal("number")) | sym(terminal("identifier")))});
```

## Validation

The grammar constructor validates a few critical invariants:

- the start symbol must not be empty
- there must be at least one rule
- the start symbol must have a matching rule

More advanced validation, such as unreachable-rule warnings or precedence conflicts, belongs in higher-level tooling.

## Left recursion and GLR

Unlike many LL-oriented DSLs, glrpp does not force you to eliminate left recursion mechanically. GLR parsing can handle many direct formulations naturally. That is one of the main ergonomic wins of the approach.

## Style recommendations

- use nonterminal names in `PascalCase` or another consistent scheme
- use descriptive terminal names unless a literal is genuinely clearer
- keep top-level productions visually simple, then factor helper nonterminals as needed
- prefer one concept per rule unless ambiguity is intentional

A good grammar should read like a precise explanation of the language, not a workaround for parser-generator limitations.



# glrpp-native-dsl-specs

This chapter treats the glrpp DSL as a formal specification language. The goal is to make the semantics of each construct explicit rather than merely intuitive.

## Symbol taxonomy

`symbol_kind` currently distinguishes:

- `terminal`
- `nonterminal`
- `literal`
- `epsilon`

A `symbol` contains a `name` string and a `kind`. Terminals and literals are both treated as terminal-like during grammar introspection.

## Expression algebra

`expr_kind` currently distinguishes:

- `atom`
- `sequence`
- `choice`
- `optional`
- `zero_or_more`
- `one_or_more`

An `expression` contains:

- a `kind`
- an `atom` symbol, meaningful when `kind == atom`
- a `children` vector for composite expressions

## Constructors and semantics

### `sym(symbol)`
Creates an atomic expression.

### `seq({e1, e2, ...})`
Creates an ordered concatenation. Successful recognition requires each child to match in order.

### `alt({e1, e2, ...})`
Creates an explicit choice. Recognition succeeds if any child succeeds.

### `operator|`
Creates a choice expression using EBNF-style infix syntax. Existing choice children are flattened.

### `opt(e)`
Shorthand for either `e` or epsilon.

### `star(e)`
Zero or more repetitions of `e`.

### `plus(e)`
One or more repetitions of `e`.

## Rule semantics

A `rule` contains:

- `lhs`: the left-hand-side nonterminal name
- `rhs`: an expression tree
- `reducer`: an optional semantic action wrapper

Even when semantic actions are not fully wired into the runtime lowering yet, the DSL reserves the slot explicitly.

## Grammar semantics

A `grammar` contains:

- a start symbol string
- an ordered vector of rules

`terminals()` walks every rule expression and collects terminal-like symbols. `nonterminals()` derives names from rule left-hand sides.

## Formal reading example

```cpp
production("List", seq({sym(terminal("item")), star(seq({sym(literal(",")), sym(terminal("item"))}))}))
```

This denotes a language of one `item` followed by zero or more comma-item suffixes.

## Lowering expectations

At parser-construction time, expression trees are flattened into native grammar productions for libglr. The wrapper therefore treats the DSL as the human-readable front-end and the native grammar as the execution form.

## Specification discipline

Think of the DSL as a typed EBNF embedded in C++. Every constructor has explicit runtime meaning, and every overloaded convenience should preserve that meaning. That is the standard by which extensions to the DSL should be judged.



# glrpp-using-native-dsl

The native DSL is pleasant because it is small enough to memorize and expressive enough to stay readable. This chapter focuses on day-to-day grammar authoring style.

## Start small and compose

Begin with tiny named pieces:

```cpp
using namespace glrpp;

const auto term = sym(nonterminal("Term"));
const auto plus_tok = sym(literal("+"));
const auto number = sym(terminal("number"));
```

Then compose them into rules:

```cpp
const auto grammar = make_grammar(
    "Expr",
    {production("Expr", term | seq({term, plus_tok, sym(nonterminal("Expr"))})),
     production("Term", number)});
```

## Prefer `|` for readable alternatives

The overloaded pipeline-style `|` operator makes grammar definitions read naturally:

```cpp
production("Atom", sym(terminal("number")) | sym(terminal("identifier")) | sym(terminal("string")));
```

Because the operator flattens nested choices, long alternation chains remain structurally tidy.

## Use helper variables for repeated fragments

C++ gives you names for common fragments. Use them.

```cpp
const auto comma_item = seq({sym(literal(",")), sym(nonterminal("Item"))});
production("List", seq({sym(nonterminal("Item")), star(comma_item)}));
```

This is clearer than duplicating the sequence inline everywhere.

## Literals versus named terminals

Use literals when the grammar truly wants exact punctuation. Use named terminals when lexical classification carries meaning.

Good examples:

- `literal("+")` for punctuation in tiny grammars
- `terminal("identifier")` for lexical categories
- `terminal("kw_if")` for contextual keywords after lexing

## Grammar factories

Wrap grammars in functions so you can reuse them in tests and examples:

```cpp
inline glrpp::dsl::grammar expr_grammar() {
  using namespace glrpp;
  return make_grammar(
      "Expr",
      {production("Expr", sym(nonterminal("Term")) | seq({sym(nonterminal("Term")), sym(literal("+")), sym(nonterminal("Expr"))})),
       production("Term", sym(terminal("number")))});
}
```

## Validation by construction

Constructing the grammar early gives you immediate feedback on missing start rules or empty starts. In practice, grammar factories plus unit tests catch most user-level mistakes before the parser is ever involved.

## Keep semantic intent visible

A grammar is documentation. When you write the DSL, optimize for future readers as much as for the compiler. If a rule looks cryptic, factor it. If an alternation is too wide, group related cases. If a terminal name is vague, rename it.

The best native DSL usage feels like executable language design notes.



# glrpp-disambiguation-dsl

GLR parsing happily records ambiguity, but production parsers rarely want to leave every ambiguity unresolved forever. Disambiguation in glrpp should be viewed as a layer on top of the grammar DSL rather than a rejection of it.

## Sources of ambiguity

Common ambiguity sources include:

- binary operators without precedence rules
- dangling-else style grammar shapes
- overlapping lexical classes
- grammar shortcuts such as broad `Expr` or `Type` categories

GLR lets you write these naturally first. Disambiguation then narrows the forest.

## DSL-level strategies

Even without a dedicated precedence DSL yet, you can express many preferences structurally:

- factor precedence levels into separate nonterminals
- isolate optional suffixes into helper rules
- distinguish lexical categories early where that improves clarity

Example precedence layering:

```cpp
production("Expr", sym(nonterminal("Add")));
production("Add", sym(nonterminal("Mul")) |
                  seq({sym(nonterminal("Add")), sym(literal("+")), sym(nonterminal("Mul"))}));
production("Mul", sym(nonterminal("Atom")) |
                  seq({sym(nonterminal("Mul")), sym(literal("*")), sym(nonterminal("Atom"))}));
```

## Post-parse filters

Sometimes the right move is to keep the grammar broad and filter the forest after parsing. This is especially attractive when the constraint is semantic rather than syntactic, such as type-directed interpretation or contextual keyword acceptance.

## Associativity

Associativity is often easier to encode by rule structure than by annotations. Left-associative addition, for example, naturally emerges from a left-recursive `Add` rule. Right-associative constructs can be expressed analogously.

## Lexer-aware disambiguation

A CTRE scanner plus hook bridge can resolve some ambiguities before the grammar sees them. Examples include:

- distinguishing identifiers from keywords
- preferring longest punctuation tokens
- recognizing numeric literal forms precisely

## Recommended workflow

1. write the clearest natural grammar you can
2. observe the forest shape on representative ambiguous inputs
3. decide whether the ambiguity is lexical, structural, or semantic
4. resolve it at the cheapest layer that preserves clarity

Disambiguation is most effective when it clarifies intent rather than merely suppressing parser conflicts.



# glrpp-disambiguation-forests

A parse forest is the right place to understand ambiguity because it records all surviving interpretations compactly. Disambiguation at the forest layer means inspecting or pruning that structure after the parser has done its generalized work.

## Why forests matter

Forests separate recognition from commitment. The parser answers "what parses are possible?" A disambiguation pass answers "which parse do I want for this application?"

This separation is valuable in tools such as:

- IDEs that prefer partial structure over early failure
- structural matchers that intentionally exploit ambiguity
- language servers that need resilient parse products

## Forest inspection patterns

Typical operations include:

- enumerate roots
- recursively inspect child node names
- detect ambiguous subtrees by node multiplicity or alternative structure
- choose a preferred subtree according to domain rules

The existing `util::dump` facility is a simple but useful starting point.

## Pruning strategies

Common pruning policies include:

- prefer the parse with the fewest error productions or recovery edges
- prefer the parse with the highest precedence interpretation
- prefer keyword interpretations over identifiers in certain contexts
- prefer shallower or deeper derivations depending on the domain

## Structural versus semantic pruning

Structural pruning relies only on forest shape. Semantic pruning consults symbol tables, type information, or external context. Both are valid; the important design question is whether the grammar should know the rule or whether a later phase should.

## Example scenario

Suppose an input fragment could be parsed as either a generic type application or a less-than comparison. A GLR grammar can keep both readings alive. Later, name resolution may show that the left side is a type constructor, at which point the forest can be pruned confidently.

## Practical implementation advice

- keep pruning deterministic and explainable
- log why a branch was rejected in development builds
- avoid mutating the original forest unless ownership and sharing rules are clear
- consider producing a semantic AST from the chosen branch rather than destructively editing forest nodes

A forest is not a nuisance to eliminate; it is the evidence needed to make a principled decision later.



# glrpp-actions-basics

Semantic actions are the first layer where a grammar stops merely recognizing shapes and starts producing meaning. In glrpp, the action model is intentionally simple so that users can begin with lightweight AST construction and gradually move toward richer semantic pipelines.

## What an action is

A semantic action is a callable attached conceptually to a production rule. When that rule reduces, the action receives the semantic values of its children and returns a new value for the parent.

In glrpp terms, the foundational pieces are:

- `glrpp::dsl::action<Fn>`
- `glrpp::dsl::make_action(fn)`
- `glrpp::dsl::identity`
- `glrpp::dsl::ast_node` and `glrpp::dsl::ast_array`

## A simple mental model

Suppose the grammar recognizes `number '+' number`. The parser layer proves that the structure exists. An action can then build something like:

- a generic AST node such as `{"add", [lhs, rhs]}`
- a typed domain node such as `binary_expr{plus, lhs, rhs}`
- a direct computed value such as `43`

The syntax and the meaning remain separate, but actions connect them deliberately.

## Action wrapper example

```cpp
auto make_sum = glrpp::dsl::make_action([](const glrpp::dsl::ast_array& children) {
  return glrpp::dsl::ast_node{"add", children};
});
```

The wrapper just stores the callable and forwards arguments. That simplicity makes actions easy to test outside the parser.

## Identity action

The `identity` helper is useful when a rule mostly exists for grammar organization and should preserve its child value unchanged. This is especially common in layered expression grammars where intermediate nonterminals are syntactic conveniences.

## Good beginner patterns

- use actions to build `ast_node` first
- keep one conceptual transformation per action
- avoid global mutable state
- treat semantic failures separately from parse failures

## Example AST construction

```cpp
glrpp::dsl::ast_node lhs{"number", std::int64_t{1}};
glrpp::dsl::ast_node rhs{"number", std::int64_t{2}};
auto add = glrpp::dsl::ast_node{"add", glrpp::dsl::ast_array{lhs, rhs}};
```

Even before the full runtime action wiring is expanded, this is the right conceptual shape for action-oriented design.

## Why actions matter early

If you delay semantic thinking too long, your grammar may become hard to map cleanly into useful program structures. If you introduce semantics too early, grammar debugging becomes harder. The sweet spot is:

1. make the grammar parse correctly
2. inspect the forest
3. start attaching small, obvious actions
4. refactor toward richer typed ASTs only when the structure is stable

Actions basics are about learning that meaning is best built in small, explicit steps.



# glrpp-semantic-basics

Semantic processing starts when syntax becomes meaning. glrpp reserves space for semantic actions in rules and also provides a lightweight generic AST model for applications that want a neutral tree representation.

## The semantic building blocks

The wrapper currently exposes:

- `dsl::action<Fn>` for callable semantic actions
- `make_action(fn)` for convenience
- `identity` as a trivial action
- `dsl::ast_node`, `ast_array`, and `ast_object` as generic semantic containers

## Generic AST nodes

`ast_node` stores:

- `kind`: a descriptive node tag
- `value`: a variant of null, string, integer, double, bool, array, or object

This makes it easy to build examples and tests without committing to a custom domain model immediately.

Example:

```cpp
glrpp::dsl::ast_node number{"number", std::int64_t{42}};
glrpp::dsl::ast_node list{"args", glrpp::dsl::ast_array{number}};
```

## Why actions matter

A parser proves structure, but most applications need more:

- evaluators need typed values
- compilers need rich AST nodes
- linters need source-linked semantic facts
- query tools need normalized syntax trees

Semantic actions are the bridge from parse recognition to those richer products.

## A good beginner workflow

1. get the grammar parsing correctly
2. inspect the forest on representative inputs
3. prototype semantic output using `ast_node`
4. only then move to custom domain types if needed

## Action signatures

The current rule type reserves an optional reducer of the rough shape:

```cpp
action<std::function<ast_node(const ast_array&)>>
```

That suggests a simple model: each reduction receives child semantic values and returns a new AST node.

## Example design

For an arithmetic grammar, a semantic layer might map:

- `number` token -> `ast_node{"number", 42}`
- `Add` rule -> `ast_node{"add", ast_array{lhs, rhs}}`

Even if the runtime wiring evolves, the conceptual structure is stable and worth designing early.

Semantic basics are about keeping syntax and meaning separate long enough to reason about both clearly.



# glrpp-semantic-actions

Semantic actions are where the parser stops being a recognizer and starts becoming a translator. In glrpp, actions are modeled as typed callables and attached to rules conceptually through the `rule::reducer` field.

## Action design principles

A good semantic action should be:

- local to one reduction or one clear abstraction boundary
- deterministic
- side-effect-light when possible
- easy to test independently of parsing

## The action wrapper

The generic wrapper is simple:

```cpp
template <typename Fn>
struct action {
  Fn fn;

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) const {
    return fn(std::forward<Args>(args)...);
  }
};
```

That simplicity is a strength. You can wrap lambdas, function objects, or ordinary functions.

## Example action shapes

### Fold children into a node

```cpp
auto make_add = glrpp::dsl::make_action([](const glrpp::dsl::ast_array& children) {
  return glrpp::dsl::ast_node{"add", children};
});
```

### Preserve an existing child

```cpp
auto first_child = glrpp::dsl::make_action([](const glrpp::dsl::ast_array& children) {
  return children.front();
});
```

## Error handling inside actions

Semantic failures are different from parse failures. A parse failure means the syntax did not fit. A semantic failure means the syntax fit but the interpretation was invalid. Model these separately. Common options are:

- return a special AST error node
- throw a domain-specific exception in batch tools
- accumulate diagnostics in an external sink

## Typed domain nodes

`ast_node` is useful, but real projects often migrate to custom types. Actions are the natural place to construct those richer nodes. Keep the generic AST around for debugging and tests even if the production pipeline uses a stronger type system.

## Capturing context

Because actions are callables, they can capture external data. Do this sparingly and intentionally. Capturing immutable configuration is usually fine. Capturing mutable global state often makes parse behavior harder to reason about.

## Testing actions

Treat actions like pure functions whenever you can. Unit-test them with synthetic child arrays before wiring them into parser assembly. That keeps grammar bugs and semantic bugs from masking each other.

Semantic actions are most powerful when they are small, explicit, and unsurprising.



# glrpp-configuration

Configuration in glrpp is lightweight and mostly architectural rather than centralized in one giant settings object. That is a good match for a header-only wrapper around a dynamic runtime.

## Main configuration surfaces

You configure glrpp through:

- build-system include paths and runtime library availability
- parser construction choices such as whether a scanner is attached
- scanner rule sets and priorities
- grammar design choices such as literal versus named terminals
- application-level diagnostic and semantic policies

## Build-time configuration

At build time, the important questions are:

- where are the headers?
- where are the third-party includes?
- will `libltdl` and libglr be available at runtime?

Because the wrapper is header-only, there are few compiled wrapper settings to worry about.

## Runtime configuration

At runtime, configuration is often embodied in which parser you construct. For example:

```cpp
glrpp::glr::parser scannerless(grammar);
glrpp::glr::parser hybrid(grammar, scanner);
```

That single choice changes how text input is interpreted and whether lexer hooks are installed.

## Scanner policy as configuration

Scanner priorities, skip rules, and token naming conventions are all configuration choices. They define the lexical contract for the rest of the parser pipeline.

## Semantic and diagnostic configuration

Most user-facing configuration lives above the parser core:

- strict versus permissive error handling
- how much ambiguity to tolerate
- whether to keep raw forests or build custom ASTs immediately
- how richly to format diagnostics

A good glrpp configuration strategy keeps the parser core small and pushes policy to explicit, testable application code.



# glrpp-pipeline-operator

The pipeline operator chapter in glrpp is about one very specific thing: overloading `operator|` to represent EBNF-style alternation in the grammar DSL.

## What `|` means in glrpp

In this wrapper, `|` is not a Unix-style pipe and not a bitwise operation. It is a grammar combinator. It means "either the left expression or the right expression".

Example:

```cpp
using namespace glrpp::dsl;

auto atom = sym(terminal("number")) |
            sym(terminal("identifier")) |
            sym(terminal("string"));
```

This builds a single `expr_kind::choice` expression.

## Why overload an operator at all

Alternation is one of the most common grammar operations. Writing it as nested `alt({...})` calls works, but it becomes noisy quickly. The operator form reads closer to EBNF and makes long alternatives easier to scan.

## Flattening behavior

The overload is designed to flatten existing choice nodes:

```cpp
auto expr = sym(terminal("number")) |
            sym(terminal("identifier")) |
            sym(literal("("));
```

Rather than creating nested binary choices such as `(a | b) | c`, glrpp appends all branches into one coherent choice vector. That simplifies later lowering and makes tests easier.

## Operand kinds

The overload accepts both:

- `dsl::symbol`
- `dsl::expression`

Symbols are promoted to atomic expressions automatically. That means you can mix concise and explicit forms naturally.

## Equivalent forms

These are conceptually equivalent:

```cpp
auto a = alt({sym(terminal("number")), sym(terminal("identifier"))});
auto b = sym(terminal("number")) | sym(terminal("identifier"));
```

Choose the one that makes the rule easiest to read.

## Best practices

- use `|` for short and medium-length alternations
- switch to helper variables when a choice gets visually dense
- keep each branch semantically coherent
- prefer named terminals when the alternatives are lexical categories

## Example: expressive EBNF style

```cpp
const auto type_expr =
    sym(terminal("identifier")) |
    seq({sym(literal("(")), sym(nonterminal("Type")), sym(literal(")"))}) |
    seq({sym(nonterminal("Type")), sym(literal("|")), sym(nonterminal("Type"))});
```

A tiny operator overload dramatically improves the readability of grammar code like this. That is exactly why the feature exists.



# glrpp-forking-parsers

Forking is native to GLR parsing. When the runtime encounters a conflict, it conceptually forks the parser state so multiple hypotheses can proceed in parallel. This chapter explains that idea from the wrapper user’s point of view.

## Why parser forking happens

Forking occurs when the current state admits more than one valid action, typically because the grammar is ambiguous or intentionally broad. Instead of choosing prematurely, the runtime explores all viable paths.

## Graph-structured stacks

Naively, forking would duplicate the full parse stack for every branch. GLR avoids that cost with a graph-structured stack. Shared prefixes remain shared, and only diverging suffixes branch.

## User-visible consequences

As a glrpp user, you usually notice forking indirectly through:

- a parse forest rather than a single tree
- more memory use on ambiguous input
- a need for later disambiguation

## Speculative parsing use cases

You can also use the idea of forking at a higher application level:

- try different start symbols against the same token stream
- parse with and without a scanner to compare behavior
- branch semantic interpretation after one broad parse

These are not the same as the runtime’s internal forks, but they follow the same philosophy: delay commitment until you have evidence.

## Example scenario

Consider a language where `<` might begin a generic argument list or a comparison. An LL parser often needs ad hoc lookahead tricks. A GLR parser can fork naturally when it reaches the ambiguous point and carry both readings until later context resolves the issue.

## Operational advice

- do not fear forking merely because it sounds expensive
- do fear uncontrolled ambiguity in large grammars without a pruning plan
- use targeted tests on known ambiguous constructs to understand branch growth

Forking is not a failure mode. It is generalized parsing doing exactly what it was designed to do.



# glrpp-rewrite-dsl

A rewrite DSL is a natural companion to a grammar DSL. Even if your current parser project starts with recognition only, you will eventually want declarative ways to transform syntax trees or normalize grammar forms.

## What rewriting means here

There are two broad rewrite targets in a glrpp-based system:

- grammar rewrites, which transform grammar definitions before parser construction
- tree rewrites, which transform parse results or semantic ASTs after parsing

A rewrite DSL gives those transformations first-class structure rather than burying them in ad hoc loops.

## Desirable properties

A useful rewrite language should make these ideas easy to express:

- pattern matching on node kinds or rule names
- replacement with new node structures
- recursive descent with stop conditions
- local normalization rules such as flattening associative chains

## Example conceptual syntax

Even if you implement rewrites in ordinary C++ first, think in these terms:

```cpp
rewrite("ParenExpr", [](const ast_array& children) {
  return children.front();
});
```

Or at the grammar level:

```cpp
rewrite_rule("List", expand_ebnf_repetition);
```

## Why declarative rewrites help

Declarative rules are easier to review than arbitrary mutation code. They also support logging, debugging, and composition much more naturally.

## Good early rewrite targets

- remove redundant grouping nodes
- collapse comma-separated suffix chains into flat arrays
- canonicalize keyword spellings or token categories
- desugar convenience grammar forms into core forms

## Relationship to the parser

Rewriting should not fight the parser. Let the parser recognize generously, then let rewrite passes simplify, normalize, or annotate the result. That separation keeps grammars readable and transformations explicit.

A rewrite DSL is valuable because parsing is usually only the first half of understanding a language.



# glrpp-rewriting-grammars

Grammar rewriting is the process of transforming one grammar into another before parser construction. In GLR-based systems, rewriting is often less about forcing determinism and more about improving organization, readability, or downstream tooling.

## Why rewrite grammars at all

Reasons include:

- expanding EBNF sugar into primitive productions
- injecting precedence or associativity scaffolding
- normalizing naming conventions
- combining imported grammar fragments
- adding instrumentation or debug productions

## glrpp expression trees as rewrite input

Because the DSL stores rules as runtime expression trees, grammar rewriting can be implemented as pure tree-to-tree transformation. That is a clean design point. You can traverse `dsl::expression`, inspect `expr_kind`, and produce a modified `dsl::expression` or `dsl::rule`.

## Example rewrite ideas

### Expand optional forms

`opt(e)` can be rewritten into a choice between `e` and epsilon if a lower layer prefers primitive forms.

### Expand repetition

`star(e)` and `plus(e)` can be lowered to helper nonterminals for runtimes that expect explicit recursion.

### Normalize literals

You might rewrite raw literals into named terminals when integrating with a large external lexer.

## Pipeline structure

A practical rewrite pipeline often looks like this:

1. parse or construct the high-level DSL grammar
2. run normalization passes
3. optionally run validation passes
4. build the native parser from the normalized grammar

## Keep rewrites explainable

Every rewrite should answer two questions clearly:

- what user-facing benefit does it provide?
- how can a developer trace the rewritten output back to the source grammar?

If a rewrite obscures the grammar more than it helps, reconsider it.

Grammar rewriting is best when it preserves the author’s intent while making the execution form more useful.



# glrpp-managing-ast

Once parsing succeeds, most real applications want an AST that is simpler and more stable than a raw parse forest. Managing that AST well is a major part of using glrpp effectively.

## Generic versus custom ASTs

glrpp provides a generic `dsl::ast_node` for convenience. It is ideal for:

- tests
- examples
- quick prototypes
- debug dumps

For large systems, a custom typed AST may still be the right choice. The generic AST is a stepping stone, not a prison.

## Ownership model

`ast_node` owns its content through ordinary C++ value semantics. Arrays and objects nest recursively, so copying a large tree can become expensive. Prefer moving nodes or storing them in larger semantic objects when performance matters.

## Mutability

A good rule is to keep AST construction mutable and AST consumption mostly immutable. Build nodes freely while reducing or rewriting, then hand later passes a stable tree.

## Visitor patterns

Even with the generic AST, visitors are useful. Simple examples include:

- pretty-printers
- evaluators
- symbol collectors
- diagnostics annotators

Because the generic AST uses a variant payload, a visitor usually dispatches first on `kind`, then on the active payload alternative.

## Normalization passes

Common AST normalization passes include:

- flattening left- or right-recursive chains into arrays
- discarding punctuation-only nodes
- converting numeric strings to integers or floats
- labeling nodes with source ranges stored externally

## AST and source mapping

Do not lose source information too early. Even if the AST node itself stays lightweight, keep a side table from semantic nodes to source spans. This makes diagnostics, refactoring tools, and formatters far easier to build later.

## Example generic AST construction

```cpp
glrpp::dsl::ast_node lhs{"identifier", std::string{"x"}};
glrpp::dsl::ast_node rhs{"number", std::int64_t{42}};
glrpp::dsl::ast_node assign{"assign", glrpp::dsl::ast_array{lhs, rhs}};
```

AST management is where syntax becomes the long-lived internal representation of your language. Treat it as a first-class design concern.



# glrpp-handling-reflections

Reflection in glrpp is intentionally modest but surprisingly useful. The wrapper exposes a few compile-time helpers that let grammars, semantic types, and utilities describe themselves more clearly.

## The reflection helpers

The current meta layer includes:

- `type_name<T>()`
- `enum_name(value)`
- `fields<T>` customizations
- `field_names_v<T>`
- the `reflectable` concept

These are not full language-level reflection, but they cover many practical parser-wrapper needs.

## Type names

`type_name<T>()` returns a compiler-dependent string view describing a type. On GCC and Clang it is based on `__PRETTY_FUNCTION__`, which is verbose but extremely handy in debugging and metaprogramming tools.

```cpp
auto name = glrpp::meta::type_name<int>();
```

## Enum names

The current `enum_name` helper returns the underlying integer value. That makes it more of an enum introspection primitive than a full symbolic name formatter, but it is still useful for logging and generated metadata.

## Declaring field names

You can specialize `glrpp::meta::fields<T>` to describe a record-like type:

```cpp
struct config { int level; bool strict; };

template <>
struct glrpp::meta::fields<config> {
  static constexpr auto names = std::array<std::string_view, 2>{"level", "strict"};
};
```

After that, `field_names_v<config>` and `reflectable<config>` become meaningful.

## Why reflection helps parser work

Reflection metadata supports:

- debug output
- AST serialization
- schema-like documentation generation
- generic visitors over semantic records

## Recommended use

Keep reflection data close to the semantic type it describes. Treat reflection declarations as part of your public semantic contract, especially if generated docs or tools depend on them.

Reflection is powerful precisely because the wrapper keeps it small and explicit.



# glrpp-managing-symbols

Symbols are the vocabulary of the grammar. If rules are sentences, symbols are the nouns and verbs those sentences are built from.

## Symbol categories

glrpp distinguishes four symbol kinds:

- terminals
- nonterminals
- literals
- epsilon

In practice, terminals and literals are both token-like from the parser’s point of view, while nonterminals define syntactic categories.

## Naming conventions

Good symbol names save enormous time later. A practical convention is:

- `Expr`, `Stmt`, `Type` for nonterminals
- `identifier`, `number`, `string` for terminals
- `kw_if`, `kw_else` for keywords if they are tokenized specially
- raw punctuation literals only when that is genuinely the clearest notation

## Lifecycle of a symbol

A symbol goes through several phases:

1. authored in the DSL
2. traversed during grammar validation and inspection
3. interned into the native grammar during parser construction
4. referenced by parse-forest nodes and diagnostics

That lifecycle is why consistency matters. A naming mismatch introduced at the DSL layer can surface later as a mysterious parse failure.

## Terminal design choices

Decide early whether punctuation should be modeled as literals or named terminals. Both are valid. Named terminals integrate better with external lexers; literals keep tiny grammars readable.

## Symbol collection utilities

The `grammar` wrapper exposes `terminals()` and `nonterminals()`. These are handy for:

- documentation generation
- grammar sanity checks
- test assertions
- building editor hints or completions

## Example

```cpp
const auto g = make_grammar(
    "Pair",
    {production("Pair", seq({sym(terminal("identifier")), sym(literal(":")), sym(terminal("number"))}))});
```

Here the grammar vocabulary is crisp: `Pair` is syntactic, `identifier` and `number` are lexical, `:` is literal punctuation.

Manage symbols carefully and the rest of the grammar becomes easier to reason about.



# glrpp-managing-context

Parsing context is all the information that influences interpretation without necessarily being visible as a grammar symbol. In glrpp-based systems, context often lives above the raw parser but must still be threaded cleanly through scanning, semantic actions, or later analysis passes.

## Examples of context

- whether a keyword is reserved in the current mode
- current indentation or layout mode
- enabled language extensions
- symbol tables or type environments for semantic disambiguation
- diagnostic sinks and configuration knobs

## Keep syntax and context separate

As a rule, keep the grammar focused on syntax and keep application context in explicit side channels. This avoids hiding semantic policy inside grammar hacks.

## Context in actions

Actions can capture immutable context easily:

```cpp
auto make_node = glrpp::dsl::make_action([cfg](const glrpp::dsl::ast_array& children) {
  return build_node(children, cfg);
});
```

Be cautious with mutable captures. They can make parse behavior hard to test.

## Context in lexing

Context-sensitive lexing is often better modeled through mode-specific scanners or scanner wrappers than through global mutable state. That keeps the parser entry point explicit: this input is being parsed under this lexical policy.

## Context in disambiguation

Many valuable disambiguation decisions require context after parsing. Name resolution, type lookup, and language edition flags belong naturally in forest-pruning or AST-construction passes.

## Recommended architecture

A clean pipeline often looks like this:

1. parse with minimal context
2. produce forest or neutral AST
3. apply contextual disambiguation and semantic analysis
4. emit diagnostics or domain artifacts

Context is inevitable. The trick is to place it where it clarifies the pipeline instead of muddying it.



# glrpp-managing-stack

One of the defining implementation ideas of GLR parsing is the graph-structured stack, often abbreviated GSS. Even if glrpp hides the raw structure, understanding it helps you reason about runtime behavior.

## Why a graph-structured stack exists

In deterministic LR parsing, one linear stack is enough. In GLR parsing, conflicts cause branching. Copying a full stack for every branch would be wasteful, so the runtime shares common prefixes in a graph.

## Conceptual model

Each stack node represents a parser state at a certain point. Edges represent predecessor relationships. When two parse paths converge to the same state and viable history, their stack representations can share structure again.

## Benefits

- branching does not imply full stack duplication
- merges become natural and cheap
- ambiguous prefixes stay compact

## Costs

- implementation complexity is higher than for ordinary LR stacks
- debugging requires thinking in graphs rather than simple stack traces
- heavily ambiguous grammars can still produce large structures

## Why wrapper users should care

Even though glrpp does not ask you to manipulate the GSS directly, stack behavior explains:

- why ambiguous inputs remain tractable longer than expected
- why forests can share substructures
- why parser traces are best visualized as graphs, not logs alone

## Practical takeaway

When you see branch growth during parsing, think in terms of graph width rather than duplicate whole-stack explosions. The runtime is already doing the smart thing; your job is to manage ambiguity and disambiguation policy so that the graph stays meaningful.



# glrpp-managing-dependencies

Real parsers rarely live in one file. Grammar fragments, scanner rules, semantic utilities, and domain models all depend on one another. Managing those dependencies well keeps a glrpp codebase readable.

## Organize by language layer

A practical project split is:

- grammar definitions
- scanning rules
- parser assembly
- semantic AST and actions
- diagnostics and tooling

Each layer should depend downward more than sideways.

## Cross-grammar composition

When a language has embedded sublanguages or reusable fragments, prefer composition through factory functions and helper expressions rather than copy-pasting rule sets.

Example:

```cpp
glrpp::dsl::grammar sql_core_grammar();
glrpp::dsl::grammar expr_fragment();
```

A higher-level builder can merge or adapt these pieces before parser construction.

## Avoid cyclic header dependencies

Because glrpp is header-only, include hygiene matters. Keep grammar declarations in headers and place heavyweight helper logic in source files where possible in your own project. If everything becomes templates, document the include structure carefully.

## Third-party dependencies

glrpp itself relies on bundled libraries such as CTRE and libltdl. Application code should avoid reaching into those dependencies directly unless it has a clear reason. Prefer the wrapper’s surface area first.

## Dependency advice for tests

Keep tests narrow:

- grammar tests should not need the full semantic layer
- scanner tests should not require the parser when avoidable
- metaprogramming tests should compile headers without dragging runtime concerns in unnecessarily

Good dependency management keeps each failure local and understandable.



# glrpp-compile-time-regex

Compile-time regular expressions are one of glrpp’s most distinctive conveniences. They let you define scanner rules as compile-time constants while keeping the matching logic efficient and type-safe.

## CTRE in the wrapper

The scanner helpers use CTRE through a fixed-pattern template form:

```cpp
token_rule<"[0-9]+">("number", 10)
skip_rule<"[ \t\n]+">("ws", 1)
```

The pattern is part of the type, which means the matcher can be generated at compile time.

## Why this is attractive

- no runtime regex compilation cost
- patterns are validated at compile time
- matcher functions become lightweight and predictable
- scanner definitions stay compact

## Matching semantics

The scanner uses `ctre::starts_with`, so each rule is anchored at the current offset. That is exactly what a lexer wants: determine what token begins here, not what token appears anywhere later.

## Example scanner

```cpp
auto scanner = std::make_shared<glrpp::scanner>(std::vector<glrpp::dsl::scan_rule>{
    glrpp::skip_rule<"[ \t]+">("ws", 1),
    glrpp::token_rule<"[A-Za-z_][A-Za-z0-9_]*">("identifier", 10),
    glrpp::token_rule<"[0-9]+">("number", 20),
    glrpp::token_rule<"\+">("plus", 30),
});
```

## Performance notes

Compile-time regex does not magically make every scanner fast, but it usually removes a major class of runtime overhead. The remaining costs are mostly:

- how many rules you try at each position
- how ambiguous the rule set is
- how much Unicode conversion the bridge performs

## Pattern design advice

- make whitespace and trivia rules explicit
- keep long overlapping patterns rare when possible
- use priority only when equal-length matches really need a tiebreaker
- test Unicode-oriented rules with real data, not only ASCII examples

CTRE gives glrpp a modern, expressive scanner definition style without turning the wrapper into a heavyweight lexer framework.



# glrpp-metaprogramming-basics

glrpp includes a small metaprogramming layer because parser wrappers benefit from light compile-time structure. The goal is not template acrobatics for its own sake; the goal is better expression, safer composition, and easier introspection.

## The main basic tools

The beginner-friendly pieces are:

- `type_list<Ts...>`
- `size_v<List>`
- `push_back_t<List, T>`
- `concat_t<Lists...>`
- `all_of<List, Pred>`
- `type_name<T>()`

These are enough to write clear compile-time checks around grammar-adjacent types.

## Type lists

A type list is just a container for types at compile time:

```cpp
using tokens = glrpp::meta::type_list<int, double, char>;
static_assert(glrpp::meta::size_v<tokens> == 3);
```

## Transforming lists

```cpp
using base = glrpp::meta::type_list<int, double>;
using more = glrpp::meta::push_back_t<base, char>;
using merged = glrpp::meta::concat_t<base, glrpp::meta::type_list<bool>>;
```

These helpers are intentionally minimal but cover many practical needs.

## Predicates over types

`all_of` lets you assert simple properties about every type in a list:

```cpp
template <typename T>
struct is_not_pointer : std::bool_constant<!std::is_pointer_v<T>> {};

static_assert(glrpp::meta::all_of<merged, is_not_pointer>::value);
```

## Reflection ties in naturally

The metaprogramming layer and reflection layer complement one another. Type lists describe sets of types. Reflection tells you about the fields or identities of those types.

## When to use these basics

Use them when they make an API contract clearer. Do not use them merely to avoid writing ordinary C++. In parser code, compile-time structure is useful when it documents semantic expectations, scanner tables, or AST schemas.

Metaprogramming basics are valuable when they stay small, obvious, and close to the domain problem.



# glrpp-metaprogramming-guide

This chapter connects the lightweight meta helpers to the broader design of glrpp. The wrapper uses compile-time programming not as a separate subsystem, but as a support layer for DSL ergonomics and correctness.

## Design philosophy

glrpp’s metaprogramming style is intentionally conservative:

- prefer small aliases and traits over giant template frameworks
- keep runtime grammar data explicit even when compile-time helpers exist
- use concepts and type traits to improve diagnostics when practical

## Compile-time and runtime meet in the DSL

The grammar DSL is runtime data, but some of its helpers are template-driven. CTRE patterns are a prime example: they are compile-time values that generate runtime scanner behavior.

Similarly, the pipeline operator uses templates and operand traits to accept both symbols and expressions cleanly:

- detect whether an operand is a `symbol` or `expression`
- promote symbols to atomic expressions
- flatten choice nodes into one runtime expression tree

That is a perfect example of compile-time code improving runtime clarity.

## Reflection as metadata plumbing

Specializing `fields<T>` gives you a structured way to associate names with semantic record types. This can power:

- generic AST serializers
- debug dumps
- schema-aware diagnostics

## Expected and type-level design

Even `util::expected` reflects a meta-guided style. It uses type parameters to encode success and error channels explicitly, which works well with parser APIs where failure is expected and structured.

## Recommended patterns

- use type lists to describe groups of semantic node types
- use reflection names to reduce boilerplate in dumps and docs
- keep template helpers in headers small and standalone
- add tests that compile the intended patterns, not only runtime tests

## Anti-patterns

- hiding runtime grammar logic inside opaque template machinery
- forcing users to understand type-level internals for ordinary DSL use
- abusing SFINAE when a simple overload or concept would do

A good metaprogramming guide for glrpp ultimately says: use compile-time machinery to make the runtime parser easier to use, not harder to understand.



# glrpp-advanced-metaprogramming

Advanced metaprogramming in glrpp begins where helper aliases stop being enough. This is the layer where you might manipulate grammar fragments as types, generate semantic glue automatically, or attach reflection-driven tooling to parser products.

## Advanced directions worth exploring

- type-level grammar fragments that lower into runtime expressions
- compile-time checked scanner tables derived from semantic enums
- reflection-driven AST visitors and serializers
- generated rewrite pipelines based on type traits

## Type-level grammar composition

Even though the runtime grammar is the current execution form, type-level descriptors can still be useful for static validation or code generation. For example, you might define a family of semantic node tags as types and derive reflection tables from them.

## Trait-driven operator control

The pipeline operator demonstrates a useful advanced pattern: constrain a generic operator with narrowly targeted traits so it remains convenient without becoming overly permissive. This same pattern can guide future DSL extensions such as rewrite combinators or precedence annotations.

## Compile-time reflection as tooling input

If you specialize `fields<T>` for semantic record types systematically, you can generate:

- JSON schemas for AST snapshots
- debug printers
- field-by-field comparison tools for tests
- documentation tables in the booklet or API docs

## Costs of going too far

Advanced metaprogramming can quickly reduce readability. The usual warning signs are:

- long instantiation backtraces from ordinary user mistakes
- runtime behavior hidden behind deeply nested aliases
- difficulty stepping through code in a debugger

## Practical advice

- keep the runtime grammar model as the source of truth
- add advanced compile-time layers only when they remove repeated user work
- pair every advanced template facility with example tests and documentation
- prefer explicit generated artifacts over magical implicit behavior

The right advanced metaprogramming in glrpp feels like a power tool. The wrong kind feels like a second language bolted onto the first.



# glrpp-debugging-grammars

Grammar debugging is easiest when you separate lexical, structural, and semantic questions. glrpp gives you enough hooks to do that systematically.

## Start with the smallest failing case

Before inspecting a full source file, reduce the failing input to the smallest fragment that still reproduces the problem. Tiny cases reveal whether the issue is:

- token mismatch
- rule mismatch
- unintended ambiguity
- runtime loader or setup failure masquerading as a parse issue

## Debug layer by layer

### 1. Check the scanner

```cpp
auto tokens = scanner->scan("sum + 42");
for (const auto& tok : tokens.value()) {
  glrpp::util::dump(tok);
}
```

### 2. Check grammar inventory

```cpp
glrpp::util::dump(grammar);
for (const auto& t : grammar.terminals()) {
  std::cout << t.name << '
';
}
```

### 3. Parse known-good token streams

Direct token streams isolate grammar problems from lexing problems.

### 4. Inspect the forest

If parsing succeeds but interpretation is wrong, dump the forest and compare it with your expected rule structure.

## Use ambiguity to your advantage

If the forest is larger than expected, that is not just noise. It tells you where the grammar is underspecified. Add tests around exactly those constructs.

## Diagnostic formatting

`parse_diagnostic::format()` is a good default string form. In larger tools, enrich it with source excerpts, carets, and explanation of the relevant rule or token class.

## Common failure patterns

- literals in the grammar but named terminals in the token stream
- scanner rules that overlap unexpectedly and win by priority
- start symbol set to a helper nonterminal instead of the top-level rule
- Unicode byte accounting mismatches in hook-driven parsing

## Maintain debug fixtures

Keep a dedicated directory of tiny grammar fixtures and tiny source fixtures. Grammar debugging is dramatically faster when your examples are small and named after the behavior they exercise.

A good grammar-debugging workflow turns mystery into classification: lexical problem, grammar problem, ambiguity problem, or semantic problem.



# glrpp-scannerless-parser

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



# glrpp-explaining-files

A parser that only says "syntax error" is rarely enough. File-level explanation is about turning parse outcomes into something a human can act on quickly.

## What a good file explanation includes

- the file path or logical source name
- a concise primary error message
- line and column coordinates
- a short source excerpt
- a note about expected versus found constructs
- optional follow-up hints or related notes

## glrpp diagnostic inputs

`parse_diagnostic` already gives you the structured core:

- `message`
- `expected`
- `found`
- `position`
- `consumed`

That is enough to build richer file explanations in your application layer.

## Example formatter shape

```cpp
void explain_file(const std::string& path,
                  const std::string& source,
                  const glrpp::util::parse_diagnostic& d);
```

A good implementation would compute the relevant line slice, print a caret under the offending column, and append any grammar-specific hint.

## Partial parses and resilience

In editor tools, file explanations should often be tolerant rather than dramatic. A parse failure may coexist with a useful partial forest or previously cached semantic information. Design the explanation layer so it can report what is known without pretending the whole file is lost.

## Multi-file projects

When files include or embed other files, keep origin metadata available. A helpful error report can then say not only where the failure occurred, but also how that source became part of the current parse job.

Good file-level explanations are where a parsing library becomes a developer tool.



# glrpp-explaining-rules

Rule-level explanation is about answering a subtler question than ordinary diagnostics: not merely where parsing failed, but which grammar ideas were active at the time.

## Why explain rules

When developing or teaching a grammar, it is often useful to know:

- which rule matched a construct
- which rules competed in an ambiguity
- which rule the parser expected next
- how a final forest branch was derived

## Sources of rule explanations

In glrpp-based tooling, rule explanations can come from several places:

- grammar metadata and symbol names
- parse-forest traversal
- debug lowering passes that annotate productions
- semantic passes that preserve origin-rule information

## Human-readable rule names

The easiest way to improve rule explanations is to write grammars with meaningful nonterminal names. `ExprTail` is already more informative than `X3`.

## Example explanation output

Imagine reporting:

- `Assignment` matched `identifier '=' Expr`
- `Expr` remained ambiguous between `CallExpr` and `Identifier`
- expected `rparen` while completing `ArgList`

Even simple text like this is far more helpful than a bare token mismatch.

## Teaching and debugging uses

Rule-level explanations are especially valuable for:

- onboarding new grammar authors
- debugging precedence and associativity issues
- generating tutorial material from live grammars
- tracing why a certain AST node exists at all

In short, rule explanation turns the grammar from implementation detail into inspectable knowledge.



# glrpp-reporting-errors

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



# glrpp-case-study-sql

SQL is a classic GLR case study because its grammar mixes precedence, keywords, optional clauses, and context-sensitive constructs in ways that strain simplistic parsers.

## Why SQL suits GLR

SQL contains:

- expression precedence
- many optional clause orderings
- identifiers that overlap with keywords depending on dialect
- nested query forms
- type and expression ambiguities in some dialects

GLR is a natural fit because it allows you to start from a readable grammar and refine ambiguities later.

## A small illustrative fragment

```cpp
using namespace glrpp;

const auto grammar = make_grammar(
    "SelectStmt",
    {production("SelectStmt", seq({sym(terminal("kw_select")), sym(nonterminal("SelectList")), sym(terminal("kw_from")), sym(nonterminal("TableRef"))})),
     production("SelectList", sym(terminal("identifier")) |
                               seq({sym(nonterminal("SelectList")), sym(literal(",")), sym(terminal("identifier"))})),
     production("TableRef", sym(terminal("identifier")))});
```

## Lexical strategy

A CTRE scanner can classify:

- keywords such as `SELECT`, `FROM`, `WHERE`
- identifiers
- numeric literals
- strings
- punctuation

Whether keywords should be distinct terminals or contextual identifier variants depends on your dialect goals.

## Ambiguity management

Many SQL ambiguities are semantic or dialect-driven. Examples include function-call-like forms, type names versus identifiers, and optional clause combinations. A good approach is:

1. keep the surface grammar readable
2. parse broadly
3. prune with dialect configuration and semantic knowledge

## Semantic output

A SQL AST usually wants node kinds such as:

- `select_stmt`
- `projection`
- `table_ref`
- `binary_predicate`
- `identifier`

The generic `ast_node` can prototype this quickly before a richer AST layer takes over.

## Lessons from SQL

- keyword policy matters enormously
- precedence layering pays off early
- dialect extensions should be modular grammar fragments where possible
- GLR helps most when you resist overfitting the grammar too early

SQL demonstrates the main philosophical strength of glrpp: start with the language as written, then refine with structure and context.



# glrpp-case-study-yaml

YAML is a compelling case study because it mixes indentation sensitivity, scalar ambiguity, and context-dependent structure. It is exactly the sort of language that benefits from a flexible parser architecture.

## Why YAML is difficult

YAML combines:

- indentation-based structure
- multiple scalar styles
- flow and block collections
- tags and anchors
- context-sensitive tokenization

A pure traditional lexer-parser split often feels strained here.

## Where glrpp helps

glrpp’s scannerless orientation and reader-hook flexibility make it easier to keep parsing and layout interpretation connected. You can model indentation and contextual tokenization as part of a coordinated pipeline instead of forcing everything into a rigid early token stream.

## Example fragment strategy

A YAML-like parser might separate concerns into:

- a reader or scanner layer that preserves newline and indentation information
- grammar rules for block entries, mappings, and sequences
- semantic passes that resolve scalar forms and tags

## Layout handling

Indentation-sensitive languages often need more than regex tokens. A good design may track indentation context outside the raw grammar while still feeding the grammar explicit INDENT/DEDENT-like events or structured layout metadata.

## Ambiguity and resilience

YAML tools often need to recover gracefully from incomplete documents, especially in editors. GLR-style ambiguity tolerance is useful here because partial structure is better than total failure.

## Lessons from YAML

- the reader layer matters as much as the grammar layer
- whitespace is syntax, not trivia
- scannerless or hybrid parsing is often the right mental model
- semantic normalization after parsing is essential

YAML shows why glrpp’s Unicode-aware reader and hook bridge are strategically important, not just implementation details.



# glrpp-case-study-syntax-highlighting

Syntax highlighting is not full compilation, but it still benefits from precise syntactic structure. glrpp is interesting here because it can produce richer syntax regions than a regex-only highlighter while remaining resilient in the face of incomplete code.

## Why parse for highlighting

Regex highlighters are fast and easy, but they often mis-handle:

- nested delimiters
- embedded languages
- ambiguous punctuation
- multiline constructs

A GLR-based highlighter can preserve multiple interpretations until enough context arrives, which is especially useful in editors.

## A highlighting pipeline

A practical pipeline looks like this:

1. read the source buffer
2. tokenize or hook-scan obvious lexical classes
3. parse into a forest or simplified AST
4. map nodes and tokens to highlight regions
5. resolve overlaps by precedence rules for styles

## Why ambiguity tolerance helps

Incomplete source is normal while editing. A GLR parser can often keep producing partial structure even when the text is temporarily malformed. That means the highlighter can continue giving useful output instead of collapsing into a uniform fallback color.

## Region extraction

Highlighting usually wants ranges for:

- keywords
- identifiers
- literals
- comments
- operators
- interpolated or embedded-language zones

The scanner can provide lexical ranges quickly, while the parser can refine them structurally.

## Example use case

Imagine a templating file mixing HTML, expressions, and string interpolation. A scannerless or hybrid glrpp pipeline can keep the boundaries coherent across those domains better than disconnected regex passes.

Syntax highlighting is an excellent reminder that parsing is not only for compilers. It is also for interactive, approximate, user-facing understanding.



# glrpp-case-study-semgrep

Structural code search and matching tools, like Semgrep-style systems, care more about syntactic shape than about full compilation. That makes GLR and glrpp a natural fit.

## Why structural matching benefits from GLR

Pattern languages often contain deliberate ambiguity:

- metavariables that stand in for many constructs
- wildcard holes
- relaxed punctuation or whitespace handling
- embedded pattern sublanguages

GLR parsing is comfortable with ambiguity and can keep alternative readings alive until the matcher decides which one is meaningful.

## A glrpp-based architecture

A structural matcher can use glrpp for two related grammars:

- the target-language grammar
- the pattern grammar, which extends the target language with metavariable constructs

## Pattern-specific lexical features

CTRE scanner rules are convenient for recognizing pattern tokens such as:

- `$X` metavariables
- ellipsis operators
- language-specific anchors

These can then feed a grammar that is deliberately broader than the ordinary source grammar.

## Matching workflow

1. parse the target source into a forest or AST
2. parse the structural pattern into a pattern AST
3. run tree or forest matching between pattern and target
4. report matches with source ranges

## Why forests are useful

Sometimes the pattern itself is ambiguous in a way that is acceptable. Rather than forcing one reading immediately, a forest-based approach can let the matcher try multiple interpretations.

## Lessons from structural search

- generalized parsing is useful even when you are not building a compiler
- grammar reuse matters because pattern grammars often extend source grammars
- good diagnostics are crucial when users write invalid structural patterns

Semgrep-like tools show that glrpp can be valuable anywhere syntax is a search space, not just where syntax is a correctness gate.



# glrpp-list-of-data-structures

This chapter is a compact reference to the most important data structures exposed by glrpp or conceptually central to its operation.

## DSL structures

### `glrpp::dsl::symbol`
Fields:

- `std::string name`
- `symbol_kind kind`

Role: names terminals, nonterminals, literals, or epsilon.

### `glrpp::dsl::expression`
Fields:

- `expr_kind kind`
- `symbol atom`
- `std::vector<expression> children`

Role: runtime grammar expression tree.

### `glrpp::dsl::rule`
Fields:

- `std::string lhs`
- `expression rhs`
- optional reducer action

Role: one production rule.

### `glrpp::dsl::grammar`
Fields:

- start symbol string
- vector of rules

Role: validated runtime grammar container.

## Lexical structures

### `glrpp::dsl::token`
Fields:

- `kind`, `lexeme`
- `offset`, `line`, `column`
- `bytes_consumed`, `codepoint`, `from_hook`

Role: lexical unit consumed by token-stream parsing or emitted by the scanner.

### `glrpp::dsl::scan_rule`
Fields:

- `name`
- `priority`
- `skip`
- matcher function pointer

Role: one CTRE-backed lexical rule.

### `glrpp::dsl::scanner`
Fields:

- vector of scan rules

Role: performs longest-match tokenization and hook-time classification.

## Semantic helper structures

### `glrpp::dsl::ast_node`
Fields:

- `kind`
- variant payload

Role: lightweight generic semantic tree node.

### `glrpp::dsl::ast_array`, `ast_object`
Role: aggregate payload containers for `ast_node`.

## Runtime wrapper structures

### `glrpp::glr::parser`
Holds:

- runtime grammar value
- optional scanner
- native grammar handle
- native parser handle
- optional lexer hook registry

Role: owns parser assembly and parse entry points.

### `glrpp::glr::reader`
Role: bridges raw text, UTF-16 decoding, and lexer hook events.

### `glrpp::glr::forest`
Role: wrapper over the native parse forest.

### `glrpp::glr::node`
Role: wrapper over individual forest nodes.

### `glrpp::glr::context`
Role: dynamic loader and symbol resolver for libglr.

## Meta structures

### `glrpp::meta::type_list<Ts...>`
Role: compile-time list of types.

### `glrpp::meta::fields<T>`
Role: customization point for reflection field names.

## Error structures

### `glrpp::util::parse_diagnostic`
Fields:

- `message`, `expected`, `found`
- source position
- consumed count

Role: structured error report.

### `glrpp::util::expected<Value, Error>`
Role: explicit success-or-error carrier used throughout the wrapper.

Understanding these structures gives you a mental map of the entire library: grammar front-end, lexical bridge, runtime parser, semantic helpers, and meta support.



