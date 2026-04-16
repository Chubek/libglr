glrpp is best understood as a header-only facade over a dynamically loaded parsing runtime. Installation therefore has two parts: make the headers visible to the compiler, and make `libglr.so` visible to the process loader.

## Header-only integration

The simplest installation model is vendoring. Copy or reference:

- `bindings/wrappers/glrpp/include/glrpp`
- the third-party include trees used by those headers

Then expose them with your build system.

### CMake example

```cmake
add_library(glrpp INTERFACE)
target_include_directories(glrpp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/wrappers/glrpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/wrappers/glrpp/third_party/compile-time-regular-expressions/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/wrappers/glrpp/third_party/libltdl
)
```

Because the wrapper is header-only, consumers usually link no wrapper object file. They still need the libglr runtime present when the program starts.

### Meson example

```meson
glrpp_inc = include_directories(
  'bindings/wrappers/glrpp/include',
  'bindings/wrappers/glrpp/third_party/compile-time-regular-expressions/include',
  'bindings/wrappers/glrpp/third_party/libltdl',
)
```

### Autotools example

Expose the include directories through `AM_CPPFLAGS`, then ensure libglr is installed into a directory your loader can discover.

## Runtime installation

The parser implementation lives in libglr. If it is installed globally, the wrapper can discover it through shared-library search paths. If it is not installed globally, common strategies are:

- install it under `/usr/local/lib` and run the platform-specific cache update
- ship it beside your application and configure the loader path appropriately
- package it as a runtime dependency of your project

## Linking notes

The user-facing wrapper does not require static linkage to libglr because loading is dynamic. The notable dependency in the wrapper layer is `libltdl`, which provides a cross-platform abstraction over `dlopen`, `LoadLibrary`, and related facilities.

That design has a few advantages:

- the wrapper remains header-only
- the application can fail gracefully if libglr is missing
- loader behavior is consistent across supported platforms

## Verifying installation from C++

A practical validation step is to instantiate a parser and try a trivial parse:

```cpp
using namespace glrpp;

const auto grammar = make_grammar(
    "Start",
    {production("Start", sym(terminal("number")))});

glr::parser parser(grammar);
auto result = parser.parse(dsl::token_stream{make_token("number", "42")});
```

If parser construction succeeds but `parse` reports a grammar issue, installation is fine and the problem is in your grammar. If parser construction fails immediately, inspect runtime library discovery first.

## Packaging guidance

When shipping glrpp to users, package these concerns separately:

- development package: headers and examples
- runtime package: libglr and any loader dependencies
- optional docs package: generated booklet and API reference

This separation keeps builds lightweight while ensuring that runtime deployment remains explicit.

## Versioning advice

Header-only libraries can drift from runtimes. Try to keep glrpp and libglr versions aligned within the same distribution or repository snapshot. If you upgrade one, rerun parser smoke tests and Unicode scanner tests immediately.
