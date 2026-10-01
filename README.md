# LibGLR

LibGLR is a compact C library for experimenting with generalized LR parsing,
shared parse forests, ambiguity management, and grammar normalization.

## What is in this tree

- `include/glr/` public headers for grammars, parsing, disambiguation, and rewriting.
- `src/glr/` library implementation.
- `disambstd/` built-in disambiguation hooks.
- `rewritelib/` standard GRL rewrite programs.
- `bindings/` SWIG interface and build glue.
- `doc/` Doxygen configuration and extra guide pages.

## Highlights

- public grammar API for symbols, productions, and start symbols
- LR(1) parse-table generation from a grammar, with no table-generation step
  to run beforehand
- a real shift/reduce engine that keeps one stack per live reading of the
  input, so a conflicting grammar is explored rather than silently truncated
- a Shared Packed Parse Forest with per-span constructors, DAG traversal, and
  an ambiguity predicate
- disambiguation hooks that prune conflicting stacks
- GLR Rewrite Language (GRL) written as S-expressions
- procedural rewrite API for normalization pipelines
- reusable rewrite library programs for common grammar cleanup passes
- editor-facing incremental parsing: report an edit by line and column, see
  which branches it invalidates, then update the tree from a snapshot
- S-expression queries over a parse forest, with a builtin action table that
  covers extraction, rewriting, disambiguation, AST building, XML, IPC,
  signalling, and exiting
- AST building from queries, with the AST itself defined in S-expression form
  and serializable back to S-expressions
- the parse forest as an XML event stream

## Parsing in one minute

Build a grammar, then parse. The parser tokenizes byte input by matching the
grammar's own terminal names against the cursor (longest match wins), builds
an LR(1) table on first use, and returns a packed forest.

```c
glr_grammar_t *grammar = glr_grammar_create ();
int expr = glr_grammar_add_symbol (grammar, GLR_SYMBOL_NONTERMINAL, "Expr");
int n    = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, "n");
int plus = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, "+");
glr_symbol_t *body[3];

body[0] = glr_grammar_get_symbol (grammar, n);
glr_grammar_add_production (grammar, expr, body, 1);
body[0] = glr_grammar_get_symbol (grammar, expr);
body[1] = glr_grammar_get_symbol (grammar, plus);
body[2] = glr_grammar_get_symbol (grammar, n);
glr_grammar_add_production (grammar, expr, body, 3);
glr_grammar_set_start_symbol (grammar, expr);

glr_parser_t *parser = glr_parser_create (grammar);
glr_parser_set_trivia (parser, " ");            /* optional whitespace */

glr_parse_result_t result = glr_parse (parser, "n + n", 5);
if (result.error == GLR_PARSE_SUCCESS)
  {
    glr_forest_t *forest = result.forest;
    glr_forest_visit (forest, forest->root, my_visitor, my_state);
  }
```

Two examples show the whole pipeline:

- `examples/calc.c` builds a precedence grammar, parses, and evaluates the
  resulting derivation. It also shows why trivia belongs in the tokenizer
  rather than in grammar rules.
- `examples/ambiguous.c` uses a grammar that has no conflict-free LR(1) table
  and reports how many readings the parser kept alive.

To generate the table yourself (to inspect conflicts, or to reuse it):

```c
char error[128];
glr_parse_table_t *table = glr_grammar_build_parse_table (grammar, error,
                                                           sizeof (error));
if (table == NULL)
  {
    fprintf (stderr, "%s\n", error);           /* the reason it failed */
  }
else
  {
    printf ("%zu states, %zu conflicts\n", table->state_count,
            glr_parse_table_conflict_count (table));
  }
```

## Editing text

An editor reports an edit as a line and column range. A live session splices
the text, marks the branches the edit can reach, and then re-parses only from
the last snapshot the edit left valid.

