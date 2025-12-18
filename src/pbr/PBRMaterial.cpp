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
        
        // 设置PBR材质属性（如果没有贴图就使用这些属性）
        shader->SetVec3("albedo", glm::vec3(properties.albedo));
        shader->SetFloat("metallic", properties.metallic);
        shader->SetFloat("roughness", properties.roughness);
        shader->SetFloat("ao", properties.ambientOcclusion);
        shader->SetFloat("material.normalScale", properties.normalScale);
        // shader->SetVec3("material.emissive", properties.emissiveColor);
        // shader->SetFloat("material.emissiveIntensity", properties.emissiveIntensity);
        
        // ... 设置材质基本属性 (albedo, roughness 等 float/vec3) ...

        // 纹理单元分配策略：
        // 0-4: PBR 基础纹理
        // 5-9: 特殊纹理 (IBL 等)
        // 10+: 阴影贴图等
        
        // 注意：此函数会占用slot 0-5，因此IBL纹理必须在之后重新绑定
        uint32_t slot = 0;
        
        // 辅助 Lambda
        auto bindTex = [&](TextureType type, const std::string& name, int explicitSlot = -1) {
            auto tex = GetTexture(type);
            bool hasTex = (tex != nullptr);
            shader->SetBool("material.use" + name + "Map", hasTex); // 统一命名规范
            
            if (hasTex) {
                uint32_t useSlot = (explicitSlot != -1) ? explicitSlot : slot++;
                tex->Bind(useSlot);
                shader->SetInt( name + "Map", useSlot); 
            }
        };

        bindTex(TextureType::DIFFUSE, "Albedo", 0);
        bindTex(TextureType::NORMAL, "Normal", 1);
        bindTex(TextureType::METALLIC, "Metallic", 2);
        bindTex(TextureType::ROUGHNESS, "Roughness", 3);
        bindTex(TextureType::AMBIENT_OCCLUSION, "AO", 4);
        bindTex(TextureType::EMISSIVE, "Emissive", 5);
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