#pragma once

#include "../../core/Result.h"
#include "IRenderDevice.h"
#include "../common/Texture.h"
#include "../../scene/Scene.h"
#include <memory>
#include <vector>

namespace HybridPBR {

    /**
     * @brief 渲染管线描述符
     * 定义渲染管线的配置参数
     */
    struct RenderPipelineDesc {
        std::string name;
        uint32_t width = 1280;
        uint32_t height = 720;
        bool enableDepthTest = true;
        bool enableStencilTest = false;
        bool enableBlending = false;
        uint32_t samples = 1; // MSAA samples
        std::vector<std::string> renderTargets;
        std::string depthTarget;
    };

    /**
     * @brief 渲染管线抽象接口
     * 定义可组合的渲染管线架构
     * 
     * 修改理由：
     * 1. 支持可插拔的渲染管线设计
     * 2. 便于实现混合渲染策略
     * 3. 提供统一的渲染管线管理
     * 4. 支持渲染管线的动态配置
     */
    class IRenderPipeline {
    public:
        virtual ~IRenderPipeline() = default;

        // 管线生命周期
        virtual Result<void> Initialize(const RenderPipelineDesc& desc) = 0;
        virtual void Shutdown() = 0;
        virtual bool IsInitialized() const = 0;

        // 渲染执行
        virtual Result<void> BeginFrame() = 0;
        virtual Result<void> Render(const Scene& scene) = 0;
        virtual Result<void> EndFrame() = 0;

        // 资源管理
        virtual Result<void> Resize(uint32_t width, uint32_t height) = 0;
        virtual void SetRenderTarget(const std::string& name, std::shared_ptr<Texture> texture) = 0;
        virtual std::shared_ptr<Texture> GetRenderTarget(const std::string& name) const = 0;

        // 配置
        virtual void SetEnabled(bool enabled) = 0;
        virtual bool IsEnabled() const = 0;
        virtual const RenderPipelineDesc& GetDescription() const = 0;

        // 调试和分析
        virtual struct RenderStats GetStats() const = 0;
        virtual void ResetStats() = 0;
    };

    /**
     * @brief 渲染管线管理器
     * 管理多个渲染管线的执行顺序和依赖关系
     */
    class IRenderPipelineManager {
    public:
        virtual ~IRenderPipelineManager() = default;

        // 管线注册和管理
        virtual Result<void> RegisterPipeline(std::shared_ptr<IRenderPipeline> pipeline) = 0;
        virtual void UnregisterPipeline(const std::string& name) = 0;
        virtual std::shared_ptr<IRenderPipeline> GetPipeline(const std::string& name) = 0;
        virtual std::vector<std::shared_ptr<IRenderPipeline>> GetAllPipelines() const = 0;

        // 执行控制
        virtual Result<void> ExecutePipelines(const Scene& scene) = 0;
        virtual void SetPipelineExecutionOrder(const std::vector<std::string>& order) = 0;
        virtual void EnablePipeline(const std::string& name, bool enabled) = 0;

        // 依赖管理
        virtual Result<void> AddPipelineDependency(const std::string& pipeline, const std::string& dependency) = 0;
        virtual void RemovePipelineDependency(const std::string& pipeline, const std::string& dependency) = 0;
        virtual std::vector<std::string> GetPipelineDependencies(const std::string& pipeline) const = 0;
    };

} // namespace HybridPBR