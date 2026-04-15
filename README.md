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
- parser, forest, stack, and disambiguation infrastructure
- GLR Rewrite Language (GRL) written as S-expressions
- procedural rewrite API for normalization pipelines
- reusable rewrite library programs for common grammar cleanup passes

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
