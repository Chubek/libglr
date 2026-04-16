# glrpp

**glrpp** is a header-only C++20 DSL wrapper for `libglr`, providing:

- Compile-time grammar definition
- Type-safe rule construction
- Zero-overhead abstractions
- Optional SWIG-generated C bindings

---

## Architecture

glrpp is divided into:

- `glrpp::dsl` — grammar DSL
- `glrpp::meta` — template metaprogramming utilities
- `glrpp::glr` — libglr bindings
- `glrpp::util` — helper utilities

---

## Example
```cpp
using namespace glrpp::dsl;

auto grammar =
rule("expr") =
term("expr") + token("+") + term("term")
| term("term");
