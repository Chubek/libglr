GLR parsing is the family of generalized LR techniques designed for grammars that are hard, awkward, or impossible to express with a single deterministic LR state machine. In ordinary LR parsing, every parser state assumes that a single action is correct. In GLR parsing, multiple actions may be valid at once, so the parser follows all viable paths and later merges equivalent states. That is the core idea behind libglr and, by extension, glrpp.

## Why generalized parsing exists

Traditional parser generators work best when the grammar has already been simplified into a deterministic form. Real languages are less polite. They contain:

- precedence-sensitive operators
- optional separators and list forms
- contextual keywords
- ambiguous constructs that need semantic filtering later
- scannerless regions where tokenization depends on parser context

GLR parsing accepts that ambiguity is normal. Instead of rejecting conflicts up front, it represents them explicitly and postpones the final choice until enough structure is available.

## How GLR differs from LL and classic LR

LL parsers read from left to right and produce a leftmost derivation. They are pleasant to hand-write and easy to reason about, but they struggle with left recursion and many natural expression grammars.

Classic LR parsers also read left to right, but they build a rightmost derivation in reverse. They are more powerful than LL parsers, especially for expression-heavy languages, but still rely on deterministic parse tables.

GLR starts from the LR model and generalizes the places where LR would otherwise fail. When a shift/reduce or reduce/reduce conflict appears, the parser forks. Internally it uses a graph-structured stack instead of separate linear stacks, so common prefixes are shared efficiently.

## Parse forests instead of single trees

A deterministic parser usually returns one parse tree or one failure. A GLR parser usually returns a forest. A forest records all surviving interpretations compactly. glrpp exposes this through `glrpp::glr::forest` and `glrpp::glr::node`, which let you inspect roots, children, and native symbol names.

That design matters because ambiguity is often useful:

```cpp
using namespace glrpp;

const auto grammar = make_grammar(
    "Expr",
    {production("Expr", sym(nonterminal("Expr")) | seq({sym(nonterminal("Expr")), sym(literal("+")), sym(nonterminal("Expr"))})),
     production("Expr", sym(terminal("number")))});
```

A deterministic parser would insist that you encode precedence and associativity immediately. A GLR parser can accept the ambiguous grammar first, then let you disambiguate later.

## When GLR is the right tool

GLR shines when one or more of the following are true:

- you need a direct transcription of a language reference grammar
- you are prototyping and do not want to normalize away every conflict first
- you want scannerless parsing or lexer hooks
- your language has embedded sublanguages or contextual operators
- ambiguity itself is valuable, as in structural search, editor tooling, or syntax highlighting

## Costs and tradeoffs

Generalization is not free. In the happy path, deterministic grammars behave close to LR performance. In the worst case, highly ambiguous inputs can grow substantially in memory use and runtime. The practical lesson is simple: embrace GLR where it helps, but still design your grammar intentionally.

## What glrpp adds on top of libglr

libglr is the parsing engine. glrpp is the C++ wrapper layer. It adds:

- a header-only user-facing API
- a grammar DSL with `sym`, `seq`, `alt`, `opt`, `star`, `plus`, and `|`
- optional CTRE-powered tokenization
- UTF-16 reader bridging for native lexer hooks
- small metaprogramming utilities for reflection and type-level composition

If you remember one idea from this chapter, make it this: GLR parsing is a disciplined way to keep the language natural while moving disambiguation to the place where you actually have enough information to decide.
