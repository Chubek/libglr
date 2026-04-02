# LibGLR: Generalized LR Parser Library

**Part of the TwinBooks Project, See PROJECT.md**

LibGLR is a minimal C library for implementing Generalized LR (GLR) parsers. It provides facilities for:

- **Grammar data structures** - Symbols, productions, and grammar representation
- **DAG-based stacks** - Efficient state management with forking support
- **SPPF (Shared Parse Forest)** - Compact representation of all parse trees
- **Ambiguity handling** - Native support for ambiguous grammars
- **Reduction operations** - GLR parsing primitives

## Quick Start

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# Run tests
ctest

# Install
sudo make install
```

## Features

- Pure C99 implementation
- Doxygen-documented API
- CMake build system
- Comprehensive test suite
- Example parsers (calculator, ambiguity demo)
- Manpage documentation

## Project Structure

```
libglr/
├── include/glr/      # Public headers
│   ├── glr.h        # Main header
│   ├── grammar.h    # Grammar API
│   ├── stack.h      # DAG-based stack
│   ├── fork.h       # Forking mechanism
│   ├── forest.h     # SPPF forest
│   ├── reduction.h  # Reduction operations
│   ├── graph.h      # Graph operations
│   └── parser.h     # Parser core
├── src/glr/         # Implementation files
├── tests/           # Test suite
├── examples/        # Example parsers
│   ├── calc.c       # Calculator parser
│   └── ambiguous.c  # Ambiguity demo
├── doc/             # Documentation
├── man/             # Manpage (3tb section)
├── scripts/         # Utility scripts
├── CMakeLists.txt   # Build configuration
└── GUIDE.md         # User guide
```

## Documentation

- **User Guide**: `GUIDE.md` - Comprehensive usage guide
- **Installation**: `INSTALL.md` - Installation instructions
- **API Reference**: Doxygen docs (after building)
- **Manpage**: `man/libglr.3tb` - System documentation

## Usage Example

```c
#include <glr/glr.h>

// Create grammar
glr_grammar_t *grammar = glr_grammar_create();
int expr = glr_grammar_add_symbol(grammar, GLR_SYMBOL_NONTERMINAL, "expr");
int num = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "NUMBER");

// Add production
glr_grammar_add_production(grammar, expr,
    (glr_symbol_t[1]){glr_grammar_get_symbol(grammar, num)}, 1);

glr_grammar_set_start_symbol(grammar, expr);

// Parse
glr_parser_t *parser = glr_parser_create(grammar);
glr_parse_result_t result = glr_parse(parser, "123", 3);

if (result.error == GLR_PARSE_SUCCESS) {
    printf("Parse successful!\\n");
}

glr_parser_destroy(parser);
glr_grammar_destroy(grammar);
```

## Building Documentation

```bash
# Install Doxygen first
# Ubuntu: sudo apt-get install doxygen
# macOS: brew install doxygen

cd doc
doxygen Doxyfile
xdg-open docs/html/index.html
```

## Testing

```bash
# Run all tests
ctest --output-on-failure

# Or use test script
./scripts/test-library.sh --build
```

## Scripts

- `scripts/test-library.sh` - Build and run tests
- `scripts/compile.sh` - Build library
- `scripts/install.sh` - Install library
- `scripts/build-docs.sh` - Build documentation
- `scripts/extract-constructs.py` - Extract C constructs (PyCParser)

## Requirements

- CMake 3.16+
- C99 compiler (GCC, Clang)
- Python 3 + PyCParser (for `extract-constructs.py`)
- Doxygen (optional, for documentation)

## License

LibGLR is part of the TwinBooks project:
- "Software Language Tooling: a Vademecum"
- "Compilonomikon"

## Contributing

1. Follow C99 coding style
2. Add Doxygen docstrings
3. Write tests for new features
4. Update documentation

## See Also

- [GUIDE.md](GUIDE.md) - User guide
- [INSTALL.md](INSTALL.md) - Installation guide
- [AGENTS.md](AGENTS.md) - Agent instructions
