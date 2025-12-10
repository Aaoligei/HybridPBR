#pragma once
#include "RHI_Types.h"
#include <glm/glm.hpp>

namespace HybridPBR {

    class RHI_CommandList {
    public:
        virtual ~RHI_CommandList() = default;

        // 1. 渲染状态设置
        virtual void SetViewport(const Rect2D& rect) = 0;
        virtual void SetScissor(const Rect2D& rect) = 0;
        virtual void SetPipelineState(PipelineHandle pipeline) = 0; // 绑定 Shader 和状态

        // 2. 资源绑定
        // 注意：这里我们使用 Slot (绑定点) 的概念，而不是 OpenGL 的名字
        virtual void BindVertexBuffer(BufferHandle buffer, uint32_t binding = 0, uint64_t offset = 0) = 0;
        virtual void BindIndexBuffer(BufferHandle buffer, uint64_t offset = 0) = 0;
        
        // 绑定描述符集/资源组 (Uniforms, Textures)
        // 这是现代 API 的核心：将 shader 资源打包绑定
        virtual void BindConstants(const void* data, uint32_t size, uint32_t slot) = 0;
        virtual void BindTexture(uint32_t slot, TextureHandle texture) = 0;
        // 绑定 Uniform Buffer (对应 layout(binding=slot) uniform Block { ... })
        virtual void BindUniformBuffer(uint32_t slot, BufferHandle buffer, uint64_t offset = 0, uint64_t size = 0) = 0;

        // 推送常量 (对应 Vulkan Push Constants / OpenGL Uniforms)
        // stageFlags 先忽略，shader 暂时设为 nullptr (如果是 GL 可以从当前绑定的 Pipeline 推断)
        virtual void BindPushConstants(PipelineHandle pipeline, uint32_t offset, uint32_t size, const void* data) = 0;

        // 3. 绘制命令
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void Clear(bool color, bool depth, const glm::vec4& colorValue = {0,0,0,1}, float depthValue = 1.0f) = 0;
        // 4. 生命周期
        virtual void Begin() = 0;
        virtual void End() = 0;
    };

} // namespace HybridPBR