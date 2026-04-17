Rule-level explanation is about answering a subtler question than ordinary diagnostics: not merely where parsing failed, but which grammar ideas were active at the time.

## Why explain rules

When developing or teaching a grammar, it is often useful to know:

- which rule matched a construct
- which rules competed in an ambiguity
- which rule the parser expected next
- how a final forest branch was derived

## Sources of rule explanations

In glrpp-based tooling, rule explanations can come from several places:

- grammar metadata and symbol names
- parse-forest traversal
- debug lowering passes that annotate productions
- semantic passes that preserve origin-rule information

## Human-readable rule names

The easiest way to improve rule explanations is to write grammars with meaningful nonterminal names. `ExprTail` is already more informative than `X3`.

## Example explanation output

Imagine reporting:

- `Assignment` matched `identifier '=' Expr`
- `Expr` remained ambiguous between `CallExpr` and `Identifier`
- expected `rparen` while completing `ArgList`

Even simple text like this is far more helpful than a bare token mismatch.

## Teaching and debugging uses

Rule-level explanations are especially valuable for:

- onboarding new grammar authors
- debugging precedence and associativity issues
- generating tutorial material from live grammars
- tracing why a certain AST node exists at all

In short, rule explanation turns the grammar from implementation detail into inspectable knowledge.
