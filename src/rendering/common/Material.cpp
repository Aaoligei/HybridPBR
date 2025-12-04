#include "Material.h"

namespace HybridPBR {

    Material::Material(const std::string& materialName, const MaterialProperties& props)
        : name(materialName), properties(props) {
    }

    std::shared_ptr<Shader> Material::GetShader() const {
        auto& shaderManager = ShaderManager::GetInstance();
        
        if (properties.customShaderName!="null") {
            // 使用自定义着色器
            return shaderManager.GetShader(properties.customShaderName);
        } else {
            // 使用预定义着色器类型
            return shaderManager.GetShader(properties.shaderType);
        }
    }

    void Material::SetTexture(TextureType type, std::shared_ptr<Texture> texture) {
        textures[type] = texture;
    }

    void Material::RemoveTexture(TextureType type) {
        auto it = textures.find(type);
        if (it != textures.end()) {
            textures.erase(it);
        }
    }

    std::shared_ptr<Texture> Material::GetTexture(TextureType type) const {
        auto it = textures.find(type);
        return it != textures.end() ? it->second : nullptr;
    }

    void Material::ApplyToShader(std::shared_ptr<Shader> shader) const {
        if (!shader) return;
        shader->Use();

        shader->SetVec3("albedo", glm::vec3(properties.albedo));
        shader->SetFloat("metallic", properties.metallic);
        shader->SetFloat("roughness", properties.roughness);
        shader->SetFloat("ao", properties.ambientOcclusion);
        shader->SetFloat("material.normalScale", properties.normalScale);
        shader->SetVec3("material.emissive", properties.emissiveColor);
        shader->SetFloat("material.emissiveIntensity", properties.emissiveIntensity);
        
        // ... 设置材质基本属性 (albedo, roughness 等 float/vec3) ...

        // 纹理单元分配策略：
        // 0-4: PBR 基础纹理
        // 5-9: 特殊纹理 (IBL 等)
        // 10+: 阴影贴图等
        
        uint32_t slot = 0;
        
        // 辅助 Lambda
        auto bindTex = [&](TextureType type, const std::string& name, int explicitSlot = -1) {
            auto tex = GetTexture(type);
            bool hasTex = (tex != nullptr);
            shader->SetBool("material.use" + name + "Map", hasTex); // 统一命名规范
            
            if (hasTex) {
                uint32_t useSlot = (explicitSlot != -1) ? explicitSlot : slot++;
                tex->Bind(useSlot);
                shader->SetInt(name + "Map", useSlot); 
            }
        };

        bindTex(TextureType::DIFFUSE, "Albedo", 0);
        bindTex(TextureType::NORMAL, "Normal", 1);
        bindTex(TextureType::METALLIC, "Metallic", 2);
        bindTex(TextureType::ROUGHNESS, "Roughness", 3);
        bindTex(TextureType::AMBIENT_OCCLUSION, "AO", 4);
        bindTex(TextureType::EMISSIVE, "Emissive", 5);
    }

    bool Material::HasRequiredTextures() const {
        // 检查是否有基本的漫反射纹理
        return GetTexture(TextureType::DIFFUSE) != nullptr;
    }

} // namespace HybridPBR