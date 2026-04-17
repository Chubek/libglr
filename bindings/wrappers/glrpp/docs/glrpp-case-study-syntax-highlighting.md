Syntax highlighting is not full compilation, but it still benefits from precise syntactic structure. glrpp is interesting here because it can produce richer syntax regions than a regex-only highlighter while remaining resilient in the face of incomplete code.

## Why parse for highlighting

Regex highlighters are fast and easy, but they often mis-handle:

- nested delimiters
- embedded languages
- ambiguous punctuation
- multiline constructs

A GLR-based highlighter can preserve multiple interpretations until enough context arrives, which is especially useful in editors.

## A highlighting pipeline

A practical pipeline looks like this:

1. read the source buffer
2. tokenize or hook-scan obvious lexical classes
3. parse into a forest or simplified AST
4. map nodes and tokens to highlight regions
5. resolve overlaps by precedence rules for styles

## Why ambiguity tolerance helps

Incomplete source is normal while editing. A GLR parser can often keep producing partial structure even when the text is temporarily malformed. That means the highlighter can continue giving useful output instead of collapsing into a uniform fallback color.

## Region extraction

Highlighting usually wants ranges for:

- keywords
- identifiers
- literals
- comments
- operators
- interpolated or embedded-language zones

The scanner can provide lexical ranges quickly, while the parser can refine them structurally.

## Example use case

Imagine a templating file mixing HTML, expressions, and string interpolation. A scannerless or hybrid glrpp pipeline can keep the boundaries coherent across those domains better than disconnected regex passes.

Syntax highlighting is an excellent reminder that parsing is not only for compilers. It is also for interactive, approximate, user-facing understanding.
