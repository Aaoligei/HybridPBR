#include "GL_Device.h"
#include "GL_CommandList.h" // 我们稍后实现这个
#include "utils/Logger.h"   // 复用你现有的日志系统
#include "utils/FileIO.h"  // 用于读取着色器文件
#include "../../resources/Mesh.h"

namespace HybridPBR {

    // ==========================================
    // 内部辅助函数：格式转换
    // 将 RHI 的枚举转换为 OpenGL 的 GLenum
    // ==========================================
    
    static GLenum ToGLInternalFormat(TextureFormat format) {
        switch (format) {
            case TextureFormat::RGBA8_UNORM:  return GL_RGBA8;
            case TextureFormat::RGBA16_FLOAT: return GL_RGBA16F;
            case TextureFormat::RGBA32_FLOAT: return GL_RGBA32F;
            case TextureFormat::D24_S8_UINT:  return GL_DEPTH24_STENCIL8;
            default: LOG_WARNING("RHI", "Unknown TextureFormat, defaulting to RGBA8"); return GL_RGBA8;
        }
    }

    static GLenum ToGLFormat(TextureFormat format) {
        switch (format) {
            case TextureFormat::D24_S8_UINT:  return GL_DEPTH_STENCIL;
            // 简单起见，颜色大多是 RGBA。实际项目中可能需要更细致的映射
            default: return GL_RGBA;
        }
    }

    static GLenum ToGLDataType(TextureFormat format) {
        switch (format) {
            case TextureFormat::RGBA8_UNORM:  return GL_UNSIGNED_BYTE;
            case TextureFormat::RGBA16_FLOAT: return GL_FLOAT;
            case TextureFormat::RGBA32_FLOAT: return GL_FLOAT;
            case TextureFormat::D24_S8_UINT:  return GL_UNSIGNED_INT_24_8;
            default: return GL_UNSIGNED_BYTE;
        }
    }

    // ==========================================
    // OpenGLDevice 实现
    // ==========================================

    OpenGLDevice::~OpenGLDevice() {
        Shutdown();
    }

