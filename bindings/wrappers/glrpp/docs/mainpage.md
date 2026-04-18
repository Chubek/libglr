@mainpage GLR++ Documentation

\tableofcontents

## Overview

GLR++ is a header-only C++ wrapper over `libglr` that keeps the power of generalized LR parsing while presenting a modern, expressive interface for grammars, semantic actions, parse forests, and parser orchestration.

This reference manual is built from two sources:

- Doxygen comments extracted from `include/glrpp`.
- The Markdown booklet chapters in `docs/booklet`, loaded in the order declared by `docs/booklet/Manifest.yaml`.

## Build Targets

The documentation toolchain exposes the same primary targets across supported build systems:

- `html` generates the HTML reference with a dark theme.
- `latex` generates the LaTeX sources emitted by Doxygen.
- `pdf` builds the LaTeX output into a PDF manual.

For CMake, a typical flow is:

```sh
mkdir build
cd build
cmake -DBUILD_DOCS=ON ..
make html
make latex
make pdf
```

## Guide Structure

The booklet chapters cover:

- parser construction and lifecycle,
- readers, lexers, and token sources,
- grammar and native DSL design,
- semantic actions and AST construction,
- ambiguity management and parse forests,
- metaprogramming, reflection, and advanced case studies.

## API Modules

The public API is organized under `include/glrpp`:

- `glr/` exposes parser-facing runtime objects.
- `dsl/` provides grammar, rule, token, and action builders.
- `meta/` contains compile-time helpers and reflection utilities.
- `util/` offers diagnostics, expected-like utilities, and string helpers.

## Reading Order

Start with the booklet chapters on setup, installation, and basic usage, then move to the parser, grammar, DSL, and semantic-action sections before diving into metaprogramming and the larger case studies.
