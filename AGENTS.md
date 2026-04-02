# Guide for Agents

This file has been provided for LLM-based agents to find their footing in the project. LibGLR is a C library that offers functionality for implementation of Tomita parsers (Generalized LR Parsers).

LibGLR is a part of the TwinBooks project. TwinBooks is a project comprised of two books: "Software Language Tooling: a Vademecum" and "Compilonomikon". In these books, we explain implementation of several PLT and SLE-related projects. LibGLR is a support library for several projects in TwinBooks project.

## Project Overview

LibGLR is a very minimal library. It is not an application, it is a library. It only provides a data structure for grammars, it does not offer any facilities for parsing of gramamr files. It offers code for DAG-based stacks, SPPFs, forking and reduction. It is meant to be plugged into a larger project that is implementing a GLR parser.

## Repository Structure

- `src/` - Main source code
- `include/` - Interface files
- `tests/` - Test files
- `doc/` - Documentation
- `scripts/` - Utility scripts

## Development Setup

We have offered a CMake file, and a NovaMK file. You are only supposed to modify the CMake file, because NovaMK is a one of the toolings offered by the TwinBooks project.

There's also several scripts in the `scripts/` directory that help with installation of the library.

Your job is to implement them.

## Testing

All the tests are in the `tests/` directory. We have provided a script, `scripts/test-library.sh` that will deal with running othe tests. You must implement the script as well.

There are two grammar files in `examples/` directory. These are not grammars written in a language like EBNF, rather, these are C files where the grammars are explicitly provided.

## Tasks

Your tasks are:

1. Implementation of the library, that includes the interface (header files in `include/`) and implemntations (files in `src/` directory);
2. Writing the CMake specifications and Makefiles, in `CMakeLists.txt` and `doc/CMakeLists.txt`;
3. Writing the scripts inside `scripts/`directory;
4. Running the tests;
5. Writing the examples;
6. Writing the `GUIDE.md` and `INSTALL.md` files, the former is a full guide on using the library;

## Documentation

### Docstrings

We use Doxygen for stripping off docstrings. The docstrings you write for LibGLR must be clear, concise, and compatible with Doxygen. These docstrings must help the user navigate LibGLR, and make sure they are not lost.

### Manpage

You must write a manpage for LibGLR, `man/libglr.3tb`. The `3tb` manpage section stands for TwinBooks. CMake must provide facilities for instaling these manpage.

### `GUIDE.md` & `INSTALL.md`

`GUIDE.md`  must contain detailed info on how to use LibGLR. `INSTALL.md` must provide information on installing LibGLR.

## Miscellany

You must write a Python script, `scripts/extract-constructs.py`, that utilizes the PyCParser library to parse C, and save all the constructs (data structures, functions, typedefs, etc) into a S-Expression, JSON or YAML file. PyCParser is globally installed on my system, so you can test it. Make sure the script has documentation on top of it.
