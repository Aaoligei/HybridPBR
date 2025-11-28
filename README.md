# HybridPBR

```
HybridPBR/
├── CMakeLists.txt
├── src/
│   ├── core/
│   │   ├── Application.cpp
│   │   ├── Application.h
│   │   ├── Input.cpp
│   │   ├── Input.h
│   │   ├── Timer.cpp
│   │   ├── Timer.h
│   │   ├── Window.cpp
│   │   └── Window.h
│   ├── pbr/
│   │   ├── BRDF.cpp
│   │   ├── BRDF.h
│   │   ├── IBL.cpp
│   │   ├── IBL.h
│   │   ├── PBRMaterial.cpp
│   │   └── PBRMaterial.h
│   ├── rendering/
│   │   ├── common/
│   │   │   ├── Light.cpp
│   │   │   ├── Light.h
│   │   │   ├── Material.cpp
│   │   │   ├── Material.h
│   │   │   ├── Texture.cpp
│   │   │   ├── Texture.h
│   │   │   ├── UniformBuffer.cpp
│   │   │   └── UniformBuffer.h
│   │   ├── deferred/
│   │   │   ├── DeferredRenderer.cpp
│   │   │   ├── DeferredRenderer.h
│   │   │   ├── GBuffer.cpp
│   │   │   └── GBuffer.h
│   │   ├── interfaces/
│   │   │   ├── IRasterizer.h
│   │   │   ├── IRayTracer.h
│   │   │   └── IRenderer.h
│   │   ├── postprocess/
│   │   │   ├── SSAO.cpp
│   │   │   └── SSAO.h
│   │   ├── rasterization/
│   │   │   ├── Camera.cpp
│   │   │   ├── Camera.h
│   │   │   ├── CameraController.cpp
│   │   │   ├── CameraController.h
│   │   │   ├── Mesh.cpp
│   │   │   ├── Mesh.h
│   │   │   ├── Rasterizer.cpp
│   │   │   ├── Rasterizer.h
│   │   │   ├── RenderPass.cpp
│   │   │   └── RenderPass.h
│   │   ├── raytracing/
│   │   │   ├── BVH.cpp
│   │   │   ├── BVH.h
│   │   │   ├── ComputeShader.cpp
│   │   │   ├── ComputeShader.h
│   │   │   ├── Denoiser.cpp
│   │   │   ├── Denoiser.h
│   │   │   ├── Ray.cpp
│   │   │   ├── Ray.h
│   │   │   ├── RayTracer.cpp
│   │   │   └── RayTracer.h
│   │   ├── ImGuiComponentManager.cpp
│   │   ├── ImGuiComponentManager.h
│   │   ├── ImGuiManager.cpp
│   │   ├── ImGuiManager.h
│   │   ├── Shader.cpp
│   │   ├── Shader.h
│   │   ├── ShaderManager.cpp
│   │   └── ShaderManager.h
│   ├── resources/
│   │   ├── ModelLoader.cpp
│   │   ├── ModelLoader.h
│   │   ├── ResourceManager.cpp
│   │   └── ResourceManager.h
│   ├── scene/
│   │   ├── Scene.cpp
│   │   ├── Scene.h
│   │   ├── SceneNode.cpp
│   │   ├── SceneNode.h
│   │   ├── Transform.cpp
│   │   └── Transform.h
│   ├── utils/
│   │   ├── FileIO.cpp
│   │   ├── FileIO.h
│   │   ├── GLCheck.h
│   │   ├── Logger.cpp
│   │   ├── Logger.h
│   │   ├── MathUtils.cpp
│   │   └── MathUtils.h
│   ├── DefferedApplication.h
│   └── main.cpp
├── assets/
│   ├── shaders/
│   │   ├── raster/
│   │   ├── compute/
│   │   └── raytracing/
│   ├── textures/
│   └── models/
├── include/                # 第三方库头文件
├── external/               # 第三方库
├── README.md
├── document.md
└── imgui.ini
```

## 项目概述
基于物理的实时渲染引擎，支持混合渲染管线（光栅化 + 光线追踪）