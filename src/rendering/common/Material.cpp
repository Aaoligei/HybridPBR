#include "Material.h"

namespace HybridPBR {

    Material::Material(const std::string& materialName, const MaterialProperties& props)
        : name(materialName), properties(props) {
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
        
        // 设置材质属性
        shader->SetVec4("material.albedo", properties.albedo);
        shader->SetFloat("material.metallic", properties.metallic);
        shader->SetFloat("material.roughness", properties.roughness);
        shader->SetFloat("material.ambientOcclusion", properties.ambientOcclusion);
        shader->SetFloat("material.normalScale", properties.normalScale);
        shader->SetVec3("material.emissiveColor", properties.emissiveColor);
        shader->SetFloat("material.emissiveIntensity", properties.emissiveIntensity);
        shader->SetVec2("material.textureScale", properties.textureScale);
        shader->SetVec2("material.textureOffset", properties.textureOffset);
        
        // 绑定纹理
        uint32_t textureUnit = 0;
        for (const auto& [type, texture] : textures) {
            if (!texture) continue;
            
            std::string uniformName;
            switch (type) {
                case TextureType::DIFFUSE:
                    uniformName = "albedoMap";
                    break;
                case TextureType::NORMAL:
                    uniformName = "normalMap";
                    break;
                case TextureType::METALLIC:
                    uniformName = "metallicMap";
                    break;
                case TextureType::ROUGHNESS:
                    uniformName = "roughnessMap";
                    break;
                case TextureType::AMBIENT_OCCLUSION:
                    uniformName = "aoMap";
                    break;
                case TextureType::EMISSIVE:
                    uniformName = "emissiveMap";
                    break;
                default:
                    continue;
            }
            
            shader->SetInt(uniformName, textureUnit);
            texture->Bind(textureUnit);
            textureUnit++;
        }
        
        // 设置纹理使用标志
        shader->SetBool("material.useAlbedoMap", GetTexture(TextureType::DIFFUSE) != nullptr);
        shader->SetBool("material.useNormalMap", GetTexture(TextureType::NORMAL) != nullptr);
        shader->SetBool("material.useMetallicMap", GetTexture(TextureType::METALLIC) != nullptr);
        shader->SetBool("material.useRoughnessMap", GetTexture(TextureType::ROUGHNESS) != nullptr);
        shader->SetBool("material.useAOMap", GetTexture(TextureType::AMBIENT_OCCLUSION) != nullptr);
        shader->SetBool("material.useEmissiveMap", GetTexture(TextureType::EMISSIVE) != nullptr);
    }

    bool Material::HasRequiredTextures() const {
        // 检查是否有基本的漫反射纹理
        return GetTexture(TextureType::DIFFUSE) != nullptr;
    }

} // namespace HybridPBR