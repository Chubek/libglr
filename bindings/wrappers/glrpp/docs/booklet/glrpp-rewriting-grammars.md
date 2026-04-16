Grammar rewriting is the process of transforming one grammar into another before parser construction. In GLR-based systems, rewriting is often less about forcing determinism and more about improving organization, readability, or downstream tooling.

## Why rewrite grammars at all

Reasons include:

- expanding EBNF sugar into primitive productions
- injecting precedence or associativity scaffolding
- normalizing naming conventions
- combining imported grammar fragments
- adding instrumentation or debug productions

## glrpp expression trees as rewrite input

Because the DSL stores rules as runtime expression trees, grammar rewriting can be implemented as pure tree-to-tree transformation. That is a clean design point. You can traverse `dsl::expression`, inspect `expr_kind`, and produce a modified `dsl::expression` or `dsl::rule`.

## Example rewrite ideas

### Expand optional forms

`opt(e)` can be rewritten into a choice between `e` and epsilon if a lower layer prefers primitive forms.

### Expand repetition

`star(e)` and `plus(e)` can be lowered to helper nonterminals for runtimes that expect explicit recursion.

### Normalize literals

You might rewrite raw literals into named terminals when integrating with a large external lexer.

## Pipeline structure

A practical rewrite pipeline often looks like this:

1. parse or construct the high-level DSL grammar
2. run normalization passes
3. optionally run validation passes
4. build the native parser from the normalized grammar

## Keep rewrites explainable

Every rewrite should answer two questions clearly:

- what user-facing benefit does it provide?
- how can a developer trace the rewritten output back to the source grammar?

If a rewrite obscures the grammar more than it helps, reconsider it.

Grammar rewriting is best when it preserves the author’s intent while making the execution form more useful.
