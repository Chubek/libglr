Parsing context is all the information that influences interpretation without necessarily being visible as a grammar symbol. In glrpp-based systems, context often lives above the raw parser but must still be threaded cleanly through scanning, semantic actions, or later analysis passes.

## Examples of context

- whether a keyword is reserved in the current mode
- current indentation or layout mode
- enabled language extensions
- symbol tables or type environments for semantic disambiguation
- diagnostic sinks and configuration knobs

## Keep syntax and context separate

As a rule, keep the grammar focused on syntax and keep application context in explicit side channels. This avoids hiding semantic policy inside grammar hacks.

## Context in actions

Actions can capture immutable context easily:

```cpp
auto make_node = glrpp::dsl::make_action([cfg](const glrpp::dsl::ast_array& children) {
  return build_node(children, cfg);
});
```

Be cautious with mutable captures. They can make parse behavior hard to test.

## Context in lexing

Context-sensitive lexing is often better modeled through mode-specific scanners or scanner wrappers than through global mutable state. That keeps the parser entry point explicit: this input is being parsed under this lexical policy.

## Context in disambiguation

Many valuable disambiguation decisions require context after parsing. Name resolution, type lookup, and language edition flags belong naturally in forest-pruning or AST-construction passes.

## Recommended architecture

A clean pipeline often looks like this:

1. parse with minimal context
2. produce forest or neutral AST
3. apply contextual disambiguation and semantic analysis
4. emit diagnostics or domain artifacts

Context is inevitable. The trick is to place it where it clarifies the pipeline instead of muddying it.
