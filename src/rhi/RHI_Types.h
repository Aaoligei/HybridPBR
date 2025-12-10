#pragma once
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace HybridPBR {

    // 资源类型枚举
    enum class ResourceType {
        Buffer,
        Texture,
        Shader,
        Pipeline,
        Sampler
    };

struct alignas(16) MaterialConstants {
        // --- 0 - 16 bytes ---
        glm::vec4 albedoFactor;      
        
        // --- 16 - 32 bytes ---
        // vec3 在 std140 占用 12 字节，紧跟一个 float 填满 16 字节
        glm::vec3 emissiveFactor;    
        float emissiveIntensity;     
        
        // --- 32 - 48 bytes ---
        float metallicFactor;        
        float roughnessFactor;       
        float aoFactor;              
        float normalScale;           
        
        // --- 48 - 96 bytes (Bindless Handles) ---
        // uint64_t 是 8 字节，这里有 6 个，共 48 字节
        // 48 (offset) + 48 (size) = 96 bytes. 对齐没问题 (8字节对齐)
        uint64_t albedoMap;          
        uint64_t normalMap;          
        uint64_t metallicMap;        
        uint64_t roughnessMap;       
        uint64_t aoMap;              
        uint64_t emissiveMap;        
        
        // --- 96 - 128 bytes (Flags) ---
        // [重点] GLSL bool 是 4 字节。必须用 int (int32_t)
        // 6 个 int = 24 字节
        int useAlbedoMap;     
        int useNormalMap;     
        int useMetallicMap;   
        int useRoughnessMap;  
        int useAOMap;         
        int useEmissiveMap;   
        
        // [重点] 补齐到 16 字节的倍数 (std140 块大小最好是 16 的倍数)
        // 目前用到 96 + 24 = 120 bytes
        // 120 不是 16 的倍数 (128 是)。差 8 字节。
        float padding0; // 4 bytes
        float padding1; // 4 bytes
        // 现在总大小 = 128 bytes
    };

    enum class MaterialTextureSlot {
        Albedo = 0,
        Normal = 1,
        Metallic = 2,
        Roughness = 3,
        AO = 4,
        Emissive = 5
    };
        // 必须与 Shader 中的 layout(std140) uniform CameraData 严格一致
    struct CameraBlock {
        glm::mat4 view;       // 64 bytes
        glm::mat4 projection; // 64 bytes
        glm::vec3 viewPos;    // 12 bytes
        float padding;        // 4 bytes (补齐 16 字节对齐)
    };

    struct GPULight {
        glm::vec3 position;  float pad0;
        glm::vec3 direction; float pad1;
        glm::vec3 color;     float intensity;
        float range;
        float constant;
        float linear;
        float quadratic;
        float innerCutoff;
        float outerCutoff;
        int type;
        float pad2, pad3, pad4; // 补齐到 16 字节边界
    };

    struct LightBlock {
        int lightCount;
        int pad0, pad1, pad2; // 补齐
        GPULight lights[16];
    };

    constexpr uint32_t CAMERA_UBO_SLOT = 0;
    constexpr uint32_t LIGHT_UBO_SLOT = 1; 
    constexpr uint32_t MATERIAL_UBO_SLOT = 2;


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
        RGBA8_SRGB
        // ... 其他格式
    };

    // [新增] 采样器配置结构
    enum class SamplerFilter { Nearest, Linear };
    enum class SamplerAddressMode { Repeat, ClampToEdge, MirroredRepeat };

    struct SamplerDesc {
        SamplerFilter minFilter = SamplerFilter::Linear;
        SamplerFilter magFilter = SamplerFilter::Linear;
        SamplerAddressMode addressU = SamplerAddressMode::Repeat;
        SamplerAddressMode addressV = SamplerAddressMode::Repeat;
        SamplerAddressMode addressW = SamplerAddressMode::Repeat;
        bool useMipmaps = true;
    };

    // 简单的矩形结构，用于 Viewport 和 Scissor
    struct Rect2D {
        int x = 0; int y = 0;
        uint32_t width = 0; uint32_t height = 0;
    };

} // namespace HybridPBR