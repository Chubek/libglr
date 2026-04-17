# glrpp: Modern C++ Wrapper for GLR Parsing

Welcome to the **glrpp** documentation — a header-only C++20 library that brings the power of Generalized LR (GLR) parsing to modern C++ with an expressive, type-safe API.

---

## What is glrpp?

**glrpp** is a C++ wrapper around **libglr**, a high-performance GLR parsing engine. Unlike traditional LR parsers that fail on ambiguous grammars, GLR parsers handle ambiguity gracefully by forking execution paths and returning a **parse forest** — a compact representation of all possible parse trees.

### Key Features

- **Header-only**: No compilation required, just include and go
- **C++20 native**: Leverages concepts, ranges, and modern metaprogramming
- **Type-safe DSL**: Define grammars directly in C++ with compile-time validation
- **Parse forests**: Inspect, traverse, and disambiguate ambiguous parses
- **Semantic actions**: Attach C++ lambdas to grammar rules for AST construction
- **Flexible lexing**: Integrate custom lexers or use the built-in scannerless mode
- **Error recovery**: Rich diagnostics and error reporting hooks

---

## Quick Start
```cpp
#include <glrpp/glr.hpp>

using namespace glrpp;

// Define a simple arithmetic grammar
const auto grammar = make_grammar(
rule("expr") = "expr" + "+" + "term" | "term",
rule("term") = "term" + "*" + "factor" | "factor",
rule("factor") = "(" + "expr" + ")" | "NUMBER"
);

// Parse input
auto forest = parse(grammar, "2 + 3 * 4");

// Walk the parse forest
for (auto& root : forest.roots()) {
std::cout << "Parse tree: " << root.symbol_name() << "\n";
// ... traverse children, build AST, etc.
}

---

## Documentation Structure

### Getting Started
- @subpage glrpp-installation — Build requirements and setup
- @subpage glrpp-setting-up — Toolchain configuration
- @subpage glrpp-basic-usage — Your first parser in 5 minutes
- @subpage glrpp-what-is-glr-parsing — Understanding GLR algorithms

### Core Concepts
- @subpage glrpp-specifying-grammars — Grammar definition syntax
- @subpage glrpp-using-native-dsl — The glrpp DSL reference
- @subpage glrpp-what-are-nodes — Parse forest structure
- @subpage glrpp-managing-symbols — Symbol tables and scoping
- @subpage glrpp-parser-object — The parser lifecycle

### Lexing & Input
- @subpage glrpp-hooking-up-lexer — Integrating external lexers
- @subpage glrpp-lexer-hooks — Custom tokenization callbacks
- @subpage glrpp-reading-tokens — Token stream API
- @subpage glrpp-reading-files — File I/O utilities
- @subpage glrpp-scannerless-parser — Character-level parsing

### Semantic Actions & AST
- @subpage glrpp-semantic-basics — Attaching actions to rules
- @subpage glrpp-actions-basics — Action API reference
- @subpage glrpp-semantic-actions — Advanced action patterns
- @subpage glrpp-managing-ast — AST construction strategies
- @subpage glrpp-managing-context — Contextual parsing state

### Disambiguation & Forests
- @subpage glrpp-disambiguation-forests — Resolving ambiguity
- @subpage glrpp-disambiguation-dsl — Disambiguation DSL
- @subpage glrpp-rewriting-grammars — Grammar transformations
- @subpage glrpp-rewrite-dsl — Rewrite rule syntax

### Advanced Topics
- @subpage glrpp-forking-parsers — Parallel parse paths
- @subpage glrpp-managing-stack — GLR stack internals
- @subpage glrpp-managing-dependencies — Grammar modularity
- @subpage glrpp-configuration — Runtime configuration
- @subpage glrpp-debugging-grammars — Debugging tools and techniques
- @subpage glrpp-reporting-errors — Error handling and recovery

### Metaprogramming
- @subpage glrpp-metaprogramming-basics — Template metaprogramming primer
- @subpage glrpp-metaprogramming-guide — Advanced metaprogramming patterns
- @subpage glrpp-advanced-metaprogramming — Type-level grammar manipulation
- @subpage glrpp-compile-time-regex — Compile-time regex integration
- @subpage glrpp-handling-reflections — C++ reflection support

### Utilities
- @subpage glrpp-string-utils — String manipulation helpers
- @subpage glrpp-utf16-conversion — Unicode handling
- @subpage glrpp-pipeline-operator — Functional composition
- @subpage glrpp-list-of-data-structures — Core data structures reference

### Case Studies
- @subpage glrpp-case-study-sql — Parsing SQL with glrpp
- @subpage glrpp-case-study-yaml — YAML parser implementation
- @subpage glrpp-case-study-semgrep — Pattern matching for code search
- @subpage glrpp-case-study-syntax-highlighting — Building a syntax highlighter

### Reference
- @subpage glrpp-native-dsl-specs — Complete DSL specification
- @subpage glrpp-explaining-rules — Rule semantics deep dive
- @subpage glrpp-explaining-files — Project structure
- @subpage glrpp-hooking-the-reader — Reader interface

---

## Architecture Overview


┌─────────────────────────────────────┐
│         Your C++ Code               │
│  (Grammar DSL + Semantic Actions)   │
└──────────────┬──────────────────────┘
│
▼
┌─────────────────────────────────────┐
│          glrpp (Header-only)        │
│  • Type-safe grammar builder        │
│  • Forest traversal API             │
│  • Action dispatch                  │
└──────────────┬──────────────────────┘
│ (via libltdl)
▼
┌─────────────────────────────────────┐
│       libglr (Native Runtime)       │
│  • GLR parsing engine               │
│  • Graph-structured stack           │
│  • Shared packed parse forest       │
└─────────────────────────────────────┘

glrpp provides a zero-cost abstraction over libglr's C API, exposing parse forests as modern C++ objects with RAII semantics and iterator support.

---

## System Requirements

- **Compiler**: GCC 10+, Clang 12+, or MSVC 2022+ with C++20 support
- **Runtime**: libglr.so (dynamically loaded via libltdl)
- **Headers**: `bindings/wrappers/glrpp/include` and `third_party/`
- **Optional**: Pandoc (for booklet generation), Doxygen (for API docs)

See @ref glrpp-installation for detailed setup instructions.

---

## Design Philosophy

glrpp embraces modern C++ idioms:

- **Zero-cost abstractions**: No runtime overhead compared to raw libglr
- **Compile-time safety**: Grammar errors caught at compile time when possible
- **Expressive DSL**: Grammars read like BNF notation
- **Composability**: Grammars are first-class values that can be combined
- **Extensibility**: Hook points for custom lexers, actions, and error handlers

---

## Example: JSON Parser

cpp
const auto json_grammar = make_grammar(
rule("value") = 
"object" | "array" | "STRING" | "NUMBER" | "true" | "false" | "null",

rule("object") = 
"{" + "members" + "}" | "{" + "}",

rule("members") = 
"pair" | "members" + "," + "pair",

rule("pair") = 
"STRING" + ":" + "value",

rule("array") = 
"[" + "elements" + "]" | "[" + "]",

rule("elements") = 
"value" | "elements" + "," + "value"
);

// Attach semantic actions
auto parser = make_parser(json_grammar)
.on("pair", [](auto& ctx) {
auto key = ctx.child(0).text();
auto val = ctx.child(2).value();
return json_object{key, val};
})
.on("array", [](auto& ctx) {
return json_array{ctx.children() | collect_values};
});

auto result = parser.parse(R"({"name": "glrpp", "version": 1})");

---

## Community & Support

- **Source**: [GitHub Repository](#) *(adjust link as needed)*
- **Issues**: Report bugs and feature requests on GitHub
- **License**: *(specify your license here)*

---

## Navigation Tips

- Use the **Modules** tab to browse topics by category
- Check **Classes** for API reference of core types like `glrpp::glr::forest` and `glrpp::glr::node`
- See **Files** for header organization
- Start with @ref glrpp-basic-usage if you're new to GLR parsing

---

**Ready to parse?** Head to @ref glrpp-installation to get started, or dive into @ref glrpp-basic-usage for a hands-on tutorial.


This mainpage gives you:

- **Clear value proposition** upfront
- **Quick start snippet** to hook readers immediately  
- **Structured navigation** linking to all your markdown docs via `@subpage`
- **Architecture diagram** showing the layering
- **Design philosophy** section for the "why"
- **Practical example** (JSON parser) showing real-world usage
- **System requirements** and community links

Adjust the GitHub link, license, and any other project-specific details as needed. The `@subpage` directives will automatically generate a clickable table of contents in Doxygen's HTML output.