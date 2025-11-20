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
        
        // 根据着色器类型应用不同的参数
        switch (properties.shaderType) {
            case ShaderType::PBR:
                // PBR材质参数
                shader->SetVec3("material.albedo", glm::vec3(properties.albedo));
                shader->SetFloat("material.metallic", properties.metallic);
                shader->SetFloat("material.roughness", properties.roughness);
                shader->SetFloat("material.ao", properties.ambientOcclusion);
                shader->SetFloat("material.normalScale", properties.normalScale);
                shader->SetVec3("material.emissive", properties.emissiveColor);
                shader->SetFloat("material.emissiveIntensity", properties.emissiveIntensity);
                
                // 设置纹理使用标志
                shader->SetBool("material.useAlbedoMap", GetTexture(TextureType::DIFFUSE) != nullptr);
                shader->SetBool("material.useNormalMap", GetTexture(TextureType::NORMAL) != nullptr);
                shader->SetBool("material.useMetallicMap", GetTexture(TextureType::METALLIC) != nullptr);
                shader->SetBool("material.useRoughnessMap", GetTexture(TextureType::ROUGHNESS) != nullptr);
                shader->SetBool("material.useAOMap", GetTexture(TextureType::AMBIENT_OCCLUSION) != nullptr);
                shader->SetBool("material.useEmissiveMap", GetTexture(TextureType::EMISSIVE) != nullptr);
                break;
                
            case ShaderType::UNLIT:
                // 无光照材质参数
                shader->SetVec3("color", glm::vec3(properties.albedo));
                if (GetTexture(TextureType::DIFFUSE)) {
                    shader->SetInt("albedoMap", 0);
                    GetTexture(TextureType::DIFFUSE)->Bind(0);
                }
                break;
                
            default:
                // 默认材质参数 - 使用我们在default.frag中定义的材质结构
                shader->SetVec3("material.ambient", glm::vec3(properties.albedo) * 0.1f);
                shader->SetVec3("material.diffuse", glm::vec3(properties.albedo));
                shader->SetVec3("material.specular", glm::vec3(1.0f));
                shader->SetFloat("material.shininess", 32.0f);
                shader->SetBool("material.useDiffuseMap", GetTexture(TextureType::DIFFUSE) != nullptr);
                shader->SetBool("material.useSpecularMap", GetTexture(TextureType::SPECULAR) != nullptr);
                break;
        }
        
        // 通用纹理绑定
        uint32_t textureUnit = 0;
        for (const auto& [type, texture] : textures) {
            if (!texture) continue;
            
            std::string uniformName;
            switch (type) {
                case TextureType::DIFFUSE:
                    uniformName = (properties.shaderType == ShaderType::PBR) ? "albedoMap" : "diffuseMap";
                    break;
                case TextureType::SPECULAR:
                    uniformName = "specularMap";
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
            
            texture->Bind(textureUnit);
            shader->SetInt(uniformName, textureUnit);
            textureUnit++;
        }
        
    }

    bool Material::HasRequiredTextures() const {
        // 检查是否有基本的漫反射纹理
        return GetTexture(TextureType::DIFFUSE) != nullptr;
    }

} // namespace HybridPBR