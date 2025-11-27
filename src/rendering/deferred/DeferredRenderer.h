#pragma once
#include "../rasterization/RenderPass.h"
#include "GBuffer.h"
#include "rendering/interfaces/IRasterizer.h"
#include "rendering/postprocess/SSAO.h"
#include "rendering/common/UniformBuffer.h"
#include <memory>

namespace HybridPBR {

    // 延迟渲染器 - 组合几何和光照通道
    class DeferredRenderer{
    public:
        DeferredRenderer();
        ~DeferredRenderer();
        
        bool Initialize(int width, int height);
        void Shutdown();
        void Render(const Scene& scene);
        void Resize(int width, int height);
        
        // 设置
        void SetIBLSystem(std::shared_ptr<IBL> ibl);
        void SetSSAOEnabled(bool enabled) { ssaoEnabled = enabled; }
        void SetWireframe(bool enabled);
        
        // 获取渲染结果
        std::shared_ptr<Texture> GetOutputTexture() const;
        
        // 调试
        void SetDebugView(int debugView) { this->debugView = debugView; }

    private:
        std::unique_ptr<GBufferPass> gBufferPass;
        std::unique_ptr<LightingPass> lightingPass;
        std::unique_ptr<SSAO> ssaoPass;
        
        bool initialized = false;
        bool ssaoEnabled = true;
        int debugView = 0;
        // UBO
        std::unique_ptr<UniformBuffer> cameraUBO;
        std::unique_ptr<UniformBuffer> lightUBO;
        // 输出FBO
        uint32_t outputFBO = 0;
        std::shared_ptr<Texture> outputTexture;
        
        bool CreateOutputFramebuffer(int width, int height);

        void UpdateGlobalUniforms(const Scene& scene);
    };

} // namespace HybridPBR