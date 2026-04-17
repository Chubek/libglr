GLR parsing happily records ambiguity, but production parsers rarely want to leave every ambiguity unresolved forever. Disambiguation in glrpp should be viewed as a layer on top of the grammar DSL rather than a rejection of it.

## Sources of ambiguity

Common ambiguity sources include:

- binary operators without precedence rules
- dangling-else style grammar shapes
- overlapping lexical classes
- grammar shortcuts such as broad `Expr` or `Type` categories

GLR lets you write these naturally first. Disambiguation then narrows the forest.

## DSL-level strategies

Even without a dedicated precedence DSL yet, you can express many preferences structurally:

- factor precedence levels into separate nonterminals
- isolate optional suffixes into helper rules
- distinguish lexical categories early where that improves clarity

Example precedence layering:

```cpp
production("Expr", sym(nonterminal("Add")));
production("Add", sym(nonterminal("Mul")) |
                  seq({sym(nonterminal("Add")), sym(literal("+")), sym(nonterminal("Mul"))}));
production("Mul", sym(nonterminal("Atom")) |
                  seq({sym(nonterminal("Mul")), sym(literal("*")), sym(nonterminal("Atom"))}));
```

## Post-parse filters

Sometimes the right move is to keep the grammar broad and filter the forest after parsing. This is especially attractive when the constraint is semantic rather than syntactic, such as type-directed interpretation or contextual keyword acceptance.

## Associativity

Associativity is often easier to encode by rule structure than by annotations. Left-associative addition, for example, naturally emerges from a left-recursive `Add` rule. Right-associative constructs can be expressed analogously.

## Lexer-aware disambiguation

A CTRE scanner plus hook bridge can resolve some ambiguities before the grammar sees them. Examples include:

- distinguishing identifiers from keywords
- preferring longest punctuation tokens
- recognizing numeric literal forms precisely

## Recommended workflow

1. write the clearest natural grammar you can
2. observe the forest shape on representative ambiguous inputs
3. decide whether the ambiguity is lexical, structural, or semantic
4. resolve it at the cheapest layer that preserves clarity

Disambiguation is most effective when it clarifies intent rather than merely suppressing parser conflicts.
