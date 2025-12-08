#pragma once
#include "../../scene/Scene.h"
#include "ComputeShader.h"
#include "ComputeBuffer.h"
#include "BVH.h"
#include "../common/Texture.h"
#include <memory>
#include "rendering/common/UniformBuffer.h"
#include "rendering/interfaces/IRenderer.h"
#include "rendering/deferred/GBuffer.h"
#include "pbr/IBL.h"

namespace HybridPBR {

    struct RayTracerConfig {
        uint32_t width = 1280;
        uint32_t height = 720;
        uint32_t maxBounces = 4;
        uint32_t samplesPerPixel = 1;
        bool accumulateFrames = true;
        bool denoiseEnabled = true;
        float denoiseStrength = 0.5f;
    };

    class RayTracer {
    public:
        RayTracer();
        ~RayTracer();
        
        bool Initialize(const RayTracerConfig& config);
        void Shutdown();
        void Render(const Scene& scene);
        void Resize(uint32_t width, uint32_t height);

        // 混合渲染专用方法
        bool InitializeHybrid();
        void RenderShadows(std::shared_ptr<GBuffer> gbuffer, const Scene& scene);
        void RenderReflections(std::shared_ptr<GBuffer> gbuffer, const Scene& scene);
        std::shared_ptr<Texture> GetShadowTexture() const { return rtShadowTexture; }
        std::shared_ptr<Texture> GetReflectionTexture() const { return rtReflectionTexture; }
        
        // 帧累积控制
        void ResetAccumulation() { accumulatedFrames = 0; }
        uint32_t GetAccumulatedFrames() const { return accumulatedFrames; }
        
        // 获取输出
        std::shared_ptr<Texture> GetOutputTexture() const { return outputTexture; }
        void DrawOutputToScreen(int width, int height);
        
        // 配置
        void SetConfig(const RayTracerConfig& config) { this->config = config; }
        const RayTracerConfig& GetConfig() const { return config; }
        
        // 状态查询
        bool IsInitialized() const { return initialized; }
        float GetLastRenderTime() const { return lastRenderTime; }

        void SetDirty() { sceneDirty = true; }
        void SetIBLSystem(std::shared_ptr<IBL> ibl) { iblSystem = ibl; }

        const std::shared_ptr<ComputeShader>& GetPathTracingShader() const { return pathTracingShader; }

    private:
        RayTracerConfig config;
        bool initialized = false;
        uint32_t accumulatedFrames = 0;
        float lastRenderTime = 0.0f;
        
        // BVH加速结构
        std::unique_ptr<BVH> bvh;
        bool sceneDirty = true; // 场景是否需要更新
        
        // 计算着色器
        std::shared_ptr<ComputeShader> rayGenerationShader;
        std::shared_ptr<ComputeShader> pathTracingShader;
        std::shared_ptr<ComputeShader> denoiserShader;
        //混合渲染
        std::shared_ptr<ComputeShader> rtShadowShader;
        std::shared_ptr<ComputeShader> rtReflectionShader;
        std::shared_ptr<Texture> rtShadowTexture;
        std::shared_ptr<Texture> rtReflectionTexture;
        std::shared_ptr<IBL> iblSystem;
        // UBO
        std::unique_ptr<UniformBuffer> cameraUBO;
        std::unique_ptr<UniformBuffer> lightUBO;
        // 计算缓冲区
        ComputeBuffer bvhNodesBuffer;
        ComputeBuffer trianglesBuffer;
        ComputeBuffer materialsBuffer;
        ComputeBuffer rayBuffer;
        ComputeBuffer hitBuffer;
        ComputeBuffer outputBuffer;
        ComputeBuffer accumulationBuffer;
        
        // 输出纹理
        std::shared_ptr<Texture> outputTexture;
        std::shared_ptr<Texture> denoisedTexture;
        // 用于存储当前场景所有被引用的纹理，顺序对应 Shader 中的索引
        std::vector<std::shared_ptr<Texture>> sceneTextures;
        // 降噪器
        std::unique_ptr<class Denoiser> denoiser;
        uint32_t blitFBO = 0; // 用于屏幕绘制的FBO
        glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        
        // 初始化方法
        bool CreateShaders();
        bool CreateBuffers();
        bool CreateTextures();
        
        // 场景更新
        void UpdateGlobalUniforms(const Scene& scene);
        bool UpdateSceneData(const Scene& scene);
        bool BuildBVH(const Scene& scene);
        
        // 渲染方法
        void GenerateRays();
        void TracePaths();
        void DenoiseResult();
        void CopyToTexture();
        
        
        // 清理
        void Cleanup();
    };

} // namespace HybridPBR