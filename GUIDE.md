# LibGLR Guide

## Overview

LibGLR exposes three layers:

1. grammar data structures in `include/glr/grammar.h`
2. parser and ambiguity infrastructure in `include/glr/parser.h` and `include/glr/disambiguate.h`
3. grammar rewriting in `include/glr/rewrite.h`

The rewrite layer is the main addition in this tree. It lets you normalize
grammars either with direct C calls or with declarative S-expression programs.

## Quick start

```c
#include <glr/glr.h>

glr_grammar_t *grammar = glr_grammar_create ();
int expr = glr_grammar_add_symbol (grammar, GLR_SYMBOL_NONTERMINAL, "Expr");
int plus = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, "+");
int term = glr_grammar_add_symbol (grammar, GLR_SYMBOL_NONTERMINAL, "Term");
glr_symbol_t *body[] = {
  glr_grammar_get_symbol (grammar, expr),
  glr_grammar_get_symbol (grammar, plus),
  glr_grammar_get_symbol (grammar, term)
};
glr_grammar_add_production (grammar, expr, body, 3);
glr_grammar_set_start_symbol (grammar, expr);
```

## The GLR Rewrite Language

GRL is a small S-expression language. A program is a `rewrite` form that
contains a name and either inline rules or a `(rules ...)` block.

### Program shape

```lisp
(rewrite
  (name "pipeline-name")
  (rules
    (rule-one ...)
    (rule-two ...)))
```

The parser also accepts a shorter form where rules are listed directly under
`rewrite`.

### Built-in rules

GRL currently supports these built-ins:

- `(add-symbol terminal NAME)`
- `(add-symbol nonterminal NAME)`
- `(drop-symbol NAME)`
- `(rename-symbol OLD NEW)`
- `(set-start NAME)`
- `(add-production HEAD (SYM1 SYM2 ...))`
- `(drop-production HEAD (SYM1 SYM2 ...))`
- `(remove-epsilon-productions)`
- `(remove-unit-productions)`
- `(eliminate-useless-symbols)`
- `(remove-left-recursion)`
- `(left-factor)`
- `(make-lr-compatible)`
- `(eliminate-ambiguity)`

### Semantics

- `add-symbol` is idempotent when a symbol with the same name and kind already exists.
- `drop-symbol` also removes productions that reference the symbol.
- `add-production` resolves body symbols by name against the current grammar.
- `remove-epsilon-productions` preserves nullable derivations and keeps only start-symbol epsilon productions.
- `remove-unit-productions` computes the nonterminal unit-closure and materializes the non-unit productions.
- `remove-left-recursion` uses ordered substitution followed by direct left-recursion elimination.
- `left-factor` performs repeated one-symbol prefix factoring and introduces helper nonterminals.
- `make-lr-compatible` runs epsilon removal, unit removal, left-recursion elimination, left factoring, and useless-symbol cleanup.
- `eliminate-ambiguity` is intentionally conservative: it currently delegates to the LR-compatibility pipeline plus final cleanup.

### Example programs

Remove nullable productions and cleanup:

```lisp
(rewrite
  (name "nullable-cleanup")
  (rules
    (remove-epsilon-productions)
    (eliminate-useless-symbols)))
```

Perform a full normalization pipeline:

```lisp
(rewrite
  (name "normalize")
  (rules
    (make-lr-compatible)))
```

Edit a grammar structurally before normalization:

```lisp
(rewrite
  (name "rename-and-normalize")
  (rules
    (rename-symbol Expr Expression)
    (set-start Expression)
    (make-lr-compatible)))
```

## Procedural rewrite API

The procedural API mirrors the GRL runtime and is useful when rewrites depend
on application logic.

### Core types

- `glr_rewrite_rule_kind_t` enumerates each supported opcode.
- `glr_rewrite_rule_t` stores one compiled rule.
- `glr_rewrite_program_t` stores an ordered list of rules.
- `glr_rewrite_report_t` reports how many rules ran and where execution failed.
- `glr_rewrite_status_t` distinguishes parse errors, missing symbols, conflicts, and memory failures.

### Loading and compiling GRL

```c
char error[256];
glr_rewrite_program_t *program =
    glr_rewrite_program_load_file ("rewritelib/remove-left-recursion.grl",
                                   error, sizeof (error));
if (program == NULL)
  {
    fprintf (stderr, "rewrite load failed: %s\n", error);
  }
```

### Applying a compiled program

```c
glr_rewrite_report_t report;
glr_rewrite_status_t status =
    glr_rewrite_program_apply (grammar, program, &report);
if (status != GLR_REWRITE_STATUS_OK)
  {
    fprintf (stderr, "rewrite failed after %zu rules: %s\n",
             report.rules_attempted, report.message);
  }
```

### Building a program procedurally

```c
glr_rewrite_program_t *program = glr_rewrite_program_create ("custom");
glr_rewrite_rule_t rule = {0};

rule.kind = GLR_REWRITE_RULE_RENAME_SYMBOL;
rule.data.rename_symbol.old_name = "Expr";
rule.data.rename_symbol.new_name = "Expression";
glr_rewrite_program_add_rule (program, &rule);

rule.kind = GLR_REWRITE_RULE_MAKE_LR_COMPATIBLE;
glr_rewrite_program_add_rule (program, &rule);
```

`glr_rewrite_program_add_rule` copies owned strings and production templates,
so stack-allocated rule values are fine.

### One-shot helpers

Each major transform is also exposed directly:

```c
glr_rewrite_add_symbol (grammar, GLR_SYMBOL_NONTERMINAL, "ExprTail");
glr_rewrite_add_production (grammar, "Expr", body_names, body_length);
glr_rewrite_remove_epsilon_productions (grammar);
glr_rewrite_remove_unit_productions (grammar);
glr_rewrite_remove_left_recursion (grammar);
glr_rewrite_left_factor (grammar);
glr_rewrite_remove_useless_symbols (grammar);
```

### Error handling guidance

- use `GLR_REWRITE_STATUS_NOT_FOUND` to detect misspelled symbol names in GRL or procedural edits
- use `GLR_REWRITE_STATUS_CONFLICT` for name collisions and invalid start-symbol updates
- treat `GLR_REWRITE_STATUS_PARSE_ERROR` as an invalid GRL source file
- treat `GLR_REWRITE_STATUS_MEMORY_ERROR` as a hard failure and keep the current grammar snapshot only if your application copied it first

## Standard rewrite library

`rewritelib/` installs reusable GRL files:

- `eliminate-epsilon-production.grl`
- `remove-unit-productions.grl`
- `remove-left-recursion.grl`
- `left-factor.grl`
- `eliminate-useless-symbols.grl`
- `make-lr-compat.grl`
- `eiminate-ambguity.grl`

The last filename keeps the historic typo already present in the tree so the
repository layout stays stable.

## Disambiguation hooks

Disambiguation remains procedural. Register hooks on `glr_parser_t` with:

```c
glr_parser_add_disambiguator (
    parser,
    glr_disambig_precedence_hook_create ("precedence", 100,
                                         resolve_precedence,
                                         my_state, destroy_state));
```

The standard helpers in `disambstd/` cover precedence, associativity,
predicates, semantic filtering, dynamic programming, and probabilistic
selection.

## Generated documentation

When Doxygen is enabled, the generated HTML and manpages cover both the parser
API and the rewrite API. The removed `man/` directory is not required anymore.
