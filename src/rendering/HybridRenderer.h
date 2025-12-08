#pragma once

#include "interfaces/IRenderer.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IRenderPipeline.h"
#include "RenderPipelineManager.h"
#include "core/Result.h"
#include <memory>
#include <vector>

namespace HybridPBR {

    /**
     * @brief 混合渲染器实现
     * 统一管理光栅化、延迟渲染和光线追踪管线
     * 
     * 修改理由：
     * 1. 提供统一的渲染器架构，替代分散的渲染器
     * 2. 支持动态管线组合和配置
     * 3. 实现错误安全的渲染流程
     * 4. 提供灵活的渲染策略切换
     */
    class HybridRenderer : public IRenderer {
    public:
        enum class RenderStrategy {
            RASTERIZATION_ONLY,    // 仅光栅化
            DEFERRED_ONLY,         // 仅延迟渲染
            RAYTRACING_ONLY,       // 仅光线追踪
            HYBRID_RASTER_RT,      // 光栅化 + 光线追踪
            HYBRID_DEFERRED_RT     // 延迟渲染 + 光线追踪
        };

        HybridRenderer();
        ~HybridRenderer() override;

        // IRenderer接口实现
        Result<void> Initialize(std::shared_ptr<IRenderDevice> device) override;
        void Shutdown() override;
        bool IsInitialized() const override { return initialized_; }

        Result<void> BeginFrame() override;
        Result<void> Render(const Scene& scene) override;
        Result<void> EndFrame() override;

        Result<void> SetViewport(int width, int height) override;
        Result<void> SetClearColor(const glm::vec4& color) override;
        Result<void> Resize(uint32_t width, uint32_t height) override;

        std::shared_ptr<IRenderPipelineManager> GetPipelineManager() override;
        Result<void> AddPipeline(std::shared_ptr<IRenderPipeline> pipeline) override;

        std::shared_ptr<Texture> GetOutputTexture() const override;
        std::shared_ptr<IRenderDevice> GetDevice() const override { return device_; }

        const RenderStats& GetStats() const override;
        void ResetStats() override;

        void SetDebugMode(bool enabled) override { debugMode_ = enabled; }
        bool IsDebugMode() const override { return debugMode_; }

        // 混合渲染器特有功能
        void SetRenderStrategy(RenderStrategy strategy);
        RenderStrategy GetRenderStrategy() const { return strategy_; }
        
        // 管线访问
        std::shared_ptr<IRenderPipeline> GetRasterizationPipeline() const;
        std::shared_ptr<IRenderPipeline> GetDeferredPipeline() const;
        std::shared_ptr<IRenderPipeline> GetRayTracingPipeline() const;

        // 配置选项
        void SetAutoPipelineSelection(bool enabled) { autoPipelineSelection_ = enabled; }
        bool IsAutoPipelineSelection() const { return autoPipelineSelection_; }
        
        void SetQualityPreset(int level); // 0=性能, 1=平衡, 2=质量
        int GetQualityPreset() const { return qualityPreset_; }

    private:
        bool initialized_ = false;
        bool debugMode_ = false;
        bool autoPipelineSelection_ = true;
        int qualityPreset_ = 1; // 默认平衡模式
        
        std::shared_ptr<IRenderDevice> device_;
        std::unique_ptr<RenderPipelineManager> pipelineManager_;
        RenderStrategy strategy_ = RenderStrategy::HYBRID_DEFERRED_RT;
        
        // 渲染统计
        mutable RenderStats stats_;
        
        // 管线引用
        std::string rasterizationPipelineName_;
        std::string deferredPipelineName_;
        std::string rayTracingPipelineName_;
        
        // 内部方法
        Result<void> InitializeDefaultPipelines();
        Result<void> SetupPipelineDependencies();
        Result<void> SelectOptimalStrategy(const Scene& scene);
        Result<void> ValidatePipelineConfiguration();
        void UpdateRenderStats();
        
        // 策略执行
        Result<void> ExecuteRasterizationStrategy(const Scene& scene);
        Result<void> ExecuteDeferredStrategy(const Scene& scene);
        Result<void> ExecuteRayTracingStrategy(const Scene& scene);
        Result<void> ExecuteHybridRasterRTStrategy(const Scene& scene);
        Result<void> ExecuteHybridDeferredRTStrategy(const Scene& scene);
    };

} // namespace HybridPBR