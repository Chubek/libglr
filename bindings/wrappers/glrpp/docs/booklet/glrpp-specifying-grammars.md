Grammar specification in glrpp is intentionally direct. The DSL is small, the runtime grammar object is simple, and the emphasis is on readable, explicit C++.

## The three core concepts

A grammar consists of:

- symbols
- expressions
- production rules

Symbols are terminals, nonterminals, literals, or epsilon. Expressions combine symbols into sequences, choices, and repetition. Rules assign an expression to a left-hand-side nonterminal.

## Symbols

Create symbols with:

```cpp
auto t = glrpp::terminal("number");
auto n = glrpp::nonterminal("Expr");
auto l = glrpp::literal("+");
auto e = glrpp::epsilon();
```

A terminal is a named token class. A literal is punctuation or other exact text you want to name directly. A nonterminal is a syntactic category.

## Expressions

Wrap symbols with `sym` to obtain atomic expressions, then combine them:

```cpp
auto atom = sym(nonterminal("Term"));
auto seq_expr = seq({sym(nonterminal("Term")), sym(literal("+")), sym(nonterminal("Expr"))});
auto alt_expr = alt({sym(terminal("number")), sym(terminal("identifier"))});
auto maybe = opt(sym(terminal("sign")));
auto many = star(sym(terminal("comma")));
auto one_or_more = plus(sym(terminal("digit")));
```

## Rules and grammars

Rules are built with `production` and grouped with `make_grammar`:

```cpp
const auto g = make_grammar(
    "Expr",
    {production("Expr", sym(nonterminal("Term")) | seq({sym(nonterminal("Term")), sym(literal("+")), sym(nonterminal("Expr"))})),
     production("Term", sym(terminal("number")) | sym(terminal("identifier")))});
```

## Validation

The grammar constructor validates a few critical invariants:

- the start symbol must not be empty
- there must be at least one rule
- the start symbol must have a matching rule

More advanced validation, such as unreachable-rule warnings or precedence conflicts, belongs in higher-level tooling.

## Left recursion and GLR

Unlike many LL-oriented DSLs, glrpp does not force you to eliminate left recursion mechanically. GLR parsing can handle many direct formulations naturally. That is one of the main ergonomic wins of the approach.

## Style recommendations

- use nonterminal names in `PascalCase` or another consistent scheme
- use descriptive terminal names unless a literal is genuinely clearer
- keep top-level productions visually simple, then factor helper nonterminals as needed
- prefer one concept per rule unless ambiguity is intentional

A good grammar should read like a precise explanation of the language, not a workaround for parser-generator limitations.
