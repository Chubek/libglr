The native DSL is pleasant because it is small enough to memorize and expressive enough to stay readable. This chapter focuses on day-to-day grammar authoring style.

## Start small and compose

Begin with tiny named pieces:

```cpp
using namespace glrpp;

const auto term = sym(nonterminal("Term"));
const auto plus_tok = sym(literal("+"));
const auto number = sym(terminal("number"));
```

Then compose them into rules:

```cpp
const auto grammar = make_grammar(
    "Expr",
    {production("Expr", term | seq({term, plus_tok, sym(nonterminal("Expr"))})),
     production("Term", number)});
```

## Prefer `|` for readable alternatives

The overloaded pipeline-style `|` operator makes grammar definitions read naturally:

```cpp
production("Atom", sym(terminal("number")) | sym(terminal("identifier")) | sym(terminal("string")));
```

Because the operator flattens nested choices, long alternation chains remain structurally tidy.

## Use helper variables for repeated fragments

C++ gives you names for common fragments. Use them.

```cpp
const auto comma_item = seq({sym(literal(",")), sym(nonterminal("Item"))});
production("List", seq({sym(nonterminal("Item")), star(comma_item)}));
```

This is clearer than duplicating the sequence inline everywhere.

## Literals versus named terminals

Use literals when the grammar truly wants exact punctuation. Use named terminals when lexical classification carries meaning.

Good examples:

- `literal("+")` for punctuation in tiny grammars
- `terminal("identifier")` for lexical categories
- `terminal("kw_if")` for contextual keywords after lexing

## Grammar factories

Wrap grammars in functions so you can reuse them in tests and examples:

```cpp
inline glrpp::dsl::grammar expr_grammar() {
  using namespace glrpp;
  return make_grammar(
      "Expr",
      {production("Expr", sym(nonterminal("Term")) | seq({sym(nonterminal("Term")), sym(literal("+")), sym(nonterminal("Expr"))})),
       production("Term", sym(terminal("number")))});
}
```

## Validation by construction

Constructing the grammar early gives you immediate feedback on missing start rules or empty starts. In practice, grammar factories plus unit tests catch most user-level mistakes before the parser is ever involved.

## Keep semantic intent visible

A grammar is documentation. When you write the DSL, optimize for future readers as much as for the compiler. If a rule looks cryptic, factor it. If an alternation is too wide, group related cases. If a terminal name is vague, rename it.

The best native DSL usage feels like executable language design notes.
