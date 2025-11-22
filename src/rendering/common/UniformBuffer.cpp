#include "UniformBuffer.h"

namespace HybridPBR {
    UniformBuffer::UniformBuffer(size_t size, uint32_t bindingPoint) 
        : bindingPoint(bindingPoint) {
        glGenBuffers(1, &uboID);
        glBindBuffer(GL_UNIFORM_BUFFER, uboID);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STATIC_DRAW);
        glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, uboID, 0, size);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    UniformBuffer::~UniformBuffer() {
        glDeleteBuffers(1, &uboID);
    }

    void UniformBuffer::SetData(const void* data, size_t size, size_t offset) {
        glBindBuffer(GL_UNIFORM_BUFFER, uboID);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    
    void UniformBuffer::Bind() const {
        // 通常不需要每帧Bind，因为BindRange已经在初始化做过了，
        // 但如果需要切换绑定点可以写在这里
    }
}