#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include "Texture.h"
#include "rendering/Shader.h"

namespace HybridPBR {

    struct MaterialProperties {
        // 基础颜色
        glm::vec4 albedo = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        
        // PBR参数
        float metallic = 0.0f;
        float roughness = 0.5f;
        float ambientOcclusion = 1.0f;
        
        // 其他参数
        float normalScale = 1.0f;
        float emissiveIntensity = 0.0f;
        glm::vec3 emissiveColor = glm::vec3(0.0f);
        
        // 纹理缩放
        glm::vec2 textureScale = glm::vec2(1.0f);
        glm::vec2 textureOffset = glm::vec2(0.0f);
    };

    class Material {
    public:
        Material(const std::string& name, const MaterialProperties& properties = MaterialProperties());
        
        // 纹理设置
        void SetTexture(TextureType type, std::shared_ptr<Texture> texture);
        void RemoveTexture(TextureType type);
        std::shared_ptr<Texture> GetTexture(TextureType type) const;
        
        // 属性设置
        void SetProperties(const MaterialProperties& props) { properties = props; }
        const MaterialProperties& GetProperties() const { return properties; }
        MaterialProperties& GetProperties() { return properties; }
        
        // 名称管理
        const std::string& GetName() const { return name; }
        void SetName(const std::string& newName) { name = newName; }
        
        // 着色器参数应用
        void ApplyToShader(std::shared_ptr<Shader> shader) const;
        
        // 验证材质是否完整
        bool HasRequiredTextures() const;

    private:
        std::string name;
        MaterialProperties properties;
        std::unordered_map<TextureType, std::shared_ptr<Texture>> textures;
    };

} // namespace HybridPBR