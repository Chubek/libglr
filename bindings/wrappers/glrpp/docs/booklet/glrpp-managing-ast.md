Once parsing succeeds, most real applications want an AST that is simpler and more stable than a raw parse forest. Managing that AST well is a major part of using glrpp effectively.

## Generic versus custom ASTs

glrpp provides a generic `dsl::ast_node` for convenience. It is ideal for:

- tests
- examples
- quick prototypes
- debug dumps

For large systems, a custom typed AST may still be the right choice. The generic AST is a stepping stone, not a prison.

## Ownership model

`ast_node` owns its content through ordinary C++ value semantics. Arrays and objects nest recursively, so copying a large tree can become expensive. Prefer moving nodes or storing them in larger semantic objects when performance matters.

## Mutability

A good rule is to keep AST construction mutable and AST consumption mostly immutable. Build nodes freely while reducing or rewriting, then hand later passes a stable tree.

## Visitor patterns

Even with the generic AST, visitors are useful. Simple examples include:

- pretty-printers
- evaluators
- symbol collectors
- diagnostics annotators

Because the generic AST uses a variant payload, a visitor usually dispatches first on `kind`, then on the active payload alternative.

## Normalization passes

Common AST normalization passes include:

- flattening left- or right-recursive chains into arrays
- discarding punctuation-only nodes
- converting numeric strings to integers or floats
- labeling nodes with source ranges stored externally

## AST and source mapping

Do not lose source information too early. Even if the AST node itself stays lightweight, keep a side table from semantic nodes to source spans. This makes diagnostics, refactoring tools, and formatters far easier to build later.

## Example generic AST construction

```cpp
glrpp::dsl::ast_node lhs{"identifier", std::string{"x"}};
glrpp::dsl::ast_node rhs{"number", std::int64_t{42}};
glrpp::dsl::ast_node assign{"assign", glrpp::dsl::ast_array{lhs, rhs}};
```

AST management is where syntax becomes the long-lived internal representation of your language. Treat it as a first-class design concern.
