# HybridPBR Agent Guidelines

## Project Overview
A hybrid 3D renderer combining PBR rasterization with ray tracing. Built with OpenGL (GLFW + Glad), GLM for math, ImGui for debugging, Assimp for model loading, and nlohmann/json for configuration.

## Build Commands

### CMake Configuration
```bash
cmake -S . -B build -G "Visual Studio 17 2022"   # Windows
cmake -S . -B build -G Ninja                     # Cross-platform
```

### Build & Run
```bash
cmake --build build --config Debug
./build/bin/HybridPBR.exe
```

### Clean Rebuild
```bash
rm -rf build && cmake -S . -B build -G "Visual Studio 17 2022" && cmake --build build --config Debug
```

## Testing
This project has no unit tests. To add Google Test:
```cmake
include(FetchContent)
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG release-1.12.1)
FetchContent_MakeAvailable(googletest)
add_executable(my_test tests/MyTest.cpp)
target_link_libraries(my_test gtest gtest_main)
```

Run tests:
```bash
ctest --output-on-failure                    # All tests
ctest -R <test_name> --verbose               # Single test
```

## Linting (Recommended)
Create `.clang-format` in project root:
```yaml
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 120
NamespaceIndentation: None
BreakBeforeBraces: Attach
AccessModifierOffset: -4
IndentAccessModifiers: true
```

Create `.clang-tidy` for static analysis:
```yaml
Checks: '-*,modernize-*,performance-*,readability-*'
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
```

Format code: `clang-format -i src/**/*.cpp src/**/*.h`
Analyze: `clang-tidy src/**/*.cpp`

## Code Style

### Naming Conventions
| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `class CameraController` |
| Structs | PascalCase | `struct RayHit` |
| Methods/Functions | CamelCase | `SetPosition()`, `GetViewMatrix()` |
| Member Variables | snake_case | `texture_id`, `window_config` |
| Constants | kPascalCase | `kMaxLights` |
| Enums | PascalCase | `enum class TextureType` |
| Enum Values | PascalCase | `TextureType::DIFFUSE` |

### File Organization
- Header: `.h` files with implementation
- Source: `.cpp` files in same directory
- Filename matches class name: `Camera.h`, `Camera.cpp`

### Header Template
```cpp
#pragma once
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace HybridPBR {

class Camera {
public:
    Camera();
    ~Camera();
    void SetPosition(const glm::vec3& position);
    
private:
    glm::vec3 position = glm::vec3(0.0f);
    float fov = 45.0f;
};

} // namespace HybridPBR
```

### Rules
- Use `#pragma once` (not include guards)
- Close namespace with `} // namespace HybridPBR`
- Group includes: system, third-party, project headers
- Prefer forward declarations to reduce includes
- Inline simple getters/setters in header

### Error Handling & Logging
Always use logging macros:
```cpp
LOG_DEBUG("Loading texture: " + filepath);
LOG_INFO("Initializing renderer");
LOG_WARNING("Fallback to default shader");
LOG_ERROR("Failed to compile shader: " + error);
LOG_CRITICAL("Cannot recover from this error");
```

Wrap OpenGL calls with `GL_CHECK`:
```cpp
GL_CHECK(glGenBuffers(1, &vbo));
GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
```

### Memory Management
- `std::unique_ptr` for single ownership
- `std::shared_ptr` for shared ownership
- Avoid raw `new`/`delete`; use RAII
- Resources self-destruct in destructor

### Types & Math
- Fixed-width integers: `uint32_t`, `int32_t`, `uint8_t`
- GLM types: `glm::vec3`, `glm::mat4`, `glm::quat`
- `enum class` for type-safe enums
- GPU data structs must follow `std140` alignment rules

### Interface Design
```cpp
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Render(const Scene& scene) = 0;
};
```
- Always `virtual ~Interface() = default`
- Pure virtual methods return values where possible

## Project Structure
```
src/
├── core/           # Application, Window, Input, Timer
├── rendering/
│   ├── interfaces/ # IRenderer, IRasterizer, IRayTracer
│   ├── rasterization/  # Rasterizer, Camera, Mesh, Shader
│   ├── raytracing/     # RayTracer, BVH, Denoiser
│   ├── deferred/       # GBuffer, DeferredRenderer
│   ├── postprocess/    # SSAO
│   └── common/        # Texture, Material, Light, UniformBuffer
├── pbr/            # PBRMaterial, IBL, BRDF
├── resources/      # ModelLoader, ResourceManager
├── scene/          # Scene, SceneNode, Transform
├── render_graph/   # (reserved for future)
└── utils/          # Logger, FileIO, MathUtils

assets/
├── shaders/        # deferred/, raster/, raytracing/, compute/
├── textures/       # PBR textures, HDRIs
└── models/         # glTF, OBJ, FBX models
```

## Common Patterns

### Lifecycle Management
```cpp
if (!subsystem->Initialize()) {
    LOG_ERROR("Failed to init subsystem");
    return false;
}
subsystem->Shutdown();
```

### Lambda Callbacks
```cpp
window->SetMouseCallback([this](double xpos, double ypos) {
    if (cameraController) {
        cameraController->OnMouseMove(xpos, ypos);
    }
});
```

### Application Subclass
```cpp
class ThreeDApp : public HybridPBR::Application {
public:
    void OnWindowConfigChanged() override { /* configure window */ }
    bool OnInitialize() override { /* setup renderer */ }
    void OnUpdate(float deltaTime) override { /* game logic */ }
    void OnRender() override { /* draw */ }
    void OnImGuiRender() override { /* debug UI */ }
    void OnShutdown() override { /* cleanup */ }
};
```

## Shader Development
Shaders go in `assets/shaders/`:
- `deferred/` - G-buffer, lighting passes
- `raster/` - Forward rendering
- `raytracing/` - Ray tracing compute shaders
- `compute/` - General compute shaders

Load via ShaderManager:
```cpp
auto shader = ShaderManager::GetInstance().LoadShader(
    "pbr",
    "assets/shaders/raster/pbr.vert",
    "assets/shaders/raster/pbr.frag"
);
```

## Key Dependencies (from CMakeLists.txt)
- GLFW 3.3.8
- GLAD v0.1.36
- GLM 0.9.9.8
- ImGui (docking branch)
- Assimp v5.2.5
- nlohmann/json v3.11.2
- stb (master)
