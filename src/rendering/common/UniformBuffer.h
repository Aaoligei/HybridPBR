#pragma once
#include <glad/glad.h>
#include <vector>

namespace HybridPBR {
    class UniformBuffer {
    public:
        UniformBuffer(size_t size, uint32_t bindingPoint);
        ~UniformBuffer();

        void SetData(const void* data, size_t size, size_t offset = 0);
        void Bind() const;

    private:
        uint32_t uboID;
        uint32_t bindingPoint;
    };
}