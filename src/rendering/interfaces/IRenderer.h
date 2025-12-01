#pragma once
#include "../common/Material.h"
#include "../common/Light.h"
#include "../../scene/Scene.h"
#include <glm/glm.hpp>

namespace HybridPBR {
    // 必须遵循 std140 内存对齐规则
    // vec3 实际上占用 vec4 的空间 (16 bytes)
    struct CameraData {
        glm::mat4 view;          // 64 bytes
        glm::mat4 projection;    // 64 bytes
        glm::vec3 viewPos;       // 12 bytes
        float padding;           // 4 bytes (补齐到16字节)
    }; 

    struct GPULight {
        glm::vec3 position;  float padding1; // 16 bytes
        glm::vec3 direction; float padding2; // 16 bytes
        glm::vec3 color;     float intensity; // 16 bytes
        
        // 聚光灯/点光源参数打包
        float range;
        float constant;
        float linear;
        float quadratic;     // 16 bytes
        
        float innerCutoff;
        float outerCutoff;
        int type;            // 0:Directional, 1:Point, 2:Spot
        float padding3;      // 16 bytes
    };

    struct LightData {
        int lightCount;
        int padding[3];       // 补齐 16 bytes
        GPULight lights[16];  // 支持最多16个光源
    };
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