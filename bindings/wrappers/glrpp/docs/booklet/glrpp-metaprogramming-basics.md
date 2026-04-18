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
