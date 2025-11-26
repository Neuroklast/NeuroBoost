#!/bin/bash
# Setup script for NeuroBoost dependencies
# This script clones and builds iPlug2 and Visage

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPS_DIR="$PROJECT_ROOT/deps"

echo "Setting up NeuroBoost dependencies..."
echo "Project root: $PROJECT_ROOT"
echo "Dependencies directory: $DEPS_DIR"

mkdir -p "$DEPS_DIR"
cd "$DEPS_DIR"

# Clone and setup iPlug2
if [ ! -d "iPlug2" ]; then
    echo "Cloning iPlug2..."
    git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/iPlug2/iPlug2.git
    echo "iPlug2 cloned successfully"
else
    echo "iPlug2 already exists, skipping clone"
fi

# Clone and build Visage
if [ ! -d "visage" ]; then
    echo "Cloning Visage..."
    git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/VitalAudio/visage.git
    cd visage
    
    echo "Building Visage..."
    mkdir -p build
    cd build
    cmake -DVISAGE_BUILD_EXAMPLES=OFF -DVISAGE_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release ..
    cmake --build . --config Release --parallel
    cd ../..
    
    echo "Visage built successfully"
else
    echo "Visage already exists, checking build..."
    if [ ! -f "visage/build/libvisage.a" ] && [ ! -f "visage/build/Release/visage.lib" ]; then
        echo "Visage not built, building now..."
        cd visage
        mkdir -p build
        cd build
        cmake -DVISAGE_BUILD_EXAMPLES=OFF -DVISAGE_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release ..
        cmake --build . --config Release --parallel
        cd ../..
    else
        echo "Visage already built"
    fi
fi

echo ""
echo "Dependencies setup complete!"
echo ""
echo "Next steps:"
echo "  1. Run: ./scripts/build.sh"
echo "  2. Or use CMake directly: mkdir build && cd build && cmake .. && cmake --build ."
