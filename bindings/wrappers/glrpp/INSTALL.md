# Installing glrpp

`glrpp` is a **header‑only C++20 DSL layer** for the **libglr Generalized LR parser**.
The only compiled component required is the **SWIG‑generated C++ binding layer** that connects the C library (`libglr`) with the C++ wrapper classes used by glrpp.

This document explains how to install all required components and how the SWIG bindings fit into the build.

---

# Overview of the Build Architecture

The project consists of three layers:

1. **libglr (C library)**
   Provides the actual GLR parsing engine.

2. **SWIG‑generated C++ bindings**
   Automatically generated wrappers that expose the C API to C++.

3. **glrpp header‑only library**
   A C++ DSL and runtime wrapper built on top of the generated bindings.

```
          +-----------------------+
          |       glrpp DSL       |
          |  header-only C++ API  |
          +-----------▲-----------+
                      |
              RAII wrappers
                      |
          +-----------▲-----------+
          |  SWIG C++ Bindings   |
          | generated from C API |
          +-----------▲-----------+
                      |
                libglr (C)
          GLR parsing implementation
```

Only the SWIG bindings and the C library must be compiled.

---

# Requirements

Minimum toolchain requirements:

- C++20 compatible compiler
  - GCC ≥ 11
  - Clang ≥ 14
  - MSVC ≥ 19.3x

Required build tools:

- **SWIG 4.1 or newer**
- **CMake ≥ 3.20** or **Meson ≥ 0.60**
- **libglr** development headers
- Python (optional, only if SWIG helpers require it)

Header‑only dependencies used by glrpp:

- Brigand
- CTRE
- Frozen
- magic_enum
- nameof
- Boost.PFR

These libraries are availbe under the `third_party` directory.

---

# Step 1 — Install libglr

You must install the base GLR parsing library first.

Example:

```
git clone https://example.org/libglr.git
cd libglr
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .
```

This installs:

```
libglr.so / libglr.a
libglr headers
```

Typical locations:

```
/usr/local/include/libglr/
/usr/local/lib/libglr.so
```

---

# Step 2 — Generate the SWIG Bindings

glrpp does **not manually maintain C++ wrappers** around the C API.

Instead, SWIG generates them from an interface definition file.

Example interface file:

```
bindings/glr.i
```

Generation command:

```
swig -c++ -I/usr/local/include \
     -o glrpp_glr_bindings.cpp \
     glr.i
```

This produces:

```
glrpp_glr_bindings.cpp
glrpp_glr_bindings.hpp
```

These files expose the C functions of libglr as C++ callable functions.

The generated code should **not be edited manually**.

---

# Step 3 — Build the Binding Layer

Compile the generated file and link it with libglr.

Example:

```
c++ -std=c++20 -c glrpp_glr_bindings.cpp
c++ -shared glrpp_glr_bindings.o -lglr -o libglrpp_glr_bindings.so
```

Result:

```
libglrpp_glr_bindings.so
```

This shared library is used internally by glrpp.

---

# Step 4 — Install glrpp Headers

Copy the public headers:

```
include/glrpp/
```

Example:

```
sudo cp -r include/glrpp /usr/local/include/
```

No compilation is required because the DSL layer is header‑only.

---

# Using CMake

The project provides a CMake build that:

1. locates libglr
2. runs SWIG
3. builds the binding library
4. installs headers

Example:

```
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .
```

The build performs:

```
SWIG → generate bindings
compile → bindings
link → libglr
install → headers + library
```

---

# Using Meson

Meson automatically runs SWIG during configuration.

Example:

```
meson setup build
meson compile -C build
sudo meson install -C build
```

---

# Directory Layout After Installation

Typical installation layout:

```
/usr/local/

include/
└── glrpp/
    ├── dsl/
    ├── glr/
    ├── meta/
    ├── util/
    ├── config.hpp
    └── glrpp.hpp

lib/
├── libglr.so
└── libglrpp_glr_bindings.so
```

---

# How glrpp Uses the SWIG Bindings

The generated bindings are **not used directly by applications**.

Instead, glrpp wraps them in safe C++ classes.

Example:

```
glrpp::glr::parser
glrpp::glr::context
glrpp::glr::forest
```

These classes provide:

- RAII ownership
- type safety
- exception‑safe error handling
- integration with the DSL grammar system

The SWIG layer itself is considered **an implementation detail**.

---

# Verifying the Installation

Create a simple test file:

```cpp
#include <glrpp/glrpp.hpp>

int main() {
    glrpp::glr::parser parser;
}
```

Compile:

```
c++ -std=c++20 test.cpp -lglrpp_glr_bindings -lglr
```

If compilation succeeds, the installation is working.

---

# Updating the SWIG Bindings

If the libglr API changes, regenerate the bindings:

```
swig -c++ -o glrpp_glr_bindings.cpp bindings/glr.i
```

Then rebuild the binding library.

No changes are required in the glrpp DSL headers unless the API changes significantly.

---

# Troubleshooting

## SWIG not found

Install SWIG:

```
sudo apt install swig
```

or

```
brew install swig
```

---

## libglr not found

Ensure the library is installed and visible:

```
ldconfig -p | grep glr
```

or add a custom path:

```
cmake -DCMAKE_PREFIX_PATH=/path/to/libglr ..
```

---

## Missing headers

Make sure `glrpp` headers are installed in:

```
/usr/include
or
/usr/local/include
```

and your compiler search path includes them.

---

# Uninstall

Remove installed files:

```
/usr/local/include/glrpp
/usr/local/lib/libglrpp_glr_bindings.*
```

---

# Additional Documentation

- `README.md` — project overview
- `docs/` — Doxygen API documentation
- `examples/` — example grammars and parsers

For development guidance see the contributor documentation.
:::
