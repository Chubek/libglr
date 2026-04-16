YAML is a compelling case study because it mixes indentation sensitivity, scalar ambiguity, and context-dependent structure. It is exactly the sort of language that benefits from a flexible parser architecture.

## Why YAML is difficult

YAML combines:

- indentation-based structure
- multiple scalar styles
- flow and block collections
- tags and anchors
- context-sensitive tokenization

A pure traditional lexer-parser split often feels strained here.

## Where glrpp helps

glrpp’s scannerless orientation and reader-hook flexibility make it easier to keep parsing and layout interpretation connected. You can model indentation and contextual tokenization as part of a coordinated pipeline instead of forcing everything into a rigid early token stream.

## Example fragment strategy

A YAML-like parser might separate concerns into:

- a reader or scanner layer that preserves newline and indentation information
- grammar rules for block entries, mappings, and sequences
- semantic passes that resolve scalar forms and tags

## Layout handling

Indentation-sensitive languages often need more than regex tokens. A good design may track indentation context outside the raw grammar while still feeding the grammar explicit INDENT/DEDENT-like events or structured layout metadata.

## Ambiguity and resilience

YAML tools often need to recover gracefully from incomplete documents, especially in editors. GLR-style ambiguity tolerance is useful here because partial structure is better than total failure.

## Lessons from YAML

- the reader layer matters as much as the grammar layer
- whitespace is syntax, not trivia
- scannerless or hybrid parsing is often the right mental model
- semantic normalization after parsing is essential

YAML shows why glrpp’s Unicode-aware reader and hook bridge are strategically important, not just implementation details.
