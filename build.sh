#!/bin/bash

# Build script for Mandelbrot Set Explorer C++
echo "Building Mandelbrot Set Explorer..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

# Change to build directory
cd build

# Run CMake configuration
echo "Configuring with CMake..."
cmake .. || {
    echo "CMake configuration failed!"
    exit 1
}

# Build the project
echo "Compiling..."
make -j$(nproc 2>/dev/null || echo 4) || {
    echo "Build failed!"
    exit 1
}

echo "Build successful!"
echo "Run the application with: ./build/bin/mandelbrotset"
echo ""
echo "Controls:"
echo "  Mouse wheel: Zoom in/out"
echo "  Left click + drag: Pan"
echo "  R: Reset view"
echo "  C: Cycle color modes"
echo "  +/-: Adjust iterations"
echo "  ESC: Exit"
