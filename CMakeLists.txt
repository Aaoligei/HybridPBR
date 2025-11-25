cmake_minimum_required(VERSION 3.15)
project(HybridPBR VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 禁用Assimp中的-Werror标志，以避免编译错误
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "If the supplementary tools for Assimp are built in addition to the library." FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "If the test suite for Assimp is built in addition to the library." FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat all warnings as errors." FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "Build the FBX importer" FORCE)

# 第三方库配置
include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLAD
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# stb_image
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# ImGui (使用支持docking的版本)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.2.5
)
# 在MakeAvailable之前设置Assimp选项
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_BUILD_FBX_IMPORTER ON)
FetchContent_MakeAvailable(assimp)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${glm_SOURCE_DIR})
include_directories(${glad_SOURCE_DIR}/include)
include_directories(${stb_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)
include_directories(${nlohmann_json_SOURCE_DIR}/include)
include_directories(${assimp_SOURCE_DIR}/include)

# 编译选项
if(MSVC)
    # 在MSVC上禁用特定警告
    target_compile_options(assimp PRIVATE /wd4244 /wd4267 /wd4996)
else()
    # 在GCC/Clang上禁用特定警告
    target_compile_options(assimp PRIVATE -Wno-dangling-reference -Wno-sign-compare)
endif()


# 可执行文件
add_executable(HybridPBR
    src/core/Application.cpp
    src/core/Window.cpp
    src/core/Input.cpp
    src/core/Timer.cpp
    src/pbr/BRDF.cpp
    src/pbr/IBL.cpp
    src/pbr/PBRMaterial.cpp
    src/rendering/Shader.cpp
    src/rendering/ImGuiManager.cpp
    src/rendering/ImGuiComponentManager.cpp
    src/rendering/ShaderManager.cpp
    src/rendering/common/Light.cpp
    src/rendering/common/Material.cpp
    src/rendering/common/Texture.cpp
    src/rendering/common/UniformBuffer.cpp
    src/rendering/rasterization/Camera.cpp
    src/rendering/rasterization/CameraController.cpp
    src/rendering/rasterization/Mesh.cpp
    src/rendering/rasterization/Rasterizer.cpp
    src/rendering/rasterization/RenderPass.cpp
    src/resources/ModelLoader.cpp
    src/resources/ResourceManager.cpp
    src/scene/Scene.cpp
    src/scene/SceneNode.cpp
    src/scene/Transform.cpp
    src/utils/Logger.cpp
    src/utils/FileIO.cpp
    src/utils/MathUtils.cpp
    src/main.cpp
    # ImGui源文件
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# 链接库
target_link_libraries(HybridPBR
    glfw
    glad
    assimp
    ${OPENGL_gl_LIBRARY}
)

# 复制资源文件
file(COPY assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})