# LibGLR User Guide

**Generalized LR Parser Library**

This guide provides comprehensive information on using LibGLR for implementing GLR parsers in C.

## Table of Contents

1. [Introduction](#introduction)
2. [Quick Start](#quick-start)
3. [Grammar Definition](#grammar-definition)
4. [Parsing](#parsing)
5. [Data Structures](#data-structures)
6. [Handling Ambiguity](#handling-ambiguity)
7. [API Reference](#api-reference)
8. [Examples](#examples)

## Introduction

LibGLR is a minimal C library that provides facilities for implementing Generalized LR (GLR) parsers. It is part of the TwinBooks project and focuses on:

- **Grammar data structures** for representing grammars
- **DAG-based stacks** for efficient state management
- **Forking mechanism** for handling ambiguity
- **SPPF (Shared Parse Forest)** for compact representation of all parses
- **Reduction operations** for GLR parsing

### What is GLR Parsing?

GLR (Generalized LR) parsing extends LR parsing to handle ambiguous grammars. Unlike traditional LR parsers that fail on ambiguous inputs, GLR parsers:

- Explore all possible parse paths simultaneously
- Use forking stacks to represent ambiguity
- Share common substructures via SPPF
- Return all valid parses

## Quick Start

### Building LibGLR

```bash
# Clone repository
cd libglr

# Build with CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# Run tests
ctest --output-on-failure

# Install
sudo make install
```

### Using LibGLR in Your Project

```cmake
find_package(libglr REQUIRED)
target_link_libraries(your_target PRIVATE libglr)
```

## Grammar Definition

### Creating a Grammar

```c
#include <glr/glr.h>

// Create empty grammar
glr_grammar_t *grammar = glr_grammar_create();
if (grammar == NULL) {
    // Handle error
}
```

### Adding Symbols

Symbols can be **terminals** (tokens) or **non-terminals** (grammar variables):

```c
// Add terminal symbol
int tok_number = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "NUMBER");

// Add non-terminal symbol  
int expr_id = glr_grammar_add_symbol(grammar, GLR_SYMBOL_NONTERMINAL, "expression");
```

Symbol IDs start from 0 and increment with each addition.

### Adding Productions

A production consists of a **head** (non-terminal) and a **body** (sequence of symbols):

```c
// Production: expression -> NUMBER
glr_symbol_t *num_body[] = {
    glr_grammar_get_symbol(grammar, tok_number)
};
glr_grammar_add_production(grammar, expr_id, num_body, 1);

// Production: expression -> expression + expression
glr_symbol_t *plus_body[] = {
    glr_grammar_get_symbol(grammar, expr_id),
    glr_grammar_get_symbol(grammar, tok_plus),
    glr_grammar_get_symbol(grammar, expr_id)
};
glr_grammar_add_production(grammar, expr_id, plus_body, 3);
```

### Setting Start Symbol

```c
// Set expression as start symbol
glr_grammar_set_start_symbol(grammar, expr_id);
```

### Complete Grammar Example

```c
glr_grammar_t *create_calculator_grammar(void) {
    glr_grammar_t *grammar = glr_grammar_create();
    
    // Add symbols
    int tok_number = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "NUMBER");
    int tok_plus = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "+");
    int tok_mult = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "*");
    
    int expr = glr_grammar_add_symbol(grammar, GLR_SYMBOL_NONTERMINAL, "expr");
    int term = glr_grammar_add_symbol(grammar, GLR_SYMBOL_NONTERMINAL, "term");
    
    // Productions
    glr_grammar_add_production(grammar, expr, 
        (glr_symbol_t*[2]){glr_grammar_get_symbol(grammar, term)}, 1);
    
    glr_grammar_add_production(grammar, term,
        (glr_symbol_t*[1]){glr_grammar_get_symbol(grammar, tok_number)}, 1);
    
    // Set start
    glr_grammar_set_start_symbol(grammar, expr);
    
    return grammar;
}
```

## Parsing

### Creating a Parser

```c
glr_grammar_t *grammar = create_calculator_grammar();
glr_parser_t *parser = glr_parser_create(grammar);
if (parser == NULL) {
    // Handle error
}
```

### Parsing Input

```c
const char *input = "1 + 2 * 3";
glr_parse_result_t result = glr_parse(parser, input, strlen(input));

if (result.error == GLR_PARSE_SUCCESS) {
    printf("Parse successful!\\n");
    printf("Consumed %zu characters\\n", result.position);
    
    // Access parse forest
    glr_forest_t *forest = glr_parser_get_forest(parser);
    // ... process forest ...
} else {
    printf("Parse error: %d\\n", result.error);
}
```

### Checking Parse Result

```c
typedef enum {
    GLR_PARSE_SUCCESS,          // Success
    GLR_PARSE_ERROR_SYNTAX,     // Syntax error
    GLR_PARSE_ERROR_MEMORY,     // Memory allocation failed
    GLR_PARSE_ERROR_GRAMMAR,    // Grammar error
    GLR_PARSE_ERROR_UNRECOVERABLE // Unrecoverable error
} glr_parse_error_t;
```

## Data Structures

### Grammar Structure

```
glr_grammar_t {
    glr_symbol_t **symbols;      // All symbols
    size_t symbol_count;         // Number of symbols
    glr_production_t **productions; // All productions
    size_t production_count;     // Number of productions
    glr_symbol_t *start_symbol;  // Start symbol
    char *name;                  // Grammar name
}
```

### Stack Structure

LibGLR uses **DAG-based stacks** where:

- Each node represents a parser state
- Nodes can fork (have multiple children)
- Common prefixes are shared
- Forks represent alternative parse paths

```c
glr_stack_t {
    void **states;       // Array of parser states
    size_t height;       // Current height
    size_t capacity;     // Allocated capacity
}
```

### SPPF (Shared Parse Forest)

The SPPF compactly represents all parse trees:

```
glr_forest_t {
    glr_forest_node_t **nodes;  // Nodes by position
    glr_forest_edge_t **edges;  // Edges by position
}
```

Nodes are of types:
- **GLR_NODE_TERMINAL**: Represents a terminal token
- **GLR_NODE_NONTERMINAL**: Represents grammar reduction
- **GLR_NODE_CONSTRUCTOR**: Represents semantic action

## Handling Ambiguity

### How LibGLR Handles Ambiguity

When ambiguity is encountered:

1. **Fork stacks**: Create new stack copies for each alternative
2. **Track forks**: Record fork points with context
3. **Build forest**: All parses share common substructures
4. **Report stacks**: Multiple stacks indicate ambiguity

### Detecting Ambiguity

```c
glr_parse_result_t result = glr_parse(parser, input, len);

if (result.error == GLR_PARSE_SUCCESS) {
    size_t stack_count = glr_parser_stack_count(parser);
    
    if (stack_count > 1) {
        printf("Ambiguous input with %zu parses\\n", stack_count);
    } else {
        printf("Unambiguous parse\\n");
    }
}
```

### Example: Ambiguous Grammar

The classic "dangling else" or operator precedence ambiguity:

```c
// Ambiguous grammar (no precedence)
glr_grammar_add_production(grammar, expr,
    (glr_symbol_t*[3]){expr_sym, plus_sym, expr_sym}, 3);  // expr + expr
glr_grammar_add_production(grammar, expr,
    (glr_symbol_t*[3]){expr_sym, mult_sym, expr_sym}, 3);  // expr * expr
glr_grammar_add_production(grammar, expr,
    (glr_symbol_t*[1]){number_sym}, 1);                    // NUMBER
```

Input `"1 + 2 * 3"` has two valid parses:
1. `(1 + 2) * 3`
2. `1 + (2 * 3)`

## API Reference

### Grammar Functions

| Function | Description |
|----------|-------------|
| `glr_grammar_create()` | Create empty grammar |
| `glr_grammar_destroy()` | Destroy grammar |
| `glr_grammar_add_symbol()` | Add symbol (terminal/non-terminal) |
| `glr_grammar_get_symbol()` | Get symbol by ID |
| `glr_grammar_add_production()` | Add production |
| `glr_grammar_get_production()` | Get production by ID |
| `glr_grammar_set_start_symbol()` | Set start symbol |

### Helper Functions

```c
bool glr_symbol_is_terminal(glr_symbol_t *symbol);
bool glr_symbol_is_nonterminal(glr_symbol_t *symbol);
```

### Parser Functions

| Function | Description |
|----------|-------------|
| `glr_parser_create()` | Create parser for grammar |
| `glr_parser_destroy()` | Destroy parser |
| `glr_parser_reset()` | Reset parser for new parse |
| `glr_parse()` | Parse input string |
| `glr_parser_set_user_data()` | Set user data |
| `glr_parser_get_user_data()` | Get user data |
| `glr_parser_get_error()` | Get last error |
| `glr_parser_get_forest()` | Get parse forest |
| `glr_parser_stack_count()` | Get active stack count |

### Stack Functions

| Function | Description |
|----------|-------------|
| `glr_stack_create()` | Create empty stack |
| `glr_stack_destroy()` | Destroy stack |
| `glr_stack_push()` | Push state |
| `glr_stack_pop()` | Pop state |
| `glr_stack_peek()` | Peek at top |
| `glr_stack_get()` | Get state by height |
| `glr_stack_fork()` | Fork at height |
| `glr_stack_height()` | Get height |
| `glr_stack_empty()` | Check if empty |

### Forest Functions

| Function | Description |
|----------|-------------|
| `glr_forest_create()` | Create empty forest |
| `glr_forest_destroy()` | Destroy forest |
| `glr_forest_get_node()` | Get/create node |
| `glr_forest_add_child()` | Add child |
| `glr_forest_get_children()` | Get children |
| `glr_forest_add_edge()` | Add edge |
| `glr_forest_get_edges()` | Get edges |

### Fork Functions

| Function | Description |
|----------|-------------|
| `glr_fork_create()` | Create fork |
| `glr_fork_destroy()` | Destroy fork |
| `glr_fork_next()` | Get next fork |
| `glr_fork_set_context()` | Set context |
| `glr_fork_get_context()` | Get context |
| `glr_fork_has_context()` | Check context |

### Version Functions

```c
const char *glr_version(void);  // Returns "1.0.0"
const char *glr_name(void);     // Returns "LibGLR"
```

## Examples

### Example 1: Basic Parser

See `examples/calc.c` for a complete calculator parser.

### Example 2: Ambiguous Grammar

See `examples/ambiguous.c` for ambiguity handling.

### Example 3: Test Suite

See `tests/` directory for comprehensive test cases.

## Building Examples

```bash
# Build all targets
cmake .. -DBUILD_EXAMPLES=ON

# Build specific example
make calc
make ambiguous

# Run examples
./build/calc "1 + 2 * 3"
./build/ambiguous "a + b * c"
```

## Documentation

### Doxygen API Reference

```bash
# Build documentation
cd doc
doxygen Doxyfile

# View HTML docs
xdg-open docs/html/index.html
```

### Manpage

```bash
# Install manpage
sudo make install

# View manpage
man -l man/libglr.3tb
# or
man 3tb libglr
```

## Troubleshooting

### Common Issues

**"Memory allocation failed"**
- Ensure sufficient memory
- Check for memory leaks
- Validate grammar before parsing

**"Grammar error"**
- Verify start symbol is set
- Ensure head is non-terminal
- Check symbol IDs are valid

**"Unrecoverable error"**
- Grammar may be invalid
- Input may be malformed
- Check parser state

### Debug Tips

```c
// Enable verbose output
glr_parser_set_user_data(parser, (void*)verbose_mode);

// Check grammar validity
printf("Symbols: %zu\\n", grammar->symbol_count);
printf("Productions: %zu\\n", grammar->production_count);

// Track parse progress
printf("Stacks: %zu\\n", glr_parser_stack_count(parser));
printf("Position: %zu\\n", result.position);
```

## Contributing

LibGLR is part of the TwinBooks project. For contributions:

1. Follow coding conventions (C99, Doxygen docs)
2. Add tests for new functionality
3. Update documentation
4. Run full test suite before submitting

## License

LibGLR is part of the TwinBooks project and follows the project's license terms.

## References

- **TwinBooks**: "Software Language Tooling: a Vademecum" and "Compilonomikon"
- **GLR Parsing**: "Generalized LR Parsing" by Jan Langer
- **SPPF**: "Parsing Ambiguous Grammars Using SPPFs" by Tomita et al.

---

**Version**: 1.0.0  **Section**: 3tb  **Library**: LibGLR
