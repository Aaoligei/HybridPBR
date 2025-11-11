cmake_minimum_required(VERSION 3.15)
project(HybridPBR VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${glm_SOURCE_DIR})
include_directories(${glad_SOURCE_DIR}/include)

# 可执行文件
add_executable(HybridPBR
    src/core/Application.cpp
    src/core/Window.cpp
    src/core/Input.cpp
    src/core/Timer.cpp
    src/rendering/Shader.cpp
    src/utils/Logger.cpp
    src/utils/FileIO.cpp
    src/utils/MathUtils.cpp
    src/main.cpp
)

# 链接库
target_link_libraries(HybridPBR
    glfw
    glad
    ${OPENGL_gl_LIBRARY}
)

# 复制资源文件
file(COPY assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})