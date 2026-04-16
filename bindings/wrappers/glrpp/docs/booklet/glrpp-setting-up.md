A productive glrpp setup has two layers: the wrapper headers themselves and the runtime library that the wrapper opens dynamically. Because glrpp is header-only, compilation is straightforward. Because it delegates actual parsing to libglr through `libltdl`, runtime availability still matters.

## Toolchain checklist

For day-to-day work, prepare the following:

- a C++20 compiler such as `g++` or `clang++`
- the glrpp headers under `bindings/wrappers/glrpp/include`
- the bundled third-party headers under `bindings/wrappers/glrpp/third_party`
- a discoverable `libglr.so` in the system shared-library search path
- optionally, Pandoc for booklet generation and Doxygen for API docs

The wrapper already assumes several bundled libraries are available. The notable ones are CTRE for compile-time regex, Brigand-style type-list utilities, and `libltdl` for portable dynamic loading.

## Recommended project layout

A small project using glrpp usually looks like this:

```text
my-parser/
  CMakeLists.txt
  include/
  src/
  grammars/
  tests/
```

Within that project, keep the grammar definition close to its semantic layer. A pleasant pattern is:

- `grammar.hpp` for the DSL grammar
- `scanner.hpp` for CTRE token rules
- `parser.cpp` for assembly and entry points
- `ast.hpp` for domain nodes

## Include paths

At compile time you need glrpp headers plus any third-party headers that are not re-exported by your build system. A direct command line often looks like this:

```bash
g++ -std=c++20   -Ibindings/wrappers/glrpp/include   -Ibindings/wrappers/glrpp/third_party/compile-time-regular-expressions/include   -Ibindings/wrappers/glrpp/third_party/libltdl   -Iinclude   src/main.cpp -o my_parser
```

## Runtime search path expectations

glrpp loads libglr from normal shared-library search locations. On Linux that commonly means one of:

- `/usr/lib`
- `/usr/local/lib`
- a directory listed in `ld.so.conf`
- a path exposed through `LD_LIBRARY_PATH`

The wrapper tries portable loader lookups such as `lt_dlopenext("libglr")` and `lt_dlopenext("glr")`. You do not hardcode the library path in ordinary use.

## Sanity-checking the environment

Before writing grammar code, verify two things:

1. a trivial program including `glrpp/glrpp.hpp` compiles
2. a parser object can be constructed without a runtime loader error

A minimal smoke test is:

```cpp
#include <glrpp/glrpp.hpp>

int main() {
  using namespace glrpp;
  const auto g = make_grammar(
      "Start",
      {production("Start", sym(terminal("word")))});
  glr::parser p(g);
}
```

If compilation fails, your include paths are wrong. If construction throws, libglr is probably not installed in a searchable location.

## Development workflow tips

- start with token-stream parsing first; introduce scanner hooks later
- keep grammar examples tiny until parser creation succeeds reliably
- add tests that only compile the headers, because many wrapper mistakes are header hygiene mistakes
- when experimenting with scannerless parsing, keep Unicode examples in your test set from the start

A good setup is not glamorous, but it removes almost every source of confusion later in the project.
