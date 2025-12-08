#pragma once

#include "../../core/Result.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace HybridPBR {

    /**
     * @brief 渲染设备抽象接口
     * 提供图形API无关的底层渲染操作
     * 
     * 修改理由：
     * 1. 抽象图形API，便于未来扩展到Vulkan/DirectX
     * 2. 统一资源创建和管理接口
     * 3. 提供错误安全的资源操作
     * 4. 支持资源生命周期管理
     */
    class IRenderDevice {
    public:
        virtual ~IRenderDevice() = default;

        // 设备管理
        virtual Result<void> Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual bool IsInitialized() const = 0;

        // 缓冲区操作
        virtual Result<uint32_t> CreateBuffer(size_t size, uint32_t usage) = 0;
        virtual void DeleteBuffer(uint32_t bufferId) = 0;
        virtual Result<void> UpdateBuffer(uint32_t bufferId, const void* data, size_t size, size_t offset = 0) = 0;
        virtual void* MapBuffer(uint32_t bufferId, size_t size, size_t offset = 0) = 0;
        virtual void UnmapBuffer(uint32_t bufferId) = 0;

        // 纹理操作
        virtual Result<uint32_t> CreateTexture2D(int width, int height, uint32_t internalFormat, 
                                                uint32_t format, uint32_t type) = 0;
        virtual Result<uint32_t> CreateTextureCube(int width, int height, uint32_t internalFormat,
                                                  uint32_t format, uint32_t type) = 0;
        virtual void DeleteTexture(uint32_t textureId) = 0;
        virtual Result<void> UpdateTexture2D(uint32_t textureId, int level, int x, int y, 
                                           int width, int height, const void* data) = 0;
        virtual Result<void> UpdateTextureCube(uint32_t textureId, uint32_t face, int level,
                                             int x, int y, int width, int height, const void* data) = 0;
        virtual void SetTextureWrap(uint32_t textureId, uint32_t wrapS, uint32_t wrapT, uint32_t wrapR) = 0;
        virtual void SetTextureFilter(uint32_t textureId, uint32_t minFilter, uint32_t magFilter) = 0;

        // 着色器操作
        virtual Result<uint32_t> CreateShader(const std::string& vertexSource, 
                                            const std::string& fragmentSource,
                                            const std::string& geometrySource = "") = 0;
        virtual Result<uint32_t> CreateComputeShader(const std::string& computeSource) = 0;
        virtual void DeleteShader(uint32_t shaderId) = 0;
        virtual void UseShader(uint32_t shaderId) = 0;
        virtual Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::mat4& value) = 0;
        virtual Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec4& value) = 0;
        virtual Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec3& value) = 0;
        virtual Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec2& value) = 0;
        virtual Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, float value) = 0;
        virtual Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, int value) = 0;
        virtual Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, bool value) = 0;

        // 帧缓冲操作
        virtual Result<uint32_t> CreateFramebuffer() = 0;
        virtual void DeleteFramebuffer(uint32_t fboId) = 0;
        virtual Result<void> BindFramebuffer(uint32_t fboId) = 0;
        virtual Result<void> UnbindFramebuffer() = 0;
        virtual Result<void> AttachTextureToFramebuffer(uint32_t fboId, uint32_t textureId, 
                                                       uint32_t attachment, int level = 0) = 0;
        virtual Result<void> SetDrawBuffers(uint32_t fboId, const uint32_t* buffers, uint32_t count) = 0;
        virtual bool IsFramebufferComplete(uint32_t fboId) = 0;

        // 渲染状态
        virtual void SetViewport(int x, int y, int width, int height) = 0;
        virtual void SetClearColor(float r, float g, float b, float a) = 0;
        virtual void Clear(uint32_t buffers) = 0;
        virtual void Enable(uint32_t capability) = 0;
        virtual void Disable(uint32_t capability) = 0;
        virtual void SetBlendFunc(uint32_t srcFactor, uint32_t dstFactor) = 0;
        virtual void SetDepthFunc(uint32_t func) = 0;
        virtual void SetCullFace(uint32_t mode) = 0;

        // 渲染操作
        virtual void DrawArrays(uint32_t primitiveType, int first, int count) = 0;
        virtual void DrawElements(uint32_t primitiveType, int count, uint32_t indexType, const void* indices) = 0;
        virtual void DrawArraysInstanced(uint32_t primitiveType, int first, int count, int instanceCount) = 0;
        virtual void DrawElementsInstanced(uint32_t primitiveType, int count, uint32_t indexType, 
                                         const void* indices, int instanceCount) = 0;

        // 计算着色器
        virtual void DispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
        virtual void SetMemoryBarrier(uint32_t barrier) = 0;

        // 查询和同步
        virtual Result<uint32_t> CreateQuery(uint32_t queryType) = 0;
        virtual void DeleteQuery(uint32_t queryId) = 0;
        virtual void BeginQuery(uint32_t queryId) = 0;
        virtual void EndQuery(uint32_t queryId) = 0;
        virtual bool GetQueryResult(uint32_t queryId, uint64_t& result) = 0;
        virtual void Finish() = 0;
        virtual void Flush() = 0;

        // 调试支持
        virtual void SetDebugLabel(uint32_t object, uint32_t type, const std::string& label) = 0;
        virtual void PushDebugGroup(const std::string& message) = 0;
        virtual void PopDebugGroup() = 0;
    };

} // namespace HybridPBR