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
