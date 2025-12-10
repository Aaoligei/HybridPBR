#include "GL_CommandList.h"
#include "GL_Device.h"
#include "utils/Logger.h"
#include <glm/glm.hpp>
#include "../../resources/Mesh.h"

namespace HybridPBR {

    OpenGLCommandList::OpenGLCommandList(OpenGLDevice* device)
        : m_device(device) {
    }

    // --- 生命周期 ---
    // 在即时模式下，Begin/End 可能不需要做什么，
    // 但如果以后做延迟提交，Begin() 会重置命令缓冲区。
    void OpenGLCommandList::Begin() {
        // 1. 绑定我们配置好的全局 VAO
        // 这确保了 glVertexAttribFormat 等格式设置生效
        GLuint globalVAO = m_device->GetGlobalVAO();
        if (globalVAO != 0) {
            glBindVertexArray(globalVAO);
        }

        // 2. 可以在这里重置其他状态，防止 ImGui 污染
        glDisable(GL_SCISSOR_TEST); 
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

    void OpenGLCommandList::End() {
        // 确保所有命令执行完毕
        // GL_CHECK(glFlush()); // 通常交给 Device::Present 处理
    }

    // --- 状态设置 ---
    void OpenGLCommandList::SetViewport(const Rect2D& rect) {
        // OpenGL 的 Viewport 原点在左下角，而 Vulkan/DX 通常在左上角
        // 这里我们假设上层传入的是适配 OpenGL 的坐标，或者在这里做翻转
        glViewport(rect.x, rect.y, rect.width, rect.height);
    }

    void OpenGLCommandList::SetScissor(const Rect2D& rect) {
        glEnable(GL_SCISSOR_TEST);
        // 同样注意 Y 轴方向
        glScissor(rect.x, rect.y, rect.width, rect.height);
    }

    void OpenGLCommandList::SetPipelineState(PipelineHandle pipeline) {
        // 从 Device 获取真实的 OpenGL Program ID
        GLuint programID = m_device->GetGLProgramID(pipeline);
        
        if (programID != 0) {
            glUseProgram(programID);
            
            // TODO: 未来在这里应用 BlendState, DepthState, RasterizerState
            // 例如:
            // if (pipeline.blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        }
    }

    // --- 资源绑定 ---
    void OpenGLCommandList::BindVertexBuffer(BufferHandle buffer, uint32_t binding, uint64_t offset) {
        GLuint glBuf = m_device->GetGLBufferID(buffer);
        if (glBuf == 0) return;

        // 修正：传入正确的 stride (sizeof(Vertex))
        // 这里的 binding 必须对应我们在 Device::Initialize 里设置的 glVertexAttribBinding 的索引
        // 我们那里设置的全是 binding point 0
        GLsizei stride = sizeof(Vertex); 
        
        glBindVertexBuffer(binding, glBuf, offset, stride);
    }

    void OpenGLCommandList::BindIndexBuffer(BufferHandle buffer, uint64_t offset) {
        GLuint glBuf = m_device->GetGLBufferID(buffer);
        if (glBuf == 0) return;

        // 将 Buffer 绑定到 Element Array Buffer 目标
        // 注意：这会改变当前绑定的 VAO 状态！
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glBuf);
    }

    // 绑定 Uniform Buffer (UBO) 或 Shader Storage Buffer (SSBO)
    void OpenGLCommandList::BindConstants(const void* data, uint32_t size, uint32_t slot) {
        // 对于 OpenGL，"BindConstants" 通常指 Push Constants (Vulkan) 或 Uniform 更新
        // 如果我们把它映射为 glUniformxx，需要知道 Shader 的反射信息。
        // 如果映射为 UBO，我们需要一个临时的动态 UBO。
        
        // 这是一个比较大的架构差异点。
        // 简化方案：假设 data 是指向一个 BufferHandle 的指针 (Hack)，或者我们暂不支持 PushConstants
        // 暂时留空，等待 Shader 系统重构。
    }

    void OpenGLCommandList::BindTexture(uint32_t slot, TextureHandle texture) {
        GLuint glTex = m_device->GetGLTextureID(texture);
        if (glTex == 0) return;

        // 绑定纹理单元
        glBindTextureUnit(slot, glTex);
    }

    // --- 绘制命令 ---
    void OpenGLCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        if (instanceCount > 1) {
            glDrawArraysInstancedBaseInstance(GL_TRIANGLES, firstVertex, vertexCount, instanceCount, firstInstance);
        } else {
            glDrawArrays(GL_TRIANGLES, firstVertex, vertexCount);
        }
    }

    void OpenGLCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        // 假设索引类型总是 unsigned int，偏移量计算需要注意
        const void* indices = (const void*)(uintptr_t)(firstIndex * sizeof(uint32_t));

        if (instanceCount > 1) {
            glDrawElementsInstancedBaseVertexBaseInstance(
                GL_TRIANGLES, 
                indexCount, 
                GL_UNSIGNED_INT, 
                indices, 
                instanceCount, 
                vertexOffset, 
                firstInstance
            );
        } else {
            glDrawElementsBaseVertex(
                GL_TRIANGLES, 
                indexCount, 
                GL_UNSIGNED_INT, 
                indices, 
                vertexOffset
            );
        }
    }

    // 绑定 UBO
    void OpenGLCommandList::BindUniformBuffer(uint32_t slot, BufferHandle buffer, uint64_t offset, uint64_t size) {
        GLuint glBuf = m_device->GetGLBufferID(buffer);
        if (glBuf == 0) return;

        if (size > 0) {
            glBindBufferRange(GL_UNIFORM_BUFFER, slot, glBuf, offset, size);
        } else {
            glBindBufferBase(GL_UNIFORM_BUFFER, slot, glBuf);
        }
    }

    // 模拟 Push Constants (使用 glUniform)
    void OpenGLCommandList::BindPushConstants(PipelineHandle pipeline, uint32_t offset, uint32_t size, const void* data) {
        // 获取当前绑定的 Program
        GLuint program = m_device->GetGLProgramID(pipeline);
        if (program == 0) return;

        // 这是一个 Hack：为了简化，我们假设 PushConstants 的前 64 字节总是 Model Matrix (mat4)
        // 在正式引擎中，你需要由 Shader 反射信息来知道 Uniform 的位置
        if (size == sizeof(glm::mat4)) {
            // 假设 Model 矩阵的 uniform 名字叫 "model"
            // 优化：应该在 Shader 创建时缓存 location
            GLint loc = glGetUniformLocation(program, "model"); 
            if (loc != -1) {
                glProgramUniformMatrix4fv(program, loc, 1, GL_FALSE, (const GLfloat*)data);
            }
            GLint locNormal = glGetUniformLocation(program, "normalMatrix");
            if (locNormal != -1) {
                glm::mat4 modelMatrix = *(const glm::mat4*)data;
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
                glProgramUniformMatrix3fv(program, locNormal, 1, GL_FALSE, (const GLfloat*)&normalMatrix);
            }
        }
    }

    void OpenGLCommandList::Clear(bool color, bool depth, const glm::vec4& colorValue, float depthValue) {
    GLbitfield flags = 0;
    
    if (color) {
        flags |= GL_COLOR_BUFFER_BIT;
        glClearColor(colorValue.r, colorValue.g, colorValue.b, colorValue.a);
    }
    
    if (depth) {
        flags |= GL_DEPTH_BUFFER_BIT;
        glClearDepth(depthValue);
        // 确保深度写入是开启的，否则清除无效
        glDepthMask(GL_TRUE);
    }
    
    if (flags != 0) {
        glClear(flags);
    }
}
} // namespace HybridPBR