#pragma once
#include "../RHI_Device.h"
#include "GL_CommandList.h"
#include "GL_Common.h"
#include <vector>
#include <mutex>

namespace HybridPBR {

    // 内部结构：真正的 OpenGL 资源
    struct GLBuffer {
        GLuint id = 0;
        uint64_t size = 0;
        uint32_t usageFlags = 0;
    };

    struct GLTexture {
        GLuint id = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        GLenum glFormat = 0;
    };
    // 新增：Shader 内部结构
    struct GLShader {
        GLuint programID = 0;
        // 可以在这里缓存 Uniform 位置，避免每次 glGetUniformLocation
        // std::unordered_map<std::string, GLint> uniformCache;
    };

    // 新增：Pipeline 内部结构 (目前只有 Shader，未来会包含更多状态)
    struct GLPipeline {
        ShaderHandle shaderHandle;
        // GLuint vao = 0; // 以后可能需要在这里缓存 VAO
        // DepthState depthState;
        // BlendState blendState;
    };

    class OpenGLDevice : public RHI_Device {
    public:
        OpenGLDevice() = default;
        ~OpenGLDevice() override;

        // --- 初始化/销毁 ---
        bool Initialize() override;
        void Shutdown() override;

        // --- 资源创建 ---
        BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) override;
        TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) override;
        // --- Shader & Pipeline 实现 ---
        ShaderHandle CreateShader(const std::string& vertPath, const std::string& fragPath) override;
        PipelineHandle CreateSimplePipeline(ShaderHandle shader) override;
        virtual void UpdateBuffer(BufferHandle handle, const void* data, uint64_t size, uint64_t offset = 0) override;

        void DestroyPipeline(PipelineHandle handle) override;
        // (记得也要在 cpp 实现 DestroyShader，虽然 RHI_Device 接口里可能漏写了，这里先补上逻辑)
        void DestroyShader(ShaderHandle handle); 

        // --- 内部访问器 ---
        GLuint GetGLProgramID(PipelineHandle pipelineHandle) const;
        // --- 资源销毁 ---
        void DestroyBuffer(BufferHandle handle) override;
        void DestroyTexture(TextureHandle handle) override;

        // --- 命令列表 ---
        std::shared_ptr<RHI_CommandList> GetImmediateCommandList() override;

        // --- 帧管理 ---
        void BeginFrame() override;
        void Present() override;

    public:
        // --- 内部公共方法 (供 CommandList 使用) ---
        // 通过 Handle 获取真实的 OpenGL ID
        GLuint GetGLBufferID(BufferHandle handle) const;
        GLuint GetGLTextureID(TextureHandle handle) const;
        GLuint GetGlobalVAO() const { return m_globalVAO; }

    private:
        // 资源池：使用 vector 存储资源，Handle.id 对应 vector 下标
        // 这种设计比 map 更快，且易于管理
        std::vector<GLBuffer> m_buffers;
        std::vector<GLTexture> m_textures;
        // 新增资源池
        std::vector<GLShader> m_shaders;
        std::vector<GLPipeline> m_pipelines;
        GLuint m_globalVAO = 0;
        
        // 简单的空闲列表，用于回收 ID（简化版可暂不实现，直接无限增长或标记删除）
        // std::queue<uint32_t> m_freeBufferIndices;

        std::shared_ptr<OpenGLCommandList> m_immediateCmdList;
        
        // 线程安全锁（如果我们要支持多线程加载资源）
        std::mutex m_resourceMutex;

        // Shader 编译辅助函数
        GLuint CompileShader(GLenum type, const std::string& source);
        GLuint LinkProgram(GLuint vertShader, GLuint fragShader);
    };

} // namespace HybridPBR