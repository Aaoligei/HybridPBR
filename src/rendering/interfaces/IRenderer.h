#pragma once
#include "../common/Material.h"
#include "../common/Light.h"
#include "../../scene/Scene.h"
#include <glm/glm.hpp>

namespace HybridPBR {

    // 渲染统计信息
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangleCount = 0;
        uint32_t vertexCount = 0;
        float frameTime = 0.0f;
        float fps = 0.0f;
        
        void Reset() {
            drawCalls = 0;
            triangleCount = 0;
            vertexCount = 0;
        }
    };

    class IRenderer {
    public:
        virtual ~IRenderer() = default;
        
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
        
        virtual void Render(const Scene& scene) = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        
        virtual void SetViewport(int width, int height) = 0;
        virtual void SetClearColor(const glm::vec4& color) = 0;
        
        const RenderStats& GetStats() const { return stats; }
        
    protected:
        RenderStats stats;
    };

} // namespace HybridPBR