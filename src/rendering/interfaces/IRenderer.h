#pragma once
#include "../../scene/Scene.h"
#include "../../core/Result.h"
#include <glm/glm.hpp>
#include <memory>

namespace HybridPBR {

    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangleCount = 0;
        uint32_t vertexCount = 0;
        float frameTime = 0.0f;
        
        void Reset() {
            drawCalls = 0;
            triangleCount = 0;
            vertexCount = 0;
            frameTime = 0.0f;
        }
    };

    /**
     * @brief 渲染器抽象接口 (RHI Refactored Version)
     */
    class IRenderer {
    public:
        virtual ~IRenderer() = default;
        
        // 生命周期管理
        // [修改] 不再需要传入 IRenderDevice，渲染器内部负责创建 RHI Device
        virtual Result<void> Initialize() = 0;
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
        
        // 统计信息
        virtual const RenderStats& GetStats() const = 0;
        virtual void ResetStats() = 0;
        
        // 调试支持
        virtual void SetDebugMode(bool enabled) = 0;
        virtual bool IsDebugMode() const = 0;
    };

} // namespace HybridPBR