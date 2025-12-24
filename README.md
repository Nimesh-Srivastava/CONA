# CONA (Clearn Open Native Animation)

**CONA** is a high-performance, modular animation engine built in C. It showcases advanced real-time graphics handling, procedural generation, and hot-reloadable architecture.

![CONA Engine Demo](example.mov)


## Key Features

### 1. Production-Grade UI
-   **Interactive Home Screen**: polished landing page with dynamic background and hover effects.
-   **Navigation**: Seamless transition between menu and simulation modes.
-   **Responsive Layouts**: UI elements scale and position dynamically.

### 2. 3D Neural Network Simulation
Visualization of a Deep Neural Network training loop for Digit Recognition (MNIST-style).
-   **Architecture**: 8x8 Input Grid -> 2 Hidden Layers -> Output Classes (0-9).
-   **Training Loop**: Visualizes Input -> Propagation -> Interference -> Backpropagation.
-   **Interactive Camera**: Full 3D mouse orbital control (Third-Person view).

### 3. Core Engine Architectures
-   **Hot-Reloading**: Edit code in `plug.c` and see changes instantly without restarting.
-   **Procedural Audio**: Real-time synthesized audio synchronized with animation events.
-   **Cross-Platform**: Designed for clean compilation on macOS (Linux/Windows extensible).

## Quick Start

### Prerequisites
-   **Compiler**: `clang` or `gcc`
-   **Dependencies**: Raylib (included in vendor directory or system installed)

### Build & Run
The project includes a self-contained build system in C (`nob.c`).

```bash
# Compile the build system and the engine
cc -o nob nob.c
./nob

# Run the engine
./main
```

## Controls
-   **Mouse Left Click**: interact with UI buttons.
-   **Mouse Drag**: Rotate camera in Simulation mode.
-   **Mouse Wheel**: Zoom in/out.

## Architecture

-   `main.c`: Core window management and plugin loader.
-   `plug.c`: The "Game Cartridge". Contains all logic for UI, Animation, and Audio.
-   `nob.c`: Zero-dependency build system.
