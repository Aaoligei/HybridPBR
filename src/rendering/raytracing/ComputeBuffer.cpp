#include "ComputeBuffer.h"
#include "utils/Logger.h"
#include "utils/GLCheck.h"
#include <glad/glad.h>

namespace HybridPBR {

    ComputeBuffer::ComputeBuffer() : bufferID(0), size(0), usage(BufferUsage::DynamicDraw) {
    }

    ComputeBuffer::~ComputeBuffer() {
        Destroy();
    }

    void ComputeBuffer::Destroy() {
        if (bufferID != 0) {
            GLCall(glDeleteBuffers(1, &bufferID));
            bufferID = 0;
            size = 0;
        }
    }

    void ComputeBuffer::Bind(uint32_t bindingPoint) {
        if (bufferID != 0) {
            GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, bufferID));
        }
    }

    void ComputeBuffer::Unbind() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

} // namespace HybridPBR