```c
glr_live_parser_t *live
    = glr_live_parser_create (grammar, "n+n", 3, error, sizeof (error));

glr_live_edit_t edit = { 1, 2, 1, 3, "-", 1 };   /* replace "n+n"[1] with "-" */
glr_live_parser_edit (live, &edit, error, sizeof (error));

for (size_t i = 0; i < glr_live_parser_dirty_count (live); i++)
  {
    const glr_forest_node_t *node
        = glr_live_parser_dirty_node (live, i, NULL, NULL);   /* marked */
  }

glr_live_parser_update (live, error, sizeof (error));

glr_live_parser_stats_t stats;
glr_live_parser_get_stats (live, &stats);
/* stats.bytes_reused + stats.bytes_reparsed == the document length */
```

The end position is exclusive, so an insertion is two equal positions with
something to insert. `examples/live.c` edits a document and prints what each
update re-parsed.

## Queries over a parse forest

A query is a list of rules pairing a pattern with an action. Patterns are
S-expressions read with `third_party/sfsexp`, the same reader the rewrite
language uses.

```c
static const char source[] =
  "(query (name \"operands\")"
  "  (ast (node Expr (nonterminal _)))"
  "  (rules"
  "    (match (constructor _ (nonterminal _) (terminal @op) (nonterminal _))"
  "           (extract \"operator: \" \"op\"))"
  "    (match (nonterminal Expr) (@node \"Expr\"))))";

glr_ast_t *ast = glr_ast_create ();
glr_query_options_t options = { 0 };
options.report = report_to_my_log;      /* where actions report their text */
options.user_data = my_state;
options.ast = ast;

glr_query_t *query = glr_query_compile (source, strlen (source), NULL,
                                        error, sizeof (error));

glr_query_run_ex (query, grammar, result.forest, result.forest->root,
                  input, input_length, &options, NULL);
```

The builtin table is `glr_global_query_actions`; an application registers its
own with `glr_query_actions_register()` and passes that table to
`glr_query_compile()`, where it shadows the builtins by name. `examples/query.c`
builds an AST from a parse and prints it as S-expressions.

## Build with CMake

```bash
cmake -S . -B build \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_DOCUMENTATION=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Useful options:

- `BUILD_REWRITELIB=ON` installs the standard `.grl` programs.
- `BUILD_DISAMBSTD=ON` builds the standard disambiguation helpers.
- `BUILD_SWIG_BINDINGS=ON` enables the SWIG-based Python module when SWIG is available.
- `ENABLE_TEST_SANITIZERS=ON` enables address/undefined sanitizers for the test binaries.

CTest labels are organized so focused runs are easy:

- `ctest --test-dir build -L core`
- `ctest --test-dir build -L rewrite`
- `ctest --test-dir build -L disambiguation`
- `ctest --test-dir build -L legacy`
- `ctest --test-dir build -L stratified`
- `ctest --test-dir build -L forest`

Convenience targets and scripts:

- `cmake --build build --target check`
- `cmake --build build --target check-verbose`
- `./scripts/test-library.sh --sanitizers --label core`

## GRL in one minute

GRL programs are S-expressions rooted at `rewrite`.

```lisp
(rewrite
  (name "make-lr-compatible")
  (rules
    (remove-epsilon-productions)
    (remove-unit-productions)
    (remove-left-recursion)
    (left-factor)
    (eliminate-useless-symbols)))
```

Load and execute a rewrite program:

```c
char error[256];
glr_rewrite_program_t *program =
    glr_rewrite_program_load_file ("rewritelib/make-lr-compat.grl",
                                   error, sizeof (error));
if (program != NULL)
  {
    glr_rewrite_program_apply (grammar, program, NULL);
    glr_rewrite_program_destroy (program);
  }
```

You can also call the procedural helpers directly:

```c
glr_rewrite_remove_epsilon_productions (grammar);
glr_rewrite_remove_unit_productions (grammar);
glr_rewrite_remove_left_recursion (grammar);
glr_rewrite_left_factor (grammar);
```

## Documentation

- `GUIDE.md` contains the user guide, GRL guide, and procedural rewrite walkthrough.
- Doxygen pages are generated from `include/glr/*.h` and `doc/*.dox`.
- The legacy `man/` directory is intentionally gone; generated Doxygen manpages are the supported manual format.

## Alternative build systems

This repository now includes starter specifications for:

- `meson.build`
- `build.ninja`
- `configure.ac` and `Makefile.am`

They mirror the same source layout as the CMake build and are intended as
maintained alternatives, not generated artifacts.
