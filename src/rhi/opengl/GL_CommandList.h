#pragma once
#include "../RHI_CommandList.h"
#include "GL_Common.h"

namespace HybridPBR {

    class OpenGLDevice; // 前向声明

    class OpenGLCommandList : public RHI_CommandList {
    public:
        OpenGLCommandList(OpenGLDevice* device);
        ~OpenGLCommandList() override = default;

        // --- 状态设置 ---
        void SetViewport(const Rect2D& rect) override;
        void SetScissor(const Rect2D& rect) override;
        void SetPipelineState(PipelineHandle pipeline) override;

        // --- 资源绑定 ---
        void BindVertexBuffer(BufferHandle buffer, uint32_t binding = 0, uint64_t offset = 0) override;
        void BindIndexBuffer(BufferHandle buffer, uint64_t offset = 0) override;
        void BindConstants(const void* data, uint32_t size, uint32_t slot) override;
        void BindTexture(uint32_t slot, TextureHandle texture) override;
        void BindUniformBuffer(uint32_t slot, BufferHandle buffer, uint64_t offset, uint64_t size) override;
        void BindPushConstants(PipelineHandle pipeline, uint32_t offset, uint32_t size, const void* data) override;
        
        // --- 绘制命令 ---
        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
        void Clear(bool color, bool depth, const glm::vec4& colorValue, float depthValue) override;
        // --- 生命周期 ---
        void Begin() override;
        void End() override;

    private:
        OpenGLDevice* m_device; // 需要访问 Device 来获取真实的 GL 资源
        
        // 这里可以添加状态缓存 (State Cache) 来减少冗余的 GL 调用
        // 例如：PipelineHandle m_currentPipeline;
    };

} // namespace HybridPBR