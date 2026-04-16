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
