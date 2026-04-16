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
