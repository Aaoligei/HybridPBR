#include "OpenGLRenderDevice.h"
#include "utils/Logger.h"
#include <glad/glad.h>
#include <sstream>

#define GL_CHECK(x) do { x; } while(0)

namespace HybridPBR {

    OpenGLRenderDevice::OpenGLRenderDevice() : initialized_(false), currentShader_(0), boundFramebuffer_(0) {
    }

    OpenGLRenderDevice::~OpenGLRenderDevice() {
        Shutdown();
    }

    Result<void> OpenGLRenderDevice::Initialize() {
        if (initialized_) {
            return Result<void>::Success();
        }

        LOG_INFO("Renderer", "Initializing OpenGL Render Device");

        // 延迟OpenGL版本检查到上下文创建后
        // 这些检查将在InitializeAfterContext()中进行

        // 启用调试输出（如果可用）
        #ifdef GL_VERSION_4_3
        if (glDebugMessageCallback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, 
                                     GLenum severity, GLsizei length, 
                                     const GLchar* message, const void* userParam) {
                LogLevel level = LogLevel::Info;
                switch (severity) {
                    case GL_DEBUG_SEVERITY_HIGH: level = LogLevel::Error; break;
                    case GL_DEBUG_SEVERITY_MEDIUM: level = LogLevel::Warning; break;
                    case GL_DEBUG_SEVERITY_LOW: level = LogLevel::Debug; break;
                    case GL_DEBUG_SEVERITY_NOTIFICATION: level = LogLevel::Trace; break;
                }
                
                switch (level) {
                    case LogLevel::Error: LOG_ERROR("OpenGL", message); break;
                    case LogLevel::Warning: LOG_WARNING("OpenGL", message); break;
                    case LogLevel::Debug: LOG_DEBUG("OpenGL", message); break;
                    case LogLevel::Trace: LOG_TRACE("OpenGL", message); break;
                    default: LOG_INFO("OpenGL", message); break;
                }
            }, nullptr);
        }
        #endif

        // 设置默认状态
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        initialized_ = true;
        LOG_INFO("Renderer", "OpenGL Render Device initialized successfully");
        
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::InitializeAfterContext() {
        if (!initialized_) {
            return Result<void>::Failure(Error(ErrorType::Initialization, "Render device not initialized"));
        }

        LOG_INFO("Renderer", "Initializing OpenGL Render Device after context creation");

        // 检查OpenGL版本
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        
        LOG_INFO("Renderer", std::string("OpenGL Version: ") + version);
        LOG_INFO("Renderer", std::string("OpenGL Renderer: ") + renderer);

        // 启用调试输出（如果可用）
        #ifdef GL_VERSION_4_3
        if (glDebugMessageCallback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, 
                                     GLenum severity, GLsizei length, 
                                     const GLchar* message, const void* userParam) {
                LogLevel level = LogLevel::Info;
                switch (severity) {
                    case GL_DEBUG_SEVERITY_HIGH: level = LogLevel::Error; break;
                    case GL_DEBUG_SEVERITY_MEDIUM: level = LogLevel::Warning; break;
                    case GL_DEBUG_SEVERITY_LOW: level = LogLevel::Debug; break;
                    case GL_DEBUG_SEVERITY_NOTIFICATION: level = LogLevel::Trace; break;
                }
                
                switch (level) {
                    case LogLevel::Error: LOG_ERROR("OpenGL", message); break;
                    case LogLevel::Warning: LOG_WARNING("OpenGL", message); break;
                    case LogLevel::Debug: LOG_DEBUG("OpenGL", message); break;
                    case LogLevel::Trace: LOG_TRACE("OpenGL", message); break;
                    default: LOG_INFO("OpenGL", message); break;
                }
            }, nullptr);
        }
        #endif

        // 设置默认状态
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        LOG_INFO("Renderer", "OpenGL Render Device post-context initialization completed");
        return Result<void>::Success();
    }

    void OpenGLRenderDevice::Shutdown() {
        if (!initialized_) {
            return;
        }

        // 清理所有资源
        for (const auto& [id, label] : bufferLabels_) {
            glDeleteBuffers(1, &id);
        }
        bufferLabels_.clear();

        for (const auto& [id, label] : textureLabels_) {
            glDeleteTextures(1, &id);
        }
        textureLabels_.clear();

        for (const auto& [id, label] : shaderLabels_) {
            glDeleteProgram(id);
        }
        shaderLabels_.clear();

        for (const auto& [id, label] : framebufferLabels_) {
            glDeleteFramebuffers(1, &id);
        }
        framebufferLabels_.clear();

        initialized_ = false;
        LOG_INFO("Renderer", "OpenGL Render Device shutdown");
    }

    Result<uint32_t> OpenGLRenderDevice::CreateBuffer(size_t size, uint32_t usage) {
        uint32_t bufferId;
        GL_CHECK(glGenBuffers(1, &bufferId));
        
        GL_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, bufferId));
        GL_CHECK(glBufferData(GL_COPY_WRITE_BUFFER, size, nullptr, usage));
        GL_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));

        bufferLabels_[bufferId] = "Buffer_" + std::to_string(bufferId);
        
        return Result<uint32_t>::Success(bufferId);
    }

    void OpenGLRenderDevice::DeleteBuffer(uint32_t bufferId) {
        if (bufferId == 0) return;
        
        GL_CHECK(glDeleteBuffers(1, &bufferId));
        bufferLabels_.erase(bufferId);
    }

    Result<void> OpenGLRenderDevice::UpdateBuffer(uint32_t bufferId, const void* data, size_t size, size_t offset) {
        if (bufferLabels_.find(bufferId) == bufferLabels_.end()) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Invalid buffer ID"));
        }

        GL_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, bufferId));
        GL_CHECK(glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data));
        GL_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));
        
        return Result<void>::Success();
    }

    void* OpenGLRenderDevice::MapBuffer(uint32_t bufferId, size_t size, size_t offset) {
        if (bufferLabels_.find(bufferId) == bufferLabels_.end()) {
            return nullptr;
        }

        void* ptr = nullptr;
        GL_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, bufferId));
        ptr = glMapBufferRange(GL_COPY_WRITE_BUFFER, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        GL_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));
        
        return ptr;
    }

    void OpenGLRenderDevice::UnmapBuffer(uint32_t bufferId) {
        GL_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, bufferId));
        glUnmapBuffer(GL_COPY_WRITE_BUFFER);
        GL_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));
    }

    Result<uint32_t> OpenGLRenderDevice::CreateTexture2D(int width, int height, uint32_t internalFormat, 
                                                        uint32_t format, uint32_t type) {
        uint32_t textureId;
        GL_CHECK(glGenTextures(1, &textureId));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureId));
        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

        textureLabels_[textureId] = "Texture2D_" + std::to_string(textureId);
        
        return Result<uint32_t>::Success(textureId);
    }

    Result<uint32_t> OpenGLRenderDevice::CreateTextureCube(int width, int height, uint32_t internalFormat,
                                                          uint32_t format, uint32_t type) {
        uint32_t textureId;
        GL_CHECK(glGenTextures(1, &textureId));
        GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, textureId));
        
        for (int i = 0; i < 6; ++i) {
            GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, 
                                 width, height, 0, format, type, nullptr));
        }
        
        GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

        textureLabels_[textureId] = "TextureCube_" + std::to_string(textureId);
        
        return Result<uint32_t>::Success(textureId);
    }

    void OpenGLRenderDevice::DeleteTexture(uint32_t textureId) {
        if (textureId == 0) return;
        
        GL_CHECK(glDeleteTextures(1, &textureId));
        textureLabels_.erase(textureId);
    }

    Result<void> OpenGLRenderDevice::UpdateTexture2D(uint32_t textureId, int level, int x, int y, 
                                                    int width, int height, const void* data) {
        if (textureLabels_.find(textureId) == textureLabels_.end()) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Invalid texture ID"));
        }

        GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureId));
        GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, level, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
        
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::UpdateTextureCube(uint32_t textureId, uint32_t face, int level,
                                                      int x, int y, int width, int height, const void* data) {
        if (textureLabels_.find(textureId) == textureLabels_.end()) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Invalid texture ID"));
        }

        GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, textureId));
        GL_CHECK(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, x, y, width, height, 
                                 GL_RGBA, GL_UNSIGNED_BYTE, data));
        GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
        
        return Result<void>::Success();
    }

    void OpenGLRenderDevice::SetTextureWrap(uint32_t textureId, uint32_t wrapS, uint32_t wrapT, uint32_t wrapR) {
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureId));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT));
        if (wrapR != 0) {
            GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapR));
        }
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    }

    void OpenGLRenderDevice::SetTextureFilter(uint32_t textureId, uint32_t minFilter, uint32_t magFilter) {
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureId));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    }

    Result<uint32_t> OpenGLRenderDevice::CreateShader(const std::string& vertexSource, 
                                                     const std::string& fragmentSource,
                                                     const std::string& geometrySource) {
        // 编译顶点着色器
        auto vertexResult = CompileShader(GL_VERTEX_SHADER, vertexSource);
        if (vertexResult.IsFailure()) {
            return vertexResult.Map([](uint32_t) { return 0u; });
        }
        uint32_t vertexShader = vertexResult.GetValue();

        // 编译片段着色器
        auto fragmentResult = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (fragmentResult.IsFailure()) {
            glDeleteShader(vertexShader);
            return fragmentResult.Map([](uint32_t) { return 0u; });
        }
        uint32_t fragmentShader = fragmentResult.GetValue();

        // 编译几何着色器（如果提供）
        uint32_t geometryShader = 0;
        if (!geometrySource.empty()) {
            auto geometryResult = CompileShader(GL_GEOMETRY_SHADER, geometrySource);
            if (geometryResult.IsFailure()) {
                glDeleteShader(vertexShader);
                glDeleteShader(fragmentShader);
                return geometryResult.Map([](uint32_t) { return 0u; });
            }
            geometryShader = geometryResult.GetValue();
        }

        // 链接着色器程序
        auto programResult = LinkProgram(vertexShader, fragmentShader, geometryShader);
        
        // 清理着色器对象
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader != 0) {
            glDeleteShader(geometryShader);
        }

        if (programResult.IsFailure()) {
            return programResult.Map([](uint32_t) { return 0u; });
        }

        uint32_t program = programResult.GetValue();
        shaderLabels_[program] = "Shader_" + std::to_string(program);
        
        return Result<uint32_t>::Success(program);
    }

    Result<uint32_t> OpenGLRenderDevice::CreateComputeShader(const std::string& computeSource) {
        auto computeResult = CompileShader(GL_COMPUTE_SHADER, computeSource);
        if (computeResult.IsFailure()) {
            return computeResult.Map([](uint32_t) { return 0u; });
        }
        uint32_t computeShader = computeResult.GetValue();

        uint32_t program = glCreateProgram();
        glAttachShader(program, computeShader);
        glLinkProgram(program);

        // 检查链接错误
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            std::string infoLog = GetProgramInfoLog(program);
            glDeleteProgram(program);
            glDeleteShader(computeShader);
            return Result<uint32_t>::Failure(Error(ErrorType::ShaderCompilation, "Compute shader link failed: " + infoLog));
        }

        glDeleteShader(computeShader);
        shaderLabels_[program] = "ComputeShader_" + std::to_string(program);
        
        return Result<uint32_t>::Success(program);
    }

    void OpenGLRenderDevice::DeleteShader(uint32_t shaderId) {
        if (shaderId == 0) return;
        
        GL_CHECK(glDeleteProgram(shaderId));
        shaderLabels_.erase(shaderId);
    }

    void OpenGLRenderDevice::UseShader(uint32_t shaderId) {
        if (currentShader_ != shaderId) {
            GL_CHECK(glUseProgram(shaderId));
            currentShader_ = shaderId;
        }
    }

    Result<void> OpenGLRenderDevice::SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::mat4& value) {
        GLint location = GetUniformLocation(shaderId, name);
        if (location == -1) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Uniform not found: " + name));
        }
        
        UseShader(shaderId);
        GL_CHECK(glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]));
        
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec4& value) {
        GLint location = GetUniformLocation(shaderId, name);
        if (location == -1) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Uniform not found: " + name));
        }
        
        UseShader(shaderId);
        GL_CHECK(glUniform4f(location, value.x, value.y, value.z, value.w));
        
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec3& value) {
        GLint location = GetUniformLocation(shaderId, name);
        if (location == -1) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Uniform not found: " + name));
        }
        
        UseShader(shaderId);
        GL_CHECK(glUniform3f(location, value.x, value.y, value.z));
        
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::SetShaderUniform(uint32_t shaderId, const std::string& name, const glm::vec2& value) {
        GLint location = GetUniformLocation(shaderId, name);
        if (location == -1) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Uniform not found: " + name));
        }
        
        UseShader(shaderId);
        GL_CHECK(glUniform2f(location, value.x, value.y));
        
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::SetShaderUniform(uint32_t shaderId, const std::string& name, float value) {
        GLint location = GetUniformLocation(shaderId, name);
        if (location == -1) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Uniform not found: " + name));
        }
        
        UseShader(shaderId);
        GL_CHECK(glUniform1f(location, value));
        
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::SetShaderUniform(uint32_t shaderId, const std::string& name, int value) {
        GLint location = GetUniformLocation(shaderId, name);
        if (location == -1) {
            return Result<void>::Failure(Error(ErrorType::InvalidParameter, "Uniform not found: " + name));
        }
        
        UseShader(shaderId);
        GL_CHECK(glUniform1i(location, value));
        
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::SetShaderUniform(uint32_t shaderId, const std::string& name, bool value) {
        return SetShaderUniform(shaderId, name, value ? 1 : 0);
    }

    Result<uint32_t> OpenGLRenderDevice::CreateFramebuffer() {
        uint32_t fboId;
        GL_CHECK(glGenFramebuffers(1, &fboId));
        
        framebufferLabels_[fboId] = "Framebuffer_" + std::to_string(fboId);
        
        return Result<uint32_t>::Success(fboId);
    }

    void OpenGLRenderDevice::DeleteFramebuffer(uint32_t fboId) {
        if (fboId == 0) return;
        
        GL_CHECK(glDeleteFramebuffers(1, &fboId));
        framebufferLabels_.erase(fboId);
        
        if (boundFramebuffer_ == fboId) {
            boundFramebuffer_ = 0;
        }
    }

    Result<void> OpenGLRenderDevice::BindFramebuffer(uint32_t fboId) {
        if (boundFramebuffer_ != fboId) {
            GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fboId));
            boundFramebuffer_ = fboId;
        }
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::UnbindFramebuffer() {
        return BindFramebuffer(0);
    }

    Result<void> OpenGLRenderDevice::AttachTextureToFramebuffer(uint32_t fboId, uint32_t textureId, 
                                                              uint32_t attachment, int level) {
        BindFramebuffer(fboId);
        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, textureId, level));
        return Result<void>::Success();
    }

    Result<void> OpenGLRenderDevice::SetDrawBuffers(uint32_t fboId, const uint32_t* buffers, uint32_t count) {
        BindFramebuffer(fboId);
        GL_CHECK(glDrawBuffers(count, buffers));
        return Result<void>::Success();
    }

    bool OpenGLRenderDevice::IsFramebufferComplete(uint32_t fboId) {
        BindFramebuffer(fboId);
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    void OpenGLRenderDevice::SetViewport(int x, int y, int width, int height) {
        GL_CHECK(glViewport(x, y, width, height));
    }

    void OpenGLRenderDevice::SetClearColor(float r, float g, float b, float a) {
        GL_CHECK(glClearColor(r, g, b, a));
    }

    void OpenGLRenderDevice::Clear(uint32_t buffers) {
        GL_CHECK(glClear(buffers));
    }

    void OpenGLRenderDevice::Enable(uint32_t capability) {
        GL_CHECK(glEnable(capability));
    }

    void OpenGLRenderDevice::Disable(uint32_t capability) {
        GL_CHECK(glDisable(capability));
    }

    void OpenGLRenderDevice::SetBlendFunc(uint32_t srcFactor, uint32_t dstFactor) {
        GL_CHECK(glBlendFunc(srcFactor, dstFactor));
    }

    void OpenGLRenderDevice::SetDepthFunc(uint32_t func) {
        GL_CHECK(glDepthFunc(func));
    }

    void OpenGLRenderDevice::SetCullFace(uint32_t mode) {
        GL_CHECK(glCullFace(mode));
    }

    void OpenGLRenderDevice::DrawArrays(uint32_t primitiveType, int first, int count) {
        GL_CHECK(glDrawArrays(primitiveType, first, count));
    }

    void OpenGLRenderDevice::DrawElements(uint32_t primitiveType, int count, uint32_t indexType, const void* indices) {
        GL_CHECK(glDrawElements(primitiveType, count, indexType, indices));
    }

    void OpenGLRenderDevice::DrawArraysInstanced(uint32_t primitiveType, int first, int count, int instanceCount) {
        GL_CHECK(glDrawArraysInstanced(primitiveType, first, count, instanceCount));
    }

    void OpenGLRenderDevice::DrawElementsInstanced(uint32_t primitiveType, int count, uint32_t indexType, 
                                                   const void* indices, int instanceCount) {
        GL_CHECK(glDrawElementsInstanced(primitiveType, count, indexType, indices, instanceCount));
    }

    void OpenGLRenderDevice::DispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
        GL_CHECK(glDispatchCompute(groupCountX, groupCountY, groupCountZ));
    }

    void OpenGLRenderDevice::SetMemoryBarrier(uint32_t barrier) {
        GL_CHECK(glMemoryBarrier(barrier));
    }

    Result<uint32_t> OpenGLRenderDevice::CreateQuery(uint32_t queryType) {
        uint32_t queryId;
        GL_CHECK(glGenQueries(1, &queryId));
        return Result<uint32_t>::Success(queryId);
    }

    void OpenGLRenderDevice::DeleteQuery(uint32_t queryId) {
        GL_CHECK(glDeleteQueries(1, &queryId));
    }

    void OpenGLRenderDevice::BeginQuery(uint32_t queryId) {
        GL_CHECK(glBeginQuery(GL_TIME_ELAPSED, queryId));
    }

    void OpenGLRenderDevice::EndQuery(uint32_t queryId) {
        GL_CHECK(glEndQuery(GL_TIME_ELAPSED));
    }

    bool OpenGLRenderDevice::GetQueryResult(uint32_t queryId, uint64_t& result) {
        GLint available = 0;
        glGetQueryObjectiv(queryId, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            glGetQueryObjectui64v(queryId, GL_QUERY_RESULT, &result);
            return true;
        }
        return false;
    }

    void OpenGLRenderDevice::Finish() {
        GL_CHECK(glFinish());
    }

    void OpenGLRenderDevice::Flush() {
        GL_CHECK(glFlush());
    }

    void OpenGLRenderDevice::SetDebugLabel(uint32_t object, uint32_t type, const std::string& label) {
        #ifdef GL_VERSION_4_3
        if (glObjectLabel) {
            glObjectLabel(type, object, static_cast<GLsizei>(label.length()), label.c_str());
        }
        #endif
    }

    void OpenGLRenderDevice::PushDebugGroup(const std::string& message) {
        #ifdef GL_VERSION_4_3
        if (glPushDebugGroup) {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(message.length()), message.c_str());
        }
        #endif
    }

    void OpenGLRenderDevice::PopDebugGroup() {
        #ifdef GL_VERSION_4_3
        if (glPopDebugGroup) {
            glPopDebugGroup();
        }
        #endif
    }

    // 私有方法实现
    Result<uint32_t> OpenGLRenderDevice::CompileShader(uint32_t type, const std::string& source) {
        uint32_t shader = glCreateShader(type);
        const char* src = source.c_str();
        GL_CHECK(glShaderSource(shader, 1, &src, nullptr));
        GL_CHECK(glCompileShader(shader));

        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            std::string infoLog = GetShaderInfoLog(shader);
            glDeleteShader(shader);
            return Result<uint32_t>::Failure(Error(ErrorType::ShaderCompilation, "Shader compilation failed: " + infoLog));
        }

        return Result<uint32_t>::Success(shader);
    }

    Result<uint32_t> OpenGLRenderDevice::LinkProgram(uint32_t vertexShader, uint32_t fragmentShader, uint32_t geometryShader) {
        uint32_t program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        if (geometryShader != 0) {
            glAttachShader(program, geometryShader);
        }
        glLinkProgram(program);

        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            std::string infoLog = GetProgramInfoLog(program);
            glDeleteProgram(program);
            return Result<uint32_t>::Failure(Error(ErrorType::ShaderCompilation, "Program link failed: " + infoLog));
        }

        return Result<uint32_t>::Success(program);
    }

    int OpenGLRenderDevice::GetUniformLocation(uint32_t shaderId, const std::string& name) {
        UseShader(shaderId);
        return glGetUniformLocation(shaderId, name.c_str());
    }

    void OpenGLRenderDevice::CheckGLError(const std::string& operation) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::stringstream ss;
            ss << "OpenGL error in " << operation << ": " << error;
            LOG_ERROR("OpenGL", ss.str());
        }
    }

    std::string OpenGLRenderDevice::GetShaderInfoLog(uint32_t shader) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        
        std::string infoLog(length, '\0');
        glGetShaderInfoLog(shader, length, nullptr, &infoLog[0]);
        
        return infoLog;
    }

    std::string OpenGLRenderDevice::GetProgramInfoLog(uint32_t program) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        
        std::string infoLog(length, '\0');
        glGetProgramInfoLog(program, length, nullptr, &infoLog[0]);
        
        return infoLog;
    }

} // namespace HybridPBR