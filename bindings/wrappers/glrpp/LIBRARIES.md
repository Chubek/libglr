# LIBRARIES.md

This document explains the purpose of each third‑party library used by **glrpp**, how it fits into the architecture, and the conventions for using it inside the codebase.

glrpp is designed as a **header‑only C++ DSL layer on top of libglr**, emphasizing compile‑time safety, zero‑overhead abstractions, and minimal runtime dependencies. The libraries below support those goals by enabling reflection‑like utilities, compile‑time parsing helpers, and efficient metadata handling.

---

# Overview

The glrpp dependency set is intentionally small and focused. Each library serves a specific role:

- brigand — template metaprogramming backbone
- compile-time-regular-expressions (CTRE) — compile‑time regex validation
- frozen — constexpr containers
- magic_enum — enum reflection utilities
- nameof — compile‑time name extraction
- pfr — reflection-like struct access

These libraries are bundled in `third_party/` to ensure stable builds and avoid external package manager requirements.

---

# brigand

Repository: https://github.com/edouarda/brigand

## Purpose

Brigand provides the **template metaprogramming infrastructure** used throughout glrpp.

It replaces traditional recursive template metaprogramming with a cleaner functional-style meta API.

## How glrpp uses it

Brigand is used to manipulate **type lists representing grammar components**.

Typical use cases include:

- grammar rule composition
- compile‑time validation of rule structures
- mapping grammar elements to parser states
- filtering and transforming type lists

Examples inside glrpp:

- constructing lists of terminals and nonterminals
- checking rule compatibility
- computing grammar traits

Example conceptually:

```
using rule_symbols = brigand::list<identifier, equals, expression>;
```

Brigand algorithms allow transforming and analyzing such lists entirely at compile time.

## Why brigand

Alternatives like Boost.MP11 or Boost.Hana were considered. Brigand was chosen because:

- extremely lightweight
- header‑only
- minimal compile‑time overhead
- simple type‑list oriented API

---

# compile-time-regular-expressions (CTRE)

Repository: https://github.com/hanickadot/compile-time-regular-expressions

## Purpose

CTRE enables **regex parsing and validation at compile time**.

This allows DSL syntax rules to be validated during compilation instead of runtime.

## How glrpp uses it

CTRE is primarily used in the **DSL layer**.

Typical uses:

- validating identifiers
- validating literal syntax
- verifying token formats
- optional DSL static assertions

Example conceptually:

```
ctre::match<"[a-zA-Z_][a-zA-Z0-9_]*">(identifier)
```

If the string does not match, the compilation fails.

## Benefits

- zero runtime cost
- early syntax validation
- expressive DSL constraints

CTRE is not used for full grammar parsing — that responsibility belongs to **libglr**.

---

# frozen

Repository: https://github.com/serge-sans-paille/frozen

## Purpose

Frozen provides **constexpr maps and sets**.

These allow building lookup tables that are fully resolved at compile time.

## How glrpp uses it

Frozen is used for:

- keyword tables
- token lookup
- DSL symbol mapping
- static grammar metadata

Example conceptually:

```
constexpr auto keywords = frozen::make_unordered_map<std::string_view, token>({
    {"if", token::if_kw},
    {"else", token::else_kw},
});
```

The resulting lookup has **zero runtime initialization cost**.

## Benefits

- no dynamic memory
- fast lookups
- deterministic compile‑time structures

---

# magic_enum

Repository: https://github.com/Neargye/magic_enum

## Purpose

magic_enum provides **compile‑time reflection for enums**.

It allows converting between enums and strings without manual tables.

## How glrpp uses it

magic_enum is used for:

- token names
- grammar symbol names
- debugging output
- error diagnostics

Example:

```
magic_enum::enum_name(token::identifier)
```

returns:

```
"identifier"
```

This is extremely useful when reporting parser errors or generating debug traces.

## Benefits

- eliminates manual enum‑string mappings
- constexpr support
- zero runtime overhead

---

# nameof

Repository: https://github.com/Neargye/nameof

## Purpose

nameof extracts **type names, variable names, and enum names at compile time**.

It is mainly used for diagnostics and debugging.

## How glrpp uses it

nameof is used when printing:

- AST node type names
- grammar rule names
- DSL object names

Example:

```
NAMEOF_TYPE(expression_node)
```

This enables informative error messages such as:

```
Unexpected symbol while parsing expression_node
```

## Benefits

- compile‑time reflection-like naming
- no manual string definitions
- helpful debugging output

---

# pfr (Precise Function Reflection)

Repository: https://github.com/boostorg/pfr

## Purpose

PFR enables **reflection-like access to struct fields without macros or annotations**.

It works by treating aggregates as tuple-like structures.

## How glrpp uses it

PFR is used for mapping parsed structures into user-defined C++ types.

Typical use cases:

- AST node introspection
- automatic serialization
- converting parse results into user structs

Example conceptually:

```
boost::pfr::for_each_field(node, visitor);
```

This allows generic operations on AST nodes without manual field enumeration.

## Benefits

- no macros
- no intrusive annotations
- works with plain C++ aggregates

---

# Design Philosophy

The selected libraries follow several principles:

- header‑only
- minimal dependencies
- compile‑time oriented
- non‑intrusive to user types
- widely used and stable

Each dependency fills a specific gap in the C++ language:

| Feature | Library |
|-------|--------|
| Template metaprogramming | brigand |
| Compile-time regex | CTRE |
| Constexpr containers | frozen |
| Enum reflection | magic_enum |
| Type name extraction | nameof |
| Struct reflection | pfr |

Together they provide the **compile‑time infrastructure necessary for glrpp's DSL and parser interfaces** without introducing heavy frameworks.

---

# Dependency Policy

glrpp follows these rules for third‑party libraries:

1. All dependencies must be **header‑only**.
2. They are **vendored in `third_party/`**.
3. They must compile with **C++20 or newer**.
4. They must not introduce runtime dependencies.

This keeps glrpp portable and easy to embed in other projects.

---

# Future Additions

Additional libraries may be considered if they satisfy the same philosophy, particularly in areas such as:

- constexpr utilities
- compile‑time reflection
- parser tooling

However, the project intentionally keeps the dependency list minimal.
