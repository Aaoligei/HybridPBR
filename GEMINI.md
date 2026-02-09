# HybridPBR Project Context

## Project Overview
**HybridPBR** is a C++ rendering engine designed to demonstrate and explore hybrid rendering techniques, combining traditional Rasterization with modern Ray Tracing. The project aims to deliver high-fidelity graphics using Physically Based Rendering (PBR) and Image-Based Lighting (IBL).

**Key Features:**
*   **Architecture:** Hybrid rendering pipeline (Rasterization + Ray Tracing).
*   **Rendering Paths:**
    *   **Deferred Rendering:** G-Buffer based geometry and lighting passes.
    *   **Ray Tracing:** Compute shader-based path tracing with BVH acceleration.
*   **Materials:** Full PBR workflow (Metallic/Roughness) with IBL support.
*   **Core Systems:** Custom windowing, input handling, and scene graph management.

## Tech Stack
*   **Language:** C++17
*   **Build System:** CMake (3.15+)
*   **Graphics API:** OpenGL 4.6 (via GLAD)
*   **Windowing/Input:** GLFW
*   **Math:** GLM
*   **GUI:** Dear ImGui (Docking branch)
*   **Asset Loading:** Assimp (Models), stb_image (Textures)
*   **JSON:** nlohmann/json

## Directory Structure
*   `src/`: Source code header and implementation files.
    *   `core/`: Application framework (Window, Input, Timer).
    *   `rendering/`: Rendering logic divided into `deferred`, `raytracing`, `rasterization`, and `common`.
    *   `pbr/`: Physics-based rendering components (BRDF, IBL).
    *   `scene/`: Scene graph and node management.
    *   `assets/`: Shaders (`.vert`, `.frag`, `.comp`), models, and textures.
*   `build/`: Build artifacts (not tracked in git).
*   `document.md`: Contains specific development notes and optimization guides (e.g., SAH BVH implementation).
*   `README.md`: Project roadmap and weekly development logs.

## Building and Running

**Prerequisites:**
*   CMake 3.15 or higher
*   C++ Compiler supporting C++17 (MSVC, GCC, or Clang)
*   GPU with OpenGL 4.6 support

**Build Instructions:**
1.  Configure the project:
    ```bash
    cmake -B build
    ```
2.  Build the project:
    ```bash
    cmake --build build --config Debug
    ```

**Running:**
*   The executable `HybridPBR` (or `HybridPBR.exe` on Windows) will be located in the `build/bin/` directory.
*   Ensure the `assets/` directory is accessible to the executable (CMake is configured to copy it).

## Development Status & Conventions
*   **Current Focus:** The project is in the optimization phase for the Ray Tracing module (Week 7+). Key tasks involve implementing SAH (Surface Area Heuristic) for BVH construction and optimizing memory layout (Structure of Arrays) for GPU traversal.
*   **Coding Style:** Follows modern C++17 conventions.
*   **Third-Party Libraries:** Managed via CMake `FetchContent`.
*   **Shaders:** Located in `assets/shaders/`. Ray tracing logic is primarily in compute shaders (`.comp`).

## Critical References
*   **`document.md`**: detailed guide on optimizing the BVH from Spatial Median Split to SAH Binning, and data structure refactoring for GPU performance.
*   **`README.md`**: Tracks the weekly progress and architectural goals.
