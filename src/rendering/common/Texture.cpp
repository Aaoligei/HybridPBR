#include "Texture.h"
#include "utils/Logger.h"
#include "utils/FileIO.h" // [新增]

// [核心修复] 将 STB 符号设为静态，防止与 Assimp 冲突
#define STB_IMAGE_STATIC 
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace HybridPBR {

    // --- [新增] 静态包装器实现 ---
    unsigned char* Texture::LoadImageFromMemory(const void* data, int len, int* width, int* height, int* channels, int desired_channels) {
        // 这里调用的 stbi_load_from_memory 是我们上面定义的 static 版本
        // 绝对不会调用到 Assimp 的版本
        stbi_set_flip_vertically_on_load(true);
        return stbi_load_from_memory((const stbi_uc*)data, len, width, height, channels, desired_channels);
    }

    float* Texture::LoadImageFloatFromMemory(const void* data, int len, int* width, int* height, int* channels, int desired_channels) {
        stbi_set_flip_vertically_on_load(true);
        return stbi_loadf_from_memory((const stbi_uc*)data, len, width, height, channels, desired_channels);
    }

    void Texture::FreeImage(void* data) {
        stbi_image_free(data);
    }

    // --- 现有成员函数 ---

    Texture::~Texture() { Cleanup(); }

    Texture::Texture(Texture&& other) noexcept { *this = std::move(other); }

    Texture& Texture::operator=(Texture&& other) noexcept {
        if (this != &other) {
            Cleanup();
            m_device = other.m_device;
            m_handle = other.m_handle;
            m_bindlessHandle = other.m_bindlessHandle;
            m_width = other.m_width;
            m_height = other.m_height;
            m_type = other.m_type;
            m_filePath = std::move(other.m_filePath);
            m_isCubemap = other.m_isCubemap;

            other.m_handle = TextureHandle::Invalid();
            other.m_bindlessHandle = 0;
            other.m_device = nullptr;
        }
        return *this;
    }

    void Texture::Cleanup() {
        if (m_device && m_handle.IsValid()) {
            m_device->DestroyTexture(m_handle);
        }
        m_handle = TextureHandle::Invalid();
        m_bindlessHandle = 0;
    }

    bool Texture::Create2D(RHI_Device* device, int w, int h, TextureFormat format, const void* data) {
        if (!device) return false;
        Cleanup();
        m_device = device;
        m_width = w; m_height = h; m_isCubemap = false;

        TextureDesc desc;
        desc.width = w; desc.height = h; desc.format = format;
        desc.name = "Texture2D"; 
        m_handle = m_device->CreateTexture(desc, data);
        SamplerDesc defaultSampler;
        defaultSampler.minFilter = SamplerFilter::Linear;
        defaultSampler.magFilter = SamplerFilter::Linear;
        // ...
        SetSamplerState(defaultSampler);
        return m_handle.IsValid();
    }

    bool Texture::LoadFromFile(RHI_Device* device, const std::string& filepath, TextureType type) {
        m_filePath = filepath;
        m_type = type;

        // 改用 FileIO + 静态包装器，统一路径
        std::vector<char> fileData = FileIO::ReadBinaryFile(filepath);
        if (fileData.empty()) {
            LOG_ERROR("Texture file not found: " + filepath);
            return false;
        }

        int width, height, channels;
        unsigned char* data = Texture::LoadImageFromMemory(fileData.data(), fileData.size(), &width, &height, &channels, 4);
        
        if (!data) {
            LOG_ERROR("Failed to decode texture: " + filepath);
            return false;
        }

        TextureFormat format = (type == TextureType::DIFFUSE || type == TextureType::EMISSIVE) 
                             ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8_UNORM;

        bool success = Create2D(device, width, height, format, data);
        if (success) {
            GenerateMipmaps();
            SamplerDesc sampler;
            sampler.useMipmaps = true;
            SetSamplerState(sampler);
        }

        FreeImage(data);
        return success;
    }

    bool Texture::LoadHDR(RHI_Device* device, const std::string& filepath) {
        m_filePath = filepath;
        m_type = TextureType::HDR;

        std::vector<char> fileData = FileIO::ReadBinaryFile(filepath);
        if (fileData.empty()) return false;

        int width, height, channels;
        float* data = Texture::LoadImageFloatFromMemory(fileData.data(), fileData.size(), &width, &height, &channels, 4);

        if (!data) {
            LOG_ERROR("Failed to load HDR: " + filepath);
            return false;
        }

        bool success = Create2D(device, width, height, TextureFormat::RGBA32_FLOAT, data);
        if (success) {
            SamplerDesc sampler;
            sampler.addressU = SamplerAddressMode::ClampToEdge;
            sampler.addressV = SamplerAddressMode::ClampToEdge;
            SetSamplerState(sampler);
        }

        FreeImage(data);
        return success;
    }

    bool Texture::CreateCubemap(RHI_Device* device, int size, TextureFormat format) {
        if (!device) return false;
        Cleanup();
        m_device = device;
        m_width = size; m_height = size; m_isCubemap = true; m_type = TextureType::CUBEMAP;
        // 暂未实现 RHI CreateCubemap，留空
        return false; 
    }

    void Texture::SetSamplerState(const SamplerDesc& desc) {
        if (m_device && m_handle.IsValid()) m_device->SetTextureSampler(m_handle, desc);
    }

    void Texture::GenerateMipmaps() {
        if (m_device && m_handle.IsValid()) m_device->GenerateMipmaps(m_handle);
    }

    uint64_t Texture::GetBindlessHandle() {
        if (m_bindlessHandle != 0) return m_bindlessHandle;
        if (m_device && m_handle.IsValid()) m_bindlessHandle = m_device->GetTextureBindlessHandle(m_handle);
        return m_bindlessHandle;
    }

} // namespace HybridPBR