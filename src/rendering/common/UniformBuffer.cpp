#include "UniformBuffer.h"

namespace HybridPBR {
    UniformBuffer::UniformBuffer(size_t size, uint32_t bindingPoint) 
        : bindingPoint(bindingPoint) {
        glCreateBuffers(1, &uboID);
        glNamedBufferStorage(uboID, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, uboID);
    }

    UniformBuffer::~UniformBuffer() {
        glDeleteBuffers(1, &uboID);
    }

    void UniformBuffer::SetData(const void* data, size_t size, size_t offset) {
        glNamedBufferSubData(uboID, offset, size, data);
    }
    
    void UniformBuffer::Bind() const {
        // 通常不需要每帧Bind，因为BindRange已经在初始化做过了，
        // 但如果需要切换绑定点可以写在这里
    }
}