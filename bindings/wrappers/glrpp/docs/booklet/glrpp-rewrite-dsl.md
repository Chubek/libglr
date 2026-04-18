A rewrite DSL is a natural companion to a grammar DSL. Even if your current parser project starts with recognition only, you will eventually want declarative ways to transform syntax trees or normalize grammar forms.

## What rewriting means here

There are two broad rewrite targets in a glrpp-based system:

- grammar rewrites, which transform grammar definitions before parser construction
- tree rewrites, which transform parse results or semantic ASTs after parsing

A rewrite DSL gives those transformations first-class structure rather than burying them in ad hoc loops.

## Desirable properties

A useful rewrite language should make these ideas easy to express:

- pattern matching on node kinds or rule names
- replacement with new node structures
- recursive descent with stop conditions
- local normalization rules such as flattening associative chains

## Example conceptual syntax

Even if you implement rewrites in ordinary C++ first, think in these terms:

```cpp
rewrite("ParenExpr", [](const ast_array& children) {
  return children.front();
});
```

Or at the grammar level:

```cpp
rewrite_rule("List", expand_ebnf_repetition);
```

## Why declarative rewrites help

Declarative rules are easier to review than arbitrary mutation code. They also support logging, debugging, and composition much more naturally.

## Good early rewrite targets

- remove redundant grouping nodes
- collapse comma-separated suffix chains into flat arrays
- canonicalize keyword spellings or token categories
- desugar convenience grammar forms into core forms

## Relationship to the parser

Rewriting should not fight the parser. Let the parser recognize generously, then let rewrite passes simplify, normalize, or annotate the result. That separation keeps grammars readable and transformations explicit.

A rewrite DSL is valuable because parsing is usually only the first half of understanding a language.
