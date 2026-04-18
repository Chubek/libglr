Semantic processing starts when syntax becomes meaning. glrpp reserves space for semantic actions in rules and also provides a lightweight generic AST model for applications that want a neutral tree representation.

## The semantic building blocks

The wrapper currently exposes:

- `dsl::action<Fn>` for callable semantic actions
- `make_action(fn)` for convenience
- `identity` as a trivial action
- `dsl::ast_node`, `ast_array`, and `ast_object` as generic semantic containers

## Generic AST nodes

`ast_node` stores:

- `kind`: a descriptive node tag
- `value`: a variant of null, string, integer, double, bool, array, or object

This makes it easy to build examples and tests without committing to a custom domain model immediately.

Example:

```cpp
glrpp::dsl::ast_node number{"number", std::int64_t{42}};
glrpp::dsl::ast_node list{"args", glrpp::dsl::ast_array{number}};
```

## Why actions matter

A parser proves structure, but most applications need more:

- evaluators need typed values
- compilers need rich AST nodes
- linters need source-linked semantic facts
- query tools need normalized syntax trees

Semantic actions are the bridge from parse recognition to those richer products.

## A good beginner workflow

1. get the grammar parsing correctly
2. inspect the forest on representative inputs
3. prototype semantic output using `ast_node`
4. only then move to custom domain types if needed

## Action signatures

The current rule type reserves an optional reducer of the rough shape:

```cpp
action<std::function<ast_node(const ast_array&)>>
```

That suggests a simple model: each reduction receives child semantic values and returns a new AST node.

## Example design

For an arithmetic grammar, a semantic layer might map:

- `number` token -> `ast_node{"number", 42}`
- `Add` rule -> `ast_node{"add", ast_array{lhs, rhs}}`

Even if the runtime wiring evolves, the conceptual structure is stable and worth designing early.

Semantic basics are about keeping syntax and meaning separate long enough to reason about both clearly.
