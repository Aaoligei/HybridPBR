#pragma once
#include "rhi/RHI_CommandList.h"
#include "rhi/RHI_Device.h"
#include "scene/Scene.h" 

namespace HybridPBR {


    // 渲染上下文：传递每一帧所需的全局数据
    struct RenderContext {
        RHI_Device* device;
        std::shared_ptr<RHI_CommandList> cmdList;
        const Scene* scene;
        
        // 全局资源映射 (临时方案：用于通过 Mesh 找 GpuMesh)
        // 在 ECS 架构中，这通常是 Component
        void* resourceCache = nullptr; 
    };

    class RenderPass {
    public:
        virtual ~RenderPass() = default;

        virtual void Initialize(RHI_Device* device) = 0;
        virtual void Execute(const RenderContext& context) = 0;
        virtual void Cleanup() = 0;
        
        virtual std::string GetName() const = 0;
    };

} // namespace HybridPBR