    bool OpenGLDevice::Initialize() {
        // 注意：在调用此函数前，Window 类必须已经初始化了 GLFW 和 GLAD。
        // RHI 这一层假设 OpenGL 上下文已经存在。
        
        // 打印一些设备信息，确认我们拿到了正确的 Context
        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* version = glGetString(GL_VERSION);
        LOG_INFO("RHI", std::string("OpenGL Device Ready: ") + (const char*)renderer + " (" + (const char*)version + ")");

        // --- 新增：创建并配置全局 VAO ---
        glCreateVertexArrays(1, &m_globalVAO);
        glBindVertexArray(m_globalVAO);

        // 我们假设所有 Mesh 都使用 HybridPBR::Vertex 结构体
        // 启用属性位置 0~4
        for (int i = 0; i < 5; ++i) glEnableVertexAttribArray(i);

        // 设置格式 (对应 Vertex 结构体)
        // Format: 属性索引, 元素数量, 类型, 是否归一化, 相对偏移
        // Binding 0: 我们约定把 VBO 绑定到 Binding Point 0

        // 0: Position (vec3)
        glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
        glVertexAttribBinding(0, 0);

        // 1: Normal (vec3)
        glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
        glVertexAttribBinding(1, 0);

        // 2: TexCoord (vec2)
        glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texcoord));
        glVertexAttribBinding(2, 0);

        // 3: Tangent (vec3)
        glVertexAttribFormat(3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
        glVertexAttribBinding(3, 0);

        // 4: Bitangent (vec3)
        glVertexAttribFormat(4, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, bitangent));
        glVertexAttribBinding(4, 0);

        // 注意：这里我们只设置了格式，没有绑定具体的 Buffer (VBO)
        // 具体的 VBO 会在 CommandList::BindVertexBuffer 时通过 glBindVertexBuffer(0, ...) 绑定
        
        LOG_INFO("RHI", "Initialized Global VAO with default Vertex Layout");

        // 创建立即模式的命令列表
        // 这是一个核心设计：所有绘制命令都通过它，而不是直接调 glDraw
        m_immediateCmdList = std::make_shared<OpenGLCommandList>(this);

        return true;
    }

    void OpenGLDevice::Shutdown() {
        std::lock_guard<std::mutex> lock(m_resourceMutex);

        // 1. 清理 Buffer
        for (const auto& buf : m_buffers) {
            if (buf.id != 0) {
                glDeleteBuffers(1, &buf.id);
            }
        }
        m_buffers.clear();

        // 2. 清理 Texture
        for (const auto& tex : m_textures) {
            if (tex.id != 0) {
                glDeleteTextures(1, &tex.id);
            }
        }
        m_textures.clear();

        if (m_globalVAO != 0) {
            glDeleteVertexArrays(1, &m_globalVAO);
            m_globalVAO = 0;
        }

        // 3. 清理 Shader (暂未实现具体存储，先预留位置)
        
        m_immediateCmdList.reset();


        LOG_INFO("RHI", "OpenGL Device Shutdown");
    }

    // ------------------------------------------------------
    // 资源创建：Buffer
    // ------------------------------------------------------
    BufferHandle OpenGLDevice::CreateBuffer(const BufferDesc& desc, const void* initialData) {
        std::lock_guard<std::mutex> lock(m_resourceMutex);

        GLBuffer glBuf;
        glBuf.size = desc.size;

        // Step 1: 创建 ID
        glCreateBuffers(1, &glBuf.id);

        // Step 2: 确定存储标志
        // glNamedBufferStorage 是 "不可变存储" (Immutable Storage)，显卡驱动能做更多优化
        // 如果数据需要频繁更新 (isDynamic)，我们需要加上 MAP_WRITE_BIT 等标志
        GLbitfield flags = 0;
        if (desc.isDynamic) {
            // 允许映射写入和动态更新
            flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT; 
        }

        // Step 3: 分配显存并上传数据 (DSA API)
        glNamedBufferStorage(glBuf.id, desc.size, initialData, flags);

        // Step 4: 设置调试名称 (RenderDoc 抓帧时能看到名字，非常有用的细节！)
        if (!desc.name.empty()) {
            glObjectLabel(GL_BUFFER, glBuf.id, -1, desc.name.c_str());
        }

        // Step 5: 存入资源池并返回 Handle
        // Handle ID = 数组下标 + 1 (0 作为无效句柄)
        m_buffers.push_back(glBuf);
        return BufferHandle{ static_cast<uint64_t>(m_buffers.size()) };
    }

    void OpenGLDevice::DestroyBuffer(BufferHandle handle) {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        
        // 校验 Handle 有效性
        if (!handle.IsValid() || handle.id > m_buffers.size()) return;

        // 获取资源引用 (ID - 1)
        GLBuffer& glBuf = m_buffers[handle.id - 1];
        
        if (glBuf.id != 0) {
            glDeleteBuffers(1, &glBuf.id);
            glBuf.id = 0; // 标记为已删除，但不从 vector 移除以保持其他 Handle 有效
            glBuf.size = 0;
        }
    }

    // ------------------------------------------------------
    // 资源创建：Texture
    // ------------------------------------------------------
    TextureHandle OpenGLDevice::CreateTexture(const TextureDesc& desc, const void* initialData) {
        std::lock_guard<std::mutex> lock(m_resourceMutex);

        GLTexture glTex;
        glTex.width = desc.width;
        glTex.height = desc.height;
        glTex.glFormat = ToGLInternalFormat(desc.format);

        // Step 1: 创建 Texture
        glCreateTextures(GL_TEXTURE_2D, 1, &glTex.id);

        // Step 2: 分配显存 (Immutable)
        // 简单起见，这里没计算 Mipmap Levels，默认只分配 1 层
        glTextureStorage2D(glTex.id, 1, glTex.glFormat, desc.width, desc.height);

        // Step 3: 设置默认采样参数 (RHI 应该有专门的 Sampler 对象，这里先硬编码默认值)
        glTextureParameteri(glTex.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(glTex.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(glTex.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(glTex.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Step 4: 上传数据 (如果有)
        if (initialData) {
            glTextureSubImage2D(glTex.id, 
                0, 0, 0,                // level, x, y
                desc.width, desc.height,// w, h
                ToGLFormat(desc.format),
                ToGLDataType(desc.format),
                initialData
            );
        }

        // Step 5: 调试名称
        if (!desc.name.empty()) {
            glObjectLabel(GL_TEXTURE, glTex.id, -1, desc.name.c_str());
        }

        m_textures.push_back(glTex);
        return TextureHandle{ static_cast<uint64_t>(m_textures.size()) };
    }

    void OpenGLDevice::DestroyTexture(TextureHandle handle) {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        if (!handle.IsValid() || handle.id > m_textures.size()) return;

        GLTexture& glTex = m_textures[handle.id - 1];
        if (glTex.id != 0) {
            glDeleteTextures(1, &glTex.id);
            glTex.id = 0;
        }
    }


    // ------------------------------------------------------
    // 命令与帧
    // ------------------------------------------------------
    std::shared_ptr<RHI_CommandList> OpenGLDevice::GetImmediateCommandList() {
        return m_immediateCmdList;
    }

    void OpenGLDevice::BeginFrame() {
        // 这里可以重置一些每帧的状态
    }

    void OpenGLDevice::Present() {
        // 在 OpenGL 中，SwapBuffers 通常由 Window 系统 (GLFW) 控制
        // 这里留空，作为接口预留
    }

    // ------------------------------------------------------
    // 内部访问器 (供 CommandList 使用)
    // ------------------------------------------------------
    GLuint OpenGLDevice::GetGLBufferID(BufferHandle handle) const {
        if (!handle.IsValid() || handle.id > m_buffers.size()) return 0;
        return m_buffers[handle.id - 1].id;
    }

    GLuint OpenGLDevice::GetGLTextureID(TextureHandle handle) const {
        if (!handle.IsValid() || handle.id > m_textures.size()) return 0;
        return m_textures[handle.id - 1].id;
    }

    // ------------------------------------------------------
    // Shader 编译辅助函数
    // ------------------------------------------------------
    GLuint OpenGLDevice::CompileShader(GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            LOG_ERROR("RHI", std::string("Shader Compile Error: ") + infoLog);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint OpenGLDevice::LinkProgram(GLuint vertShader, GLuint fragShader) {
        GLuint program = glCreateProgram();
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            LOG_ERROR("RHI", std::string("Program Link Error: ") + infoLog);
            glDeleteProgram(program);
            return 0;
        }
        return program;
    }

    // ------------------------------------------------------
    // 资源创建：Shader
    // ------------------------------------------------------
    ShaderHandle OpenGLDevice::CreateShader(const std::string& vertPath, const std::string& fragPath) {
        std::lock_guard<std::mutex> lock(m_resourceMutex);

        // 1. 读取源码
        std::string vertSource = FileIO::ReadTextFile(vertPath);
        std::string fragSource = FileIO::ReadTextFile(fragPath);

        if (vertSource.empty() || fragSource.empty()) {
            LOG_ERROR("RHI", "Failed to load shader source: " + vertPath + " / " + fragPath);
            return ShaderHandle::Invalid();
        }

        // 2. 编译
        GLuint vertID = CompileShader(GL_VERTEX_SHADER, vertSource);
        if (vertID == 0) return ShaderHandle::Invalid();

        GLuint fragID = CompileShader(GL_FRAGMENT_SHADER, fragSource);
        if (fragID == 0) {
            glDeleteShader(vertID);
            return ShaderHandle::Invalid();
        }

        // 3. 链接
        GLuint programID = LinkProgram(vertID, fragID);
        
        // 链接后可以删除 Shader Object
        glDeleteShader(vertID);
        glDeleteShader(fragID);

        if (programID == 0) return ShaderHandle::Invalid();

        // 4. 存入池
        GLShader shader;
        shader.programID = programID;
        
        m_shaders.push_back(shader);
        LOG_INFO("RHI", "Created Shader: " + vertPath);
        
        return ShaderHandle{ static_cast<uint64_t>(m_shaders.size()) };
    }

    void OpenGLDevice::DestroyShader(ShaderHandle handle) {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        if (!handle.IsValid() || handle.id > m_shaders.size()) return;

        GLShader& shader = m_shaders[handle.id - 1];
        if (shader.programID != 0) {
            glDeleteProgram(shader.programID);
            shader.programID = 0;
            LOG_INFO("RHI", "Destroyed Shader Program");
        }
    }

    // ------------------------------------------------------
    // 资源创建：Simple Pipeline
    // ------------------------------------------------------
    PipelineHandle OpenGLDevice::CreateSimplePipeline(ShaderHandle shader) {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        
        // 只是简单的把 ShaderHandle 包装一下，未来会在这里设置 Blend/Depth 状态
        GLPipeline pipeline;
        pipeline.shaderHandle = shader;
        
        m_pipelines.push_back(pipeline);
        return PipelineHandle{ static_cast<uint64_t>(m_pipelines.size()) };
    }

    void OpenGLDevice::DestroyPipeline(PipelineHandle handle) {
        // Pipeline 本身只是状态聚合，没有独立的 GL 对象需要销毁
        // (除非以后用 glCreateProgramPipelines)
        // 这里留空或做标记清理
    }

    // ------------------------------------------------------
    // 内部访问器
    // ------------------------------------------------------
    GLuint OpenGLDevice::GetGLProgramID(PipelineHandle pipelineHandle) const {
        if (!pipelineHandle.IsValid() || pipelineHandle.id > m_pipelines.size()) return 0;
        
        // 1. 找到 Pipeline
        const GLPipeline& pipeline = m_pipelines[pipelineHandle.id - 1];
        
        // 2. 找到 Shader
        ShaderHandle shaderHandle = pipeline.shaderHandle;
        if (!shaderHandle.IsValid() || shaderHandle.id > m_shaders.size()) return 0;
        
        // 3. 返回 Program ID
        return m_shaders[shaderHandle.id - 1].programID;
    }

    void OpenGLDevice::UpdateBuffer(BufferHandle handle, const void* data, uint64_t size, uint64_t offset) {
        // 校验
        if (!handle.IsValid() || handle.id > m_buffers.size()) return;
        GLBuffer& glBuf = m_buffers[handle.id - 1];

        // 使用 DSA 更新
        glNamedBufferSubData(glBuf.id, offset, size, data);
    }

} // namespace HybridPBR