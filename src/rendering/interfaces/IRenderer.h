#pragma once
#include "../common/Material.h"
#include "../common/Light.h"
#include "../../scene/Scene.h"
#include "../../core/Result.h"
#include "IRenderDevice.h"
#include "IRenderPipeline.h"
#include <glm/glm.hpp>
#include <memory>

namespace HybridPBR {

    /**
     * @brief 渲染统计信息
     * 提供详细的渲染性能数据
     */
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangleCount = 0;
        uint32_t vertexCount = 0;
        uint32_t computeDispatches = 0;
        float frameTime = 0.0f;
        float gpuTime = 0.0f;
        float cpuTime = 0.0f;
        float fps = 0.0f;
        size_t memoryUsed = 0;
        size_t memoryAllocated = 0;
        
        void Reset() {
            drawCalls = 0;
            triangleCount = 0;
            vertexCount = 0;
            computeDispatches = 0;
            frameTime = 0.0f;
            gpuTime = 0.0f;
            cpuTime = 0.0f;
            fps = 0.0f;
        }
    };

    /**
     * @brief 渲染器抽象接口
     * 提供统一的渲染器架构
     * 
     * 修改理由：
     * 1. 使用Result类型提供错误安全的操作
     * 2. 添加设备抽象，支持多图形API
     * 3. 增加管线管理，支持复杂渲染策略
     * 4. 提供更详细的统计信息
     */
    class IRenderer {
    public:
        virtual ~IRenderer() = default;
        
        // 生命周期管理
        virtual Result<void> Initialize(std::shared_ptr<IRenderDevice> device) = 0;
        virtual void Shutdown() = 0;
        virtual bool IsInitialized() const = 0;
        
        // 渲染执行
        virtual Result<void> BeginFrame() = 0;
        virtual Result<void> Render(const Scene& scene) = 0;
        virtual Result<void> EndFrame() = 0;
        
        // 配置管理
        virtual Result<void> SetViewport(int width, int height) = 0;
        virtual Result<void> SetClearColor(const glm::vec4& color) = 0;
        virtual Result<void> Resize(uint32_t width, uint32_t height) = 0;
        
        // 管线管理
        virtual std::shared_ptr<IRenderPipelineManager> GetPipelineManager() = 0;
        virtual Result<void> AddPipeline(std::shared_ptr<IRenderPipeline> pipeline) = 0;
        
        // 资源管理
        virtual std::shared_ptr<Texture> GetOutputTexture() const = 0;
        virtual std::shared_ptr<IRenderDevice> GetDevice() const = 0;
        
        // 统计信息
        virtual const RenderStats& GetStats() const = 0;
        virtual void ResetStats() = 0;
        
        // 调试支持
        virtual void SetDebugMode(bool enabled) = 0;
        virtual bool IsDebugMode() const = 0;
    };

} // namespace HybridPBR