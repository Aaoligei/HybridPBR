#pragma once
#include "IRenderer.h"

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
    // 光栅化渲染器专用接口
    class IRasterizer : public IRenderer {
    public:
        virtual ~IRasterizer() = default;
        
        // 渲染模式设置
        virtual void SetWireframe(bool enabled) = 0;
        virtual void SetBackfaceCulling(bool enabled) = 0;
        virtual void SetDepthTest(bool enabled) = 0;
        
        // 渲染状态
        virtual bool IsWireframe() const = 0;
        virtual bool IsBackfaceCulling() const = 0;
        virtual bool IsDepthTest() const = 0;
    };

} // namespace HybridPBR