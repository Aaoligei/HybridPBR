#pragma once
#include <cstdint>
#include <string>

namespace HybridPBR {

    // 资源类型枚举
    enum class ResourceType {
        Buffer,
        Texture,
        Shader,
        Pipeline,
        Sampler
    };

    // 强类型句柄模板：防止把 Texture 的 ID 传给 Buffer
    // 这是一个非常“简历加分”的设计，体现了类型安全意识
    template<typename Tag>
    struct Handle {
        uint64_t id = 0;
        bool IsValid() const { return id != 0; }
        bool operator==(const Handle& other) const { return id == other.id; }
        static Handle Invalid() { return {0}; }
    };

    // 定义具体的资源句柄
    struct BufferTag {};
    struct TextureTag {};
    struct ShaderTag {};
    struct PipelineTag {};

    using BufferHandle   = Handle<BufferTag>;
    using TextureHandle  = Handle<TextureTag>;
    using ShaderHandle   = Handle<ShaderTag>;
    using PipelineHandle = Handle<PipelineTag>;

    // 常见枚举定义 (对应 Vulkan/OpenGL 的枚举)
    enum class BufferUsageBits : uint32_t {
        VertexBuffer   = 1 << 0,
        IndexBuffer    = 1 << 1,
        UniformBuffer  = 1 << 2,
        StorageBuffer  = 1 << 3,
        TransferSrc    = 1 << 4,
        TransferDst    = 1 << 5
    };

    enum class TextureFormat {
        RGBA8_UNORM,
        RGBA16_FLOAT,
        RGBA32_FLOAT,
        D24_S8_UINT,
        // ... 其他格式
    };

    // 简单的矩形结构，用于 Viewport 和 Scissor
    struct Rect2D {
        int x = 0; int y = 0;
        uint32_t width = 0; uint32_t height = 0;
    };

} // namespace HybridPBR