# HybridPBR Renderer 开发文档

## 项目目录结构

```
HybridPBR/
├── CMakeLists.txt
├── src/
│   ├── core/
│   │   ├── Application.cpp/.h           # 主应用循环
│   │   ├── Window.cpp/.h                # 窗口管理
│   │   ├── Input.cpp/.h                 # 输入处理
│   │   ├── Timer.cpp/.h                 # 计时器
│   │   └── ...
│   ├── rendering/
│   │   ├── interfaces/
│   │   │   ├── IRenderer.h              # 渲染器接口
│   │   │   ├── IRasterizer.h            # 光栅化接口
│   │   │   └── IRayTracer.h             # 光追接口
│   │   ├── rasterization/
│   │   │   ├── Rasterizer.cpp/.h        # 光栅化渲染器
│   │   │   ├── Shader.cpp/.h            # 着色器管理
│   │   │   ├── GBuffer.cpp/.h           # G-Buffer
│   │   │   ├── Mesh.cpp/.h              # 网格数据
│   │   │   ├── Camera.cpp/.h            # 相机
│   │   │   ├── RenderPass.cpp/.h        # 渲染通道
│   │   │   ├── CameraController.cpp/.h  # 相机控制器
│   │   │   └── ...
│   │   ├── raytracing/
│   │   │   ├── RayTracer.cpp/.h         # 光追渲染器
│   │   │   ├── BVH.cpp/.h               # BVH加速结构
│   │   │   ├── Ray.h                    # 光线定义
│   │   │   ├── Denoiser.cpp/.h          # 降噪器
│   │   │   ├── ComputeShader.cpp/.h     # 计算着色器
│   │   │   ├── ComputeBuffer.cpp/.h     # 计算缓冲区
│   │   │   └── ...
│   │   ├── deferred/
│   │   │   ├── DeferredRenderer.cpp/.h  # 延迟渲染器
│   │   │   ├── GBuffer.cpp/.h           # G缓冲区
│   │   │   └── ...
│   │   ├── postprocess/
│   │   │   ├── SSAO.cpp/.h              # 屏幕空间环境光遮蔽
│   │   │   └── ...
│   │   ├── common/
│   │   │   ├── Texture.cpp/.h           # 纹理管理
│   │   │   ├── Material.cpp/.h          # 材质系统
│   │   │   ├── Light.cpp/.h             # 光源
│   │   │   ├── UniformBuffer.cpp/.h     # 统一缓冲区
│   │   │   └── ...
│   │   ├── ImGuiManager.cpp/.h          # ImGui管理器
│   │   ├── ImGuiComponentManager.cpp/.h # ImGui组件管理器
│   │   ├── ShaderManager.cpp/.h         # 着色器管理器
│   │   └── ...
│   ├── pbr/
│   │   ├── PBRMaterial.cpp/.h           # PBR材质
│   │   ├── IBL.cpp/.h                   # 图像Based光照
│   │   ├── BRDF.cpp/.h                  # BRDF函数
│   │   └── ...
│   ├── resources/
│   │   ├── ModelLoader.cpp/.h           # 模型加载
│   │   ├── ResourceManager.cpp/.h       # 资源管理
│   │   └── ...
│   ├── scene/
│   │   ├── Scene.cpp/.h                 # 场景
│   │   ├── SceneNode.cpp/.h             # 场景节点
│   │   ├── Transform.cpp/.h             # 变换
│   │   └── ...
│   ├── utils/
│   │   ├── MathUtils.cpp/.h             # 数学工具
│   │   ├── FileIO.cpp/.h                # 文件IO
│   │   ├── Logger.cpp/.h                # 日志系统
│   │   ├── GLCheck.h                    # OpenGL检查
│   │   └── ...
│   ├── main.cpp                         # 主入口
│   └── DefferedApplication.h            # 延迟渲染应用
├── assets/
│   ├── shaders/                         # 着色器文件
│   │   ├── deferred/                    # 延迟渲染着色器
│   │   ├── compute/                     # 计算着色器
│   │   ├── raster/                      # 光栅化着色器
│   │   └── raytracing/                  # 光线追踪着色器
│   ├── textures/                        # 纹理资源
│   └── models/                          # 模型文件
├── include/                             # 第三方库头文件
└── external/                            # 第三方库
```

## 每周开发计划

### 第1周：基础框架搭建
**目标**：建立可运行的最小化OpenGL应用

**任务清单**：
- [ ] 创建CMake构建系统，集成GLFW、Glad、GLM
- [ ] 实现Window类（窗口创建、事件回调）
- [ ] 实现Application类（主循环、状态管理）
- [ ] 实现基础Shader类（编译、链接、使用）
- [ ] 创建简单的三角形渲染测试
- [ ] 建立日志系统和错误处理

**关键代码**：
```cpp
// Application.h 基础框架
class Application {
public:
    bool Initialize();
    void Run();
    void Shutdown();
    
private:
    std::unique_ptr<Window> window_;
    std::unique_ptr<ShaderLibrary> shaderLibrary_;
    bool isRunning_ = false;
};
```

### 第2周：核心渲染架构
**目标**：建立模块化渲染系统框架

**任务清单**：
- [ ] 设计并实现渲染器接口（IRenderer, IRasterizer, IRayTracer）
- [ ] 实现资源管理器（ResourceManager）
- [ ] 实现基础Mesh类（VAO/VBO/EBO管理）
- [ ] 实现Camera类（视图/投影矩阵）
- [ ] 创建基础光照系统
- [ ] 实现简单的场景图

