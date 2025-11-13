#pragma once
#include "IRenderer.h"

namespace HybridPBR {

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