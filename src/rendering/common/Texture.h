#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../../rhi/RHI_Device.h"
#include "../../rhi/RHI_Types.h"

namespace HybridPBR {

    // 保持旧的枚举定义，或者逐步替换为 RHI_Types 中的定义
    // 这里为了兼容现有代码，我们暂时保留 TextureType
    enum class TextureType {
        DIFFUSE,
        SPECULAR,
        NORMAL,
        HEIGHT,
        ROUGHNESS,
        METALLIC,
        AMBIENT_OCCLUSION,
        EMISSIVE,
        HDR,
        CUBEMAP
    };

    class Texture {
    public:
        Texture() = default;
        ~Texture();

        // 禁止拷贝，允许移动
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        // --- 创建/加载接口 ---
        
        // 创建空纹理
        bool Create2D(RHI_Device* device, int width, int height, 
                     TextureFormat format = TextureFormat::RGBA8_UNORM, 
                     const void* data = nullptr);

        // 从文件加载 (自动处理 sRGB)
        bool LoadFromFile(RHI_Device* device, const std::string& filepath, TextureType type = TextureType::DIFFUSE);
        
        // 加载 HDR
        bool LoadHDR(RHI_Device* device, const std::string& filepath);

        // 创建 Cubemap
        bool CreateCubemap(RHI_Device* device, int size, TextureFormat format = TextureFormat::RGBA16_FLOAT);
        // bool LoadCubemap(...) // 暂时略过，逻辑类似
        // --- [新增] 静态图片加载包装器 (解决符号冲突) ---
        // 使用内存加载，配合 FileIO，这是最健壮的方式
        static unsigned char* LoadImageFromMemory(const void* data, int len, int* width, int* height, int* channels, int desired_channels);
        static float* LoadImageFloatFromMemory(const void* data, int len, int* width, int* height, int* channels, int desired_channels);
        static void FreeImage(void* data);

        // --- 状态设置 ---
        // 注意：必须在获取 BindlessHandle 之前调用这些设置
        void SetSamplerState(const SamplerDesc& desc);
        void GenerateMipmaps();

        // --- 获取器 ---
        TextureHandle GetHandle() const { return m_handle; }
        
        // 获取 Bindless Handle (核心功能)
        // 第一次调用时会请求 Device 生成 Handle 并设为 Resident
        uint64_t GetBindlessHandle();

        // 基础信息
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        TextureType GetType() const { return m_type; }
        const std::string& GetFilePath() const { return m_filePath; }

    private:
        RHI_Device* m_device = nullptr;
        TextureHandle m_handle = TextureHandle::Invalid();
        uint64_t m_bindlessHandle = 0; // 缓存的 Bindless Handle

        int m_width = 0;
        int m_height = 0;
        TextureType m_type = TextureType::DIFFUSE;
        std::string m_filePath;
        bool m_isCubemap = false;

        // 辅助清理函数
        void Cleanup();
    };

} // namespace HybridPBR