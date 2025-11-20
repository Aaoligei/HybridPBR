#pragma once
#include "../rendering/common/Material.h"

namespace HybridPBR {

    class PBRMaterial : public Material {
    public:
        PBRMaterial(const std::string& name, const MaterialProperties& properties = MaterialProperties());
        
        // PBR特定方法
        void SetAlbedo(const glm::vec3& albedo);
        void SetMetallic(float metallic);
        void SetRoughness(float roughness);
        void SetAO(float ao);
        void SetEmissive(const glm::vec3& emissive);
        
        // 纹理设置
        void SetAlbedoMap(std::shared_ptr<Texture> texture);
        void SetNormalMap(std::shared_ptr<Texture> texture);
        void SetMetallicMap(std::shared_ptr<Texture> texture);
        void SetRoughnessMap(std::shared_ptr<Texture> texture);
        void SetAOMap(std::shared_ptr<Texture> texture);
        void SetEmissiveMap(std::shared_ptr<Texture> texture);
        
        // 应用到PBR着色器
        void ApplyToShader(std::shared_ptr<Shader> shader) const override;
        
        // 验证PBR材质完整性
        bool IsPBRComplete() const;

    private:
        // PBR特定验证
        void ValidatePBRParameters();
    };

} // namespace HybridPBR