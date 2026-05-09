# Equinox E-graph Library - User Guide

This guide provides comprehensive documentation for using the Equinox E-graph library, from basic concepts to advanced usage patterns.

## Table of Contents

- [Introduction](#introduction)
- [Core Concepts](#core-concepts)
- [Getting Started](#getting-started)
- [Basic Usage](#basic-usage)
- [Advanced Features](#advanced-features)
- [Rewriting and Optimization](#rewriting-and-optimization)
- [Performance Tuning](#performance-tuning)
- [Best Practices](#best-practices)
- [Common Patterns](#common-patterns)
- [Examples](#examples)
- [API Reference Summary](#api-reference-summary)

---

## Introduction

Equinox is a high-performance C library for equality saturation using e-graphs (equality graphs). E-graphs are data structures that efficiently represent equivalence classes of terms and enable powerful program optimization and theorem proving techniques.

### What are E-graphs?

An **e-graph** is a data structure that:
- Represents multiple equivalent expressions compactly
- Maintains congruence closure automatically
- Enables efficient pattern matching and rewriting
- Supports equality saturation for optimization

### Key Features

- **Efficient representation**: Hash consing eliminates duplicate terms
- **Automatic congruence**: Maintains structural equivalence
- **Flexible rewriting**: Pattern-based term rewriting with conditions
- **Memory efficient**: Uses union-find for equivalence classes
- **Configurable**: Tunable parameters for different use cases

---

## Core Concepts

### E-nodes

An **e-node** represents a term in the e-graph:
- Has an operator (function symbol)
- Has zero or more children (references to e-classes)
- Belongs to exactly one e-class
```c
// Example: f(a, b) is represented as an e-node
// operator = "f"
// children = [eclass_of_a, eclass_of_b]

### E-classes

An **e-class** (equivalence class) represents a set of equivalent e-nodes:
- Contains multiple e-nodes that are considered equal
- Has a unique canonical representative
- Tracks parent e-nodes for congruence closure

c
// Example: If we know x = y, then:
// eclass_1 contains both x and y
// f(x) and f(y) are in the same e-class (by congruence)

### Union-Find

The **union-find** data structure:
- Tracks which e-classes have been merged
- Provides efficient find operation with path compression
- Supports union by rank for balanced trees

### Hash Consing

**Hash consing** ensures structural sharing:
- Identical terms map to the same e-node
- Reduces memory usage
- Enables fast equality checks (pointer comparison)

### Congruence Closure

**Congruence closure** maintains the invariant:
- If `a = b`, then `f(a) = f(b)` for any function `f`
- Automatically propagated during rebuild
- Essential for correctness of equality reasoning

---

## Getting Started

### Including the Library

c
#include <equinox/egraph.h>
#include <equinox/rewrite.h>

### Linking

Using pkg-config:
bash
gcc myprogram.c $(pkg-config --cflags --libs equinox) -o myprogram

Manual linking:
bash
gcc myprogram.c -I/usr/local/include -L/usr/local/lib -lequinox -o myprogram

### Basic Initialization

c
#include <equinox/egraph.h>

int main(void) {
// Create e-graph with default configuration
egraph_config_t config = egraph_default_config();
egraph_t *graph = egraph_create(&config);

// Use the e-graph...

// Clean up
egraph_destroy(graph);
return 0;
}

---

## Basic Usage

### Creating an E-graph

c
// Default configuration
egraph_config_t config = egraph_default_config();
egraph_t *g = egraph_create(&config);

// Custom configuration
egraph_config_t custom_config = {
.initial_capacity = 1024,      // Initial hash table size
.rebuild_limit = 10,           // Max rebuild iterations
.load_factor = 0.75            // Hash table load factor
};
egraph_t *g2 = egraph_create(&custom_config);

### Adding Terms

c
// Add a constant (0-arity term)
eclass_id_t a = egraph_add(g, "a", NULL, 0);
eclass_id_t b = egraph_add(g, "b", NULL, 0);

// Add a unary term: f(a)
eclass_id_t children1[] = {a};
eclass_id_t fa = egraph_add(g, "f", children1, 1);

// Add a binary term: g(a, b)
eclass_id_t children2[] = {a, b};
eclass_id_t gab = egraph_add(g, "g", children2, 2);

// Add nested term: h(f(a), g(a, b))
eclass_id_t children3[] = {fa, gab};
eclass_id_t result = egraph_add(g, "h", children3, 2);

### Asserting Equality

c
// Assert that a = b
egraph_union(g, a, b);

// Rebuild to propagate congruences
egraph_rebuild(g);

// Now f(a) = f(b) automatically (by congruence)

### Checking Equality

c
eclass_id_t x = egraph_add(g, "x", NULL, 0);
eclass_id_t y = egraph_add(g, "y", NULL, 0);

// Initially not equal
assert(!egraph_equiv(g, x, y));

// Assert equality
egraph_union(g, x, y);
egraph_rebuild(g);

// Now equal
assert(egraph_equiv(g, x, y));

### Querying E-graph State

c
// Number of e-classes
size_t num_classes = egraph_num_eclasses(g);

// Number of e-nodes
size_t num_nodes = egraph_num_enodes(g);

// Get statistics
egraph_stats_t stats = egraph_stats(g);
printf("Unions: %zu\n", stats.num_unions);
printf("Rebuilds: %zu\n", stats.num_rebuilds);
printf("Merges: %zu\n", stats.num_merges);

---

## Advanced Features

### Iterating Over E-classes

c
egraph_iter_t iter = egraph_iter_create(g);
eclass_id_t id;

while (egraph_iter_next(&iter, &id)) {
printf("E-class: %u\n", id);

// Get the e-class
eclass_t *ec = egraph_get_eclass(g, id);

// Iterate over nodes in this e-class
for (size_t i = 0; i < eclass_num_nodes(ec); i++) {
enode_t *node = eclass_get_node(ec, i);
printf("  Node operator: %s\n", enode_get_operator(node));
}
}

### Working with E-nodes Directly

c
// Create e-node manually
eclass_id_t children[] = {a, b};
enode_t *node = enode_create("add", children, 2);

// Get node properties
const char *op = enode_get_operator(node);
size_t arity = enode_get_arity(node);
eclass_id_t child0 = enode_get_child(node, 0);

// Compute hash
uint32_t hash = enode_hash(node);

// Compare nodes
enode_t *node2 = enode_create("add", children, 2);
bool equal = enode_equal(node, node2);

// Clean up
enode_destroy(node);
enode_destroy(node2);

### Custom Hash Consing

c
// Create hash cons table
hashcons_t *hc = hashcons_create(256, 0.75);

// Insert e-node
enode_t *node = enode_create("f", children, 1);
enode_t *canonical = hashcons_lookup(hc, node);

if (canonical == NULL) {
// First time seeing this term
hashcons_insert(hc, node);
canonical = node;
} else {
// Already exists, use canonical version
enode_destroy(node);
}

// Clean up
hashcons_destroy(hc);

---

## Rewriting and Optimization

### Pattern Matching

c
// Create pattern: add(?x, 0) where ?x is a variable
pattern_t *var_x = pattern_var("x");
pattern_t *zero = pattern_app("0", NULL, 0);
pattern_t *children[] = {var_x, zero};
pattern_t *lhs = pattern_app("add", children, 2);

// Create substitution map
subst_t *subst = subst_create();

// Try to match against a term
eclass_id_t term_children[] = {some_expr, zero_id};
enode_t *term = enode_create("add", term_children, 2);

if (pattern_match(lhs, term, g, subst)) {
// Match succeeded
eclass_id_t x_value = subst_lookup(subst, "x");
printf("Matched! x = %u\n", x_value);
}

// Clean up
pattern_destroy(lhs);
subst_destroy(subst);
enode_destroy(term);

### Creating Rewrite Rules

c
// Rule: add(?x, 0) => ?x
pattern_t *lhs = /* pattern for add(?x, 0) */;
pattern_t *rhs = pattern_var("x");

rewrite_rule_t *rule = rewrite_rule_create(lhs, rhs, NULL);

// Apply rule to e-graph
size_t num_rewrites = rewrite_apply(rule, g);
printf("Applied rule %zu times\n", num_rewrites);

// Clean up
rewrite_rule_destroy(rule);

### Conditional Rewriting

c
// Condition function: only apply if x != 0
bool not_zero_condition(const subst_t *subst, const egraph_t *g, void *user_data) {
eclass_id_t x = subst_lookup(subst, "x");
eclass_id_t zero = *(eclass_id_t *)user_data;
return !egraph_equiv(g, x, zero);
}

// Create rule with condition
eclass_id_t zero_id = egraph_add(g, "0", NULL, 0);
rewrite_rule_t *rule = rewrite_rule_create(lhs, rhs, not_zero_condition);
rule->user_data = &zero_id;

// Apply conditional rule
size_t num_rewrites = rewrite_apply(rule, g);

### Bulk Rewriting

c
// Create multiple rules
rewrite_rule_t *rules[] = {
rule1,  // add(?x, 0) => ?x
rule2,  // mul(?x, 1) => ?x
rule3,  // mul(?x, 0) => 0
rule4   // add(?x, ?x) => mul(2, ?x)
};

// Apply all rules with iteration limit
size_t total_rewrites = rewrite_apply_rules(rules, 4, g, 10);
printf("Total rewrites: %zu\n", total_rewrites);

### Equality Saturation

c
// Equality saturation: apply rules until fixpoint
size_t iteration = 0;
size_t prev_nodes = 0;

while (iteration < 100) {
size_t rewrites = rewrite_apply_rules(rules, num_rules, g, 1);
size_t curr_nodes = egraph_num_enodes(g);

if (rewrites == 0 || curr_nodes == prev_nodes) {
printf("Saturated after %zu iterations\n", iteration);
break;
}

prev_nodes = curr_nodes;
iteration++;
}

---

## Performance Tuning

### Configuration Parameters

c
egraph_config_t config = {
// Initial capacity: larger = fewer resizes, more memory
.initial_capacity = 4096,

// Rebuild limit: higher = more thorough, slower
.rebuild_limit = 20,

// Load factor: lower = fewer collisions, more memory
.load_factor = 0.6
};

### Memory Management

c
// Pre-allocate for known workload
egraph_config_t config = egraph_default_config();
config.initial_capacity = expected_term_count * 2;
egraph_t *g = egraph_create(&config);

// Batch operations before rebuild
for (size_t i = 0; i < num_equalities; i++) {
egraph_union(g, pairs[i].left, pairs[i].right);
}
// Single rebuild for all unions
egraph_rebuild(g);

### Avoiding Redundant Rebuilds

c
// BAD: Rebuild after every union
for (size_t i = 0; i < n; i++) {
egraph_union(g, a[i], b[i]);
egraph_rebuild(g);  // Expensive!
}

// GOOD: Batch unions, rebuild once
for (size_t i = 0; i < n; i++) {
egraph_union(g, a[i], b[i]);
}
egraph_rebuild(g);  // Single rebuild

### Efficient Pattern Matching

c
// Cache compiled patterns
static pattern_t *cached_pattern = NULL;
if (cached_pattern == NULL) {
cached_pattern = /* build pattern once */;
}

// Reuse substitution maps
subst_t *subst = subst_create();
for (size_t i = 0; i < num_terms; i++) {
subst_clear(subst);  // Reset instead of recreate
if (pattern_match(cached_pattern, terms[i], g, subst)) {
// Process match
}
}
subst_destroy(subst);

---

## Best Practices

### Resource Management

c
// Always destroy created objects
egraph_t *g = egraph_create(&config);
// ... use g ...
egraph_destroy(g);  // Don't forget!

// Use RAII-style wrappers in C++ if available

### Error Handling

c
// Check for NULL returns
egraph_t *g = egraph_create(&config);
if (g == NULL) {
fprintf(stderr, "Failed to create e-graph\n");
return -1;
}

// Validate e-class IDs
eclass_id_t id = egraph_add(g, "x", NULL, 0);
if (id == ECLASS_ID_INVALID) {
fprintf(stderr, "Failed to add term\n");
egraph_destroy(g);
return -1;
}

### Debugging

c
// Enable debug printing
#define EQUINOX_DEBUG
#include <equinox/egraph.h>

// Print e-graph state
egraph_print(g);

// Print specific e-class
eclass_t *ec = egraph_get_eclass(g, id);
eclass_print(ec);

// Print e-node
enode_t *node = eclass_get_node(ec, 0);
enode_print(node);

### Testing Equivalence

c
// Always rebuild before checking equivalence
egraph_union(g, a, b);
egraph_rebuild(g);  // Required!

if (egraph_equiv(g, x, y)) {
printf("x and y are equivalent\n");
}

---

## Common Patterns

### Building Expression Trees

c
// Helper function to build expressions
eclass_id_t build_expr(egraph_t *g, const char *op, 
eclass_id_t *children, size_t arity) {
return egraph_add(g, op, children, arity);
}

// Build: (a + b) * (c + d)
eclass_id_t a = build_expr(g, "a", NULL, 0);
eclass_id_t b = build_expr(g, "b", NULL, 0);
eclass_id_t c = build_expr(g, "c", NULL, 0);
eclass_id_t d = build_expr(g, "d", NULL, 0);

eclass_id_t ab[] = {a, b};
eclass_id_t add_ab = build_expr(g, "add", ab, 2);

eclass_id_t cd[] = {c, d};
eclass_id_t add_cd = build_expr(g, "add", cd, 2);

eclass_id_t mul_args[] = {add_ab, add_cd};
eclass_id_t result = build_expr(g, "mul", mul_args, 2);

### Algebraic Simplification

c
// Define algebraic rules
rewrite_rule_t *algebra_rules[] = {
// Additive identity: x + 0 = x
create_rule("add(?x, 0)", "?x"),

// Multiplicative identity: x * 1 = x
create_rule("mul(?x, 1)", "?x"),

// Multiplicative zero: x * 0 = 0
create_rule("mul(?x, 0)", "0"),

// Commutativity: x + y = y + x
create_rule("add(?x, ?y)", "add(?y, ?x)"),

// Associativity: (x + y) + z = x + (y + z)
create_rule("add(add(?x, ?y), ?z)", "add(?x, add(?y, ?z))")
};

// Apply until saturation
equality_saturate(g, algebra_rules, 5, 100);

### Constant Folding

c
// Condition: both arguments are constants
bool both_constants(const subst_t *subst, const egraph_t *g, void *data) {
eclass_id_t x = subst_lookup(subst, "x");
eclass_id_t y = subst_lookup(subst, "y");

return is_constant(g, x) && is_constant(g, y);
}

// Rule: add(const1, const2) => computed_result
// (Requires custom instantiation logic)

### Common Subexpression Elimination

c
// E-graphs automatically perform CSE via hash consing
eclass_id_t expr1 = build_expr(g, "add", children, 2);
eclass_id_t expr2 = build_expr(g, "add", children, 2);

// expr1 == expr2 (same e-class, shared representation)
assert(expr1 == expr2);

---

## Examples

### Example 1: Simple Arithmetic

c
#include <equinox/egraph.h>
#include <stdio.h>

int main(void) {
egraph_config_t config = egraph_default_config();
egraph_t *g = egraph_create(&config);

// Build: 2 + 3
eclass_id_t two = egraph_add(g, "2", NULL, 0);
eclass_id_t three = egraph_add(g, "3", NULL, 0);
eclass_id_t children[] = {two, three};
eclass_id_t sum = egraph_add(g, "add", children, 2);

// Build: 5
eclass_id_t five = egraph_add(g, "5", NULL, 0);

// Assert: 2 + 3 = 5
egraph_union(g, sum, five);
egraph_rebuild(g);

// Verify
if (egraph_equiv(g, sum, five)) {
printf("Successfully proved: 2 + 3 = 5\n");
}

egraph_destroy(g);
return 0;
}

### Example 2: Commutativity

c
#include <equinox/egraph.h>
#include <equinox/rewrite.h>

int main(void) {
egraph_t *g = egraph_create(&egraph_default_config());

// Build: f(a, b)
eclass_id_t a = egraph_add(g, "a", NULL, 0);
eclass_id_t b = egraph_add(g, "b", NULL, 0);
eclass_id_t ab[] = {a, b};
eclass_id_t fab = egraph_add(g, "f", ab, 2);

// Build: f(b, a)
eclass_id_t ba[] = {b, a};
eclass_id_t fba = egraph_add(g, "f", ba, 2);

// Initially not equal
printf("Before: f(a,b) == f(b,a)? %s\n", 
egraph_equiv(g, fab, fba) ? "yes" : "no");

// Apply commutativity rule: f(?x, ?y) => f(?y, ?x)
pattern_t *x = pattern_var("x");
pattern_t *y = pattern_var("y");
pattern_t *lhs_children[] = {x, y};
pattern_t *lhs = pattern_app("f", lhs_children, 2);
pattern_t *rhs_children[] = {y, x};
pattern_t *rhs = pattern_app("f", rhs_children, 2);

rewrite_rule_t *rule = rewrite_rule_create(lhs, rhs, NULL);
rewrite_apply(rule, g);

// Now equal
printf("After: f(a,b) == f(b,a)? %s\n", 
egraph_equiv(g, fab, fba) ? "yes" : "no");

rewrite_rule_destroy(rule);
egraph_destroy(g);
return 0;
}

### Example 3: Optimization Pipeline

c
#include <equinox/egraph.h>
#include <equinox/rewrite.h>

// Optimize arithmetic expression
eclass_id_t optimize_expr(egraph_t *g, eclass_id_t expr) {
// Define optimization rules
rewrite_rule_t *rules[] = {
/* identity, zero, constant folding rules */
};

// Apply rules until saturation
size_t iteration = 0;
while (iteration++ < 100) {
size_t rewrites = rewrite_apply_rules(rules, 10, g, 1);
if (rewrites == 0) break;
}

// Extract best term (lowest cost)
return extract_best(g, expr);
}

---

## API Reference Summary

### E-graph Operations

| Function | Description |
|----------|-------------|
| `egraph_create()` | Create new e-graph |
| `egraph_destroy()` | Destroy e-graph |
| `egraph_add()` | Add term to e-graph |
| `egraph_union()` | Assert equality |
| `egraph_rebuild()` | Restore congruence closure |
| `egraph_equiv()` | Check equivalence |
| `egraph_num_eclasses()` | Count e-classes |
| `egraph_num_enodes()` | Count e-nodes |

### E-node Operations

| Function | Description |
|----------|-------------|
| `enode_create()` | Create e-node |
| `enode_destroy()` | Destroy e-node |
| `enode_get_operator()` | Get operator string |
| `enode_get_arity()` | Get number of children |
| `enode_get_child()` | Get child e-class ID |
| `enode_hash()` | Compute hash |
| `enode_equal()` | Compare e-nodes |

### Pattern Matching

| Function | Description |
|----------|-------------|
| `pattern_var()` | Create variable pattern |
| `pattern_app()` | Create application pattern |
| `pattern_match()` | Match pattern against term |
| `pattern_instantiate()` | Instantiate pattern |
| `pattern_destroy()` | Destroy pattern |

### Rewriting

| Function | Description |
|----------|-------------|
| `rewrite_rule_create()` | Create rewrite rule |
| `rewrite_rule_destroy()` | Destroy rule |
| `rewrite_apply()` | Apply single rule |
| `rewrite_apply_rules()` | Apply multiple rules |

---

## Further Reading

- **API Documentation**: Build with `make docs` and see `build/docs/html/`
- **Installation Guide**: See `INSTALL.md`
- **Source Code**: Explore `src/` directory for implementation details
- **Tests**: See `tests/` for usage examples

---

**Happy optimizing with Equinox!**


This comprehensive guide covers all aspects of using the Equinox library, from basic concepts to advanced optimization techniques, with practical examples throughout.