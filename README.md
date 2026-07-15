[Читать на русском](README.ru.md) | [Read in English](README.md)

# Dawn Boids 3D: Flocking Simulation on WebGPU

An interactive 3D flocking behavior simulation (based on Craig Reynolds' Boids algorithm) powered by a modern graphics API. Written in **C++** using **WebGPU (Google Dawn)**, this project demonstrates how offloading mathematical calculations to Compute Shaders enables the real-time processing of up to 150,000 independent agents.

## Simulation Demo
![Dawn Boids 3D Demo](assets/demo.gif)

[Video](https://youtu.be/idvMb-dypzo)

## Simulation Features

The simulation engine runs entirely on the GPU and implements several key interaction scenarios:

1. **Basic Flocking Behavior (Boids):** Every agent evaluates its neighbors in parallel and applies three classic rules: Separation, Alignment, and Cohesion.
2. **Multi-Flocks and Hostility:** Birds can be divided into three independent color groups. Agents of the same group try to stick together, but approaching strangers triggers a repulsion factor (Strange Force), forcing them to aggressively avoid collisions.
3. **Spatial Constraints:** The entire flock is confined within an invisible cube. Upon approaching the boundaries, agents smoothly adjust their velocity vector (Turn Factor) to avoid flying out of the visible area.

## Technical Features

1. **Frame Counter:** The control panel includes a built-in frame counter and an FPS limiting function.
2. **Debug Options:** The ability to display velocity vectors and the local center of mass.
3. **Camera Control:** Support for two 3D camera modes: Free and Orbital.

The source code utilizes `C2Profiler`, a high-precision CPU profiler; use it to measure frame preparation time.

## Customization (`CoreData.hpp`)

You can fine-tune the physics, constraint geometry, and simulation performance in real-time via the ImGui panel. These settings are driven by the `SimulationParams` struct, which synchronizes with the GPU every frame:

```cpp
struct SimulationParams {
    float deltaTime;
    float visualRange = 2.0f;            // Bird's field of view radius
    float protectedRange = 0.5f;         // Personal space (for collision avoidance)
    float strangerProtectedRange = 2.5f; // Reaction radius to a foreign flock
    float maxSpeed = 5.0f;               // Maximum flight speed
    float minSpeed = 2.0f;               // Minimum flight speed
    float cubeSize = 4.5f;               // Dimensions of the bounding cube
    float cohesionFactor = 0.005f;       // Attraction force to the neighbors' center of mass
    float alignmentFactor = 0.05f;       // Velocity vector alignment force
    float separationFactor = 0.05f;      // Repulsion force from neighbors
    float turnFactor = 0.15f;            // Turn force at the cube boundaries
    float strangeForceFactor = 10.0f;    // Repulsion force from strangers
    float visionRadius = 240;            // Field of view angle (in degrees)
    float margin = 1.0f;                 // Margin from the cube boundaries to start turning
    uint32_t activeBoidsCount = 1000;    // Current number of active birds
    uint32_t divideFlocks = 0;           // Flag to divide into different flocks (0 or 1)
};
```


## Building and Running

### Prerequisites

The project is cross-platform. To build it, you will need:

* A compiler with **C++17** support (MSVC, GCC, Clang)
* **CMake** (version 3.20 or higher)

### Building (CMake)

```bash
git clone https://github.com/con2222/Dawn-Boids-3D.git
cd Dawn-Boids-3d
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDEV_MODE=OFF # On Linux, specify USE_WAYLAND=ON or USE_X11=ON
cmake --build build --config Release
```

## A Little Note

> Currently, the algorithm runs via a brute-force approach with a complexity of $O(N^2)$. While the GPU handles this exceptionally well, a Spatial Hashing optimization will be added in the near future. This will significantly reduce the mathematical load and allow for rendering an even larger number of birds.