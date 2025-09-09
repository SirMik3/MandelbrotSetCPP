# Mandelbrot Set Explorer - C++ Edition

A high-performance, interactive Mandelbrot set renderer implemented in C++ using SFML and OpenGL shaders. This project is a complete rewrite of the Python-based [MandelbrotSet renderer](https://github.com/SirMik3/MandelbrotSet) into modern C++ with enhanced performance and interactivity.

## Features

- **Real-time GPU rendering** using OpenGL 3.3+ core profile and GLSL shaders
- **Interactive exploration** with smooth zoom and pan controls
- **Multiple color palettes** with dynamic switching
- **High-performance** fractal computation on the GPU
- **Adjustable iteration count** for varying levels of detail
- **Cross-platform support** (Windows, macOS, Linux)

## Controls

| Input | Action |
|-------|--------|
| **Mouse Wheel** | Zoom in/out at cursor position |
| **Left Click + Drag** | Pan the view |
| **R** | Reset view to default position and zoom |
| **C** | Cycle through color modes (3 different palettes) |
| **+** | Increase iteration count (+10) |
| **-** | Decrease iteration count (-10) |
| **ESC** | Exit application |

## Building

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- OpenGL 3.3+ compatible graphics card and drivers

### Dependencies

The project uses the following libraries (automatically managed by CMake):
- **SFML 3.0+** - Window management and OpenGL context creation
- **OpenGL 3.3+** - Graphics rendering

### Build Instructions

1. Clone the repository:
```bash
git clone <repository-url>
cd MandelbrotSet
```

2. Create build directory and compile:
```bash
mkdir build
cd build
cmake ..
make
```

3. Run the application:
```bash
./bin/mandelbrotset
```

## Project Structure

```
MandelbrotSet/
├── src/
│   └── main.cpp              # Main application logic and event handling
├── res/
│   └── shaders/
│       ├── vertex.glsl       # Vertex shader (fullscreen quad)
│       └── fragment.glsl     # Fragment shader (Mandelbrot computation)
├── include/
│   └── matrix.h              # Matrix utilities (if needed)
├── CMakeLists.txt            # Build configuration
└── README.md                 # This file
```

## Technical Implementation

### GPU-Accelerated Rendering

The Mandelbrot set computation is performed entirely on the GPU using GLSL fragment shaders. Each pixel's complex number is calculated in parallel, making the rendering extremely efficient even at high zoom levels.

### Shader Architecture

- **Vertex Shader**: Sets up a fullscreen quad and passes UV coordinates
- **Fragment Shader**: 
  - Converts screen coordinates to complex plane coordinates
  - Performs Mandelbrot iteration for each pixel
  - Applies color mapping based on escape time
  - Supports multiple color palettes

### Key Features

1. **Zoom-to-Mouse**: Zoom operations are centered on the mouse cursor position for intuitive exploration
2. **Aspect Ratio Correction**: Maintains proper proportions regardless of window size
3. **Dynamic Parameters**: All rendering parameters can be adjusted in real-time
4. **Efficient Memory Usage**: Minimal CPU-GPU data transfer

## Color Palettes

The renderer includes three distinct color palettes:

1. **Classic Blue-White** (Mode 0): Traditional gradient from deep blue to white
2. **Fire Colors** (Mode 1): Warm colors using cosine-based palette generation
3. **Rainbow** (Mode 2): Full spectrum rainbow colors

## Performance

The C++ implementation offers significant performance improvements over the original Python version:

- **GPU Acceleration**: All fractal computation on graphics hardware
- **Compiled Code**: Native machine code execution
- **Optimized Memory Layout**: Efficient data structures and minimal allocations
- **Real-time Interaction**: Smooth 60 FPS rendering with immediate response to user input

## Comparison to Original Python Version

This C++ version provides the following enhancements over the [original Python implementation](https://github.com/SirMik3/MandelbrotSet):

| Feature | Python Version | C++ Version |
|---------|----------------|-------------|
| **Performance** | Moderate (PyOpenGL) | High (Native OpenGL) |
| **Startup Time** | Slow (Interpreter) | Fast (Compiled) |
| **Memory Usage** | Higher | Lower |
| **Zoom Depth** | Limited | Extended |
| **Responsiveness** | Good | Excellent |
| **Dependencies** | Python + NumPy + PyOpenGL + GLFW | Self-contained with SFML |

## Mathematical Background

The Mandelbrot set is defined as the set of complex numbers `c` for which the function `f(z) = z² + c` does not diverge when iterated from `z = 0`.

For each point in the complex plane:
1. Start with `z₀ = 0`
2. Iterate `zₙ₊₁ = zₙ² + c`
3. If `|z| > 2`, the point escapes (not in the set)
4. Color based on how quickly the point escapes

## Customization

The renderer can be easily customized by modifying:

- **Iteration Count**: Adjust `maxIterations` for more detail (trade-off with performance)
- **Color Palettes**: Modify the color functions in `fragment.glsl`
- **Initial View**: Change default zoom and offset in `MandelbrotParams`
- **Zoom Speed**: Adjust the zoom factor in the mouse wheel handler

## Contributing

Contributions are welcome! Areas for improvement:

- Additional color palettes
- Julia set mode
- Double precision for deeper zoom levels
- Save/load view positions
- Animation recording
- Multi-threading for CPU fallback

## License

This project is licensed under the MIT License - see the original repository for details.

## Acknowledgments

- Original Python implementation by [SirMik3](https://github.com/SirMik3/MandelbrotSet)
- SFML development team for the excellent multimedia library
- OpenGL community for graphics programming resources
