Structural code search and matching tools, like Semgrep-style systems, care more about syntactic shape than about full compilation. That makes GLR and glrpp a natural fit.

## Why structural matching benefits from GLR

Pattern languages often contain deliberate ambiguity:

- metavariables that stand in for many constructs
- wildcard holes
- relaxed punctuation or whitespace handling
- embedded pattern sublanguages

GLR parsing is comfortable with ambiguity and can keep alternative readings alive until the matcher decides which one is meaningful.

## A glrpp-based architecture

A structural matcher can use glrpp for two related grammars:

- the target-language grammar
- the pattern grammar, which extends the target language with metavariable constructs

## Pattern-specific lexical features

CTRE scanner rules are convenient for recognizing pattern tokens such as:

- `$X` metavariables
- ellipsis operators
- language-specific anchors

These can then feed a grammar that is deliberately broader than the ordinary source grammar.

## Matching workflow

1. parse the target source into a forest or AST
2. parse the structural pattern into a pattern AST
3. run tree or forest matching between pattern and target
4. report matches with source ranges

## Why forests are useful

Sometimes the pattern itself is ambiguous in a way that is acceptable. Rather than forcing one reading immediately, a forest-based approach can let the matcher try multiple interpretations.

## Lessons from structural search

- generalized parsing is useful even when you are not building a compiler
- grammar reuse matters because pattern grammars often extend source grammars
- good diagnostics are crucial when users write invalid structural patterns

Semgrep-like tools show that glrpp can be valuable anywhere syntax is a search space, not just where syntax is a correctness gate.
