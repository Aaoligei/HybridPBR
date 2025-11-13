#pragma once
#include "IRenderer.h"

namespace HybridPBR {

    // 光线追踪渲染器专用接口
    class IRayTracer : public IRenderer {
    public:
        virtual ~IRayTracer() = default;
        
        // 采样控制
        virtual void SetSampleCount(uint32_t samples) = 0;
        virtual void SetMaxBounces(uint32_t bounces) = 0;
        virtual void SetRussianRoulette(bool enabled) = 0;
        
        // 降噪设置
        virtual void SetDenoiserEnabled(bool enabled) = 0;
        virtual void SetDenoiserStrength(float strength) = 0;
        
        // 渐进式渲染
        virtual void ResetAccumulation() = 0;
        virtual uint32_t GetAccumulatedFrames() const = 0;
    };

} // namespace HybridPBR