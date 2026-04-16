Grammar debugging is easiest when you separate lexical, structural, and semantic questions. glrpp gives you enough hooks to do that systematically.

## Start with the smallest failing case

Before inspecting a full source file, reduce the failing input to the smallest fragment that still reproduces the problem. Tiny cases reveal whether the issue is:

- token mismatch
- rule mismatch
- unintended ambiguity
- runtime loader or setup failure masquerading as a parse issue

## Debug layer by layer

### 1. Check the scanner

```cpp
auto tokens = scanner->scan("sum + 42");
for (const auto& tok : tokens.value()) {
  glrpp::util::dump(tok);
}
```

### 2. Check grammar inventory

```cpp
glrpp::util::dump(grammar);
for (const auto& t : grammar.terminals()) {
  std::cout << t.name << '
';
}
```

### 3. Parse known-good token streams

Direct token streams isolate grammar problems from lexing problems.

### 4. Inspect the forest

If parsing succeeds but interpretation is wrong, dump the forest and compare it with your expected rule structure.

## Use ambiguity to your advantage

If the forest is larger than expected, that is not just noise. It tells you where the grammar is underspecified. Add tests around exactly those constructs.

## Diagnostic formatting

`parse_diagnostic::format()` is a good default string form. In larger tools, enrich it with source excerpts, carets, and explanation of the relevant rule or token class.

## Common failure patterns

- literals in the grammar but named terminals in the token stream
- scanner rules that overlap unexpectedly and win by priority
- start symbol set to a helper nonterminal instead of the top-level rule
- Unicode byte accounting mismatches in hook-driven parsing

## Maintain debug fixtures

Keep a dedicated directory of tiny grammar fixtures and tiny source fixtures. Grammar debugging is dramatically faster when your examples are small and named after the behavior they exercise.

A good grammar-debugging workflow turns mystery into classification: lexical problem, grammar problem, ambiguity problem, or semantic problem.
