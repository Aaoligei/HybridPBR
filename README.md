# HybridPBR
HybridPBR/
├── CMakeLists.txt
├── src/
│   ├── core/
│   │   ├── Application.cpp/.h           # 主应用循环
│   │   ├── Window.cpp/.h               # 窗口管理
│   │   ├── Input.cpp/.h                # 输入处理
│   │   └── Timer.cpp/.h                # 计时器
│   ├── rendering/
│   │   ├── interfaces/
│   │   │   ├── IRenderer.h             # 渲染器接口
│   │   │   ├── IRasterizer.h           # 光栅化接口
│   │   │   └── IRayTracer.h            # 光追接口
│   │   ├── rasterization/
│   │   │   ├── Rasterizer.cpp/.h       # 光栅化渲染器
│   │   │   ├── Shader.cpp/.h           # 着色器管理
│   │   │   ├── GBuffer.cpp/.h          # G-Buffer
│   │   │   ├── Mesh.cpp/.h             # 网格数据
│   │   │   └── Camera.cpp/.h           # 相机
│   │   ├── raytracing/
│   │   │   ├── RayTracer.cpp/.h        # 光追渲染器
│   │   │   ├── BVH.cpp/.h              # BVH加速结构
│   │   │   ├── Ray.cpp/.h              # 光线定义
│   │   │   └── Denoiser.cpp/.h         # 降噪器
│   │   └── common/
│   │       ├── Texture.cpp/.h          # 纹理管理
│   │       ├── Material.cpp/.h         # 材质系统
│   │       └── Light.cpp/.h            # 光源
│   ├── pbr/
│   │   ├── PBRMaterial.cpp/.h          # PBR材质
│   │   ├:: IBL.cpp/.h                  # 图像Based光照
│   │   ├:: BRDF.cpp/.h                 # BRDF函数
│   │   └:: LutGenerator.cpp/.h         # LUT生成器
│   ├── resources/
│   │   ├:: ShaderLibrary.cpp/.h        # 着色器库
│   │   ├:: ModelLoader.cpp/.h          # 模型加载
│   │   └:: ResourceManager.cpp/.h      # 资源管理
│   └── utils/
│       ├:: MathUtils.cpp/.h            # 数学工具
│       ├:: FileIO.cpp/.h               # 文件IO
│       └:: Logger.cpp/.h               # 日志系统
├── assets/
│   ├:: shaders/                        # 着色器文件
│   │   ├:: raster/
│   │   ├:: compute/
│   │   └:: raytracing/
│   ├:: textures/                       # 纹理资源
│   └:: models/                         # 模型文件
├── include/                            # 第三方库头文件
└── external/                           # 第三方库
