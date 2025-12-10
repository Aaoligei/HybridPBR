#pragma once

#include "interfaces/IRenderer.h"
#include "core/Result.h"
#include <memory>
#include <vector>

// --- 新增 RHI 头文件 ---
#include "../rhi/RHI_Device.h"
#include "../rhi/RHI_CommandList.h"
#include "passes/GeometryPass.h" // 我们刚写的 Pass

namespace HybridPBR {

    class HybridRenderer : public IRenderer {
    public:
        HybridRenderer();
        ~HybridRenderer() override;

        // --- IRenderer 接口实现 ---
        // 注意：旧的 IRenderDevice 参数我们可以暂时忽略，因为我们内部会创建新的 OpenGLDevice
        Result<void> Initialize(std::shared_ptr<IRenderDevice> device) override;
        void Shutdown() override;
        bool IsInitialized() const override { return initialized_; }

        // 核心渲染循环
        Result<void> BeginFrame() override;
        Result<void> Render(const Scene& scene) override;
        Result<void> EndFrame() override;

        // ... 其他接口保持原样或留空 ...
        Result<void> SetViewport(int width, int height) override;
        Result<void> SetClearColor(const glm::vec4& color) override;
        Result<void> Resize(uint32_t width, uint32_t height) override;
        
        // 暂时不需要旧的 PipelineManager，因为我们要重写它
        std::shared_ptr<IRenderPipelineManager> GetPipelineManager() override { return nullptr; }
        Result<void> AddPipeline(std::shared_ptr<IRenderPipeline> pipeline) override { return Result<void>::Success(); }
        std::shared_ptr<Texture> GetOutputTexture() const override { return nullptr; }
        std::shared_ptr<IRenderDevice> GetDevice() const override { return nullptr; } // 返回旧接口可能为空
        
        const RenderStats& GetStats() const override { return stats_; }
        void ResetStats() override { stats_ = RenderStats(); }
        void SetDebugMode(bool enabled) override {}
        bool IsDebugMode() const override { return false; }

    private:
        bool initialized_ = false;
        RenderStats stats_;

        // --- RHI 核心组件 ---
        std::unique_ptr<RHI_Device> m_rhiDevice;
        std::unique_ptr<GeometryPass> m_geometryPass;
        
        // 窗口大小缓存
        uint32_t m_width = 1280;
        uint32_t m_height = 720;
        glm::vec4 m_clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
    };

} // namespace HybridPBR