#include "PBRMaterial.h"

namespace HybridPBR {

    PBRMaterial::PBRMaterial(const std::string& name, const MaterialProperties& properties)
        : Material(name, properties) {
        ValidatePBRParameters();
    }

    void PBRMaterial::SetAlbedo(const glm::vec3& albedo) {
        properties.albedo = glm::vec4(albedo, properties.albedo.a);
        ValidatePBRParameters();
    }

    void PBRMaterial::SetMetallic(float metallic) {
        properties.metallic = glm::clamp(metallic, 0.0f, 1.0f);
    }

    void PBRMaterial::SetRoughness(float roughness) {
        properties.roughness = glm::clamp(roughness, 0.0f, 1.0f);
    }

    void PBRMaterial::SetAO(float ao) {
        properties.ambientOcclusion = glm::clamp(ao, 0.0f, 1.0f);
    }

    void PBRMaterial::SetEmissive(const glm::vec3& emissive) {
        properties.emissiveColor = emissive;
    }

    void PBRMaterial::SetAlbedoMap(std::shared_ptr<Texture> texture) {
        SetTexture(TextureType::DIFFUSE, texture);
    }

    void PBRMaterial::SetNormalMap(std::shared_ptr<Texture> texture) {
        SetTexture(TextureType::NORMAL, texture);
    }

    void PBRMaterial::SetMetallicMap(std::shared_ptr<Texture> texture) {
        SetTexture(TextureType::METALLIC, texture);
    }

    void PBRMaterial::SetRoughnessMap(std::shared_ptr<Texture> texture) {
        SetTexture(TextureType::ROUGHNESS, texture);
    }

    void PBRMaterial::SetAOMap(std::shared_ptr<Texture> texture) {
        SetTexture(TextureType::AMBIENT_OCCLUSION, texture);
    }

    void PBRMaterial::SetEmissiveMap(std::shared_ptr<Texture> texture) {
        SetTexture(TextureType::EMISSIVE, texture);
    }

    void PBRMaterial::ApplyToShader(std::shared_ptr<Shader> shader) const {
        if (!shader) return;
        
        shader->Use();
        
        // 设置PBR材质属性
        shader->SetVec3("material.albedo", glm::vec3(properties.albedo));
        shader->SetFloat("material.metallic", properties.metallic);
        shader->SetFloat("material.roughness", properties.roughness);
        shader->SetFloat("material.ao", properties.ambientOcclusion);
        shader->SetFloat("material.normalScale", properties.normalScale);
        shader->SetVec3("material.emissive", properties.emissiveColor);
        shader->SetFloat("material.emissiveIntensity", properties.emissiveIntensity);
        
        // 绑定PBR纹理
        uint32_t textureUnit = 0;
        
        if (auto albedoMap = GetTexture(TextureType::DIFFUSE)) {
            albedoMap->Bind(textureUnit);
            shader->SetInt("material.albedoMap", textureUnit);
            textureUnit++;
        }
        
        if (auto normalMap = GetTexture(TextureType::NORMAL)) {
            normalMap->Bind(textureUnit);
            shader->SetInt("material.normalMap", textureUnit);
            textureUnit++;
        }
        
        if (auto metallicMap = GetTexture(TextureType::METALLIC)) {
            metallicMap->Bind(textureUnit);
            shader->SetInt("material.metallicMap", textureUnit);
            textureUnit++;
        }
        
        if (auto roughnessMap = GetTexture(TextureType::ROUGHNESS)) {
            roughnessMap->Bind(textureUnit);
            shader->SetInt("material.roughnessMap", textureUnit);
            textureUnit++;
        }
        
        if (auto aoMap = GetTexture(TextureType::AMBIENT_OCCLUSION)) {
            aoMap->Bind(textureUnit);
            shader->SetInt("material.aoMap", textureUnit);
            textureUnit++;
        }
        
        if (auto emissiveMap = GetTexture(TextureType::EMISSIVE)) {
            emissiveMap->Bind(textureUnit);
            shader->SetInt("material.emissiveMap", textureUnit);
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

    bool PBRMaterial::IsPBRComplete() const {
        // 检查是否有完整的PBR纹理集
        return GetTexture(TextureType::DIFFUSE) != nullptr &&
               GetTexture(TextureType::NORMAL) != nullptr &&
               GetTexture(TextureType::METALLIC) != nullptr &&
               GetTexture(TextureType::ROUGHNESS) != nullptr &&
               GetTexture(TextureType::AMBIENT_OCCLUSION) != nullptr;
    }

    void PBRMaterial::ValidatePBRParameters() {
        // 确保PBR参数在有效范围内
        properties.metallic = glm::clamp(properties.metallic, 0.0f, 1.0f);
        properties.roughness = glm::clamp(properties.roughness, 0.0f, 1.0f);
        properties.ambientOcclusion = glm::clamp(properties.ambientOcclusion, 0.0f, 1.0f);
        properties.normalScale = glm::clamp(properties.normalScale, 0.0f, 2.0f);
        properties.emissiveIntensity = glm::max(properties.emissiveIntensity, 0.0f);
    }

} // namespace HybridPBR