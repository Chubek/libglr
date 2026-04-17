This chapter connects the lightweight meta helpers to the broader design of glrpp. The wrapper uses compile-time programming not as a separate subsystem, but as a support layer for DSL ergonomics and correctness.

## Design philosophy

glrpp’s metaprogramming style is intentionally conservative:

- prefer small aliases and traits over giant template frameworks
- keep runtime grammar data explicit even when compile-time helpers exist
- use concepts and type traits to improve diagnostics when practical

## Compile-time and runtime meet in the DSL

The grammar DSL is runtime data, but some of its helpers are template-driven. CTRE patterns are a prime example: they are compile-time values that generate runtime scanner behavior.

Similarly, the pipeline operator uses templates and operand traits to accept both symbols and expressions cleanly:

- detect whether an operand is a `symbol` or `expression`
- promote symbols to atomic expressions
- flatten choice nodes into one runtime expression tree

That is a perfect example of compile-time code improving runtime clarity.

## Reflection as metadata plumbing

Specializing `fields<T>` gives you a structured way to associate names with semantic record types. This can power:

- generic AST serializers
- debug dumps
- schema-aware diagnostics

## Expected and type-level design

Even `util::expected` reflects a meta-guided style. It uses type parameters to encode success and error channels explicitly, which works well with parser APIs where failure is expected and structured.

## Recommended patterns

- use type lists to describe groups of semantic node types
- use reflection names to reduce boilerplate in dumps and docs
- keep template helpers in headers small and standalone
- add tests that compile the intended patterns, not only runtime tests

## Anti-patterns

- hiding runtime grammar logic inside opaque template machinery
- forcing users to understand type-level internals for ordinary DSL use
- abusing SFINAE when a simple overload or concept would do

A good metaprogramming guide for glrpp ultimately says: use compile-time machinery to make the runtime parser easier to use, not harder to understand.
