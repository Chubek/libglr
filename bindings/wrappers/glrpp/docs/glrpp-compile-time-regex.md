Compile-time regular expressions are one of glrpp’s most distinctive conveniences. They let you define scanner rules as compile-time constants while keeping the matching logic efficient and type-safe.

## CTRE in the wrapper

The scanner helpers use CTRE through a fixed-pattern template form:

```cpp
token_rule<"[0-9]+">("number", 10)
skip_rule<"[ \t\n]+">("ws", 1)
```

The pattern is part of the type, which means the matcher can be generated at compile time.

## Why this is attractive

- no runtime regex compilation cost
- patterns are validated at compile time
- matcher functions become lightweight and predictable
- scanner definitions stay compact

## Matching semantics

The scanner uses `ctre::starts_with`, so each rule is anchored at the current offset. That is exactly what a lexer wants: determine what token begins here, not what token appears anywhere later.

## Example scanner

```cpp
auto scanner = std::make_shared<glrpp::scanner>(std::vector<glrpp::dsl::scan_rule>{
    glrpp::skip_rule<"[ \t]+">("ws", 1),
    glrpp::token_rule<"[A-Za-z_][A-Za-z0-9_]*">("identifier", 10),
    glrpp::token_rule<"[0-9]+">("number", 20),
    glrpp::token_rule<"\+">("plus", 30),
});
```

## Performance notes

Compile-time regex does not magically make every scanner fast, but it usually removes a major class of runtime overhead. The remaining costs are mostly:

- how many rules you try at each position
- how ambiguous the rule set is
- how much Unicode conversion the bridge performs

## Pattern design advice

- make whitespace and trivia rules explicit
- keep long overlapping patterns rare when possible
- use priority only when equal-length matches really need a tiebreaker
- test Unicode-oriented rules with real data, not only ASCII examples

CTRE gives glrpp a modern, expressive scanner definition style without turning the wrapper into a heavyweight lexer framework.
