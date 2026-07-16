# 3D Virtual Art Gallery - Compilation and User Manual

This file provides the necessary instructions to configure, build, run, and interact with the development stages of the project (Stage 00 through Stage 12, the final delivered version).

## 1. How to Build All Stages Together

To build the project, open your terminal, navigate to the root directory of the project, and execute the following commands:

```bash
# Configure the project and generate the build directory
cmake -S . -B build

# Compile all executable stages
cmake --build build
```

### Command lines to launch the executables

Each stage compiles to its own executable, named `TappaNN` (e.g. `Tappa00`, `Tappa01`, ... `Tappa12`), following the numbering of its corresponding source file in `src/`.

On Windows:
```bash
# To run Stage 00
.\build\Tappa00.exe
```

On Linux/macOS:
```bash
# Relative execution paths for UNIX environments:
./build/Tappa00
```

## 2. Schematic List of User Interface Controls

**Movement**

- `W` / `S` / `A` / `D`: Move forward/backward/strafe left/right, relative to where the camera is facing. Available from Stage 02 onward.
- Mouse: Look around (rotate the camera's yaw and pitch). Available from Stage 03 onward.
- From Stage 08 onward, movement is restricted to the horizontal plane (the camera can no longer float up/down by looking up/down while moving), simulating walking on the floor.

**Window and Mouse Control (Stage 11 onward)**

- `Tab`: Toggle between "camera mode" (mouse hidden and locked to control the view) and "window mode" (mouse free, to resize, move, or close the window normally)
- `Escape`: Close the application
- The window can be freely resized, maximized, or moved like any standard desktop window while in "window mode"

