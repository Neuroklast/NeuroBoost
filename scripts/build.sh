#!/bin/bash
# Build script for NeuroBoost
# Builds VST3 plugin and standalone application

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

# Check if dependencies are set up
if [ ! -d "$PROJECT_ROOT/deps/iPlug2" ] || [ ! -d "$PROJECT_ROOT/deps/visage" ]; then
    echo ""
    echo "Dependencies not found. Please run setup_deps.sh first:"
    echo "  ./scripts/setup_deps.sh"
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo ""
echo "Configuring with CMake..."
cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_VST3=ON \
    -DBUILD_STANDALONE=ON \
    ..

# Build
echo ""
echo "Building..."
cmake --build . --config "$BUILD_TYPE" --parallel

echo ""
echo "Build complete!"
echo ""
echo "Output locations:"
if [ -f "$BUILD_DIR/NeuroBoost.vst3" ]; then
    echo "  VST3: $BUILD_DIR/NeuroBoost.vst3"
fi
if [ -f "$BUILD_DIR/NeuroBoost" ]; then
    echo "  Standalone: $BUILD_DIR/NeuroBoost"
fi
