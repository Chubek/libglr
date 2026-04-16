# glrpp — Modern C++ Header‑Only DSL Wrapper for libglr

**glrpp** is a **header‑only C++20 DSL layer** built on top of **libglr**, a fast and flexible Generalized LR parser.
It provides a **C++‑native grammar construction interface**, a **strongly typed AST system**, and a **safe RAII wrapper** for the underlying C parser.

This project is specifically designed to be easy to extend by **automation tools and LLM agents**, with a predictable directory structure, consistent naming conventions, and strict layering of responsibilities.

---

## Key Features

### 1. Header‑only C++20 frontend
All DSL, grammar, type reflection, and AST logic are implemented purely in headers under:

```
include/glrpp/
```

### 2. Generated C++ backend via SWIG
The low‑level C++ interface to libglr is **automatically generated using SWIG**.
These bindings wrap the raw C API and expose a minimal and stable C++ surface used by glrpp.

### 3. Compile‑time grammar DSL
The DSL uses:

• **Brigand** for type‑list metaprogramming
• **CTRE** for compile‑time validation (identifiers, patterns)
• **Frozen** for constexpr maps/sets
• **magic_enum** and **nameof** for diagnostics
• **Boost.PFR** for struct reflection

Grammars are written as C++ types:

```cpp
using Expr =
    rule<"Expr",
        seq<Term,
            star<seq<plus_op, Term>>>>;
```

No generators. No macros. All types.

### 4. Strongly typed AST
AST nodes can be:

• `std::variant`‑based sum types
• PFR‑reflected structs
• user‑defined node classes

### 5. Zero runtime overhead in the DSL layer
The C++ DSL compiles to constexpr metadata.
The only runtime component is the actual GLR parsing from libglr.

---

# Project Structure

```
glrpp/
├── include/glrpp/            # Public header-only API
│   ├── dsl/                  # Grammar DSL + Rules + AST
│   ├── glr/                  # C++ wrapper over SWIG-generated bindings
│   ├── meta/                 # Brigand + CTRE + Frozen + reflection helpers
│   ├── util/                 # Debug, error, expected<>
│   ├── config.hpp            # Version + feature toggles
│   └── glrpp.hpp             # Master public entry point
├── examples/                 # Calculator + JSON sample grammars
├── tests/                    # Unit tests
├── docs/                     # Doxygen config
├── cmake/                    # Install + export scripts
├── meson.build               # Alternative build system
└── configure.ac / Makefile   # Autotools support
```

This structure is **stable and machine‑navigable**, allowing LLM agents to infer where to put new functionality and where existing components live.

---

# Building glrpp

glrpp is **header‑only** except for the SWIG‑generated libglr wrapper, which must be built.

You need:

• libglr (C library)
• SWIG (C++ bindings generator)
• CMake or Meson (your choice)

## Option 1 — Build with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

## Option 2 — Build with Meson

```bash
meson setup build
meson compile -C build
sudo meson install -C build
```

## Option 3 — Autotools

```bash
./configure
make
sudo make install
```

---

# Using glrpp in Your Project

### The simplest way:

```cpp
#include <glrpp/glrpp.hpp>
```

Example grammar:

```cpp
using Expr =
    rule<"Expr",
        seq<Term,
            star<seq<plus_op, Term>>>>;
```

Example parser:

```cpp
#include <glrpp/glrpp.hpp>

int main() {
    glrpp::glr::parser p{Expr{}};
    auto result = p.parse("1 + 2 * 3");
    auto ast = result.get_ast();
    // ... work with AST ...
}
```

---

# SWIG‑Generated Bindings Explained

glrpp relies on **SWIG** to create the internal C++ wrapper over libglr.

### Why SWIG?
libglr exposes a **C API with complex pointer-based structures** for forests, contexts, and GLR nodes.
Maintaining a hand-written binding layer is error-prone. Using SWIG ensures:

• pointer ownership semantics remain stable
• the interface is consistent with upstream libglr updates
• memory safety rules are enforced uniformly
• generating bindings is repeatable and deterministic
• new GLR features can be surfaced automatically

### How SWIG interacts with glrpp

The workflow is:

1. SWIG generates `glrpp_glr_bindings.cpp` + `.hpp`.
2. glrpp’s `include/glrpp/glr/` wraps these generated bindings with:
   - RAII
   - type-safe getters
   - safe C++ API
3. The DSL layer (`include/glrpp/dsl/`) builds AST + grammar metadata.
4. The GLR backend (`include/glrpp/glr/parser.hpp`) combines them.

All generated files live **outside** the `include/glrpp/` tree to preserve header-only purity.

---

# Design Principles

glrpp follows a strict set of rules to keep layering clean and predictable:

### Rule 1 — The DSL layer is 100% compile‑time
No runtime behavior. Everything in `dsl/` is types and constexpr metadata.

### Rule 2 — The GLR layer is a thin runtime shim
Minimal logic. Only wraps SWIG-generated C++ bindings safely.

### Rule 3 — Meta layer contains *all* template metaprogramming
Wrappers for Brigand, CTRE, Frozen, nameof, magic_enum, PFR helpers live in `meta/`. The actual source code for these libraries reside in `third_party`. The build tools take care of bundling and installing these third-party libraries, and the user must not worry about that.

### Rule 4 — No cross‑layer leakage
• DSL cannot include from `glr/`
• glr/ cannot depend on dsl/ except *AST types*
• meta/ does not depend on glr/
• util/ can be used by all layers

This structure is easy for LLMs to reason about and prevents cyclic complexity.

---

# Examples

Examples are fully documented and designed to be easy for automated tools to extend.

### Calculator

```
examples/calc/
│── grammar.hpp
│── ast_printer.hpp
└── main.cpp
```

### JSON parser

```
examples/json/
│── grammar.hpp
│── ast_printer.hpp
└── main.cpp
```

---

# License

glrpp is licensed under a the permissive MIT license.

---

# Need autogenerated API docs?

Run:

```bash
doxygen docs/Doxyfile
```

---

