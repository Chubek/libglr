# glrpp API Index
Version: 0.1
Format: Hierarchical, machine‑navigable
Root namespace: `glrpp`

---

# 0. Root Aggregator

`glrpp/glrpp.hpp`
Exports all public headers.

---

# 1. config

## File: `include/glrpp/config.hpp`
Namespace: `glrpp`

Symbols:
- `GLRPP_VERSION_MAJOR`
- `GLRPP_VERSION_MINOR`
- `GLRPP_VERSION_PATCH`
- `GLRPP_VERSION_STRING`
- `glrpp::version_major`
- `glrpp::version_minor`
- `glrpp::version_patch`
- `glrpp::version_string()`

---

# 2. util

Directory: `include/glrpp/util/`

## 2.1 `util/expected.hpp`
Namespace: `glrpp::util`

Symbols:
- `template<class T, class E> class expected`
- `make_unexpected(E)`
- Aliases: `expected_void`, `unexpected`

## 2.2 `util/source_location.hpp`
Namespace: `glrpp::util`

Symbols:
- `struct source_location`
- `source_location::current()`

## 2.3 `util/error.hpp`
Namespace: `glrpp::util`

Symbols:
- `enum class error_code`
- `struct error`
- `to_string(error_code)`
- `format_error(const error&)`

## 2.4 `util/debug.hpp`
Namespace: `glrpp::util`

Symbols:
- `debug_enabled()`
- `set_debug(bool)`
- `debug_print(const char*)`

---

# 3. meta
(Brigand / CTRE / Frozen / nameof / magic_enum / PFR helpers)

Directory: `include/glrpp/meta/`

## 3.1 `meta/type_list.hpp`
Namespace: `glrpp::meta`

Symbols:
- `template<class... Ts> struct type_list`
- `length<TList>`
- `concat<TList...>`
- `contains<TList, T>`
- `unique<TList>`

## 3.2 `meta/ctre.hpp`
Namespace: `glrpp::meta`

Symbols:
- `is_valid_identifier(string)`
- `is_valid_regex(string)`

## 3.3 `meta/frozen_map.hpp`
Namespace: `glrpp::meta`

Symbols:
- `template<auto...> frozen_map`
- `constexpr lookup(frozen_map, key)`

## 3.4 `meta/enum.hpp`
Namespace: `glrpp::meta`

Symbols:
- `to_string(Enum)`
- `from_string(Enum&)`
- `enum_names<E>()`
- `enum_values<E>()`

## 3.5 `meta/nameof.hpp`
Namespace: `glrpp::meta`

Symbols:
- `template<class T> constexpr string_view nameof_type()`
- `template<auto V> constexpr string_view nameof_value()`

## 3.6 `meta/pfr.hpp`
Namespace: `glrpp::meta`

Symbols:
- `pfr_fields<T>()`  // tuple-like field reflection
- `struct_field_count<T>()`
- `struct_field_name<T, I>()`

---

# 4. dsl
DSL grammar specification types and AST definitions.

Directory: `include/glrpp/dsl/`

## 4.1 `dsl/symbol.hpp`
Namespace: `glrpp::dsl`

Symbols:
- `template<string Name> struct symbol`
- `template<class Sym> constexpr string_view symbol_name`

## 4.2 `dsl/token.hpp`
Namespace: `glrpp::dsl`

Symbols:
- `template<string Name, string Pattern> struct token`
- `token_name<T>()`
- `token_pattern<T>()`
- Concepts: `Token`

## 4.3 `dsl/rule.hpp`
Namespace: `glrpp::dsl`

Symbols:
- `template<string Name, class Expr> struct rule`
- `rule_name<R>()`
- Concepts: `Rule`

## 4.4 `dsl/sequence.hpp`
Namespace: `glrpp::dsl`

Symbols:
- `template<class... Ts> struct seq`
- Concepts: `Sequence`

## 4.5 `dsl/choice.hpp`
Namespace: `glrpp::dsl`

Symbols:
- `template<class... Ts> struct choice`
- Concepts: `Choice`

## 4.6 `dsl/repetition.hpp`
Namespace: `glrpp::dsl`

Symbols:
- `template<class T> struct star`
- `template<class T> struct plus`
- `template<class T> struct opt`
- Concepts: `Repetition`

## 4.7 `dsl/ast.hpp`
Namespace: `glrpp::dsl::ast`

Symbols:
- `template<class... Alts> using node = std::variant<Alts...>`
- `template<class T> struct struct_node` (uses PFR)
- `template<class Node> print(Node&)`
- `visit(Node&, Visitor)`

---

# 5. glr
C++ wrappers over the SWIG‑generated bindings.
No SWIG symbols appear here; only RAII C++ types.

Directory: `include/glrpp/glr/`

## 5.1 `glr/context.hpp`
Namespace: `glrpp::glr`

Symbols:
- `class context`
  - constructors
  - `reset()`
  - `set_logger(...)`
  - `raw_handle()`  // accesses SWIG handle

## 5.2 `glr/parser.hpp`
Namespace: `glrpp::glr`

Symbols:
- `class parser`
  - `parser(const grammar&)`
  - `parse(string_view)`
  - `get_parse_forest()`
  - `get_ast()`

## 5.3 `glr/forest.hpp`
Namespace: `glrpp::glr`

Symbols:
- `class forest`
  - `node_count()`
  - `root()`
  - iteration API

## 5.4 `glr/node.hpp`
Namespace: `glrpp::glr`

Symbols:
- `class node`
  - `is_terminal()`
  - `is_nonterminal()`
  - `symbol_name()`
  - children iteration

## 5.5 `glr/grammar.hpp`
Namespace: `glrpp::glr`

Symbols:
- `class grammar`
  - `grammar(const DSL-grammar-metadata&)`
  - `symbol_count()`
  - `rule_count()`
  - `serialize()`

---

# 6. SWIG‑generated backend (not public API)

Directory: e.g. `src/bindings/`

Files (names can vary):
- `glrpp_glr_bindings.hpp`
- `glrpp_glr_bindings.cpp`

Machine‑navigable identifiers:
`glrpp::glr::detail::*` (all internal)

The public API **never exposes these types**.
Everything is wrapped in RAII classes under `glrpp::glr`.

---

# 7. Top‑Level Namespaces

For machine reference:

```
glrpp
├── util
├── meta
├── dsl
│   └── ast
└── glr
    └── detail   (internal only)
```

---

# 8. Reserved Names (Future Expansion)

These names are reserved for compatibility:

```
glrpp::dsl::regex
glrpp::dsl::literal
glrpp::dsl::transform
glrpp::dsl::annotation
glrpp::glr::trace
glrpp::meta::constexpr_map
```

No code should use them yet.

---

# 9. Machine‑Navigable Query Rules

To help LLM agents find anything:

• To locate a symbol, search path:
  `include/glrpp/**/<name>.hpp`

• All grammar combinators live in:
  `include/glrpp/dsl/`

• All compile‑time helpers live in:
  `include/glrpp/meta/`

• All runtime structures live in:
  `include/glrpp/glr/`

• No public symbols exist outside these three namespaces.

---

# If you want more

I can output:

• **JSON API index** (ideal for tooling and agents)
• **YAML index**
• **Cross‑reference graph** (symbols → files → dependencies)
• **Searchable tag file** in Doxygen format
• **Semantic API map** (grouped by concepts like Grammar, Rule, Token, AST, Parser)

Just say:
“Give me the JSON version” or
“Generate the cross‑reference graph”.