**关键设计**：
```cpp
// 渲染器接口设计
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Initialize() = 0;
    virtual void Render(const Scene& scene) = 0;
    virtual void Shutdown() = 0;
};
```

### 第3周：PBR材质系统
**目标**：实现完整的PBR材质工作流

**任务清单**：
- [ ] 实现PBRMaterial类（金属度、粗糙度、AO等）
- [ ] 实现BRDF核心函数（GGX NDF, Smith几何遮蔽等）
- [ ] 创建HDR纹理加载和处理
- [ ] 实现立方体贴图支持
- [ ] 编写基础PBR着色器
- [ ] 创建材质测试场景

**关键技术**：
```cpp
// PBR BRDF实现
class BRDF {
public:
    static float DistributionGGX(float NdotH, float roughness);
    static float GeometrySmith(float NdotV, float NdotL, float roughness);
    static glm::vec3 FresnelSchlick(float cosTheta, const glm::vec3& F0);
};
```

### 第4周：图像Based光照（IBL）
**目标**：实现高质量的全局光照

**任务清单**：
- [ ] 实现辐照度图预计算
- [ ] 实现预滤波环境贴图
- [ ] 生成BRDF积分查找表
- [ ] 集成IBL到PBR着色器
- [ ] 优化立方体贴图采样
- [ ] 创建HDR环境测试

**核心流程**：
```cpp
// IBL预处理
class IBL {
public:
    bool PrecomputeIrradianceMap(const Texture& envMap);
    bool PrecomputePrefilterMap(const Texture& envMap);
    bool GenerateBRDFLUT();
};
```

### 第5周：延迟渲染管线
**目标**：建立高性能的延迟渲染架构

**任务清单**：
- [ ] 设计G-Buffer格式（位置、法线、材质等）
- [ ] 实现几何通道（Geometry Pass）
- [ ] 实现光照通道（Lighting Pass）
- [ ] 创建多光源支持（点光、方向光、聚光灯）
- [ ] 实现屏幕空间环境光遮蔽（SSAO）
- [ ] 性能优化：减少带宽使用

### 第6周：光线追踪基础
**目标**：实现计算着色器光线追踪

**任务清单**：
- [ ] 设计光线数据结构
- [ ] 实现BVH加速结构构建
- [ ] 创建光线生成计算着色器
- [ ] 实现光线-三角形相交测试
- [ ] 建立基础路径追踪器
- [ ] 集成到主渲染框架

**关键结构**：
```cpp
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float tMin;
    float tMax;
};

struct BVHNode {
    AABB bounds;
    int leftChild;
    int firstPrim;
    int primCount;
};
```

### 第7周：混合渲染集成
**目标**：将光追与光栅化结合

**任务清单**：
- [ ] 设计混合渲染策略
- [ ] 实现光追反射（屏幕空间 + 世界空间）
- [ ] 实现光追软阴影（PCSS）
- [ ] 创建结果合成系统
- [ ] 实现动态切换机制
- [ ] 性能分析与优化

**混合渲染流程**：
```cpp
void HybridRenderer::RenderFrame() {
    // 1. 光栅化G-Buffer
    rasterizer_->RenderGBuffer(scene);
    
    // 2. 光线追踪特效
    raytracer_->TraceReflections(gbuffer, scene);
    raytracer_->TraceShadows(gbuffer, scene);
    
    // 3. 合成最终结果
    composer_->Composite(gbuffer, raytraceResults);
}
```

### 第8周：实时降噪与抗锯齿
**目标**：提升图像质量和性能

**任务清单**：
- [ ] 实现时空降噪器（SVGF）
- [ ] 实现双边滤波
- [ ] 创建时间性抗锯齿（TAA）
- [ ] 优化降噪参数
- [ ] 集成到渲染管线
- [ ] 性能与质量平衡

### 第9周：体积渲染与特效
**目标**：添加高级渲染特效

**任务清单**：
- [ ] 实现体积光（光线步进）
- [ ] 创建参与介质渲染
- [ ] 实现头发/毛发渲染（Kajiya-Kay）
- [ ] 添加后处理效果（Bloom、色调映射）
- [ ] 优化性能开销

### 第10周：场景与资源系统
**目标**：完善内容管线

**任务清单**：
- [ ] 实现自定义场景格式
- [ ] 创建场景序列化/反序列化
- [ ] 实现模型加载器（支持glTF）
- [ ] 优化资源加载性能
- [ ] 创建测试场景

### 第11周：优化与调试
**目标**：提升性能和稳定性

**任务清单**：
- [ ] 性能分析（RenderDoc、Nsight）
- [ ] GPU性能优化
- [ ] 内存使用优化
- [ ] 实现调试可视化
- [ ] 修复已知问题

### 第12周：演示与文档
**目标**：完成项目展示

**任务清单**：
- [ ] 创建最终演示场景
- [ ] 录制演示视频
- [ ] 编写用户文档
- [ ] 性能基准测试
- [ ] 代码整理和注释

## 开发原则

1. **接口稳定**：核心接口在第2周确定后不再修改
2. **模块独立**：各模块通过定义良好的接口通信
3. **增量开发**：每周都有可运行的成果
4. **测试驱动**：每个功能都包含测试用例
5. **性能意识**：从开始就考虑性能影响

以上是我的开发日志，目前第六周结束，该进行第七周工作，由于输入限制，我会分批次给你发送我目前的源代码，你要仔细充分阅读并理解，阅读没问题后给我发送“理解完毕”，我发完后，我会对你说”请开始第七周工作“，之后你就可以写代码了。