Semantic actions are the first layer where a grammar stops merely recognizing shapes and starts producing meaning. In glrpp, the action model is intentionally simple so that users can begin with lightweight AST construction and gradually move toward richer semantic pipelines.

## What an action is

A semantic action is a callable attached conceptually to a production rule. When that rule reduces, the action receives the semantic values of its children and returns a new value for the parent.

In glrpp terms, the foundational pieces are:

- `glrpp::dsl::action<Fn>`
- `glrpp::dsl::make_action(fn)`
- `glrpp::dsl::identity`
- `glrpp::dsl::ast_node` and `glrpp::dsl::ast_array`

## A simple mental model

Suppose the grammar recognizes `number '+' number`. The parser layer proves that the structure exists. An action can then build something like:

- a generic AST node such as `{"add", [lhs, rhs]}`
- a typed domain node such as `binary_expr{plus, lhs, rhs}`
- a direct computed value such as `43`

The syntax and the meaning remain separate, but actions connect them deliberately.

## Action wrapper example

```cpp
auto make_sum = glrpp::dsl::make_action([](const glrpp::dsl::ast_array& children) {
  return glrpp::dsl::ast_node{"add", children};
});
```

The wrapper just stores the callable and forwards arguments. That simplicity makes actions easy to test outside the parser.

## Identity action

The `identity` helper is useful when a rule mostly exists for grammar organization and should preserve its child value unchanged. This is especially common in layered expression grammars where intermediate nonterminals are syntactic conveniences.

## Good beginner patterns

- use actions to build `ast_node` first
- keep one conceptual transformation per action
- avoid global mutable state
- treat semantic failures separately from parse failures

## Example AST construction

```cpp
glrpp::dsl::ast_node lhs{"number", std::int64_t{1}};
glrpp::dsl::ast_node rhs{"number", std::int64_t{2}};
auto add = glrpp::dsl::ast_node{"add", glrpp::dsl::ast_array{lhs, rhs}};
```

Even before the full runtime action wiring is expanded, this is the right conceptual shape for action-oriented design.

## Why actions matter early

If you delay semantic thinking too long, your grammar may become hard to map cleanly into useful program structures. If you introduce semantics too early, grammar debugging becomes harder. The sweet spot is:

1. make the grammar parse correctly
2. inspect the forest
3. start attaching small, obvious actions
4. refactor toward richer typed ASTs only when the structure is stable

Actions basics are about learning that meaning is best built in small, explicit steps.
