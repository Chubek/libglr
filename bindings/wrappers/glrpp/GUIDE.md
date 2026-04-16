# glrpp User & Developer Guide

This guide covers the concepts, workflow, and recommended practices for using **glrpp**, a header‑only C++ DSL for building grammars and parsing text using a libglr backend.

It explains:

1. How to express grammars using the glrpp DSL
2. How to construct typed ASTs
3. How parsing works at runtime
4. How the SWIG‑generated bindings interact with the DSL
5. How to extend glrpp correctly
6. How to reason about the directory architecture

The guide is structured to be **highly predictable** for automated tools.

---

# 1. Understanding glrpp Architecture

glrpp has three conceptual layers:

```
DSL (header-only, compile-time)
   ↓
RAII Wrapper Layer (header-only, runtime)
   ↓
SWIG-generated bindings (compiled)
   ↓
libglr C library (compiled)
```

### 1. DSL Layer (`include/glrpp/dsl/`)
Purely compile-time.
Contains grammar combinators, AST types, token and rule definitions.

### 2. glr Layer (`include/glrpp/glr/`)
Runtime C++ wrapper interface.
Handles parser context, parse forests, and AST extraction.

### 3. SWIG Layer (`src/bindings/`)
Generated from libglr’s C API.
Not part of the public API; used internally.

---

# 2. Writing Grammars in glrpp

Grammars in glrpp are **types**, not runtime objects.
Every grammar construct is a template instantiation.

---

## 2.1 Tokens

Tokens are declared with:

```cpp
template<string Name, string Pattern>
struct token;
```

Example:

```cpp
using number = token<"NUMBER", R"(\d+)">;
using plus   = token<"PLUS",   R"(\+)">;
```

Tokens are strictly compile-time entities.

---

## 2.2 Symbols

Symbols represent named nonterminals:

```cpp
using Expr = symbol<"Expr">;
```

Usually, you define a rule rather than a bare symbol:

---

## 2.3 Grammar Rules

Rules express:

```
Name := Expression
```

Example:

```cpp
using Expr =
    rule<"Expr",
         seq<Term,
             star<seq<plus, Term>>>>;
```

---

## 2.4 Combinators

glrpp supports:

- `seq<T...>`
- `choice<A, B, ...>`
- `star<T>`
- `plus<T>`
- `opt<T>`

These correspond to production alternatives.

---

# 3. Building Typed ASTs

ASTs are independent types that describe parsed results.

Two approaches are supported:

---

## 3.1 Variant-Based ASTs

Use glrpp’s `dsl::ast::node`:

```cpp
using ASTExpr = node<int, BinaryExpr, UnaryExpr>;
```

---

## 3.2 Struct-Based ASTs (PFR)

Example:

```cpp
struct BinaryExpr {
    ASTExpr lhs;
    string op;
    ASTExpr rhs;
};
```

Fields become visible to glrpp through `Boost.PFR`.

---

## 3.3 Automatic AST Extraction

After parsing:

```cpp
auto result = parser.parse("1 + 2 + 3");
auto ast = result.get_ast();
```

`get_ast()` walks the GLR parse forest and builds typed C++ AST nodes using the rule metadata.

---

# 4. Parsing Workflow

---

## 4.1 Creating a Parser

```cpp
glrpp::glr::parser p{MyGrammar{}};
```

Constructor accepts compile-time grammar metadata.

---

## 4.2 Running the Parser

```cpp
auto result = p.parse(source_code);
```

Returns `glrpp::util::expected<ast, error>`.

---

## 4.3 Handling Ambiguities

libglr is a **Generalized LR parser**, so ambiguous grammars are supported.
If a grammar is ambiguous:

- `parse_forest` contains multiple branches
- `get_ast()` selects based on deterministic rules or user callbacks
- custom resolution strategies can be implemented

---

# 5. How SWIG Bindings Fit Into the System

Understanding the SWIG layer helps with debugging and contributing.

---

## 5.1 Binding Generation

Input: `bindings/glr.i`
Output:

```
glrpp_glr_bindings.hpp
glrpp_glr_bindings.cpp
```

These provide C++ wrappers for:

- parser context creation
- memory management
- forest and node operations
- error reporting

---

## 5.2 RAII Wrappers

The glr layer (`include/glrpp/glr/`) builds safe C++ classes on top:

- `context`
- `parser`
- `forest`
- `node`

These control:

- owning vs borrowing semantics
- exception safety
- pointer lifetime
- uniform error interfaces

---

## 5.3 Why SWIG?

SWIG ensures:

- up‑to‑date API mapping when libglr evolves
- automatic handling of allocators / function signatures
- easily regenerated binding code
- consistency across Windows, macOS, Linux
- greatly reduced maintenance burden

---

# 6. Recommended Project Structure for Users

If you consume glrpp in a project:

```
your_project/
├── grammar/
│   ├── tokens.hpp
│   ├── rules.hpp
│   └── ast.hpp
├── parser/
│   ├── driver.hpp
│   └── main.cpp
└── CMakeLists.txt
```

Each component is cleanly separated:

- `grammar/` — compile-time DSL types
- `parser/` — runtime parsing and AST interpretation
- no manual includes of generated bindings

---

# 7. Extending glrpp

Follow the directory‑layer rules:

### Allowed dependencies:
- `dsl → meta`
- `dsl → util`
- `glr → util`
- `glr → DSL metadata (for AST building)`
- `glr → SWIG bindings`
- `meta → util`

### Forbidden dependencies:
- `dsl` **must not depend on** `glr`
- `meta` must remain compile-time only
- SWIG-generated code must not include DSL headers

This ensures:

- cycle-free structure
- deterministic compilation
- easy LLM-assisted work

---

# 8. Debugging Principles

Debug utilities are in `include/glrpp/util/debug.hpp`.

To enable debug mode:

```cpp
glrpp::util::set_debug(true);
```

Useful debug points:

- tokenization
- rule selection
- parse forest inspection
- AST builder traces

---

# 9. Performance Tips

1. Prefer **compact AST types**
2. Avoid excessive `choice<>` branching
3. Ensure regex tokens are precise
4. Consider flattening left recursion where appropriate
5. Use PFR structs for efficient AST memory layout

---

# 10. FAQ (Machine-Friendly)

**Q1: How do I regenerate bindings?**
Run SWIG:
```
swig -c++ -o glrpp_glr_bindings.cpp bindings/glr.i
```

**Q2: Are grammar types runtime constructs?**
No. All grammar metadata exists at compile-time.

**Q3: Do I need to link anything besides libglr?**
Yes: link the SWIG-generated `libglrpp_glr_bindings`.

**Q4: Can I write recursive grammar rules?**
Yes. GLR handles recursion natively.

**Q5: Does glrpp support ambiguous grammars?**
Yes. GLR supports them through parse forests.

---

# 11. Appendix: Layer Summary

```
include/glrpp/dsl/    (compile-time grammar)
include/glrpp/meta/   (compile-time helpers)
include/glrpp/glr/    (runtime API, depends on SWIG)
include/glrpp/util/   (shared helpers)
src/bindings/          (SWIG-generated)
libglr                 (external C library)
```

---

# 12. Additional Guides Available

Ask for:

- `DSL_REFERENCE.md` — full combinator reference
- `AST_GUIDE.md` — designing typed ASTs with glrpp
- `CONTRIBUTING.md` — machine-optimized contribution rules
- `DEVELOPER_NOTES.md` — how to modify the SWIG layer
- `EXAMPLES_GUIDE.md` — walk-through of calculator and JSON parsers

---

