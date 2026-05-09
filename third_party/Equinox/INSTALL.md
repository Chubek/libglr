# Installation Guide

This document provides detailed instructions for building and installing the Equinox E-graph library.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build Options](#build-options)
- [Platform-Specific Instructions](#platform-specific-instructions)
- [Installation Locations](#installation-locations)
- [Verifying Installation](#verifying-installation)
- [Uninstalling](#uninstalling)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Tools

- **C Compiler**: GCC 4.9+, Clang 3.5+, or MSVC 2015+
- **CMake**: Version 3.10 or higher
- **Make** or **Ninja**: Build system (Make is standard on Unix-like systems)

### Optional Tools

- **Doxygen**: For generating API documentation (version 1.8.0+)
- **Git**: For fetching test dependencies (Catch2)

### System Requirements

- C11-compliant compiler
- Minimum 50 MB disk space for build artifacts
- 256 MB RAM for compilation

---

## Quick Start

### Unix-like Systems (Linux, macOS, BSD)
```bash
# Clone or extract the source
cd equinox

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the library
make -j$(nproc)

# Run tests (optional)
make test

# Install (may require sudo)
sudo make install

### Windows (Visual Studio)

cmd
REM Open Developer Command Prompt for VS

cd equinox
mkdir build
cd build

cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release

REM Run tests
ctest -C Release

REM Install (run as Administrator)
cmake --install . --config Release

---

## Build Options

Configure the build using CMake options with `-D<OPTION>=<VALUE>`:

### Library Type

bash
# Build shared library (default)
cmake .. -DBUILD_SHARED_LIBS=ON

# Build static library
cmake .. -DBUILD_SHARED_LIBS=OFF

# Build both shared and static
cmake .. -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=ON

### Testing

bash
# Enable tests (default: ON)
cmake .. -DBUILD_TESTS=ON

# Disable tests
cmake .. -DBUILD_TESTS=OFF

### Documentation

bash
# Enable documentation generation (requires Doxygen)
cmake .. -DBUILD_DOCS=ON

# Build documentation
make docs

# Documentation will be in build/docs/html/

### Installation Prefix

bash
# Install to custom location
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/equinox

# Install to user directory (no sudo needed)
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local

### Build Type

bash
# Debug build (with symbols, no optimization)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build (optimized, default)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Release with debug info
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

### Complete Example

bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_STATIC_LIBS=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=ON \
  -DCMAKE_INSTALL_PREFIX=/usr/local

---

## Platform-Specific Instructions

### Ubuntu/Debian

bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake git doxygen

# Build and install
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
sudo ldconfig  # Update library cache

### Fedora/RHEL/CentOS

bash
# Install dependencies
sudo dnf install gcc cmake make git doxygen

# Build and install
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
sudo ldconfig

### macOS (Homebrew)

bash
# Install dependencies
brew install cmake doxygen

# Build and install
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
sudo make install

### Windows (MinGW)

bash
# Using MSYS2/MinGW64
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make

mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j4
mingw32-make install

### FreeBSD

bash
# Install dependencies
sudo pkg install cmake doxygen

# Build and install
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
sudo make install

---

## Installation Locations

Default installation paths (with `CMAKE_INSTALL_PREFIX=/usr/local`):

| Component | Location |
|-----------|----------|
| Headers | `/usr/local/include/equinox/` |
| Shared library | `/usr/local/lib/libequinox.so` (Linux)<br>`/usr/local/lib/libequinox.dylib` (macOS)<br>`C:\Program Files\equinox\bin\equinox.dll` (Windows) |
| Static library | `/usr/local/lib/libequinox.a` |
| pkg-config | `/usr/local/lib/pkgconfig/equinox.pc` |
| Documentation | `/usr/local/share/doc/equinox/` |

### Custom Installation

bash
# Install to /opt/equinox
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/equinox
make install

# Update PKG_CONFIG_PATH
export PKG_CONFIG_PATH=/opt/equinox/lib/pkgconfig:$PKG_CONFIG_PATH

# Update LD_LIBRARY_PATH (Linux)
export LD_LIBRARY_PATH=/opt/equinox/lib:$LD_LIBRARY_PATH

# Or update DYLD_LIBRARY_PATH (macOS)
export DYLD_LIBRARY_PATH=/opt/equinox/lib:$DYLD_LIBRARY_PATH

---

## Verifying Installation

### Check Installed Files

bash
# List installed files
ls /usr/local/include/equinox/
ls /usr/local/lib/libequinox.*

# Verify pkg-config
pkg-config --modversion equinox
pkg-config --cflags equinox
pkg-config --libs equinox

### Test Compilation

Create `test.c`:

c
#include <equinox/egraph.h>
#include <stdio.h>

int main(void) {
egraph_config_t config = egraph_default_config();
egraph_t *g = egraph_create(&config);

printf("Equinox e-graph created successfully!\n");
printf("Initial e-class count: %zu\n", egraph_num_eclasses(g));

egraph_destroy(g);
return 0;
}

Compile and run:

bash
# Using pkg-config
gcc test.c $(pkg-config --cflags --libs equinox) -o test
./test

# Or manually
gcc test.c -I/usr/local/include -L/usr/local/lib -lequinox -o test
./test

Expected output:

Equinox e-graph created successfully!
Initial e-class count: 0

---

## Uninstalling

### Using CMake

bash
cd build
sudo make uninstall  # If uninstall target exists

### Manual Removal

bash
# Remove installed files
sudo rm -rf /usr/local/include/equinox
sudo rm -f /usr/local/lib/libequinox.*
sudo rm -f /usr/local/lib/pkgconfig/equinox.pc
sudo rm -rf /usr/local/share/doc/equinox

# Update library cache (Linux)
sudo ldconfig

---

## Troubleshooting

### CMake Cannot Find Compiler

bash
# Specify compiler explicitly
cmake .. -DCMAKE_C_COMPILER=gcc

# Or use environment variables
export CC=gcc
cmake ..

### Missing Dependencies

**Error**: `Could not find Git (missing: GIT_EXECUTABLE)`

**Solution**: Install Git or disable tests:
bash
cmake .. -DBUILD_TESTS=OFF

**Error**: `Doxygen not found`

**Solution**: Install Doxygen or disable documentation:
bash
cmake .. -DBUILD_DOCS=OFF

### Library Not Found at Runtime

**Linux**:
bash
# Add library path to ld.so.conf
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/equinox.conf
sudo ldconfig

# Or use LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

**macOS**:
bash
export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH

**Windows**:
- Add installation `bin/` directory to system PATH
- Or copy DLL to application directory

### Permission Denied During Installation

bash
# Use sudo for system-wide installation
sudo make install

# Or install to user directory
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install

### Build Fails with C11 Errors

Ensure your compiler supports C11:

bash
# Check GCC version (need 4.9+)
gcc --version

# Check Clang version (need 3.5+)
clang --version

# Update compiler if needed (Ubuntu)
sudo apt-get install gcc-9
cmake .. -DCMAKE_C_COMPILER=gcc-9

### Tests Fail to Build

bash
# Clean build directory
rm -rf build/*

# Reconfigure with tests enabled
cmake .. -DBUILD_TESTS=ON

# Ensure internet connection for Catch2 download
# Or disable tests if not needed
cmake .. -DBUILD_TESTS=OFF

### Out of Memory During Compilation

bash
# Reduce parallel jobs
make -j2  # Instead of -j$(nproc)

# Or build without parallelism
make

---

## Additional Resources

- **User Manual**: See `docs/manual.i` for detailed API documentation
- **Usage Examples**: See `USAGE.md` for code examples
- **API Reference**: Build documentation with `make docs`
- **Issue Tracker**: Report bugs and request features on the project repository

---

**Installation complete!** You can now use Equinox in your projects. See `USAGE.md` for usage examples.


This comprehensive installation guide covers all major platforms, build configurations, troubleshooting scenarios, and verification steps.