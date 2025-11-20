#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include "Texture.h"
#include "rendering/Shader.h"
#include "rendering/ShaderManager.h"

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

        // 着色器类型
        ShaderType shaderType = ShaderType::DEFAULT;
        std::string customShaderName = "null"; // 自定义着色器名称
    };

    class Material {
    public:
        Material(const std::string& name, const MaterialProperties& properties = MaterialProperties());
        virtual ~Material() = default; // 添加虚析构函数使Material成为多态类型
        
        // 纹理设置
        void SetTexture(TextureType type, std::shared_ptr<Texture> texture);
        void RemoveTexture(TextureType type);
        std::shared_ptr<Texture> GetTexture(TextureType type) const;
        
        // 属性设置
        void SetProperties(const MaterialProperties& props) { properties = props; }
        const MaterialProperties& GetProperties() const { return properties; }
        MaterialProperties& GetProperties() { return properties; }
        
        // 着色器管理
        void SetShaderType(ShaderType type) { properties.shaderType = type; }
        void SetCustomShader(const std::string& shaderName) { properties.customShaderName = shaderName; }
        ShaderType GetShaderType() const { return properties.shaderType; }
        inline std::string GetShaderTypeString() const {
            switch (properties.shaderType) {
                case ShaderType::DEFAULT: return "DEFAULT";
                case ShaderType::PBR: return "PBR";
                case ShaderType::SKYBOX: return "SKYBOX";
                case ShaderType::UNLIT: return "UNLIT";
                case ShaderType::WIREFRAME: return "WIREFRAME";
                case ShaderType::DEPTH: return "DEPTH";
                case ShaderType::POST_PROCESS: return "POST_PROCESS";
                case ShaderType::CUSTOM: return "CUSTOM";
                default: return "UNKNOWN";
            }
        }
        std::string GetCustomShaderName() const { return properties.customShaderName; }
        
        // 获取对应的着色器
        std::shared_ptr<Shader> GetShader() const;

        // 名称管理
        const std::string& GetName() const { return name; }
        void SetName(const std::string& newName) { name = newName; }
        
        // 着色器参数应用
        virtual void ApplyToShader(std::shared_ptr<Shader> shader) const;
        
        // 验证材质是否完整
        bool HasRequiredTextures() const;

    protected:
        std::string name;
        MaterialProperties properties;
        std::unordered_map<TextureType, std::shared_ptr<Texture>> textures;
    };

} // namespace HybridPBR