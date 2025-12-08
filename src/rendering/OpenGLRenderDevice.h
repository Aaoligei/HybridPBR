#pragma once

#include "interfaces/IRenderDevice.h"
#include "core/Result.h"
#include <unordered_map>
#include <string>

namespace HybridPBR {

    /**
     * @brief OpenGL渲染设备实现
     * 实现IRenderDevice接口，提供OpenGL特定的渲染操作
     * 
     * 修改理由：
     * 1. 抽象OpenGL具体实现，便于未来扩展其他图形API
     * 2. 提供错误安全的OpenGL操作
     * 3. 统一资源管理和生命周期控制
     * 4. 支持调试和性能分析
     */
    class OpenGLRenderDevice : public IRenderDevice {
    public:
        OpenGLRenderDevice();
        ~OpenGLRenderDevice() override;

        // IRenderDevice接口实现
        Result<void> Initialize() override;
        void Shutdown() override;
        bool IsInitialized() const override { return initialized_; }
        
        // 在OpenGL上下文创建后调用
        Result<void> InitializeAfterContext();

        // 缓冲区操作
        Result<uint32_t> CreateBuffer(size_t size, uint32_t usage) override;
        void DeleteBuffer(uint32_t bufferId) override;
        Result<void> UpdateBuffer(uint32_t bufferId, const void* data, size_t size, size_t offset = 0) override;
        void* MapBuffer(uint32_t bufferId, size_t size, size_t offset = 0) override;
        void UnmapBuffer(uint32_t bufferId) override;

        // 纹理操作
        Result<uint32_t> CreateTexture2D(int width, int height, uint32_t internalFormat, 
                                        uint32_t format, uint32_t type) override;
        Result<uint32_t> CreateTextureCube(int width, int height, uint32_t internalFormat,
                                          uint32_t format, uint32_t type) override;
        void DeleteTexture(uint32_t textureId) override;
        Result<void> UpdateTexture2D(uint32_t textureId, int level, int x, int y, 
                                    int width, int height, const void* data) override;
        Result<void> UpdateTextureCube(uint32_t textureId, uint32_t face, int level,
                                      int x, int y, int width, int height, const void* data) override;
        void SetTextureWrap(uint32_t textureId, uint32_t wrapS, uint32_t wrapT, uint32_t wrapR) override;
        void SetTextureFilter(uint32_t textureId, uint32_t minFilter, uint32_t magFilter) override;

        // 着色器操作
        Result<uint32_t> CreateShader(const std::string& vertexSource, 
                                     const std::string& fragmentSource,
                                     const std::string& geometrySource = "") override;
        Result<uint32_t> CreateComputeShader(const std::string& computeSource) override;
        void DeleteShader(uint32_t shaderId) override;
        void UseShader(uint32_t shaderId) override;
        Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::mat4& value) override;
        Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec4& value) override;
        Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec3& value) override;
        Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec2& value) override;
        Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, float value) override;
        Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, int value) override;
        Result<void> SetShaderUniform(uint32_t shaderId, const std::string& name, bool value) override;

        // 帧缓冲操作
        Result<uint32_t> CreateFramebuffer() override;
        void DeleteFramebuffer(uint32_t fboId) override;
        Result<void> BindFramebuffer(uint32_t fboId) override;
        Result<void> UnbindFramebuffer() override;
        Result<void> AttachTextureToFramebuffer(uint32_t fboId, uint32_t textureId, 
                                               uint32_t attachment, int level = 0) override;
        Result<void> SetDrawBuffers(uint32_t fboId, const uint32_t* buffers, uint32_t count) override;
        bool IsFramebufferComplete(uint32_t fboId) override;

        // 渲染状态
        void SetViewport(int x, int y, int width, int height) override;
        void SetClearColor(float r, float g, float b, float a) override;
        void Clear(uint32_t buffers) override;
        void Enable(uint32_t capability) override;
        void Disable(uint32_t capability) override;
        void SetBlendFunc(uint32_t srcFactor, uint32_t dstFactor) override;
        void SetDepthFunc(uint32_t func) override;
        void SetCullFace(uint32_t mode) override;

        // 渲染操作
        void DrawArrays(uint32_t primitiveType, int first, int count) override;
        void DrawElements(uint32_t primitiveType, int count, uint32_t indexType, const void* indices) override;
        void DrawArraysInstanced(uint32_t primitiveType, int first, int count, int instanceCount) override;
        void DrawElementsInstanced(uint32_t primitiveType, int count, uint32_t indexType, 
                                   const void* indices, int instanceCount) override;

        // 计算着色器
        void DispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
        void SetMemoryBarrier(uint32_t barrier) override;

        // 查询和同步
        Result<uint32_t> CreateQuery(uint32_t queryType) override;
        void DeleteQuery(uint32_t queryId) override;
        void BeginQuery(uint32_t queryId) override;
        void EndQuery(uint32_t queryId) override;
        bool GetQueryResult(uint32_t queryId, uint64_t& result) override;
        void Finish() override;
        void Flush() override;

        // 调试支持
        void SetDebugLabel(uint32_t object, uint32_t type, const std::string& label) override;
        void PushDebugGroup(const std::string& message) override;
        void PopDebugGroup() override;

    private:
        bool initialized_ = false;
        uint32_t currentShader_ = 0;
        uint32_t boundFramebuffer_ = 0;
        
        // 资源跟踪（用于调试和清理）
        std::unordered_map<uint32_t, std::string> bufferLabels_;
        std::unordered_map<uint32_t, std::string> textureLabels_;
        std::unordered_map<uint32_t, std::string> shaderLabels_;
        std::unordered_map<uint32_t, std::string> framebufferLabels_;

        // 辅助方法
        Result<uint32_t> CompileShader(uint32_t type, const std::string& source);
        Result<uint32_t> LinkProgram(uint32_t vertexShader, uint32_t fragmentShader, uint32_t geometryShader);
        int GetUniformLocation(uint32_t shaderId, const std::string& name);
        void CheckGLError(const std::string& operation);
        std::string GetShaderInfoLog(uint32_t shader);
        std::string GetProgramInfoLog(uint32_t program);
    };

} // namespace HybridPBR