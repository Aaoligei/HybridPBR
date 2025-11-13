#pragma once
#include <glm/glm.hpp>
#include <memory>
#include<string>

namespace HybridPBR {

    enum class LightType {
        DIRECTIONAL,
        POINT,
        SPOT
    };

    struct LightProperties {
        glm::vec3 color = glm::vec3(1.0f);
        float intensity = 1.0f;
        
        // 点光源和聚光灯参数
        float range = 10.0f;
        float constant = 1.0f;
        float linear = 0.09f;
        float quadratic = 0.032f;
        
        // 聚光灯参数
        float innerCutoff = glm::cos(glm::radians(12.5f));
        float outerCutoff = glm::cos(glm::radians(17.5f));
    };

    class Light {
    public:
        Light(LightType type, const std::string& name = std::string("Light"));
        
        // 类型管理
        LightType GetType() const { return type; }
        void SetType(LightType newType) { type = newType; }
        
        // 属性管理
        void SetProperties(const LightProperties& props) { properties = props; }
        const LightProperties& GetProperties() const { return properties; }
        LightProperties& GetProperties() { return properties; }
        
        // 变换相关
        void SetPosition(const glm::vec3& position) { this->position = position; }
        void SetDirection(const glm::vec3& direction) { this->direction = glm::normalize(direction); }
        
        const glm::vec3& GetPosition() const { return position; }
        const glm::vec3& GetDirection() const { return direction; }
        
        // 名称管理
        const std::string& GetName() const { return name; }
        void SetName(const std::string& newName) { name = newName; }
        
        // 启用/禁用
        bool IsEnabled() const { return enabled; }
        void SetEnabled(bool enable) { enabled = enable; }

    private:
        LightType type;
        std::string name;
        LightProperties properties;
        
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
        
        bool enabled = true;
    };

} // namespace HybridPBR