#!/bin/bash
# test-library.sh - Run LibGLR test suite
#
# Usage: ./scripts/test-library.sh [options]
# Options:
#   --build     Build tests before running
#   --clean     Clean build artifacts before building
#   --verbose   Enable verbose output

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --build     Build tests before running"
    echo "  --clean     Clean build artifacts before building"
    echo "  --verbose   Enable verbose output"
    echo "  --help      Show this help message"
    echo ""
}

build() {
    local verbose=""
    if [[ "$VERBOSE" == "true" ]]; then
        verbose="-v"
    fi
    
    log_info "Building LibGLR tests..."
    
    if [[ "$CLEAN" == "true" ]]; then
        log_info "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake .. -DCMAKE_BUILD_TYPE=Release $verbose
    cmake --build . $verbose
    
    if [[ $? -ne 0 ]]; then
        log_error "Build failed"
        exit 1
    fi
    
    log_info "Build successful"
}

run_tests() {
    log_info "Running tests..."
    cd "$BUILD_DIR"
    
    local verbose=""
    if [[ "$VERBOSE" == "true" ]]; then
        verbose="--verbose"
    fi
    
    ctest $verbose --output-on-failure
    
    if [[ $? -ne 0 ]]; then
        log_error "Tests failed"
        exit 1
    fi
    
    log_info "All tests passed"
}

# Parse arguments
BUILD=false
CLEAN=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --build)
            BUILD=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Main execution
if [[ "$BUILD" == "true" ]]; then
    build
fi

run_tests
