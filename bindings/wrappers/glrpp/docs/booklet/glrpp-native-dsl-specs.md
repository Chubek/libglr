This chapter treats the glrpp DSL as a formal specification language. The goal is to make the semantics of each construct explicit rather than merely intuitive.

## Symbol taxonomy

`symbol_kind` currently distinguishes:

- `terminal`
- `nonterminal`
- `literal`
- `epsilon`

A `symbol` contains a `name` string and a `kind`. Terminals and literals are both treated as terminal-like during grammar introspection.

## Expression algebra

`expr_kind` currently distinguishes:

- `atom`
- `sequence`
- `choice`
- `optional`
- `zero_or_more`
- `one_or_more`

An `expression` contains:

- a `kind`
- an `atom` symbol, meaningful when `kind == atom`
- a `children` vector for composite expressions

## Constructors and semantics

### `sym(symbol)`
Creates an atomic expression.

### `seq({e1, e2, ...})`
Creates an ordered concatenation. Successful recognition requires each child to match in order.

### `alt({e1, e2, ...})`
Creates an explicit choice. Recognition succeeds if any child succeeds.

### `operator|`
Creates a choice expression using EBNF-style infix syntax. Existing choice children are flattened.

### `opt(e)`
Shorthand for either `e` or epsilon.

### `star(e)`
Zero or more repetitions of `e`.

### `plus(e)`
One or more repetitions of `e`.

## Rule semantics

A `rule` contains:

- `lhs`: the left-hand-side nonterminal name
- `rhs`: an expression tree
- `reducer`: an optional semantic action wrapper

Even when semantic actions are not fully wired into the runtime lowering yet, the DSL reserves the slot explicitly.

## Grammar semantics

A `grammar` contains:

- a start symbol string
- an ordered vector of rules

`terminals()` walks every rule expression and collects terminal-like symbols. `nonterminals()` derives names from rule left-hand sides.

## Formal reading example

```cpp
production("List", seq({sym(terminal("item")), star(seq({sym(literal(",")), sym(terminal("item"))}))}))
```

This denotes a language of one `item` followed by zero or more comma-item suffixes.

## Lowering expectations

At parser-construction time, expression trees are flattened into native grammar productions for libglr. The wrapper therefore treats the DSL as the human-readable front-end and the native grammar as the execution form.

## Specification discipline

Think of the DSL as a typed EBNF embedded in C++. Every constructor has explicit runtime meaning, and every overloaded convenience should preserve that meaning. That is the standard by which extensions to the DSL should be judged.
