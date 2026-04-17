Semantic actions are where the parser stops being a recognizer and starts becoming a translator. In glrpp, actions are modeled as typed callables and attached to rules conceptually through the `rule::reducer` field.

## Action design principles

A good semantic action should be:

- local to one reduction or one clear abstraction boundary
- deterministic
- side-effect-light when possible
- easy to test independently of parsing

## The action wrapper

The generic wrapper is simple:

```cpp
template <typename Fn>
struct action {
  Fn fn;

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) const {
    return fn(std::forward<Args>(args)...);
  }
};
```

That simplicity is a strength. You can wrap lambdas, function objects, or ordinary functions.

## Example action shapes

### Fold children into a node

```cpp
auto make_add = glrpp::dsl::make_action([](const glrpp::dsl::ast_array& children) {
  return glrpp::dsl::ast_node{"add", children};
});
```

### Preserve an existing child

```cpp
auto first_child = glrpp::dsl::make_action([](const glrpp::dsl::ast_array& children) {
  return children.front();
});
```

## Error handling inside actions

Semantic failures are different from parse failures. A parse failure means the syntax did not fit. A semantic failure means the syntax fit but the interpretation was invalid. Model these separately. Common options are:

- return a special AST error node
- throw a domain-specific exception in batch tools
- accumulate diagnostics in an external sink

## Typed domain nodes

`ast_node` is useful, but real projects often migrate to custom types. Actions are the natural place to construct those richer nodes. Keep the generic AST around for debugging and tests even if the production pipeline uses a stronger type system.

## Capturing context

Because actions are callables, they can capture external data. Do this sparingly and intentionally. Capturing immutable configuration is usually fine. Capturing mutable global state often makes parse behavior harder to reason about.

## Testing actions

Treat actions like pure functions whenever you can. Unit-test them with synthetic child arrays before wiring them into parser assembly. That keeps grammar bugs and semantic bugs from masking each other.

Semantic actions are most powerful when they are small, explicit, and unsurprising.
