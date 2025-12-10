#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include "Texture.h"
#include "rhi/RHI_Device.h"

namespace HybridPBR {

class Material {
    public:
        Material(const std::string& name);
        virtual ~Material();

        // --- 资源初始化与更新 ---
        // 在这一步，我们需要传入 RHI_Device 来创建 UBO
        void Initialize(RHI_Device* device);
        
        // 当属性发生变化时，上传数据到 GPU
        void UpdateToGPU();

        // --- 绑定操作 ---
        // 将此材质的资源绑定到 CommandList
        void Bind(RHI_CommandList* cmdList);

        // --- 属性设置 ---
        void SetAlbedoColor(const glm::vec4& color);
        void SetMetallic(float val);
        void SetRoughness(float val);
        // ... 其他 Setters ...
        void SetAO(float val);
        void SetEmissiveColor(const glm::vec3& color);

        // --- 纹理设置 (传入 RHI Handle) ---
        void SetTexture(MaterialTextureSlot slot, TextureHandle handle);
        TextureHandle GetTexture(MaterialTextureSlot slot) const;

        // 获取名称
        const std::string& GetName() const { return m_name; }
        MaterialConstants& GetConstants() { return m_constants; }

    private:
        std::string m_name;
        RHI_Device* m_device = nullptr;

        // CPU 端数据
        MaterialConstants m_constants;
        bool m_dirty = true;

        // GPU 端资源
        BufferHandle m_constantBuffer = BufferHandle::Invalid();
        
        // 纹理句柄数组 (对应 enum MaterialTextureSlot)
        TextureHandle m_textures[6] = { TextureHandle::Invalid() };
    };

} // namespace HybridPBR