#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace HybridPBR {
    
    // 数学工具函数
    class MathUtils {
    public:
        static constexpr float PI = 3.14159265359f;
        static constexpr float DEG2RAD = PI / 180.0f;
        static constexpr float RAD2DEG = 180.0f / PI;
        
        static float ToRadians(float degrees);
        static float ToDegrees(float radians);
        static glm::mat4 CreateTransformMatrix(const glm::vec3& position, 
                                              const glm::vec3& rotation, 
                                              const glm::vec3& scale);
    };

} // namespace HybridPBR