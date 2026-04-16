A parse forest is the right place to understand ambiguity because it records all surviving interpretations compactly. Disambiguation at the forest layer means inspecting or pruning that structure after the parser has done its generalized work.

## Why forests matter

Forests separate recognition from commitment. The parser answers "what parses are possible?" A disambiguation pass answers "which parse do I want for this application?"

This separation is valuable in tools such as:

- IDEs that prefer partial structure over early failure
- structural matchers that intentionally exploit ambiguity
- language servers that need resilient parse products

## Forest inspection patterns

Typical operations include:

- enumerate roots
- recursively inspect child node names
- detect ambiguous subtrees by node multiplicity or alternative structure
- choose a preferred subtree according to domain rules

The existing `util::dump` facility is a simple but useful starting point.

## Pruning strategies

Common pruning policies include:

- prefer the parse with the fewest error productions or recovery edges
- prefer the parse with the highest precedence interpretation
- prefer keyword interpretations over identifiers in certain contexts
- prefer shallower or deeper derivations depending on the domain

## Structural versus semantic pruning

Structural pruning relies only on forest shape. Semantic pruning consults symbol tables, type information, or external context. Both are valid; the important design question is whether the grammar should know the rule or whether a later phase should.

## Example scenario

Suppose an input fragment could be parsed as either a generic type application or a less-than comparison. A GLR grammar can keep both readings alive. Later, name resolution may show that the left side is a type constructor, at which point the forest can be pruned confidently.

## Practical implementation advice

- keep pruning deterministic and explainable
- log why a branch was rejected in development builds
- avoid mutating the original forest unless ownership and sharing rules are clear
- consider producing a semantic AST from the chosen branch rather than destructively editing forest nodes

A forest is not a nuisance to eliminate; it is the evidence needed to make a principled decision later.
