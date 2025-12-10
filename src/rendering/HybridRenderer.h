#pragma once

#include "interfaces/IRenderer.h"
#include "core/Result.h"
#include <memory>

// RHI Components
#include "../rhi/RHI_Device.h"
#include "../rhi/RHI_CommandList.h"
#include "passes/GeometryPass.h"

namespace HybridPBR {

    class HybridRenderer : public IRenderer {
    public:
        HybridRenderer();
        ~HybridRenderer() override;

        // IRenderer Interface
        Result<void> Initialize() override; // [修改] 参数移除
        void Shutdown() override;
        bool IsInitialized() const override { return initialized_; }

        Result<void> BeginFrame() override;
        Result<void> Render(const Scene& scene) override;
        Result<void> EndFrame() override;

        Result<void> SetViewport(int width, int height) override;
        Result<void> SetClearColor(const glm::vec4& color) override;
        Result<void> Resize(uint32_t width, uint32_t height) override;
        
        const RenderStats& GetStats() const override { return stats_; }
        void ResetStats() override { stats_ = RenderStats(); }
        void SetDebugMode(bool enabled) override {}
        bool IsDebugMode() const override { return false; }

        // 获取内部 RHI Device (供其他系统如 ModelLoader 使用)
        RHI_Device* GetRHIDevice() const { return m_rhiDevice.get(); }

    private:
        bool initialized_ = false;
        RenderStats stats_;

        std::unique_ptr<RHI_Device> m_rhiDevice;
        std::unique_ptr<GeometryPass> m_geometryPass;
        
        uint32_t m_width = 1280;
        uint32_t m_height = 720;
        glm::vec4 m_clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
    };

} // namespace HybridPBR