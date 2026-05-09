# Equinox

**A high-performance C library for equality saturation using e-graphs**

Equinox is a compact, efficient implementation of e-graphs (equality graphs) designed for program optimization, theorem proving, and term rewriting. It provides automatic congruence closure, pattern-based rewriting, and equality saturation capabilities in a clean C API.

---

## Features

- **Efficient E-graph Implementation**: Hash consing, union-find with path compression, and automatic congruence closure
- **Pattern Matching & Rewriting**: Flexible pattern language with variables and conditional rules
- **Equality Saturation**: Apply rewrite rules until fixpoint for comprehensive optimization
- **Memory Efficient**: TLSF allocator integration and configurable memory management
- **Deterministic Hashing**: Uses libdag-ethash for reproducible builds
- **Well-Tested**: Comprehensive test suite using Catch2
- **Documented**: Full API documentation with Doxygen

---

## Quick Start

### Installation
```bash
# Clone the repository
git clone https://github.com/yourusername/equinox.git
cd equinox

# Build and install
mkdir build && cd build
cmake ..
make
sudo make install
```
See [INSTALL.md](INSTALL.md) for detailed installation instructions.

### Basic Example

```c
#include <equinox/egraph.h>
#include <stdio.h>

int main(void) {
// Create e-graph
egraph_config_t config = egraph_default_config();
egraph_t *g = egraph_create(&config);

// Add terms: a, b, f(a)
eclass_id_t a = egraph_add(g, "a", NULL, 0);
eclass_id_t b = egraph_add(g, "b", NULL, 0);
eclass_id_t children[] = {a};
eclass_id_t fa = egraph_add(g, "f", children, 1);

// Assert equality: a = b
egraph_union(g, a, b);
egraph_rebuild(g);

// Check congruence: f(a) = f(b)?
eclass_id_t fb_children[] = {b};
eclass_id_t fb = egraph_add(g, "f", fb_children, 1);

if (egraph_equiv(g, fa, fb)) {
printf("Congruence holds: f(a) = f(b)\n");
}

// Clean up
egraph_destroy(g);
return 0;
}
```
Compile and run:
```bash
gcc example.c $(pkg-config --cflags --libs equinox) -o example
./example
```
---

## What are E-graphs?

An **e-graph** (equality graph) is a data structure that:

1. **Represents equivalence classes of terms**: Multiple expressions that are known to be equal are grouped together
2. **Maintains congruence closure**: If `a = b`, then `f(a) = f(b)` automatically
3. **Enables efficient rewriting**: Pattern matching and term rewriting operate on equivalence classes
4. **Supports equality saturation**: Apply rewrite rules exhaustively to discover all equivalent forms

### Example: Algebraic Simplification


Initial: (x + 0) * 1

After applying rules:
  - x + 0 → x
  - x * 1 → x

Result: x

E-graphs can represent all intermediate forms simultaneously and find the optimal one.

---

## Core Concepts

### E-nodes
An **e-node** represents a term with an operator and children:
```c
// Represents: add(x, y)
eclass_id_t children[] = {x_id, y_id};
eclass_id_t result = egraph_add(g, "add", children, 2);
```
### E-classes
An **e-class** is an equivalence class containing multiple e-nodes:
```c
// After: egraph_union(g, x, y)
// x and y are in the same e-class
// All terms containing x are congruent to terms containing y
```
### Hash Consing
Identical terms are automatically shared:
```c
eclass_id_t expr1 = egraph_add(g, "f", children, 1);
eclass_id_t expr2 = egraph_add(g, "f", children, 1);
// expr1 == expr2 (same e-class)
```
### Congruence Closure
Structural equivalence is maintained automatically:
```c
egraph_union(g, a, b);  // Assert a = b
egraph_rebuild(g);       // Propagate: f(a) = f(b), g(a,x) = g(b,x), etc.
```
---

## Rewriting Example

```c
#include <equinox/egraph.h>
#include <equinox/rewrite.h>

// Create pattern: add(?x, 0)
pattern_t *x = pattern_var("x");
pattern_t *zero = pattern_app("0", NULL, 0);
pattern_t *lhs_children[] = {x, zero};
pattern_t *lhs = pattern_app("add", lhs_children, 2);

// Create replacement: ?x
pattern_t *rhs = pattern_var("x");

// Create rule: add(?x, 0) => ?x
rewrite_rule_t *rule = rewrite_rule_create(lhs, rhs, NULL);

// Apply to e-graph
size_t num_rewrites = rewrite_apply(rule, g);
printf("Applied rule %zu times\n", num_rewrites);

// Clean up
rewrite_rule_destroy(rule);
```
---

## Project Structure

```
equinox/
├── include/equinox/     # Public headers
│   ├── egraph.h         # E-graph API
│   ├── enode.h          # E-node operations
│   ├── eclass.h         # E-class operations
│   ├── unionfind.h      # Union-find data structure
│   ├── hashcons.h       # Hash consing
│   └── rewrite.h        # Pattern matching and rewriting
├── src/                 # Implementation
│   ├── egraph.c
│   ├── enode.c
│   ├── eclass.c
│   ├── unionfind.c
│   ├── hashcons.c
│   └── rewrite.c
├── tests/               # Test suite
├── docs/                # Documentation
├── third_party/         # Dependencies (uthash, TLSF, libdag-ethash)
├── CMakeLists.txt       # Build configuration
├── README.md            # This file
├── INSTALL.md           # Installation guide
├── GUIDE.md             # User guide
└── LICENSE              # MIT License
```
---

## Documentation

- **[Installation Guide](INSTALL.md)**: Detailed build and installation instructions
- **[User Guide](GUIDE.md)**: Comprehensive usage documentation with examples
- **API Reference**: Build with `make docs` (requires Doxygen)
  
```bash
  cd build
  make docs
  # Open build/docs/html/index.html
```
