Symbols are the vocabulary of the grammar. If rules are sentences, symbols are the nouns and verbs those sentences are built from.

## Symbol categories

glrpp distinguishes four symbol kinds:

- terminals
- nonterminals
- literals
- epsilon

In practice, terminals and literals are both token-like from the parser’s point of view, while nonterminals define syntactic categories.

## Naming conventions

Good symbol names save enormous time later. A practical convention is:

- `Expr`, `Stmt`, `Type` for nonterminals
- `identifier`, `number`, `string` for terminals
- `kw_if`, `kw_else` for keywords if they are tokenized specially
- raw punctuation literals only when that is genuinely the clearest notation

## Lifecycle of a symbol

A symbol goes through several phases:

1. authored in the DSL
2. traversed during grammar validation and inspection
3. interned into the native grammar during parser construction
4. referenced by parse-forest nodes and diagnostics

That lifecycle is why consistency matters. A naming mismatch introduced at the DSL layer can surface later as a mysterious parse failure.

## Terminal design choices

Decide early whether punctuation should be modeled as literals or named terminals. Both are valid. Named terminals integrate better with external lexers; literals keep tiny grammars readable.

## Symbol collection utilities

The `grammar` wrapper exposes `terminals()` and `nonterminals()`. These are handy for:

- documentation generation
- grammar sanity checks
- test assertions
- building editor hints or completions

## Example

```cpp
const auto g = make_grammar(
    "Pair",
    {production("Pair", seq({sym(terminal("identifier")), sym(literal(":")), sym(terminal("number"))}))});
```

Here the grammar vocabulary is crisp: `Pair` is syntactic, `identifier` and `number` are lexical, `:` is literal punctuation.

Manage symbols carefully and the rest of the grammar becomes easier to reason about.
