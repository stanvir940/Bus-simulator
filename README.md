# Bus Stand Simulator

Simple 3D bus stand simulator in C++ using **OpenGL fixed pipeline + GLUT**.

## Features

- 3D interactive environment:
  - Main station building
  - Multiple bus platforms
  - Road network
  - Moving buses
  - Trees, light poles, boundary walls, terrain, lake, and jungle area
- Dynamic buses:
  - Player-controlled primary bus (keyboard driving)
  - Physics-inspired vehicle behavior (acceleration, braking force, rolling resistance, drag, steering response)
  - NPC buses with translation-based looped path animation
  - Collision-safe spacing for loop traffic
- Camera system:
  - Top view
  - Driver view (follows lead bus)
  - Free camera with rotate, zoom, and pan
- Added details:
  - Ticket counter house with multiple counter sections
  - Human NPC models around platform and counter areas
  - Enhanced road markings, covered platforms, realistic shading/fog, and richer environment
  - Procedural hilly terrain outside station area, plus more roadside trees and lights
- Rendering:
  - Real-time continuous update with idle loop
  - Basic ambient + diffuse lighting
  - Smooth shading

## Folder Structure

```text
BusStandSimulator/
  CMakeLists.txt
  README.md
  main.cpp
  scene.cpp
  scene.h
  bus.cpp
  bus.h
  camera.cpp
  camera.h
```

## Controls

- `1` -> Top camera
- `2` -> Driver camera
- `3` -> Free camera
- `W` -> Accelerate player bus
- `S` -> Brake/reverse player bus
- `A` / `D` or `Left` / `Right` -> Steer player bus
- `Mouse drag` -> Rotate free camera
- `+` / `-` -> Zoom in/out (free camera)
- `I J K L` -> Pan free camera center
- `ESC` -> Exit

## Build & Run (macOS)

From the `BusStandSimulator` folder:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./BusStandSimulator
```

## Build & Run (Linux)

Install dependencies first (example):

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake freeglut3-dev libglu1-mesa-dev mesa-common-dev
```

Then:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./BusStandSimulator
```

## Notes

- This project intentionally focuses on structure, animation, and interaction rather than high-end graphics.
- Primitive-based modeling is used for all major objects (cuboids, cylinders, spheres, cones).
