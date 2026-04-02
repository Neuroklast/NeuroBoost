#!/bin/bash
# Build script for NeuroBoost
# Builds DSP test, VST3 plugin, and standalone application

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

BUILD_TYPE="${1:-Release}"
PLATFORM="${2:-$(uname -s)}"

echo "Building NeuroBoost..."
echo "  Build type: $BUILD_TYPE"
echo "  Platform: $PLATFORM"
echo "  Build directory: $BUILD_DIR"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Check if we can build full plugin or just DSP test
BUILD_FULL_PLUGIN="OFF"
if [ -d "$PROJECT_ROOT/deps/iPlug2" ] && [ -d "$PROJECT_ROOT/deps/visage" ]; then
    if [ -f "$PROJECT_ROOT/deps/visage/build/libvisage.a" ] || \
       [ -f "$PROJECT_ROOT/deps/visage/build/Release/visage.lib" ]; then
        BUILD_FULL_PLUGIN="ON"
        echo "  Full plugin build: enabled (dependencies found)"
    else
        echo "  Full plugin build: disabled (Visage not built)"
        echo "  Run ./scripts/setup_deps.sh to build Visage"
    fi
else
    echo "  Full plugin build: disabled (dependencies not found)"
    echo "  Run ./scripts/setup_deps.sh first for full plugin build"
fi

# Configure with CMake
echo ""
echo "Configuring with CMake..."
cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_DSP_TEST=ON \
    -DBUILD_VST3="$BUILD_FULL_PLUGIN" \
    -DBUILD_STANDALONE="$BUILD_FULL_PLUGIN" \
    ..

# Build
echo ""
echo "Building..."
cmake --build . --config "$BUILD_TYPE" --parallel

# Run tests
echo ""
echo "Running tests..."
ctest --output-on-failure || true

echo ""
echo "Build complete!"
echo ""
echo "Output locations:"
if [ -f "$BUILD_DIR/bin/test_sequencer" ]; then
    echo "  DSP Test: $BUILD_DIR/bin/test_sequencer"
fi
if [ -d "$BUILD_DIR/bin" ]; then
    ls -la "$BUILD_DIR/bin/" 2>/dev/null || true
fi
if [ -d "$BUILD_DIR/vst3" ]; then
    echo "  VST3: $BUILD_DIR/vst3/"
    ls -la "$BUILD_DIR/vst3/" 2>/dev/null || true
fi
