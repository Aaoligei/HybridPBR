#pragma once
#include <glad/glad.h>
#include <vector>
#include <cstdint>

namespace HybridPBR {

    enum class BufferUsage {
        StaticDraw = GL_STATIC_DRAW,
        DynamicDraw = GL_DYNAMIC_DRAW,
        StreamDraw = GL_STREAM_DRAW
    };

    class ComputeBuffer {
    public:
        ComputeBuffer();
        ~ComputeBuffer();
        
        template<typename T>
        bool Create(const std::vector<T>& data, BufferUsage usage = BufferUsage::StaticDraw);
        
        template<typename T>
        bool Create(size_t elementCount, BufferUsage usage = BufferUsage::DynamicDraw);
        
        template<typename T>
        bool Update(const std::vector<T>& data, size_t offset = 0);
        
        void Destroy();
        
        // 绑定到着色器存储块
        void Bind(uint32_t bindingPoint);
        void Unbind();
        
        // 获取信息
        uint32_t GetID() const { return bufferID; }
        size_t GetSize() const { return size; }
        bool IsValid() const { return bufferID != 0; }

    private:
        uint32_t bufferID = 0;
        size_t size = 0;
        BufferUsage usage;
    };

    // 模板方法实现
    template<typename T>
    bool ComputeBuffer::Create(const std::vector<T>& data, BufferUsage bufferUsage) {
        if (data.empty()) return false;
        
        usage = bufferUsage;
        size = data.size() * sizeof(T);
        
        if (bufferID == 0) {
            glCreateBuffers(1, &bufferID);
        }
        
        glNamedBufferData(bufferID, size, data.data(), static_cast<GLenum>(usage));
        
        return true;
    }
    
    template<typename T>
    bool ComputeBuffer::Create(size_t elementCount, BufferUsage bufferUsage) {
        if (elementCount == 0) return false;
        
        usage = bufferUsage;
        size = elementCount * sizeof(T);
        
        if (bufferID == 0) {
            glCreateBuffers(1, &bufferID);
        }
        
        glNamedBufferData(bufferID, size, nullptr, static_cast<GLenum>(usage));
        
        return true;
    }
    
    template<typename T>
    bool ComputeBuffer::Update(const std::vector<T>& data, size_t offset) {
        if (bufferID == 0 || data.empty()) return false;
        
        size_t updateSize = data.size() * sizeof(T);
        if (offset + updateSize > size) return false;
        
        glNamedBufferSubData(bufferID, offset, updateSize, data.data());
        
        return true;
    }

} // namespace HybridPBR