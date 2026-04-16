In glrpp, nodes are the units of structure you inspect after parsing. They live inside a parse forest and represent recognized symbols, derivations, or ambiguous alternatives depending on the runtime shape.

## Trees versus forests

A tree assumes there is one correct parse. A forest records all surviving parse structures compactly. Because glrpp is built on GLR parsing, the forest abstraction is primary and node inspection happens inside that larger structure.

## The wrapper types

The main runtime-facing types are:

- `glrpp::glr::forest`
- `glrpp::glr::node`

A forest yields its root nodes, and each node can reveal its name and children.

## Node identity

A node corresponds to a native libglr node handle plus wrapper metadata. Identity is therefore not merely textual. Two nodes with the same symbol name may still represent different spans or derivations.

## Parenting and shared substructure

Because the parse product is a forest, substructures may be shared. Conceptually, a node can participate in multiple higher-level derivations. This is one reason forests are more memory-efficient than naively materializing every tree separately.

## Inspecting nodes

The debug utilities make node inspection straightforward:

```cpp
for (const auto& root : forest.roots()) {
  glrpp::util::dump(root);
}
```

The dump routine recursively prints node names with indentation. That is enough for smoke tests and initial debugging.

## Node names

Node names usually come from grammar symbols. If your grammar names terminals and nonterminals clearly, your forests will read clearly too. This is another reason to invest in good symbol naming early.

## Nodes versus AST values

Do not confuse parse-forest nodes with semantic AST nodes. glrpp also provides a generic `dsl::ast_node` structure for semantic work, but that is a separate layer. Parse nodes are syntactic facts; AST nodes are your chosen interpretation of those facts.

## Practical advice

- use forest nodes to debug grammar behavior
- use semantic AST nodes to build language tools
- expect shared structure when ambiguity exists
- never assume a node name alone determines meaning

The parser proves what was syntactically possible. Nodes are how that proof becomes inspectable.
