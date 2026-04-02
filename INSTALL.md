# Installing LibGLR

This document describes how to install LibGLR on your system.

## Prerequisites

### Build Tools

- **CMake** 3.16 or later
- **C compiler** (GCC, Clang, or compatible)
- **make** or equivalent build tool

### Documentation Tools (Optional)

- **Doxygen** 1.8.0 or later (for API documentation)

## Quick Install

### Using System Package Manager

If available in your distribution:

```bash
# Ubuntu/Debian
sudo apt-get install libglr-dev

# Fedora/RHEL
sudo dnf install libglr-devel

# Arch Linux
sudo pacman -S libglr
```

### Build from Source

#### Standard Installation

```bash
# Clone repository
git clone https://github.com/twinbooks/libglr.git
cd libglr

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make

# Install (requires root/sudo)
sudo make install
```

#### Installation Location

By default, LibGLR installs to:
- **Headers**: `/usr/local/include/glr/`
- **Library**: `/usr/local/lib/libglr.a`
- **Manpage**: `/usr/local/share/man/man3/libglr.3tb`
- **CMake config**: `/usr/local/lib/cmake/libglr/`

## Installation with CMake Options

### Custom Installation Prefix

Install to a custom location:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/libglr
make
sudo make install
```

### Build Options

```bash
# Build without tests
cmake .. -DBUILD_TESTS=OFF

# Build without examples
cmake .. -DBUILD_EXAMPLES=OFF

# Build without documentation
cmake .. -DBUILD_DOCUMENTATION=OFF

# Debug build with symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Full Custom Installation

```bash
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/libglr \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_DOCUMENTATION=ON

make -j$(nproc)
sudo make install
```

## Manual Installation

If you prefer not to use `make install`:

### 1. Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### 2. Install Files Manually

```bash
# Create directories
sudo mkdir -p /usr/local/include/glr
sudo mkdir -p /usr/local/lib
sudo mkdir -p /usr/local/share/man/man3

# Copy headers
sudo cp ../include/glr/*.h /usr/local/include/glr/

# Copy library
sudo cp libglr.a /usr/local/lib/

# Install manpage
sudo cp ../man/libglr.3tb /usr/local/share/man/man3/

# Update man page database
sudo mandb
```

### 3. Update Library Path

```bash
# Add to /etc/ld.so.conf.d/libglr.conf
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/libglr.conf

# Update cache
sudo ldconfig
```

## User Installation (No Root)

Install to user's local directory:

```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
```

Add to environment:

```bash
export CMAKE_PREFIX_PATH="$HOME/.local:$CMAKE_PREFIX_PATH"
export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"
export MANPATH="$HOME/.local/share/man:$MANPATH"
```

## Verifying Installation

### Check Library

```bash
# Check if library exists
ls -l /usr/local/lib/libglr.a

# Check header files
ls -l /usr/local/include/glr/*.h
```

### Check CMake Configuration

```bash
# Create test project
mkdir test && cd test
cat > CMakeLists.txt << 'TEST'
cmake_minimum_required(VERSION 3.16)
project(test)
find_package(libglr REQUIRED)
target_link_libraries(main PRIVATE libglr)
TEST

cmake .
make
```

### Check Documentation

```bash
# List installed manpages
man -w libglr

# View manpage
man 3tb libglr
```

## Uninstallation

### Using make uninstall

```bash
cd build
sudo make uninstall
```

### Manual Uninstallation

```bash
# Remove files
sudo rm -f /usr/local/lib/libglr.a
sudo rm -rf /usr/local/include/glr
sudo rm -f /usr/local/share/man/man3/libglr.3tb

# Update man database
sudo mandb

# Remove CMake config
sudo rm -rf /usr/local/lib/cmake/libglr
```

## Building Documentation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install doxygen

# Fedora/RHEL
sudo dnf install doxygen

# macOS
brew install doxygen
```

### Build HTML Documentation

```bash
cd doc
doxygen Doxyfile

# View in browser
xdg-open docs/html/index.html
```

### Build Manpage

Manpage is already in `man/libglr.3tb`. Install with:

```bash
make install
# or
sudo make install
```

## Testing

### Run Test Suite

```bash
# After building
cd build
ctest --output-on-failure

# Or use the test script
../scripts/test-library.sh --build
```

### Test Results

Expected output:

```
=== LibGLR Grammar Tests ===
Testing: create_destroy... PASSED
Testing: add_symbols... PASSED
...
=== Results ===
Passed: 18
Failed: 0
```

## Common Issues

### CMake Not Found

```bash
# Install CMake
sudo apt-get install cmake
# or
sudo dnf install cmake
```

### Compiler Errors

Ensure C99 support:

```bash
gcc --version  # Should support -std=c99
clang --version
```

### Permission Denied (Install)

Use `sudo` or install to user directory:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
```

### Library Not Found at Runtime

```bash
# Add library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Or edit ld config
sudo echo "/usr/local/lib" >> /etc/ld.so.conf.d/libglr.conf
sudo ldconfig
```

### Manpage Not Found

```bash
# Check installation
ls /usr/share/man/man3/libglr.3tb
# or
ls /usr/local/share/man/man3/libglr.3tb

# Update man database
sudo mandb

# View with full path
man -l /usr/share/man/man3/libglr.3tb
```

## Next Steps

After installation:

1. **Read the Guide**: See `GUIDE.md` for usage instructions
2. **Try Examples**: Build and run `examples/calc.c`
3. **Run Tests**: Verify installation with test suite
4. **Check Docs**: View API documentation with Doxygen

## Support

For issues:
- Report bugs on the repository
- Check troubleshooting section
- Review `GUIDE.md` for usage

---

**Library**: LibGLR  
**Version**: 1.0.0  
**Project**: TwinBooks
