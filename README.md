# CONA (Clean Open Native Animation)

CONA is a high-performance, modular animation engine written in C. It leverages hot-reloading native plugins to provide a seamless development experience for real-time graphics and procedural audio.

## Overview

The engine utilizes a phase-based state machine to transition between different application states (Splash, Boot, Main). Each state is encapsulated in a dynamically loaded library, allowing for runtime updates without restarting the application.

## Features

- **Hot Reloading:** Modify C code solely for the active phase and reload instantly (Press 'R' in the Main phase).
- **Modular Architecture:** Separation of concerns through distinct shared libraries for Splash, Boot, and Main loops.
- **Advanced Rendering:**
  - Real-time post-processing (Bloom/Glow shaders).
  - 3D spatial visualization.
- **Procedural Audio:**
  - Integrated FM Synthesis for real-time music generation.
  - Audio-reactive visual elements synchronized with the synthesis engine.
- **Cross-Platform Design:** Built on Raylib with abstraction layers for OS-specific dynamic loading (macOS, Windows, Linux).

## Build Instructions

### Prerequisites
- C Compiler (clang/gcc)
- MacOS (for current pre-compiled dependencies) or Raylib installed globally.

### Compilation
The project uses a custom build system (`nob.c`) that handles dependency linking and multiple plugin compilation.

```sh
# Build the build system and then the project
cc -o nob nob.c && ./nob
```

### Running
```sh
./main
```

## Internal Architecture

The engine passes through three distinct phases:

1. **Splash Phase:** Introduction sequence.
2. **Boot Phase:** System initialization simulation (Matrix Rain effect).
3. **Main Phase:** Infinite interactive loop with procedural audio visualization and 3D rendering.

## Project Structure

- `main.c`: Core engine entry point and plugin management.
- `plug.h`: Shared interface definitions.
- `splash.c`: Splash screen implementation.
- `boot.c`: Boot sequence implementation.
- `plug.c`: Main visualizer and audio engine.
- `nob.c`: Build system implementation.
