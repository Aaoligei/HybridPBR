#include "Material.h"
#include <cstring>
#include "utils/Logger.h"

namespace HybridPBR {
Material::Material(const std::string& name) : m_name(name) {
        // 初始化默认值
        // 显式清零内存，防止垃圾数据
        std::memset(&m_constants, 0, sizeof(MaterialConstants));
        
        m_constants.albedoFactor = glm::vec4(1.0f);
        m_constants.metallicFactor = 0.0f;
        m_constants.roughnessFactor = 0.5f;
        m_constants.aoFactor = 1.0f;
        m_constants.normalScale = 1.0f;
        m_constants.emissiveFactor = glm::vec3(0.0f);
        m_constants.emissiveIntensity = 1.0f;
    }

    Material::~Material() {
        if (m_device && m_constantBuffer.IsValid()) {
            m_device->DestroyBuffer(m_constantBuffer);
        }
    }

    void Material::Initialize(RHI_Device* device) {
        m_device = device;

        BufferDesc desc;
        desc.name = m_name + "_UBO";
        desc.size = sizeof(MaterialConstants);
        desc.usage = (uint32_t)BufferUsageBits::UniformBuffer;
        desc.isDynamic = true;

        m_constantBuffer = m_device->CreateBuffer(desc, &m_constants);
        m_dirty = false;
        
        // [调试] 打印结构体大小，确保是 128
        // LOG_INFO("Material UBO Size: " + std::to_string(sizeof(MaterialConstants)));
    }

    void Material::UpdateToGPU() {
        if (!m_device || !m_constantBuffer.IsValid()) return;
        
        // 强制更新（调试期间暂时忽略 dirty 标记，确保数据总是最新的）
        // if (!m_dirty) return;

        m_device->UpdateBuffer(m_constantBuffer, &m_constants, sizeof(MaterialConstants));
        m_dirty = false;
    }

    void Material::Bind(RHI_CommandList* cmdList) {
        if (!cmdList) return;

        if (m_constantBuffer.IsValid()) {
            // [调试] 每一帧都更新，防止初始化顺序问题
            if (m_dirty) UpdateToGPU(); 
            
            // 绑定到 Slot 2
            cmdList->BindUniformBuffer(MATERIAL_UBO_SLOT, m_constantBuffer, 0, sizeof(MaterialConstants));
        }
    }

    void Material::SetAlbedoColor(const glm::vec4& color) {
        m_constants.albedoFactor = color;
        m_dirty = true;
    }
    void Material::SetMetallic(float val) {
        m_constants.metallicFactor = val;
        m_dirty = true;
    }
    void Material::SetRoughness(float val) {
        m_constants.roughnessFactor = val;
        m_dirty = true;
    }
    void Material::SetAO(float val) {
        m_constants.aoFactor = val;
        m_dirty = true;
    }
    void Material::SetEmissiveColor(const glm::vec3& color) {
        m_constants.emissiveFactor = color;
        m_dirty = true;
    }
    
    void Material::SetTexture(MaterialTextureSlot slot, TextureHandle handle) {
        // 1. 保存 Handle 引用 (用于资源计数等，虽然 Bindless 不强引用，但为了生命周期管理最好存着)
        m_textures[static_cast<int>(slot)] = handle;

        // 2. 获取 GPU Bindless Handle
        uint64_t bindlessHandle = 0;
        if (handle.IsValid() && m_device) {
            bindlessHandle = m_device->GetTextureBindlessHandle(handle);
        }

// 更新 UBO 数据
        // 注意：这里将 bool 转换为了 int (0 或 1)
        switch(slot) {
            case MaterialTextureSlot::Albedo: 
                m_constants.albedoMap = bindlessHandle; 
                m_constants.useAlbedoMap = (bindlessHandle != 0 ? 1 : 0);
                LOG_WARNING("Material","Albedo Texture Bindless Handle: " + std::to_string(m_constants.useAlbedoMap));
                break;
            case MaterialTextureSlot::Normal:
                m_constants.normalMap = bindlessHandle;
                m_constants.useNormalMap = (bindlessHandle != 0 ? 1 : 0);
                break;
            case MaterialTextureSlot::Metallic:
                m_constants.metallicMap = bindlessHandle;
                m_constants.useMetallicMap = (bindlessHandle != 0 ? 1 : 0);
                break;
            case MaterialTextureSlot::Roughness:
                m_constants.roughnessMap = bindlessHandle;
                m_constants.useRoughnessMap = (bindlessHandle != 0 ? 1 : 0);
                break;
            case MaterialTextureSlot::AO:
                m_constants.aoMap = bindlessHandle;
                m_constants.useAOMap = (bindlessHandle != 0 ? 1 : 0);
                break;
            case MaterialTextureSlot::Emissive:
                m_constants.emissiveMap = bindlessHandle;
                m_constants.useEmissiveMap = (bindlessHandle != 0 ? 1 : 0);
                break;
        }
        m_dirty = true;
    }

    TextureHandle Material::GetTexture(MaterialTextureSlot slot) const {
        return m_textures[static_cast<int>(slot)];
    }


} // namespace HybridPBR