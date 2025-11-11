#include "MathUtils.h"

namespace HybridPBR {
    
    float MathUtils::ToRadians(float degrees) {
        return degrees * DEG2RAD;
    }
    
    float MathUtils::ToDegrees(float radians) {
        return radians * RAD2DEG;
    }
    
    glm::mat4 MathUtils::CreateTransformMatrix(const glm::vec3& position, 
                                              const glm::vec3& rotation, 
                                              const glm::vec3& scale) {
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = glm::rotate(transform, ToRadians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, ToRadians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, ToRadians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, scale);
        return transform;
    }

} // namespace HybridPBR